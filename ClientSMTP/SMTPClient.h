#pragma once
#include <string>
#include <WinSock2.h>
#include <iostream>

class SMTPClient {
public:
	enum Etaps {
		EHLO,
		Login,
		Username,
		Password,
		MailFrom,
		MailTo,
		Data,
		Sending,
		PointAtTheEnd,
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

	static const char* commandEHLO_GP; // EHLO GP
	static const char* loginCommand;   // reques Login\r\n
	static const char* data;           // DATA\r\n
	static const char* dot;            // .\r\n
	static const char* quit;           // QUIT\r\n
	std::string usernameSend;          // to send usernameSend
	std::string passwordSend;          // to send password 
	        
	SMTPClient() {
		recvReplyFromServer = new char[1];
		*(recvReplyFromServer) = '\0';
	}

	void ProcessFunction(WSABUF& buffer, bool& continueSending) {
		ZeroMemory(buffer.buf, buffer.len);
		std::string temp;
		switch (currentEtap) {
		case EHLO:
			buffer.len = strlen(commandEHLO_GP);
			memcpy_s(buffer.buf, buffer.len, commandEHLO_GP, strlen(commandEHLO_GP));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		case Login: // AUTH LOGIN
			buffer.len = strlen(loginCommand);
			memcpy_s(buffer.buf, buffer.len, loginCommand, strlen(loginCommand));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		case Username:
			temp = username + "\r\n";
			buffer.len = strlen(temp.c_str());
			memcpy_s(buffer.buf, buffer.len, temp.c_str(), strlen(temp.c_str()));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		case Password:
			temp = password + "\r\n";
			buffer.len = strlen(temp.c_str());
			memcpy_s(buffer.buf, buffer.len, temp.c_str(), strlen(temp.c_str()));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		case MailFrom: // here also checking for successfully login
			if (strcmp(recvReplyFromServer, "535 Error credentials\r\n") == 0) {
				continueSending = false;
				std::cout << "catch the error with creadentials\n";
				break;
			}
			else {
				std::cout << "\n=====\n" << buffer.buf << "\n=====\n";
			}
			temp = "MAIL FROM: <" + mailFrom + ">\r\n";
			buffer.len = strlen(temp.c_str());
			memcpy_s(buffer.buf, buffer.len, temp.c_str(), strlen(temp.c_str()));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		case MailTo: // here also checking for mail from properly 
			if (strcmp(recvReplyFromServer, "500 Error SenderMail\r\n") == 0) {
				continueSending = false;
				std::cout << "catch the error with creadentials\n";
				break;
			}
			else {
				std::cout << "\n=====\n" << buffer.buf << "\n=====\n";
			}
			temp = "RCP TO: <" + mailTo + ">\r\n";
			buffer.len = strlen(temp.c_str());
			memcpy_s(buffer.buf, buffer.len, temp.c_str(), strlen(temp.c_str()));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		case Data:
			if (strcmp(recvReplyFromServer, "550 Error RecipientMail\r\n") == 0) {
				continueSending = false;
				std::cout << "catch the error with creadentials\n";
				break;
			}
			else {
				std::cout << "\n=====\n" << buffer.buf << "\n=====\n";
			}
			buffer.len = strlen(data);
			memcpy_s(buffer.buf, buffer.len, data, strlen(data));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		case PointAtTheEnd:
			buffer.len = strlen(dot);
			memcpy_s(buffer.buf, buffer.len, dot, strlen(dot));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		case Quit:
			buffer.len = strlen(quit);
			memcpy_s(buffer.buf, buffer.len, quit, strlen(quit));
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		}
		delete[] recvReplyFromServer;
		recvReplyFromServer = new char[1];
		*recvReplyFromServer = '\0';
	}
};
