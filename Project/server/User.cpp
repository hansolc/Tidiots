
#include "User.h"
#include "login_server_App.h"

const int User::MAXSTRLEN = 255;

User::User(SOCKET cs, SOCKADDR_IN ca) : client_socket(cs), client_address(ca) {
}

User::~User() {
	closeSession();
	ThreadClose();
}

SOCKET User::getSocket() const {
	return this->client_socket;
}

char* User::getIP() const {
	static char *address = inet_ntoa(this->client_address.sin_addr);
	return address;
}

int User::getPort() const {
	return ntohs(this->client_address.sin_port);
}

void User::closeSession() {
	closesocket(this->client_socket);
}

DWORD User::run(void) {
	char buf[User::MAXSTRLEN];

	while (true) {
		try {
			recvMessage(buf);
			stringstream oss;
			oss << "ID Input : " << buf << "(" << getIP() << ":" << getPort() << ")";
			sendMessage(getSocket(), oss.str().c_str());
			oss << "ID Input : " << buf << "(" << getIP() << ":" << getPort() << ")" << endl;
		}
		catch (ChatException e) {
			if (e.getCode() == 1101) // 메인 서버로의 리디렉션인 경우
			{
				cout << "(" << getIP() << ":" << getPort() << ")" << " User disconnects this server, and connects main server." << endl;
				break;
			}
			else
			{
				cout << "(" << getIP() << " : " << getPort() << ")" << " User disconnected."  << endl;
				break;
			}
		}
	}

	WaitForSingleObject(login_server_App::hMutex, INFINITE);
	int len = login_server_App::userList.size();
	for (int i = 0; i < len; i++) {
		User *user = login_server_App::userList.at(i);
		if (user->getSocket() == this->getSocket()) {
			login_server_App::userList.erase(login_server_App::userList.begin() + i);
			break;
		}
	}
	ReleaseMutex(login_server_App::hMutex);
	delete this;
	return 0;
}

bool User::JsonLoginMessageRecv(std::string message)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(message,root);
	int type = root["type"].asInt();

	switch (type)
	{
	case LOGIN_PASS:
		if (root["id"].asString() == "abc") // ID 대조 작업, 추후에 DB를 연동한 계정 대조 기능 추가해야 함.
		{
			cout << "id match!" << endl;
			return true;
		}
		break;
	defalut:
		return false;
		break;
	}
	return false;
}

std::string User::JsonLoginMessageSend(bool pass)
{
	Json::Value root;
	Json::FastWriter fastWriter;
	root["type"] = 0;
	root["pass"] = pass;
	root["ip"] = "127.0.0.1"; // 메인서버의 고정 ip, port 송신
	root["port"] = 3495;
	return fastWriter.write(root);
}

void User::recvMessage(char *buf) {
	Message msg;
	int len = 0;
	memset(&msg, 0, sizeof(Message));

	if (recv(this->client_socket, (char*)&msg, sizeof(Message), 0) <= 0) {
		throw ChatException(1100);
	}
	len = strnlen(msg.data, User::MAXSTRLEN);
	strncpy(buf, msg.data, strnlen(msg.data, User::MAXSTRLEN));
	buf[len] = 0;

	std::string message(buf);
	if (JsonLoginMessageRecv(message)) // 로그인에 성공하면
	{
		std::string message = JsonLoginMessageSend(true);
		sendMessage(getSocket(), message.c_str());
		cout << "Main server connection succeeded. " << " (" << getIP() << " : " << getPort() << ")" << endl;
		throw ChatException(1101);
	}
	else
	{
		std::string message = JsonLoginMessageSend(false);
		cout << "id mismatch. " << " (" << getIP() << " : " << getPort() << ")" << endl;
		sendMessage(getSocket(), message.c_str());
	}
}

void User::sendMessageAll(const char *buf) {
	int len = login_server_App::userList.size();
	for (int i = 0; i < len; i++) {
		User *user = login_server_App::userList.at(i);
		try {
			sendMessage(user->getSocket(), buf);
		}
		catch (ChatException e) {
		}
	}
}

void User::sendMessage(SOCKET socket, const char *buf) {
	Message msg;
	memset(&msg, 0, sizeof(Message));

	if (buf != nullptr) {
		int len = strnlen(buf, User::MAXSTRLEN);
		strncpy(msg.data, buf, len);
		msg.data[len] = 0;
	}

	WaitForSingleObject(login_server_App::hMutex, INFINITE);
	if (send(socket, (const char*)&msg, sizeof(Message), 0) <= 0) {
		ReleaseMutex(login_server_App::hMutex);
		throw ChatException(1100);
	}
	ReleaseMutex(login_server_App::hMutex);
}
