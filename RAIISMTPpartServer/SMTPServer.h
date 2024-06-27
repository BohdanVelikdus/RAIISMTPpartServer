#pragma once
#include <string>
#include <WinSock2.h>
#include <iostream>

#include "SQLConnection.h"
#include "MemoryCheckpoint.h"
#include "HeapCheck.h"

class SMTPServer
{
public:
	enum Etaps {
		FirstMessage,
		HelloMessage,

		OkMessageAuth,

		RecivingUserName,
		ReceivingPassword,
		ValidatingMailSender,
		ValidatingMailRecepient,

		AgreedDATA,
		ReceivingData,
		CompletioID,
		End,
		Complete
	};


	mSQL::SQLConnection& connectionSQL;
	WSABUF dummyWSA;
	CHAR buffer[8192];
	
	Etaps currentEtap;
	bool authenticated = true;
	bool sendingMailErrors = false;

	char* username = nullptr;
	char* password = nullptr;

	char* mail = nullptr;

	SMTPServer(mSQL::SQLConnection& connection) : connectionSQL(connection) {
		dummyWSA.buf = buffer;
		dummyWSA.len = 8192;
		currentEtap = FirstMessage;

		mail = reinterpret_cast<char*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(char)));
		*mail = '\0';

		id = new char[9];
		ProcessFunction(dummyWSA, dummyWSA);
	}

	char* senderMail = nullptr;
	char* recepientMail = nullptr;
	char* id = nullptr;

	static const char* firstMessageFromServer; // 220 server ready
	static const char* helloMessage; //  "250- 127.0.0.1 SMTP server\r\nLogin\r\n";
	static const char* okMessage; // 334 OK 
	static const char* ok250Message; // 250 OK 
	static const char* okData; // agree to add data
	static const char* okId;

	std::string userNameRecv;// username recv
	// 334 OK 
	
	// 334 OK
	std::string authSuccedOrNot; //  OK 535 if fail or 235 Authentication Succeded\r\n

	char* validSenderMailOrNot = nullptr; // 250 OK if succeded | 550 sender addres rejected
	char* validRecepientMailOrNot = nullptr; // 250 OK if succeded | 550 sender addres rejected

	bool ProcessFunction(WSABUF& buffer, WSABUF& received);

	~SMTPServer() {
		delete[] username;
		delete[] password;
		HeapFree(GetProcessHeap(), 0, mail);
		delete[] senderMail;
		delete[] recepientMail;
		delete[] id;
		std::cout << "Call destructor SMTPServer\n";
	}
};




enum IOOperation {
	IORecv,
	IOSend
};

typedef struct _IO_INFO {
	WSAOVERLAPPED overlapped;

	IOOperation ioOperation;
	SOCKET socket;

	int bytesRecv;
	int bytesSend;
	CHAR buffer[8192];

	WSABUF wsaBuf;
	class SMTPServer *srv;

} IO_INFO, * PIO_INFO;



typedef struct _SOCKET_CONNECTED {
	SOCKET socket;
	PIO_INFO io;

	struct _SOCKET_CONNECTED* nextSocket;
	struct _SOCKET_CONNECTED* prevSocket;
} SOCKET_CONNECTED, * PSOCKET_CONNECTED;