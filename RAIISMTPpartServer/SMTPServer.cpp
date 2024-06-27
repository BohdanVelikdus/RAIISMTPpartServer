#include "SMTPServer.h"
#include <iostream>
#include <string>

const char* SMTPServer::firstMessageFromServer = "220- 127.0.0.1 SMTP server edu ready\r\n";
const char* SMTPServer::helloMessage = "250- 127.0.0.1 SMTP server\r\nLogin\r\n";
const char* SMTPServer::okMessage = "334- OK\r\n";
const char* SMTPServer::ok250Message = "250 OK\r\n";
const char* SMTPServer::okData = "354 Enter message, ending with .\\r\\n on a line\r\n";
const char* SMTPServer::okId = "250 OK id=";

bool SMTPServer::ProcessFunction(WSABUF& buffer, WSABUF& received)
{
	char* forStr = new char[received.len];
	memcpy_s(forStr, received.len, received.buf, received.len);
	std::string tempRecv(forStr, forStr + strlen(forStr));
	
	delete[] forStr;
	
	std::string email;
	size_t start;
	size_t end;

	char* message = nullptr;

	char* temp = nullptr;
	std::string str;

	switch (currentEtap) {
	case FirstMessage:
		buffer.len = strlen(firstMessageFromServer);
		memcpy_s(buffer.buf, buffer.len, firstMessageFromServer, strlen(firstMessageFromServer));
		std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case HelloMessage:
		tempRecv[strlen(tempRecv.c_str()) - 1] = '\0';
		tempRecv[strlen(tempRecv.c_str()) - 1] = '\0';
		if (strcmp(tempRecv.c_str(), "EHLO GP")) {
			return false;
		}
		buffer.len = strlen(helloMessage);
		memcpy_s(buffer.buf, buffer.len, helloMessage, strlen(helloMessage));
		std::cout << "\n=====\nOn a second etap dummy buffer: " << buffer.buf << "\n============\n";
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case OkMessageAuth:
		tempRecv[strlen(tempRecv.c_str()) - 1] = '\0';
		tempRecv[strlen(tempRecv.c_str()) - 1] = '\0';
		if (strcmp(tempRecv.c_str(), "AUTH LOGIN")) {
			return false;
		}
		buffer.len = strlen(okMessage);
		memcpy_s(buffer.buf, buffer.len, okMessage, strlen(okMessage));
		std::cout << "\n=====\nOn a third etap dummy buffer: " << buffer.buf << "\n============\n";
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case RecivingUserName:
		tempRecv[strlen(tempRecv.c_str()) - 1] = '\0';
		tempRecv[strlen(tempRecv.c_str()) - 1] = '\0';

		// without null terminator + 1 - nullterminator
		username = new char[tempRecv.size() + 1];
		strcpy_s(username, tempRecv.size(), tempRecv.c_str());
		std::cout << "Username: " << username << "Len: " << strlen(username) << "\n";

		buffer.len = strlen(okMessage);
		memcpy_s(buffer.buf, buffer.len, okMessage, strlen(okMessage));
		std::cout << "\n=====\nOn a fourth etap dummy buffer: " << buffer.buf << "\n============\n";
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case ReceivingPassword:
		tempRecv[strlen(tempRecv.c_str()) - 1] = '\0';
		tempRecv[strlen(tempRecv.c_str()) - 1] = '\0';

		// without null terminator + 1 - nullterminator
		password = new char[tempRecv.size() + 1];
		strcpy_s(password, tempRecv.size(), tempRecv.c_str());
		std::cout << "Password: " << password << "\n";


		if (connectionSQL.Login(username, password)) {
			authSuccedOrNot = "235 Authentication Succeded\r\n";
			authenticated = true;
		}
		else {
			authSuccedOrNot = "535 Error credentials\r\n";
			authenticated = false;
		}
		buffer.len = strlen(authSuccedOrNot.c_str());
		memcpy_s(buffer.buf, buffer.len, authSuccedOrNot.c_str(), strlen(authSuccedOrNot.c_str()));
		std::cout << "\n=====\nOn a etc etap dummy buffer: " << buffer.buf << "\n============\n";
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case ValidatingMailSender:
		tempRecv[strlen(tempRecv.c_str()) - 1] = '\0';
		tempRecv[strlen(tempRecv.c_str()) - 1] = '\0';

		// exctracting mail 
		start = tempRecv.find('<');
		end = tempRecv.find('>');

		email = tempRecv.substr(start + 1, end - start - 1);
		email += '\0';
		//

		// without null terminator + 1 - nullterminator
		senderMail = new char[email.size() + 1];
		strcpy_s(senderMail, email.size(), email.c_str());
		std::cout << "Sender Mail: " << senderMail << "\n";

		// checking if matches sender mail and username(who loged in)
		if (strcmp(senderMail, username) == 0) {
			validSenderMailOrNot = new char[9];
			memcpy_s(validSenderMailOrNot, 9, "250 OK\r\n", 9); // "250 OK\r\n";
			sendingMailErrors = false;
		}
		else {
			validSenderMailOrNot = new char[23];
			memcpy_s(validSenderMailOrNot, 23, "500 Error SenderMail\r\n", 23);// "550 Error SenderMail\r\n";
			sendingMailErrors = true;
		}

		
		buffer.len = strlen(validSenderMailOrNot);
		memcpy_s(buffer.buf, buffer.len, validSenderMailOrNot, strlen(validSenderMailOrNot));
		std::cout << "\n=====\nOn a etc etap dummy buffer: " << buffer.buf << "\n============\n";
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case ValidatingMailRecepient:
		tempRecv[strlen(tempRecv.c_str()) - 1] = '\0';
		tempRecv[strlen(tempRecv.c_str()) - 1] = '\0';

		// exctracting mail 
		start = tempRecv.find('<');
		end = tempRecv.find('>');

		email = tempRecv.substr(start + 1, end - start - 1);
		email += '\0';
		//

		recepientMail = new char[email.size() + 1];
		strcpy_s(recepientMail, email.size(), email.c_str());
		std::cout << "Recepient Mail: " << recepientMail << "\n";

		if (connectionSQL.MailExist(recepientMail)) {
			validRecepientMailOrNot = new char[9];
			memcpy_s(validRecepientMailOrNot, 9, "250 OK\r\n", 9);// "250 OK\r\n";
			sendingMailErrors = false;
		}
		else {
			validRecepientMailOrNot = new char[26];
			memcpy_s(validRecepientMailOrNot, 26, "550 Error RecipientMail\r\n", 26);// "550 Error RecipientMail\r\n";
			sendingMailErrors = true;
		}


		buffer.len = strlen(validRecepientMailOrNot);
		memcpy_s(buffer.buf, buffer.len, validRecepientMailOrNot, strlen(validRecepientMailOrNot));
		std::cout << "\n=====\nOn a etc etap dummy buffer: " << buffer.buf << "\n============\n";
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case AgreedDATA: // here 
		buffer.len = strlen(okData);
		memcpy_s(buffer.buf, buffer.len, okData, strlen(okData));
		std::cout << "\n=====\nOn a etc etap dummy buffer: " << buffer.buf << "\n============\n";
		//currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	// sending id
	case CompletioID:
		id = new char[10];
		memcpy_s(id, 10, "111111111\0", 10);
		connectionSQL.SendMessageToDB(recepientMail, senderMail, std::chrono::system_clock::now(), mail);
		message = new char[strlen(id) + strlen(okId) + 3];
		strcpy_s(message, strlen(id) + strlen(okId) + 2, okId);
		

		strcat_s(message, strlen(id) + strlen(okId) + 2, id);
		memcpy_s(message + strlen(id) + strlen(okId), 2, "\r\n", 2);
		buffer.len = strlen(id) + strlen(okId) + 2;
		memcpy_s(buffer.buf, buffer.len, message, strlen(id) + strlen(okId) + 2);
		delete[] message;
		std::cout << "\n=====\nOn a the end etap dummy buffer: " << buffer.buf << "\n============\n";
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case End:
		temp = new char[received.len];
		memcpy_s(temp, received.len, received.buf, received.len);
		str = std::string(temp, received.len);
		str += '\0';
		if (strcmp(str.c_str(), "QUIT\r\n") == 0) {
			return false;
		}
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	}

	
	return true;
}

