#pragma once

#ifndef __CHATTINGCLIENT_CONF__
#define __CHATTINGCLIENT_CONF__

#include <iostream>
#include <cstring>
#include <cstdio>
#include <WinSock2.h>
#include "CThread.h"
#include <thread>
#include "ChatException.h"
#include "json.h"

class ChattingClient;
class SendThread;
class RecvThread;
class SendRecvInterface;

namespace UserCommand {
	const char* const EXIT = "/exit";
};

typedef struct _MSG {
	char data[256];
} Message;

class ChattingClient {
private:
	SendThread *st;
	RecvThread *rt;
	SOCKET client_socket;
	SOCKADDR_IN server_address;
	void connectServer();

public:
	ChattingClient(const char *ip, int port);
	~ChattingClient();

	ChattingClient& getChattingClient();
	SOCKET& getClientSocket();
	void RedirectConnection(const char *ip, int port);
	void RedirectSocket(SOCKET sock);
	int run();

	static const int MAXSTRLEN;
};


class SendRecvInterface : public CThread {
public:
	virtual DWORD run(void) = 0;
	int sendMessage(SOCKET socket, const char *buf);
	int recvMessage(SOCKET socket, char *buf);
	void gotoxy(int x, int y);
};


class SendThread : public SendRecvInterface {
private:
	SOCKET client_socket;
	ChattingClient chatting_client;
public:
	SendThread(SOCKET cs, ChattingClient& cc);
	void RedirectSocket(SOCKET sock);
	virtual DWORD run(void);
	bool exitUser(const char *buf);
	void printcin(const char*);
	std::string ConvertMessageToJson();
};

class RecvThread : public SendRecvInterface {
private:
	SOCKET client_socket;
	ChattingClient chatting_client;
public:
	RecvThread(SOCKET cs, ChattingClient& cc);
	void RedirectSocket(SOCKET sock);
	virtual DWORD run(void);
	Json::Value ParseMessage(std::string message);
};

#endif