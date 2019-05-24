
#include "User.h"
#include "main_server_App.h"

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
	printRoomInfoList();

	while (true) {
		try {
			recvMessage(buf);
		}
		catch (ChatException e) {
			if (e.getCode() == 1101) // 로그인 서버로의 리디렉션인 경우
			{
				cout << "(" << getIP() << ":" << getPort() << ")" << " User disconnects this server, and connects login server." << endl;
				break;
			}
			else
			{
				cout << "(" << getIP() << " : " << getPort() << ")" << " User disconnected." << endl;
				break;
			}
		}
	}

	WaitForSingleObject(main_server_App::hMutex, INFINITE);
	int len = main_server_App::userList.size();

	for (int i = 0; i < len; i++) {
		User *user = main_server_App::userList.at(i);
		if (user->getSocket() == this->getSocket()) {
			main_server_App::userList.erase(main_server_App::userList.begin() + i);
			break;
		}
	}

	vector<User*>::iterator iter;
	for (iter = main_server_App::Room_userList.begin(); iter!= main_server_App::Room_userList.end(); iter++)
	{
		if (getSocket() == (*iter)->getSocket())
		{
			main_server_App::Room_userList.erase(iter);
			break;
		}
	}

	ReleaseMutex(main_server_App::hMutex);
	delete this;
	return 0;
}

void User::ParseMessage(std::string message)
{
	Json::Reader reader;
	Json::Value root;
	reader.parse(message, root);
	int type = root["type"].asInt();

	switch (type)
	{
	case MessageType::TEXT_MESSAGE:
		cout << root["text"] << " > message received from " << getSocket() << endl;
		sendMessageInRoom(root["text"].asString().c_str());
		break;
	case MessageType::ENTERROOM_REQUSET:
		cout << getSocket() << " User enters the room." << endl;
		vector<User*> temp = main_server_App::Room_userList;
		bool isInRoom = false;
		for (int i = 0; i < temp.size(); i++)
		{
			if (getSocket() == temp[i]->getSocket())
			{
				isInRoom = true;
				break;
			}
		}
		if (isInRoom == false)
			main_server_App::Room_userList.push_back(this);

		printRoomInfoList();
		break;
	}
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
	ParseMessage(message);
}

void User::sendMessageAll(const char *buf) {
	int len = main_server_App::userList.size();
	for (int i = 0; i < len; i++) {
		User *user = main_server_App::userList.at(i);
		try {
			sendMessage(user->getSocket(), buf);
		}
		catch (ChatException e) {}
	}
}

void User::sendMessageInRoom(const char *buf)
{
	// 해당 방 내의 유저들에게 메세지를 날리는 메소드.
	int len = main_server_App::Room_userList.size();

	for (int i = 0; i < len; i++)
	{
		User *user = main_server_App::Room_userList.at(i);
		if (getSocket() == user->getSocket())
		{
			Json::Value root;
			Json::FastWriter fastWriter;
			std::string str;
			root["type"] = MessageType::TEXT_MESSAGE;
			root["id"] = getSocket();
			root["text"] = buf;
			str = fastWriter.write(root);

			for (int i = 0; i < len; i++)
			{
				User *user = main_server_App::Room_userList.at(i);
				try {
					sendMessage(user->getSocket(), str.c_str());
				}
				catch (ChatException e) {}
			}
			break;
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

	WaitForSingleObject(main_server_App::hMutex, INFINITE);
	if (send(socket, (const char*)&msg, sizeof(Message), 0) <= 0) {
		ReleaseMutex(main_server_App::hMutex);
		throw ChatException(1100);
	}
	ReleaseMutex(main_server_App::hMutex);
}

void User::printRoomInfoList()
{
	cout << "=============RoomInfoList=============" << endl;
	vector<User*> room_userList = main_server_App::Room_userList;
	if (!room_userList.empty())
	{
		for (int i = 0; i < room_userList.size(); i++)
		{
			if (room_userList.size() == i - 1)
			{
				cout << room_userList[i]->getIP() << "/ " << room_userList[i]->getPort() << " Client is in the room." << endl;
				break;
			}
			cout << room_userList[i]->getIP() << "/ " << room_userList[i]->getPort() << " Client" << endl;
		}
	}
	else
	{
		cout << "        The room is empty" << endl;
	}
	cout << "======================================" << endl;
}