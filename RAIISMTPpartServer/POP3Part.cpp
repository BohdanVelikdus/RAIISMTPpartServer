#include "POP3Part.h"

const char* POP3Part::firstMessageFromServer = "+OK POP3 Server ready 127.0.0.1\r\n"; // +OK POP3 Server ready 127.0.0.1
const char* POP3Part::userAccepted = "+OK User accepted\r\n"; // +OK User accepted
const char* POP3Part::passwordAccepted = "+OK Password accepted\r\n"; // +OK Password accepted
const char* POP3Part::invalidUser = "-ERR Invalid user\r\n"; // -ERR Invalid user
const char* POP3Part::invalidPassword = "-ERR Invalid password\r\n"; // -ERR Invalid password
const char* POP3Part::retOk = "+OK "; // +OK how much octets
const char* POP3Part::noMoreMessages = "-ERR No More Messages\r\n";

bool POP3Part::ProcessFunction(WSABUF& buffer, WSABUF& received)
{
	char* forStr = new char[received.len];
	memcpy_s(forStr, received.len, received.buf, received.len);
	std::string tempRecv(forStr, forStr + strlen(forStr));
	
	delete[] forStr;
	std::string tempData;
	size_t start;
	size_t end;

	char* message = nullptr;
	char* temp = nullptr;
	std::string str;
	
	int len;
	
	std::string sendingTemp;
	switch (currentEtap) {
	case FirstMessage:
		buffer.len = strlen(firstMessageFromServer);
		memcpy_s(buffer.buf, buffer.len, firstMessageFromServer, strlen(firstMessageFromServer));
		std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case RecvUsername:
		tempData = tempRecv.substr(tempRecv.find(" ")+1, tempRecv.find("\r\n") - (tempRecv.find(" ") + 1) );
		std::cout << "The Login we get: " << tempData;
		username = new char[tempData.size()+1];
		memcpy_s(username, tempData.size()+1, tempData.c_str(), tempData.size() + 1);
		if (connectionSQL.MailExist(username)) {
			buffer.len = strlen(userAccepted);
			memcpy_s(buffer.buf, buffer.len, userAccepted, strlen(userAccepted));
			std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
			authenticated = true;
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		}
		else {
			buffer.len = strlen(invalidUser); 
			memcpy_s(buffer.buf, buffer.len, invalidUser, strlen(invalidUser));
			std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
			authenticated = false;
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		}
	case RecvPassword:
		tempData = tempRecv.substr(tempRecv.find(" ") + 1, tempRecv.find("\r\n") - (tempRecv.find(" ") + 1));
		std::cout << "The Password we get: " << tempData;
		password = new char[tempData.size() + 1];
		memcpy_s(password, tempData.size() + 1, tempData.c_str(), tempData.size() + 1);
		if (connectionSQL.Login(username, password)) {
			buffer.len = strlen(passwordAccepted);
			memcpy_s(buffer.buf, buffer.len, passwordAccepted, strlen(passwordAccepted));
			std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
			authenticated = true;
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		}
		else {
			buffer.len = strlen(invalidPassword);
			memcpy_s(buffer.buf, buffer.len, invalidPassword, strlen(invalidPassword));
			std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
			authenticated = false;
			currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
			break;
		}
	case Retriving:
		tempData = tempRecv.substr(tempRecv.find(" ") + 1, tempRecv.find("\r\n") - (tempRecv.find(" ") + 1));
		ret = connectionSQL.RetMessaffeFromDbReceived(username, std::stoi(tempData));

		if (ret->size() == 0) {
			currentEtap = EndE;
			buffer.len = strlen(noMoreMessages);
			memcpy_s(buffer.buf, buffer.len, noMoreMessages, strlen(noMoreMessages));
			std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
			break;
		}

		tempData.clear();
		To = new char[std::get<0>(*(ret->begin())).length() + 1 + 2];// +3 for '\r', '\n', and '\0'
		strcpy_s(To, std::get<0>(*(ret->begin())).length() + 1, std::get<0>(*(ret->begin())).c_str());
		strcat_s(To, std::get<0>(*(ret->begin())).length() + 1 + 2, "\r\n");


		From = new char[std::get<1>(*(ret->begin())).length() + 1 + 2];// +3 for '\r', '\n', and '\0'
		strcpy_s(From, std::get<1>(*(ret->begin())).length() + 1, std::get<1>(*(ret->begin())).c_str());
		strcat_s(From, std::get<1>(*(ret->begin())).length() + 3, "\r\n");

		Date = new char[std::get<2>(*(ret->begin())).length() + 1 + 2];// +3 for '\r', '\n', and '\0'
		strcpy_s(Date, std::get<2>(*(ret->begin())).length() + 1, std::get<2>(*(ret->begin())).c_str());
		strcat_s(Date, std::get<2>(*(ret->begin())).length() + 3, "\r\n");

		RN = new char[2];
		memcpy_s(RN, 2, "\r\n", 2);

		Body = new char[std::get<3>(*(ret->begin())).length() + 1 + 2];// +3 for '\r', '\n', and '\0'
		strcpy_s(Body, std::get<3>(*(ret->begin())).length() + 1, std::get<3>(*(ret->begin())).c_str());
		strcat_s(Body, std::get<3>(*(ret->begin())).length() + 3, "\r\n");
		
		End = new char[3];
		memcpy_s(End, 3, ".\r\n", 3);

		len = std::get<0>(*(ret->begin())).length() + 1 + 2 + std::get<1>(*(ret->begin())).length() + 1 + 2 + std::get<2>(*(ret->begin())).length() + 1 + 2 + std::get<3>(*(ret->begin())).length() + 1 + 2 + 2 + 3;

		tempData = retOk +  std::to_string(len) + " octets\r\n";
		buffer.len = strlen(tempData.c_str());
		memcpy_s(buffer.buf, buffer.len, tempData.c_str(), strlen(tempData.c_str()));
		std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
		authenticated = true;
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		
		break;
	case MailTo: // sending data 
		sendingTemp = std::string("To: ") + To;
		//buffer.len = std::get<0>(*(ret->begin())).length() + 1 + 2;
		buffer.len = sendingTemp.size();
		memcpy_s(buffer.buf, buffer.len, sendingTemp.c_str(), sendingTemp.size());
		std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
		authenticated = true;
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case MailFrom:
		sendingTemp = std::string("From: ") + From;
		buffer.len = sendingTemp.size();
		memcpy_s(buffer.buf, buffer.len, sendingTemp.c_str(), sendingTemp.size());
		std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
		authenticated = true;
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case DateE:
		sendingTemp = std::string("Date: ") + Date;
		buffer.len = sendingTemp.size();
		memcpy_s(buffer.buf, buffer.len, sendingTemp.c_str(), sendingTemp.size());
		std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
		authenticated = true;
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case EmptyRN:
		buffer.len = 2;
		memcpy_s(buffer.buf, buffer.len, RN, 2);
		std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
		authenticated = true;
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case BodyOfMail:
		buffer.len = std::get<3>(*(ret->begin())).length() + 1 + 2;
		memcpy_s(buffer.buf, buffer.len, Body, std::get<3>(*(ret->begin())).length() + 1 + 2);
		std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
		authenticated = true;
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case DotRN:
		buffer.len = 3;
		memcpy_s(buffer.buf, buffer.len, End, 3);
		std::cout << "\n=====\nOn a first etap dummy buffer: " << buffer.buf << "\n============\n";
		authenticated = true;
		currentEtap = static_cast<Etaps>(static_cast<int>(currentEtap) + 1);
		break;
	case EndE:
		return false;
	}
	return true;
}
