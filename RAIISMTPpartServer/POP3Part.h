#pragma once
#include <WinSock2.h>
#include "SQLConnection.h"
#include <string>
#include <list>
#include <tuple>

class POP3Part
{
public:
	enum Etaps {
		FirstMessage,
		RecvUsername,
		RecvPassword,
		Retriving,
		MailTo,
		MailFrom,
		DateE,
		EmptyRN,
		BodyOfMail,
		DotRN,
		EndE
	};

	mSQL::SQLConnection& connectionSQL;
	WSABUF dummyWSA;
	CHAR buffer[8192];

	Etaps currentEtap;
	bool authenticated = true;
	bool sendingMailErrors = false;

	char* username = nullptr;
	char* password = nullptr;
	std::list<std::tuple<std::string, std::string, std::string, std::string>>* ret = nullptr;

	POP3Part(mSQL::SQLConnection& connection) : connectionSQL(connection) {
		dummyWSA.buf = buffer;
		dummyWSA.len = 8192;
		currentEtap = FirstMessage;
		
		id = new char[9];
		ProcessFunction(dummyWSA, dummyWSA);
	}
	char* senderMail = nullptr;
	char* recepientMail = nullptr;
	char* id = nullptr;

	static const char* firstMessageFromServer; // +OK POP3 Server ready 127.0.0.1
	static const char* userAccepted; // +OK User accepted
	static const char* passwordAccepted; // +OK Password accepted
	static const char* invalidUser; // -ERR Invalid user
	static const char* invalidPassword; // -ERR Invalid password
	static const char* retOk; // +OK how much octets
	static const char* noMoreMessages; // "-ERR no more messages

	char* To = nullptr;
	char* From = nullptr;
	char* Date = nullptr;
	char* RN = nullptr;
	char* Body = nullptr;
	char* End = nullptr;

	std::string userNameRecv;// username recv
	// 334 OK 

	// 334 OK
	std::string authSuccedOrNot; //  OK 535 if fail or 235 Authentication Succeded\r\n

	char* validSenderMailOrNot = nullptr; // 250 OK if succeded | 550 sender addres rejected
	char* validRecepientMailOrNot = nullptr; // 250 OK if succeded | 550 sender addres rejected

	bool ProcessFunction(WSABUF& buffer, WSABUF& received);

	~POP3Part() {
		delete[] username;
		delete[] password;
		
		delete[] senderMail;
		delete[] recepientMail;
		delete[] id;
		delete ret;
		std::cout << "Call destructor POPServer\n";
	}
};


enum IOOperationPOP {
	IORecvPOP,
	IOSendPOP
};

typedef struct _IO_INFO_POP {
	WSAOVERLAPPED overlapped;

	IOOperationPOP ioOperation;
	SOCKET socket;

	int bytesRecv;
	int bytesSend;
	CHAR buffer[8192];

	WSABUF wsaBuf;
	class POP3Part* srv;

} IO_INFO_POP, * PIO_INFO_POP;



typedef struct _SOCKET_CONNECTED_POP {
	SOCKET socket;
	PIO_INFO_POP io;

	struct _SOCKET_CONNECTED_POP* nextSocket;
	struct _SOCKET_CONNECTED_POP* prevSocket;
} SOCKET_CONNECTED_POP, * PSOCKET_CONNECTED_POP;