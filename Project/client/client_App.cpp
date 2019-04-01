#pragma once

#include "client_App.h"

HANDLE client_App::hMutex = CreateMutex(NULL, FALSE, NULL);

client_App::client_App() : serverport(0) {
	memset(serverip, 0, sizeof(serverip));
}

client_App::~client_App() {
	delete chattingclient;
}

void client_App::start() {
	int result = 0;

	printStart();

	char buf[16] = "127.0.0.1";
	int port = 3490;

	sprintf(this->serverip, "%s", buf);
	this->serverport = port;

	chattingclient = new ChattingClient(this->serverip, this->serverport);

	result = chattingclient->run();

	/*try {
		result = chattingclient->run();
	}
	catch (ChatException e) {
		e.printError();
		return;
	}

	if (result == 0) {
		printExit();
	}
	else {
		printError();
	}*/
}

void client_App::printStart() {
	cout << "======== CLIENT PROGRAM ========\n";
}

void client_App::printExit() {
	cout << "The connection to the server has been terminated." << endl;
}
void client_App::printError() {
	cout << "The server is disconnected." << endl;
}