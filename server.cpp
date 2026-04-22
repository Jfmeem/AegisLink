#define NOMINMAX
#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<tchar.h>
#include<thread>
#include<mutex>
#include<vector>
#include<algorithm>
#include<cstdio>
#include "DES.h"
#include "AES.h"
#include "sha512.h"
using namespace std;
#pragma comment(lib, "ws2_32.lib")

const string ENCRYPTION_KEY = "mykey123";
mutex clientsMutex;
mutex consoleMutex;

bool Initialize() {
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
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

static bool sendAll(SOCKET s, const char* buf, int len) {
    int total = 0;
    while (total < len) {
        int r = send(s, buf + total, len - total, 0);
        if (r == SOCKET_ERROR) return false;
        total += r;
    }
    return true;
}

void receiveFile(SOCKET clientSocket, int encType) {
    string encMethod = (encType == 1) ? "DES" : "AES";

    int filenameLen = 0;
    if (!recvAll(clientSocket, reinterpret_cast<char*>(&filenameLen), sizeof(filenameLen))) {
        lock_guard<mutex> lock(consoleMutex);
        cout << "Error receiving filename length." << endl;
        return;
    }
    if (filenameLen <= 0 || filenameLen >= 512) {
        lock_guard<mutex> lock(consoleMutex);
        cout << "Invalid filename length: " << filenameLen << endl;
        return;
    }

    char fname[512];
    if (!recvAll(clientSocket, fname, filenameLen)) {
        lock_guard<mutex> lock(consoleMutex);
        cout << "Error receiving filename." << endl;
        return;
    }
    fname[filenameLen] = '\0';

    string safeFilename(fname);
    size_t slashPos = safeFilename.find_last_of("/\\");
    if (slashPos != string::npos)
        safeFilename = safeFilename.substr(slashPos + 1);
    if (safeFilename.empty()) safeFilename = "received_file";

    int encryptedFilesize = 0;
    if (!recvAll(clientSocket, reinterpret_cast<char*>(&encryptedFilesize), sizeof(encryptedFilesize))) {
        lock_guard<mutex> lock(consoleMutex);
        cout << "Error receiving file size." << endl;
        return;
    }
    if (encryptedFilesize <= 0 || encryptedFilesize > 100 * 1024 * 1024) {
        lock_guard<mutex> lock(consoleMutex);
        cout << "Invalid file size: " << encryptedFilesize << endl;
        return;
    }

    {
        lock_guard<mutex> lock(consoleMutex);
        cout << "Receiving encrypted file: " << safeFilename
            << " (" << encryptedFilesize << " bytes) [" << encMethod << "]" << endl;
    }

    char* encryptedData = new char[encryptedFilesize];
    if (!recvAll(clientSocket, encryptedData, encryptedFilesize)) {
        lock_guard<mutex> lock(consoleMutex);
        cout << "Client disconnected during file transfer." << endl;
        delete[] encryptedData;
        return;
    }

    string encryptedContent(encryptedData, encryptedFilesize);
    delete[] encryptedData;

    string decryptedContent;
    if (encType == 1)
        decryptedContent = decryptString(encryptedContent, ENCRYPTION_KEY);
    else
        decryptedContent = decryptStringAES(encryptedContent, ENCRYPTION_KEY);

    string cleanContent;
    bool intact = verifyHash(decryptedContent, cleanContent);
    {
        lock_guard<mutex> lock(consoleMutex);
        if (intact)
            cout << "[INTACT] File integrity verified: " << safeFilename << endl;
        else
            cout << "[TAMPERED!] File integrity check failed: " << safeFilename << endl;
    }

    FILE* fp;
    fopen_s(&fp, safeFilename.c_str(), "wb");
    if (!fp) {
        lock_guard<mutex> lock(consoleMutex);
        cout << "Could not open file for writing: " << safeFilename << endl;
        return;
    }
    fwrite(cleanContent.c_str(), 1, cleanContent.size(), fp);
    fclose(fp);

    lock_guard<mutex> lock(consoleMutex);
    cout << "File \"" << safeFilename << "\" saved successfully! [" << encMethod << "]" << endl;
}

void InteractWithClient(SOCKET clientSocket, vector<SOCKET>& Clients) {
    {
        lock_guard<mutex> lock(consoleMutex);
        cout << "Client connected." << endl;
    }

    while (true) {
        char marker;
        if (!recvAll(clientSocket, &marker, 1)) {
            lock_guard<mutex> lock(consoleMutex);
            cout << "Client Disconnected!" << endl;
            break;
        }

        if ((unsigned char)marker == 0xFF) {
            char encTypeByte;
            if (!recvAll(clientSocket, &encTypeByte, 1)) {
                lock_guard<mutex> lock(consoleMutex);
                cout << "Client disconnected during file transfer handshake." << endl;
                break;
            }
            receiveFile(clientSocket, (int)(unsigned char)encTypeByte);
            continue;
        }

        if (marker == 1 || marker == 2) {
            int payloadLen = 0;
            if (!recvAll(clientSocket, reinterpret_cast<char*>(&payloadLen), sizeof(payloadLen))) {
                lock_guard<mutex> lock(consoleMutex);
                cout << "Client Disconnected!" << endl;
                break;
            }

            if (payloadLen <= 0 || payloadLen > 10 * 1024 * 1024) {
                lock_guard<mutex> lock(consoleMutex);
                cout << "[Warning] Invalid payload length: " << payloadLen << ", skipping." << endl;
                continue;
            }

            string encryptedMessage(payloadLen, '\0');
            if (!recvAll(clientSocket, &encryptedMessage[0], payloadLen)) {
                lock_guard<mutex> lock(consoleMutex);
                cout << "Client Disconnected!" << endl;
                break;
            }

            string decryptedMessage;
            if (marker == 1)
                decryptedMessage = decryptString(encryptedMessage, ENCRYPTION_KEY);
            else
                decryptedMessage = decryptStringAES(encryptedMessage, ENCRYPTION_KEY);

            string cleanMessage;
            bool intact = verifyHash(decryptedMessage, cleanMessage);
            {
                lock_guard<mutex> lock(consoleMutex);
                if (intact)
                    cout << "[INTACT][" << (marker == 1 ? "DES" : "AES") << "] " << cleanMessage << endl;
                else
                    cout << "[TAMPERED!][" << (marker == 1 ? "DES" : "AES") << "] " << cleanMessage << endl;
            }

            string packet;
            packet += marker;
            packet.append(reinterpret_cast<const char*>(&payloadLen), sizeof(payloadLen));
            packet += encryptedMessage;

            lock_guard<mutex> lock(clientsMutex);
            for (auto client : Clients) {
                if (client != clientSocket) {
                    sendAll(client, packet.c_str(), (int)packet.size());
                }
            }
        }
        else {
            lock_guard<mutex> lock(consoleMutex);
            cout << "[Warning] Unknown packet marker: "
                << (int)(unsigned char)marker << ", skipping." << endl;
        }
    }

    {
        lock_guard<mutex> lock(clientsMutex);
        auto it = find(Clients.begin(), Clients.end(), clientSocket);
        if (it != Clients.end())
            Clients.erase(it);
    }
    closesocket(clientSocket);
}

int main() {
    cout << "   ENCRYPTED SERVER - DES & AES Support  " << endl;

    if (!Initialize()) {
        cout << "Winsock Initialization failed" << endl;
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        cout << "Socket creation failed!" << endl;
        return 1;
    }

    int port = 346;
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (InetPton(AF_INET, _T("0.0.0.0"), &serverAddr.sin_addr) != 1) {
        cout << "Setting address structure failed" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Bind failed!" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Listen failed!" << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server started on port: " << port << endl;
    cout << "Supports DES and AES encryption" << endl;
    cout << "Waiting for clients...\n" << endl;

    vector<SOCKET> Clients;

    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            cout << "Failed to accept client." << endl;
            continue;
        }
        {
            lock_guard<mutex> lock(clientsMutex);
            Clients.push_back(clientSocket);
        }
        thread t1(InteractWithClient, clientSocket, ref(Clients));
        t1.detach();
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}