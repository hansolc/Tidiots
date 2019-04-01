#pragma once

#ifndef __APP_CONF__
#define __APP_CONF__

#include <iostream>
#include <vector>
#include <sstream>
#include "ChattingServer.h"
#include "User.h"
#include "CThread.h"
#include "ChatException.h"
#include <Windows.h>

using std::vector;
using std::stringstream;
using std::cout;
using std::endl;

namespace MessageType
{
	enum Type { LOGIN_PASS = 1, TEXT_MESSAGE = 2, ENTERROOM_REQUSET = 3 };
}

class login_server_App {
private:
	ChattingServer chattingserver;
	static const int MAXUSER; // 10Έν
public:
	login_server_App();

	void start();
	void printNewUser(const User *) const;
	void printExceedUser(const User *) const;

	static vector<User*> userList;
	static HANDLE hMutex;
};

#endif