
#include "main_server_App.h"
#include <iostream>

vector<User*> main_server_App::userList = vector<User*>();
vector<User*> main_server_App::Room_userList = vector<User*>();
const int main_server_App::MAXUSER = 10;
HANDLE main_server_App::hMutex = CreateMutex(NULL, FALSE, NULL);

main_server_App::main_server_App() : chattingserver(nullptr, 3495) {
}

void main_server_App::printNewUser(const User *user) const {
	cout << "New User Connects with this server. (" << user->getIP() << ", " << user->getPort() << ")" << endl;
}

void main_server_App::printExceedUser(const User *user) const {
	cout << "New User fails connecting with this server. (" << user->getIP() << ", " << user->getPort() << ")" << endl;
}

void main_server_App::start() {
	chattingserver.binding();
	chattingserver.listening(10);

	cout << "============ MAIN SERVER ==========\n";
	cout << "Log : " << endl;

	while (true) {
		User *user = chattingserver.acceptUser();
		if (main_server_App::userList.size() >= main_server_App::MAXUSER) {
			printExceedUser(user);
			continue;
		}

		WaitForSingleObject(main_server_App::hMutex, INFINITE);
		main_server_App::userList.push_back(user);
		ReleaseMutex(main_server_App::hMutex);
		printNewUser(user);
		user->start();
	}
}