#pragma once

#ifndef __APP_CONF__
#define __APP_CONF__

#include <iostream>
#include "ChattingClient.h"
#include <Windows.h>

using std::cout;
using std::cin;
using std::endl;

class client_App {
private:
	char serverip[16];
	int serverport;
	ChattingClient *chattingclient;

public:
	client_App();
	~client_App();

	void start();
	void printStart();
	void printExit();
	void printError();

	static HANDLE hMutex;
};

#endif