
#include "login_server_App.h"
#include <iostream>

vector<User*> login_server_App::userList = vector<User*>();
const int login_server_App::MAXUSER = 10;
HANDLE login_server_App::hMutex = CreateMutex(NULL, FALSE, NULL);

login_server_App::login_server_App() : chattingserver(nullptr, 3490) {
}

void login_server_App::printNewUser(const User *user) const {
	cout << "New User Connects with this server. (" << user->getIP() << ", " << user->getPort() << ")" << endl;
}

void login_server_App::printExceedUser(const User *user) const {
	cout << "New User fails connecting with this server. (" << user->getIP() << ", " << user->getPort() << ")" << endl;
}

void login_server_App::start() {
	chattingserver.binding();
	chattingserver.listening(10);

	cout << "============ LOG-IN SERVER ==========\n";
	cout << "Log : " << endl;

	while (true) {
		User *user = chattingserver.acceptUser();
		if (login_server_App::userList.size() >= login_server_App::MAXUSER) {
			printExceedUser(user);
			continue;
		}

		WaitForSingleObject(login_server_App::hMutex, INFINITE);
		login_server_App::userList.push_back(user);
		ReleaseMutex(login_server_App::hMutex);
		printNewUser(user);
		stringstream oss;
		oss << "(" << user->getIP() << ":" << user->getPort() << ") : " << "User connected. ";
		user->start();
	}
}