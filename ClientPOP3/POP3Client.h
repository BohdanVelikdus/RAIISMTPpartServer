#pragma once

#pragma once
#include <string>
#include <WinSock2.h>
#include <iostream>

class POP3Client {
public:
	enum Etaps {
		UsernameSending,
		PasswordSending,
		Retriving,
		RecvMailTo,
		RecvMailFrom,
		RecvDateE,
		RecvRN,
		RecvBody,
		RecvDotRN,
		Quit,
		End
	};

	Etaps currentEtap;
	char* recvReplyFromServer = nullptr;


	std::string username;
	std::string password;

	std::string mailFrom;
	std::string mailTo;
	std::string mail;
	std::string date;

	static const char* USER;		   // USER --- \r\n
	static const char* PASSWORD;       // PASSWORD --- \r\n
	static const char* RETR;           // RETR ---- \r\n
	static const char* QUIT;           // QUIT\r\n
	std::string usernameSend;          // to send usernameSend
	std::string passwordSend;          // to send password 
	int retrPos = 0;
	bool noMore = false;


	POP3Client() {
		recvReplyFromServer = new char[1];
		*(recvReplyFromServer) = '\0';
	}

	void ProcessFunction(WSABUF& buffer, bool& continueSending) {
		ZeroMemory(buffer.buf, buffer.len);
		std::string temp;
		switch (currentEtap) {
		case UsernameSending:
			temp = USER + username + "\r\n";
			buffer.len = strlen(temp.c_str());
			memcpy_s(buffer.buf, buffer.len, temp.c_str(), strlen(temp.c_str()));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		case PasswordSending:
			if (strcmp(recvReplyFromServer, "-ERR Invalid user\r\n") != 0) {
				temp = PASSWORD + password + "\r\n";
				buffer.len = strlen(temp.c_str());
				memcpy_s(buffer.buf, buffer.len, temp.c_str(), strlen(temp.c_str()));
				currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
				break;
			}
			else {
				continueSending = false;
				break;
			}
		case Retriving:
			if (strcmp(recvReplyFromServer, "-ERR Invalid password\r\n") != 0) {
				temp = RETR + std::to_string(retrPos) + "\r\n";
				buffer.len = strlen(temp.c_str());
				memcpy_s(buffer.buf, buffer.len, temp.c_str(), strlen(temp.c_str()));
				currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
				break;
			}
			else {
				continueSending = false;
				break;
			}
			break;
		case RecvMailTo:
			mailTo = std::string(recvReplyFromServer, recvReplyFromServer + strlen(recvReplyFromServer));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			if (mailTo.compare("-ERR No More Messages\r\n") == 0) {
				continueSending = false;
				noMore = true;
				std::cout << "No more mail inside the mailbox\n=================\n";
			}
			delete[] recvReplyFromServer;
			recvReplyFromServer = new char[1];
			*(recvReplyFromServer) = '\0';
			currentEtap = Quit;
			break;
		case RecvMailFrom:
			mailFrom = std::string(recvReplyFromServer, recvReplyFromServer + strlen(recvReplyFromServer));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			delete[] recvReplyFromServer;
			recvReplyFromServer = new char[1];
			*(recvReplyFromServer) = '\0';
			break;
		case RecvDateE:
			date = std::string(recvReplyFromServer, recvReplyFromServer + strlen(recvReplyFromServer));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			delete[] recvReplyFromServer;
			recvReplyFromServer = new char[1];
			*(recvReplyFromServer) = '\0';
			break;
		case RecvRN:
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			delete[] recvReplyFromServer;
			recvReplyFromServer = new char[1];
			*(recvReplyFromServer) = '\0';
			break;
		case RecvBody:
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			delete[] recvReplyFromServer;
			recvReplyFromServer = new char[1];
			*(recvReplyFromServer) = '\0';
			break;
		case RecvDotRN:
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			delete[] recvReplyFromServer;
			recvReplyFromServer = new char[1];
			*(recvReplyFromServer) = '\0';
			break;
		case Quit:
			mail = std::string(recvReplyFromServer, recvReplyFromServer + strlen(recvReplyFromServer));
			buffer.len = strlen(QUIT);
			memcpy_s(buffer.buf, buffer.len, QUIT, strlen(QUIT));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		case End:
			break;
		}
		delete[] recvReplyFromServer;
		recvReplyFromServer = new char[1];
		*recvReplyFromServer = '\0';
	}

	~POP3Client() {
		if(!noMore)
			std::cout << "\n=========\nThe mail is:\n" << mail << "\n==========\n";
		delete[] recvReplyFromServer;
	}
};
