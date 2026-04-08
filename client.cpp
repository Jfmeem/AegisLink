#define NOMINMAX
#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<string>
#include<thread>
#include<mutex>
#include<cstdio>
#include<limits>
#include<iomanip>
#include<sstream>
#include "DES.h"
#include "AES.h"
#include "sha512.h"

using namespace std;
#pragma comment(lib, "ws2_32.lib")

const string ENCRYPTION_KEY = "mykey123";
bool isRunning = true;
//bool showProof = false;
mutex consoleMutex;

bool Initialization() {
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

static bool sendAll(SOCKET s, const char* buf, int len) {
    int total = 0;
    while (total < len) {
        int r = send(s, buf + total, len - total, 0);
        if (r == SOCKET_ERROR) return false;
        total += r;
    }
    return true;
}

static bool recvAll(SOCKET s, char* buf, int len) {
    int total = 0;
    while (total < len) {
        int r = recv(s, buf + total, len - total, 0);
        if (r <= 0) return false;
        total += r;
    }
    return true;
}

/*
string toHex(const string& data, int maxBytes = 24) {
    ostringstream oss;
    int limit = min((int)data.size(), maxBytes);
    for (int i = 0; i < limit; i++)
        oss << hex << setw(2) << setfill('0') << (int)(unsigned char)data[i] << " ";
    if ((int)data.size() > maxBytes) oss << "...";
    return oss.str();
}

void printProof(const string& plaintext, const string& ciphertext, const string& encType) {
    cout << "\n+--------------------------------------------------+" << endl;
    cout << "  ENCRYPTION PROOF (" << encType << ")"               << endl;
    cout << "+--------------------------------------------------+" << endl;
    cout << "  [PLAINTEXT]  " << plaintext                          << endl;
    cout << "  [HEX PLAIN]  " << toHex(plaintext)                  << endl;
    cout << "  [ENCRYPTED]  " << toHex(ciphertext)                 << endl;
    cout << "  [KEY USED]   " << ENCRYPTION_KEY                    << endl;
    cout << "+--------------------------------------------------+" << endl;
    cout << "  ^ This scrambled hex is what travels over network" << endl;
    cout << "+--------------------------------------------------+\n" << endl;
}

void printRecvProof(const string& ciphertext, const string& plaintext, const string& encType) {
    cout << "\n+--------------------------------------------------+" << endl;
    cout << "  RECEIVED & DECRYPTED (" << encType << ")"           << endl;
    cout << "+--------------------------------------------------+" << endl;
    cout << "  [ENC HEX]    " << toHex(ciphertext)                 << endl;
    cout << "  [DECRYPTED]  " << plaintext                         << endl;
    cout << "+--------------------------------------------------+\n" << endl;
}
*/

void sendMessage(SOCKET s, const string& name, const string& text, int encChoice) {
    string encType = (encChoice == 1) ? "DES" : "AES";
    string msg = name + " : " + text;

    string msgWithHash = attachHash(msg);

    string encryptedMsg;
    if (encChoice == 1)
        encryptedMsg = encryptString(msgWithHash, ENCRYPTION_KEY);
    else
        encryptedMsg = encryptStringAES(msgWithHash, ENCRYPTION_KEY);

    //if (showProof) printProof(msgWithHash, encryptedMsg, encType);

    int payloadLen = (int)encryptedMsg.size();
    string packet;
    packet += (char)encChoice;
    packet.append(reinterpret_cast<char*>(&payloadLen), sizeof(payloadLen));
    packet += encryptedMsg;

    sendAll(s, packet.c_str(), (int)packet.size());
}

void sendFile(SOCKET s, const string& filepath, int encChoice) {
    string encType = (encChoice == 1) ? "DES" : "AES";

    FILE* fp;
    fopen_s(&fp, filepath.c_str(), "rb");
    if (!fp) { cout << "[Error] File not found." << endl; return; }

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* fileData = new char[filesize];
    fread(fileData, 1, filesize, fp);
    fclose(fp);

    string fileContent(fileData, filesize);
    delete[] fileData;

    string filename = filepath;
    size_t slashPos = filepath.find_last_of("/\\");
    if (slashPos != string::npos) filename = filepath.substr(slashPos + 1);

    string fileWithHash = attachHash(fileContent);
    string encryptedContent;
    if (encChoice == 1)
        encryptedContent = encryptString(fileWithHash, ENCRYPTION_KEY);
    else
        encryptedContent = encryptStringAES(fileWithHash, ENCRYPTION_KEY);
    /*
    if (showProof) {
        cout << "\n+--------------------------------------------------+" << endl;
        cout << "  FILE ENCRYPTION PROOF (" << encType << ")"           << endl;
        cout << "+--------------------------------------------------+" << endl;
        cout << "  [FILE]       " << filename                            << endl;
        cout << "  [ORIGINAL]   " << filesize << " bytes"               << endl;
        cout << "  [ENCRYPTED]  " << encryptedContent.size() << " bytes"<< endl;
        cout << "  [PLAIN HEX]  " << toHex(fileContent)                 << endl;
        cout << "  [ENC HEX]    " << toHex(encryptedContent)            << endl;
        cout << "  [KEY USED]   " << ENCRYPTION_KEY                     << endl;
        cout << "+--------------------------------------------------+\n" << endl;
    }
    */

    char header[2] = { (char)0xFF, (char)encChoice };
    sendAll(s, header, 2);

    int filenameLen = (int)filename.size();
    sendAll(s, reinterpret_cast<char*>(&filenameLen), sizeof(filenameLen));
    sendAll(s, filename.c_str(), filenameLen);

    int encryptedSize = (int)encryptedContent.size();
    sendAll(s, reinterpret_cast<char*>(&encryptedSize), sizeof(encryptedSize));
    sendAll(s, encryptedContent.c_str(), encryptedSize);

    cout << "[File sent: " << filename << " | " << encType << "]" << endl;
}

void ReceiveMsg(SOCKET s) {
    while (isRunning) {
        char marker;
        if (!recvAll(s, &marker, 1)) {
            lock_guard<mutex> lock(consoleMutex);
            cout << "\n[Disconnected from server]" << endl;
            isRunning = false; break;
        }

        int payloadLen = 0;
        if (!recvAll(s, reinterpret_cast<char*>(&payloadLen), sizeof(payloadLen))) {
            lock_guard<mutex> lock(consoleMutex);
            isRunning = false; break;
        }

        if (payloadLen <= 0 || payloadLen > 10 * 1024 * 1024) continue;

        string encryptedMsg(payloadLen, '\0');
        if (!recvAll(s, &encryptedMsg[0], payloadLen)) {
            lock_guard<mutex> lock(consoleMutex);
            isRunning = false; break;
        }

        string decryptedMsg;
        string encType;
        if (marker == 1) {
            decryptedMsg = decryptString(encryptedMsg, ENCRYPTION_KEY);
            encType = "DES";
        }
        else if (marker == 2) {
            decryptedMsg = decryptStringAES(encryptedMsg, ENCRYPTION_KEY);
            encType = "AES";
        }
        else {
            decryptedMsg = "[Unknown]";
            encType = "?";
        }

        string cleanMsg;
        bool intact = verifyHash(decryptedMsg, cleanMsg);

        lock_guard<mutex> lock(consoleMutex);

        //if (showProof)
        //    printRecvProof(encryptedMsg, decryptedMsg, encType);
        //else
        
        if (intact) {
            cout << "\n>> " << cleanMsg << "\n> " << flush;
        }
        else {
            cout << "\n [TAMPERED!] " << cleanMsg << "\n " << flush;
        }
    }
}

void ChatLoop(SOCKET s) {
    cout << "\nEnter your name: ";
    string name;
    getline(cin, name);

    int encChoice = 0;
    while (encChoice != 1 && encChoice != 2) {
        cout << "Choose encryption (1=DES / 2=AES): ";
        cin >> encChoice;
        cin.ignore((numeric_limits<streamsize>::max)(), '\n');
        if (encChoice != 1 && encChoice != 2)
            cout << "Enter 1 or 2." << endl;
    }
    string encType = (encChoice == 1) ? "DES" : "AES";

    cout << "\n[Chat ready | " << name << " | " << encType << "]" << endl;
    cout << "  /file <path> - send a file" << endl;
    cout << "  /quit        - exit" << endl;
    // cout << "  /proof       - toggle encryption proof on/off" << endl;  // <-- PROOF
    cout << "------------------------------------------" << endl;

    while (isRunning) {
        cout << "> ";
        string input;
        if (!getline(cin, input)) break;
        if (input.empty()) continue;

        if (input == "/quit") {
            cout << "Exiting..." << endl;
            isRunning = false;
            break;
        }
        //else if (input == "/proof") {
        //    showProof = !showProof;
        //    cout << "[Proof mode: " << (showProof ? "ON" : "OFF") << "]" << endl;
        //}
        else if (input.rfind("/file ", 0) == 0) {
            sendFile(s, input.substr(6), encChoice);
        }
        else {
            sendMessage(s, name, input, encChoice);
        }
    }
}

int main() {
    cout << "--- ENCRYPTED CHAT (DES & AES) ---" << endl;

    if (!Initialization()) { cout << "Initialization failed!" << endl; return 1; }

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) { cout << "Socket creation failed!" << endl; return 1; }

    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(314159);
    inet_pton(AF_INET, "127.0.0.1", &(serveraddr.sin_addr));

    if (connect(s, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR) {
        cout << "Unable to connect to the server!" << endl;
        closesocket(s); WSACleanup(); return 1;
    }

    cout << "Connected to server!" << endl;

    thread receiveThread(ReceiveMsg, s);
    ChatLoop(s);

    isRunning = false;
    closesocket(s);
    receiveThread.join();
    WSACleanup();
    return 0;
}