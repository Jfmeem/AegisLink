#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include <tchar.h>
#include<thread>
#include<vector>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

//step-1: initialize 
bool Initialize() {

	WSADATA data; //structure for winsock info

	int res = WSAStartup(MAKEWORD(2, 2), &data); // starrt winsock library

	return (res == 0); //if true return success

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
		cout << "Print message: " << message << endl;

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

	if (!Initialize()) {
		cout << " winsock Initialization failed" << endl;
		return 1;
	}

	cout << "fucking spl" << endl;

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
			cout << "invalid client socket" << endl;
		}

		Clients.push_back(clientSocket);

		thread t1(InterectWithClient, clientSocket, std::ref(Clients));

		t1.detach();

	}

	

	closesocket(listenSocket);

	//Finalize
	WSACleanup();
	return 0;
}