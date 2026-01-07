#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include <tchar.h>
#include<thread>
#include<vector>
#include<algorithm>
#include<cstdio>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

//step-1: initialize 
bool Initialize() {

	WSADATA data; //structure for winsock info

	int res = WSAStartup(MAKEWORD(2, 2), &data); // starrt winsock library

	return (res == 0); //if true return success

}

void recievedFile(SOCKET clientSocket) {
	int filenameLen;
	recv(clientSocket, reinterpret_cast<char*>(&filenameLen), sizeof(filenameLen), 0);

	char fname[256];
	recv(clientSocket, fname, filenameLen, 0);
	fname[filenameLen] = '\0';

	long filesize;
	recv(clientSocket, reinterpret_cast<char*>(&filesize), sizeof(filesize), 0);

	FILE* fp = fopen(fname, "wb");
	if (!fp) {
		cout << "did not able to open the file." << endl;
	}

	char buffer[1024];
	long totalReceived = 0;
	while (totalReceived < filesize) {
		int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
		
		if (bytes <= 0) {
			cout << "client disconnected." << endl;
			break;
		}

		fwrite(buffer, 1, bytes, fp);
		totalReceived += bytes;
	}

	fclose(fp);

	cout << "File \"" << fname << "\" received successfully!" << endl;
	
}

void InterectWithClient(SOCKET clientSocket, vector<SOCKET>& Clients) {
	//send/rcv client
	cout << "Client connected" << endl;

	char buffer[9301];
	
	while (1) {
		int byterecvd = recv(clientSocket, buffer, sizeof(buffer), 0);

		if (byterecvd <= 0) {
			cout << "Client Disconnected!" << endl;
			break;
		}

		//message print
		string message(buffer, byterecvd);

		//check for file transfer
		if (message == "file_transfer") {
			recievedFile(clientSocket);
			continue;
		}

		cout << "Message: " << message << endl;

		//broadcast to other clients
		for (auto client : Clients) {
			if (client != clientSocket) {
				send(client, message.c_str(), message.size(), 0);
			}
		}

	}

	auto it = find(Clients.begin(), Clients.end(), clientSocket);
	if (it != Clients.end()) {
		Clients.erase(it);
	}

	closesocket(clientSocket);
}

int main() {
	cout << "SPL_1" << endl;

	if (!Initialize()) {
		cout << " winsock Initialization failed" << endl;
		return 1;
	}

	//cretaing socket

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (listenSocket == INVALID_SOCKET) {
		cout << "Socket creation failed!" << endl;
		return 1;
	}

	//create address structure
	int port = 314159;
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);


	//convert the ip address(0.0.0.0) put it inside sin_family into binary formate
	if (InetPton(AF_INET, _T("0.0.0.0"), &serverAddr.sin_addr) != 1) {
		cout << "setting address structure failed" << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	//bind
	if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
		cout << "Bind failed!" << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	//listen
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		cout << "Listen failed!" << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	cout << "server has started listening on port:" << port << endl;
	//for multiple client msg
	vector<SOCKET> Clients;


	//accept
	while (1) {

		SOCKET clientSocket = accept(listenSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET) {
			cout << "failed to accept client." << endl;
			continue;
		}

		Clients.push_back(clientSocket);

		thread t1(InterectWithClient, clientSocket, ref(Clients));

		t1.detach();

	}

	

	closesocket(listenSocket);
	WSACleanup();

	return 0;
}