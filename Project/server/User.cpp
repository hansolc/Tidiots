
#include "User.h"
#include "login_server_App.h"
#include "mysql.h"

#define DB_HOST "127.0.0.1"
#define DB_ID "root"
#define DB_PW "root"
#define DB_NAME "Chat_Member"


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
				cout << "(" << getIP() << " : " << getPort() << ")" << " User disconnected." << endl;
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

void User::ParseMessage(std::string message)
{
	// summery : Client -> LoginServer 메세지 Json Parsing, 계정 정보의 유효성 확인
	Json::Reader reader;
	Json::Value root;
	reader.parse(message, root);
	int type = root["type"].asInt();

	switch (type)
	{
	case MessageType::LOGIN_PASS: // ID 대조작업 DB 구현 GUI 연결되야 구현 가능
		MYSQL* connection = NULL, conn;  //connect
		MYSQL_RES* sql_result;
		MYSQL_ROW sql_row;
		int query_stat;
		int i;

		char query[255];

		mysql_init(&conn);

		//DB 연결
		connection = mysql_real_connect(&conn, DB_HOST, DB_ID, DB_PW, DB_NAME,
			3306, (char*)NULL, 0);
		if (connection == NULL)
		{
			cout << "Mysql connection error: " << mysql_error(&conn) << endl;
		}

		// SELECT문 구현 IDtext, PWtext 부분이 GUI 에서 입력값 받는부분
		// id,pw 에 입력된 정보와 Student table 의 레코드 값과 일치하는지 확인한다.
		query_stat = mysql_query(connection, "select * from Chat_Member.student where studentID = '" 
										+ IDtext->Text + "' and studentPW = '" + PWtext->Text + "'; ");

		if (query_stat != 0)
		{
			std::string message = JsonLoginMessageSend(false);
			sendMessage(getSocket(), message.c_str());
			cout << " 로그인 실패 " << mysql_error(&conn) << endl;
		}
		else
		{
			cout << " 로그인 성공 " << endl;
			std::string message = JsonLoginMessageSend(true);
			sendMessage(getSocket(), message.c_str());
			throw ChatException(1101);
		}
		break;
	case MessageType::TEXT_MESSAGE:
	{
	}
	break;
	case MessageType::NEW_ACCOUNT:
		MYSQL* connection = NULL, conn;  //connect
		MYSQL_RES* sql_result;
		MYSQL_ROW sql_row;
		int query_stat;
		int i;

		char query[255];

		mysql_init(&conn);

		//DB 연결
		connection = mysql_real_connect(&conn, DB_HOST, DB_ID, DB_PW, DB_NAME,
			3306, (char*)NULL, 0);
		if (connection == NULL)
		{
			cout << "Mysql connection error: " << mysql_error(&conn) << endl;
	    }
		//레코드 삽입 회원가입 정보 입력되는 부분
		// IDtext 와 PWtext 는 따로 GUI 와 연동되야한다.
		//INSERT 문으로 Student 테이블에 새로운 레코드값을 등록한다.
		query_stat = mysql_query(connection, "INSERT INTO Student VALUES ('" +
								IDtext->text + "','" + PWtext->text + "','" + ");");

		
		if (query_stat != 0)
		{
			std::string message = JsonLoginMessageSend(false); // 이부분 회원가입 실패 메세지로 바꿔야함
			sendMessage(getSocket(), message.c_str());
			cout << " 회원가입 실패 " << mysql_error(&conn) << endl;
		}
		else
		{
			cout << " 회원가입 성공 " << endl;
			std::string message = JsonLoginMessageSend(true); // 이부분 회원가입 완료 메세지로 바꿔야함
			sendMessage(getSocket(), message.c_str());
			throw ChatException(1101);  
		}

		break;
	default:
		break;
	}
}

std::string User::JsonLoginMessageSend(bool pass)
{
	// summery : LoginServer -> Client 메세지 송신 이전에 Json Formatting.
	Json::Value root;
	Json::FastWriter fastWriter;
	root["type"] = 1;
	root["pass"] = pass;
	root["ip"] = "127.0.0.1"; // 메인서버의 고정 ip
	root["port"] = 3495;	  // 메인서버의 고정 port
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
	ParseMessage(message);
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
