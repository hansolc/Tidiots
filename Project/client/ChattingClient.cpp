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

void ChattingClient::cc_bind(const char *ip, int port)
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
}

void ChattingClient::connectServer() {
	cout << "server connecting..." << endl;
	if (connect(this->client_socket, (SOCKADDR*)&this->server_address, sizeof(this->server_address)) == SOCKET_ERROR) {
		cout << "server connection failed." << endl;
		throw ChatException(1001);
	}
	cout << "server connection complete." << endl;
}

int ChattingClient::run() {
	this->st = new SendThread(this->client_socket);
	this->rt = new RecvThread(this->client_socket, this->getChattingClient());
	connectServer();
	st->start();
	rt->start();
	st->join();
	rt->join();
	return st->getExitCode();
}





void SendRecvInterface::recvMessage(SOCKET socket, char *buf) {
	Message msg;
	int len = 0;
	memset(&msg, 0, sizeof(Message));

	if (recv(socket, (char*)&msg, sizeof(Message), 0) <= 0) {
		throw ChatException(1100);
	}
	len = strnlen(msg.data, ChattingClient::MAXSTRLEN);
	strncpy(buf, msg.data, len);
	buf[len] = 0;
}

void SendRecvInterface::sendMessage(SOCKET socket, const char *buf) {
	Message msg;
	memset(&msg, 0, sizeof(Message));

	if (buf != nullptr) {
		int len = strnlen(buf, ChattingClient::MAXSTRLEN);
		strncpy(msg.data, buf, len);
		msg.data[len] = 0;
	}

	WaitForSingleObject(client_App::hMutex, INFINITE);
	if (send(socket, (const char*)&msg, sizeof(Message), 0) <= 0) {
		ReleaseMutex(client_App::hMutex);
		throw ChatException(1100);
	}
	ReleaseMutex(client_App::hMutex);
}

void SendRecvInterface::gotoxy(int x, int y) {
	COORD Cur;
	Cur.X = x;
	Cur.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cur);
}



SendThread::SendThread(SOCKET cs) : client_socket(cs) {

}

std::string SendThread::LoginRequestMessageToJson(std::string message)
{
	// summery : Client -> LoginServer 전송하기 이전에 메세지 Json Formatting.
	Json::Value root;
	root["type"] = LOGIN_PASS;
	root["id"] = message;
	Json::FastWriter fastWriter;
	std::string str = fastWriter.write(root);
	return str;
}

DWORD SendThread::run(void) {
	int result = -1;
	char buf[ChattingClient::MAXSTRLEN];
	while (true) {
		try {
			cin >> buf;
			if (exitUser(buf)) {
				result = 0;
				throw ChatException(2100);
			}
			std::string message = LoginRequestMessageToJson(buf);
			sendMessage(this->client_socket, message.c_str());
		}
		catch (ChatException e) {
			closesocket(this->client_socket);
			break;
		}
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

Json::Value RecvThread::LoginRequestMessageRecv(std::string message)
{
	// summery : LoginServer -> Client 메세지를 Json Parsing.
	Json::Reader reader;
	Json::Value root;
	reader.parse(message, root);

	if (root["type"].asInt() != LOGIN_PASS)
		return false;

	if (root["pass"].asBool() == true)
	{
		return root;
	}
	else
	{
		return NULL;
	}
}

DWORD RecvThread::run(void) {
	char buf[ChattingClient::MAXSTRLEN];
	while (true) {
		try {
			recvMessage(this->client_socket, buf);
			std::string message(buf);
			Json::Value JsonToMessage = LoginRequestMessageRecv(message);
			if (JsonToMessage != NULL)
			{
				cout << "Log-in success." << endl;

				std::string temp(JsonToMessage["ip"].asString());
				std::vector<char> writable(temp.begin(), temp.end());
				writable.push_back('\0');

				char* ip = &writable[0];
				int port = JsonToMessage["port"].asInt();
				cout << ip << ", " << port << endl;

				chatting_client.cc_bind(ip, port);
				chatting_client.run();
			}
		}
		catch (ChatException e) {
			closesocket(this->client_socket);
			break;
		}
	}
	return 0;
}