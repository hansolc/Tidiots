#pragma once

#ifndef __CHATTINGCLIENT_CONF__
#define __CHATTINGCLIENT_CONF__

#include <iostream>
#include <cstring>
#include <cstdio>
#include <WinSock2.h>
#include "CThread.h"
#include "ChatException.h"
#include "json.h"

class ChattingClient;
class SendThread;
class RecvThread;
class SendRecvInterface;

namespace UserCommand {
	const char * const EXIT = "/exit";
};

typedef struct _MSG{
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

	void cc_bind(const char *ip, int port);
	ChattingClient& getChattingClient();
	int run();

	static const int MAXSTRLEN;
};


class SendRecvInterface : public CThread {
public:
	virtual DWORD run(void)=0;
	void sendMessage(SOCKET socket, const char *buf);
	void recvMessage(SOCKET socket, char *buf);
	void gotoxy(int x, int y);
};


class SendThread : public SendRecvInterface {
private:
	SOCKET client_socket;

public:
	enum { LOGIN_PASS = 0 }; // 메세지 타입 정의

	SendThread(SOCKET cs);
	virtual DWORD run(void);

	bool exitUser(const char *buf);
	void printcin(const char*);
	std::string LoginRequestMessageToJson(std::string message);
};

class RecvThread : public SendRecvInterface {
private:
	SOCKET client_socket;
	ChattingClient chatting_client;
	

public:
	enum { LOGIN_PASS = 0 }; // 메세지 타입 정의

	RecvThread(SOCKET cs, ChattingClient& cc);
	virtual DWORD run(void);
	void printcout(const char*);
	bool check_login(const char* buf);
	Json::Value LoginRequestMessageRecv(std::string message);
};

#endif