#include<iostream>
#include<Winsock2.h>
#include<WS2tcpip.h>
#include<string>
#include<thread>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

//initialization
bool Initialization() {

	WSADATA data;
	return WSAStartup(MAKEWORD(2, 2), &data) == 0;

}

void SendMsg(SOCKET s) {
	cout << "Enter your name: ";

	string name;
	getline(cin, name);
	
	string message;

	while (1) {
		getline(cin, message);
		string msg = name + " : " + message;

		int bytesent = send(s, msg.c_str(), msg.size(), 0);
		if (bytesent == SOCKET_ERROR) {
			cout << "Error sending message!" << endl;
			break;
		}

		if (message == "Quit") {
			cout << "Stopping the application." << endl;
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
			msg = string(buffer, rcvLength);
			cout << msg << endl;
		}
	}

}

int main(){


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

	//creating thread
	thread senderthread(SendMsg, s);
	thread receivethread(ReceiveMsg, s);

	senderthread.join();
	receivethread.join();

	closesocket(s);
	WSACleanup();

	return 0;

}