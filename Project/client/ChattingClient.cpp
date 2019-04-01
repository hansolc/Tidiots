#pragma once

#include "ChattingClient.h"
#include "client_App.h"

const int ChattingClient::MAXSTRLEN = 255;

ChattingClient::ChattingClient(const char *ip, int port) {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	this->client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->client_socket == INVALID_SOCKET) {
		throw ChatException(1000);
		WSACleanup();
	}

	memset(&this->server_address, 0, sizeof(this->server_address));
	this->server_address.sin_addr.S_un.S_addr = inet_addr(ip);
	this->server_address.sin_port = htons(port);
	this->server_address.sin_family = AF_INET;
}

ChattingClient::~ChattingClient() {
	closesocket(this->client_socket);
	delete this->st;
	delete this->rt;
	WSACleanup();
}

ChattingClient& ChattingClient::getChattingClient()
{
	return *this;
}

SOCKET& ChattingClient::getClientSocket()
{
	return this->client_socket;
}

void ChattingClient::RedirectSocket(SOCKET sock)
{
	st->RedirectSocket(sock);
}

void ChattingClient::RedirectConnection(const char *ip, int port)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	this->client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->client_socket == INVALID_SOCKET) {
		throw ChatException(1000);
		WSACleanup();
	}

	memset(&this->server_address, 0, sizeof(this->server_address));
	this->server_address.sin_addr.S_un.S_addr = inet_addr(ip);
	this->server_address.sin_port = htons(port);
	this->server_address.sin_family = AF_INET;

	connectServer();
}

void ChattingClient::connectServer() {
	if (connect(this->client_socket, (SOCKADDR*)&this->server_address, sizeof(this->server_address)) == SOCKET_ERROR) {
		throw ChatException(1001);
	}
}

int ChattingClient::run() {
	this->st = new SendThread(this->client_socket, getChattingClient());
	this->rt = new RecvThread(this->client_socket, getChattingClient());
	connectServer();
	st->start();
	rt->start();
	st->join();
	rt->join();

	return st->getExitCode();
}

int SendRecvInterface::recvMessage(SOCKET socket, char *buf) {
	Message msg;
	int len = 0;
	memset(&msg, 0, sizeof(Message));

	if (recv(socket, (char*)&msg, sizeof(Message), 0) <= 0) {
		return -1;
	}
	len = strnlen(msg.data, ChattingClient::MAXSTRLEN);
	strncpy(buf, msg.data, len);
	buf[len] = 0;
	return 0;
}

int SendRecvInterface::sendMessage(SOCKET socket, const char *buf) {
	Message msg;
	memset(&msg, 0, sizeof(Message));

	if (buf != nullptr) {
		int len = strnlen(buf, ChattingClient::MAXSTRLEN);
		strncpy(msg.data, buf, len);
		msg.data[len] = 0;
	}
	if (send(socket, (const char*)&msg, sizeof(Message), 0) <= 0) {
		return -1;
	}
	return 0;
}

void SendRecvInterface::gotoxy(int x, int y) {
	COORD Cur;
	Cur.X = x;
	Cur.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cur);
}



SendThread::SendThread(SOCKET cs, ChattingClient& cc) : client_socket(cs), chatting_client(cc) {

}

void SendThread::RedirectSocket(SOCKET sock)
{
	client_socket = sock;
}

std::string SendThread::ConvertMessageToJson()
{
	int input;
	Json::Value root;
	Json::FastWriter fastWriter;
	std::string str;

	cout << "=============== Command =================" << endl;
	cout << "1. Log-in  2. Message  3. Enter the room" << endl;
	cout << "=========================================" << endl;

	do {
		cin >> input;
	} while (!cin);

	switch (input)
	{
	case MessageType::LOGIN_PASS:
	{
		// summery : Client -> LoginServer 전송하기 이전에 메세지 Json Formatting.
		std::string buf;
		cout << "LOG-IN || ID : ";
		cin.clear();
		cin.ignore();
		cin >> buf;
		root["type"] = MessageType::LOGIN_PASS;
		root["id"] = buf;
		str = fastWriter.write(root);
		return str;
	}
	case MessageType::TEXT_MESSAGE:
	{
		std::string buf;
		cout << "MESSAGE || INPUT : ";
		cin.clear();
		cin.ignore();
		cin >> buf;
		root["type"] = MessageType::TEXT_MESSAGE;
		root["text"] = buf;
		str = fastWriter.write(root);
		return str;
	}
	case MessageType::ENTERROOM_REQUSET:
	{
		cout << "ENTER THE ROOM." << endl;
		root["type"] = MessageType::ENTERROOM_REQUSET;
		str = fastWriter.write(root);
		return str;
	}
	default:
		break;
	}
}

DWORD SendThread::run(void) {
	int result = -1;
	char buf[ChattingClient::MAXSTRLEN];
	while (true) {
		try {
			std::string str = ConvertMessageToJson();
			const char* message = str.c_str();
			if (exitUser(message)) {
				result = 0;
				throw ChatException(2100);
			}

			if (sendMessage(this->client_socket, message) < 0)
			{
				RedirectSocket(chatting_client.getClientSocket());
			}
		}
		catch (ChatException e) {
			closesocket(this->client_socket);
			break;
		}
		Sleep(50);
	}
	return result;
}

bool SendThread::exitUser(const char *buf) {
	if (stricmp(buf, UserCommand::EXIT) == 0) {
		return true;
	}
	return false;
}

void SendThread::printcin(const char*) {
	gotoxy(1, 1);
}

RecvThread::RecvThread(SOCKET cs, ChattingClient& cc) : client_socket(cs), chatting_client(cc) {

}

void RecvThread::RedirectSocket(SOCKET sock)
{
	client_socket = sock;
}

Json::Value RecvThread::ParseMessage(std::string message)
{
	// summery : LoginServer -> Client 메세지를 Json Parsing.
	Json::Reader reader;
	Json::Value root;
	reader.parse(message, root);
	return root;
}

DWORD RecvThread::run(void) {
	char buf[ChattingClient::MAXSTRLEN];
	while (true) {
		try {
			if (recvMessage(this->client_socket, buf) >= 0)
			{
				std::string message(buf);
				Json::Value JsonToMessage = ParseMessage(message);

				switch (JsonToMessage["type"].asInt())
				{
				case MessageType::LOGIN_PASS:
				{
					if (JsonToMessage["pass"].asBool() == true)
					{
						cout << "log-in pass." << endl;
						std::string temp(JsonToMessage["ip"].asString());
						std::vector<char> writable(temp.begin(), temp.end());
						writable.push_back('\0');

						char* ip = &writable[0];
						int port = JsonToMessage["port"].asInt();
						chatting_client.RedirectConnection(ip, port);
					}
					break;
				}
				case MessageType::TEXT_MESSAGE:
				{
					cout << JsonToMessage["id"] << " : " << JsonToMessage["text"] << endl;
					break;
				}
				default:
					break;
				
				}
			}
			else // 리디렉션 예외가 발생했을 경우
			{
				cout << "log-in server connection redirects to main server socket." << endl;
				RedirectSocket(chatting_client.getClientSocket());
				chatting_client.RedirectSocket(chatting_client.getClientSocket());
			}
		}
		catch (ChatException e) {
			closesocket(this->client_socket);
			break;
		}
	}
	return 0;
}