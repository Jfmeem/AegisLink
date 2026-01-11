#include<iostream>
#include<winsock2.h>
#include<wS2tcpip.h>
#include<string>
#include<thread>
#include<cstdio>
#include<limits>
#include "DES.h"
using namespace std;
#pragma comment(lib, "ws2_32.lib")

//shared encryption key
const string ENCRYPTION_KEY = "mykey123";

//initialization
bool Initialization() {

	WSADATA data;
	return WSAStartup(MAKEWORD(2, 2), &data) == 0;

}

void SendFile(SOCKET s) {
	if (cin.peek() == '\n') cin.ignore();
	string filename;
	cout << "Enter file name to send: ";
	getline(cin, filename);

	FILE* fp;
	fopen_s(&fp, filename.c_str(), "rb");
	if (fp == NULL) {
		cout << "File not found!" << endl;
		return;
	}

	//get file size
	fseek(fp, 0, SEEK_END);
	long filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	//read entire file into memory
	char* fileData = new char[filesize];
	fread(fileData, 1, filesize, fp);
	fclose(fp);

	//convert file data to string for encryption
	string fileContent(fileData, filesize);
	delete[] fileData;
	//encrypt the file content
	cout << "Encrypting file..."<<endl;
	string encryptedContent = encryptString(fileContent, ENCRYPTION_KEY);

	//send file transfer cmd
	string cmd = "file_transfer";
	send(s, cmd.c_str(), cmd.size(), 0);
	Sleep(100);

	//send filename length
	int filenamelen = filename.size();
	send(s, reinterpret_cast<char*>(&filenamelen), sizeof(filenamelen), 0);
	send(s, filename.c_str(), filenamelen, 0);

	//send encrypted file size
	long encryptedSize = encryptedContent.size();
	send(s, reinterpret_cast<char*>(&encryptedSize), sizeof(encryptedSize), 0);

	////send file size
	//send(s, reinterpret_cast<char*>(&filesize), sizeof(filesize), 0);

	cout << "Sending encrypted file...." << endl;
	const char* dataPtr = encryptedContent.c_str();
	long totalSent = 0;
	while (totalSent < encryptedSize) {
		int chunkSize = min(1024L, encryptedSize - totalSent);
		int sent = send(s, dataPtr + totalSent, chunkSize, 0);
		if (sent == SOCKET_ERROR) {
			cout << "Error sending file!" << endl;
			return;
		}
		totalSent += sent;
	}

	cout << "File encrypted and sent successfully!" << endl;
	return;
}

void SendMsg(SOCKET s) {
	cout << "Enter your name: ";
	string name;
	getline(cin, name);
	string message;

	while (1) {
		getline(cin, message);

		// file send cmd 
		if (message == "sendfile") {
			thread t(SendFile, s);
			t.detach();
			continue;
		}
		string msg = name + " : " + message;

		//encrypt the msg
		string encryptedMsg = encryptString(msg, ENCRYPTION_KEY);
		//send encrypted msg
		int bytesent = send(s,encryptedMsg.c_str(), encryptedMsg.size(), 0);
		if (bytesent == SOCKET_ERROR) {
			cout << "Error sending message!" << endl;
			break;
		}

		if (message == "Quit") {
			cout << "Stopping the application." << endl;
			break;
		}
	}

}

void ReceiveMsg(SOCKET s) {
	char buffer[9301];
	int rcvLength;
	string msg = "";
	while (1) {
		rcvLength = recv(s, buffer, sizeof(buffer), 0);
		if (rcvLength <= 0) {
			cout << "Disconnected from the server." << endl;
			break;
		}
		else {
		//decrypt the received msg
			string encryptedMsg(buffer, rcvLength);
			string decryptingMsg = decryptString(encryptedMsg, ENCRYPTION_KEY);
			cout << decryptedMsg << endl;
		}
	}

}

int main() {
	
	cout << "----Encrypted Client----" <<endl;
	cout << "client program started!" << endl;

	if (!Initialization()) {
		cout << "initialization failed!" << endl;

		return 1;
	}

	//create socket 
	SOCKET s;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET) {
		cout << "socket creation failed!" << endl;
		return 1;
	}

	string serveraddress = "127.0.0.1";
	int port = 314159;
	sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	inet_pton(AF_INET, serveraddress.c_str(), &(serveraddr.sin_addr));

	if (connect(s, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR) {
		cout << "not able to connect the server!" << endl;
		closesocket(s);
		WSACleanup();
		return 1;
	}

	cout << "successfully connect to the server!" << endl;
	cout << "All messages and files will be encrypted with DES" << endl;

	//creating thread
	thread senderthread(SendMsg, s);
	thread receivethread(ReceiveMsg, s);
	senderthread.join();
	receivethread.join();

	closesocket(s);
	WSACleanup();

	return 0;

}

