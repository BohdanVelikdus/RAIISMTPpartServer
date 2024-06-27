#include "SQLConnection.h"

namespace mSQL {
	SQLConnection* SQLConnection::connection = nullptr;

	SQLConnection::SQLConnection() {
		InitializeCriticalSection(&criticalSection);

		// Allocating environment handle 
		SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);

		// set attribute -- version of ODBC 
		SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

		//Allocating a handle to database connection
		SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbcVerification);

		//casting away a constantness 
		SQLWCHAR* connStringVerification = new SQLWCHAR[wcslen(szConnStrInVerification) + 1];
		memcpy_s(connStringVerification, 2 * (wcslen(szConnStrInVerification) + 1), szConnStrInVerification, 2 * (wcslen(szConnStrInVerification) + 1));

		SQLWCHAR szConnStrOut[1024];

		SQLSMALLINT cbConnStrOut;

		//SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

		//retcode
		retcode = SQLDriverConnectW(hDbcVerification, NULL, connStringVerification, SQL_NTS, szConnStrOut, 1024, &cbConnStrOut, SQL_DRIVER_COMPLETE);
		if (!SQL_SUCCEEDED(retcode)) {
			throw std::exception("Error with connecting to Verification!");
		}
		std::cout << "Connected to Verification DB\n";
		delete[] connStringVerification;

		SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbcEmail);

		//casting away a constantness 
		SQLWCHAR* connStringEmails = new SQLWCHAR[wcslen(szConnStrInMails) + 1];
		memcpy_s(connStringEmails, 2 * (wcslen(szConnStrInMails) + 1), szConnStrInMails, 2 * (wcslen(szConnStrInMails) + 1));

		retcode = SQLDriverConnectW(hDbcEmail, NULL, connStringEmails, SQL_NTS, szConnStrOut, 1024, &cbConnStrOut, SQL_DRIVER_COMPLETE);
		if (!SQL_SUCCEEDED(retcode)) {
			throw std::exception("Error with connecting to Emails!");
		}

		std::cout << "Connected to Emails DB\n";
		delete[] connStringEmails;
	}

	std::unordered_map<std::string, std::string>* SQLConnection::getAllUsersCredentials()
	{
		// sql query 
		SQLWCHAR sqlQuery[] = L"SELECT * FROM Verification";
		auto* map = new std::unordered_map<std::string, std::string>();

		EnterCriticalSection(&criticalSection);
		SQLAllocHandle(SQL_HANDLE_STMT, hDbcVerification, &hStmtVerification);
		// quering an sql to db 
		retcode = SQLExecDirectW(hStmtVerification, sqlQuery, SQL_NTS);

		if (SQL_SUCCEEDED(retcode)) {
			// char for buffer 
			SQLCHAR columnData[256];
			SQLCHAR columnData2[256];

			// length retrieved from sizeof which get
			SQLLEN columnDataLen;

			std::string password;
			while (SQLFetch(hStmtVerification) == SQL_SUCCESS) {
				SQLGetData(hStmtVerification, 1, SQL_C_CHAR, columnData, sizeof(columnData), &columnDataLen);
				std::string username(reinterpret_cast<const char*>(columnData));
				SQLGetData(hStmtVerification, 2, SQL_C_CHAR, columnData2, sizeof(columnData2), &columnDataLen);
				std::string password(reinterpret_cast<const char*>(columnData2));
				map->emplace(username, password);
			}
		}
		SQLFreeHandle(SQL_HANDLE_STMT, hStmtVerification);
		LeaveCriticalSection(&criticalSection);
		return map;
	}

	bool SQLConnection::Login(const std::string& username, const std::string& password)
	{
		SQLWCHAR sqlQuery[] = L"SELECT Password FROM Verification WHERE Username = ";
		std::wstring wstr(sqlQuery);
		int bytesSize = strlen(username.c_str()) + 1;
		SQLWCHAR* usernameW = new SQLWCHAR[bytesSize];
		MultiByteToWideChar(CP_UTF8, 0, username.c_str(), -1, usernameW, strlen(username.c_str()) + 1);
		std::wstring wstrUsername(usernameW, usernameW + wcslen(usernameW));
		wstr += L"'" + wstrUsername + L"'";
		delete[] usernameW;

		EnterCriticalSection(&criticalSection);
		SQLAllocHandle(SQL_HANDLE_STMT, hDbcVerification, &hStmtVerification);
		retcode = SQLExecDirectW(hStmtVerification, const_cast<wchar_t*>(wstr.c_str()), SQL_NTS);
		if (SQL_SUCCEEDED(retcode)) {
			// char for buffer 
			SQLCHAR columnData[256];
			SQLCHAR columnData2[256];

			// length retrieved from sizeof which get
			SQLLEN columnDataLen;

			// potencial UB
			if(SQLFetch(hStmtVerification) == SQL_SUCCESS) {
				SQLGetData(hStmtVerification, 1, SQL_C_CHAR, columnData, sizeof(columnData), &columnDataLen);
				SQLFreeHandle(SQL_HANDLE_STMT, hStmtVerification);
				LeaveCriticalSection(&criticalSection);
				std::string passwordFetched(reinterpret_cast<const char*>(columnData));
				std::cout << "fetched password: " << passwordFetched << "\n";
				std::cout << "Result of comparing: " << password.compare(passwordFetched) << "\n";
				bool res = password.compare(passwordFetched) == 0;
				return res;
			}
		}
		SQLFreeHandle(SQL_HANDLE_STMT, hStmtVerification);
		LeaveCriticalSection(&criticalSection);
		std::cout << "No such user exist in db" << "\n";
		return false;

	}

	bool SQLConnection::Register(const std::string& username, const std::string& password)
	{
		SQLWCHAR sqlQuery[] = L"SELECT * FROM Verification WHERE Username = ";
		std::wstring wstr(sqlQuery);
		int bytesSize = strlen(username.c_str()) + 1;
		SQLWCHAR* usernameW = new SQLWCHAR[bytesSize];
		MultiByteToWideChar(CP_UTF8, 0, username.c_str(), -1, usernameW, strlen(username.c_str()) + 1);
		std::wstring wstrUsername(usernameW, usernameW + wcslen(usernameW));
		wstr += L"'" + wstrUsername + L"'";

		delete[] usernameW;

		EnterCriticalSection(&criticalSection);
		SQLAllocHandle(SQL_HANDLE_STMT, hDbcVerification, &hStmtVerification);

		retcode = SQLExecDirectW(hStmtVerification, const_cast<wchar_t*>(wstr.c_str()), SQL_NTS);
		if (SQL_SUCCEEDED(retcode)) {
			// char for buffer 
			SQLCHAR columnData[256];
			SQLCHAR columnData2[256];

			// length retrieved from sizeof which get
			SQLLEN columnDataLen;

			if(SQLFetch(hStmtVerification) == SQL_SUCCESS) {
				SQLFreeHandle(SQL_HANDLE_STMT, hStmtVerification);
				LeaveCriticalSection(&criticalSection);
				std::cout << "User with such a Username exist\n";
				
				return false;
			}
		}
		SQLFreeHandle(SQL_HANDLE_STMT, hStmtVerification);
		LeaveCriticalSection(&criticalSection);

		

		// creating a new SQL query
		SQLWCHAR sqlQueryInsert[] = L"INSERT INTO Verification (Username, Password) VALUES ";
		std::wstring wStrQueryInsert(sqlQueryInsert);

		// coberting an password to wchar_t* 
		bytesSize = strlen(password.c_str()) + 1;
		SQLWCHAR* passwordW = new SQLWCHAR[bytesSize];
		MultiByteToWideChar(CP_UTF8, 0, password.c_str(), -1, passwordW, strlen(password.c_str()) + 1);
		std::wstring wstrPassword(passwordW, passwordW + wcslen(passwordW));

		delete[] passwordW;

		wStrQueryInsert += L"('" + wstrUsername + L"','" + wstrPassword + L"')";
		std::wcout << wStrQueryInsert << "\n";

		SQLWCHAR* insertSQLstar = new SQLWCHAR[wcslen(wStrQueryInsert.c_str()) + 1];
		memcpy_s(insertSQLstar, 2 * (wcslen(wStrQueryInsert.c_str()) + 1), wStrQueryInsert.c_str(), 2 * (wcslen(wStrQueryInsert.c_str()) + 1));


		EnterCriticalSection(&criticalSection);
		SQLAllocHandle(SQL_HANDLE_STMT, hDbcVerification, &hStmtVerification);
		retcode = SQLExecDirectW(hStmtVerification, insertSQLstar, SQL_NTS);
		if (SQL_SUCCEEDED(retcode)) {
			std::cout << "Should insert into table value\n";
			SQLFreeHandle(SQL_HANDLE_STMT, hStmtVerification);
			LeaveCriticalSection(&criticalSection);
			return true;
		}
		else {
			SQLWCHAR sqlState[6];
			SQLINTEGER nativeError;
			SQLWCHAR errMsg[SQL_MAX_MESSAGE_LENGTH];
			SQLSMALLINT msgLen;

			SQLGetDiagRec(SQL_HANDLE_STMT, hStmtVerification, 1, sqlState, &nativeError, errMsg, SQL_MAX_MESSAGE_LENGTH, &msgLen);
			printf("Error %d (%s): %s\n", nativeError, sqlState, errMsg);

		}
		SQLFreeHandle(SQL_HANDLE_STMT, hStmtVerification);
		LeaveCriticalSection(&criticalSection);
		std::cout << "Does not insert into\n";
		return false;
	}

	void SQLConnection::SendMessageToDB(const std::string& MailRecp, const std::string& MailSender, const std::chrono::time_point<std::chrono::system_clock>& time, const std::string& text)
	{
		SQLWCHAR sqlQueryInsert[] = L"INSERT INTO Mails (MailRecepient, MailSender, Date, Message) VALUES ";
		std::wstring sqlQueryString(sqlQueryInsert, sqlQueryInsert + wcslen(sqlQueryInsert));
		// Mail recp
		SQLWCHAR* MailRecpW = new SQLWCHAR[strlen(MailRecp.c_str()) + 1];
		MultiByteToWideChar(CP_UTF8, 0, MailRecp.c_str(), -1, MailRecpW, strlen(MailRecp.c_str()) + 1);
		std::wstring toIn(MailRecpW, MailRecpW + wcslen(MailRecpW));
		sqlQueryString += L"('" + toIn + L"', ";
		
		// Mail sender
		SQLWCHAR* MailSenderW = new SQLWCHAR[strlen(MailSender.c_str()) + 1];
		MultiByteToWideChar(CP_UTF8, 0, MailSender.c_str(), -1, MailSenderW, strlen(MailSender.c_str()) + 1);
		std::wstring toIn2(MailSenderW, MailSenderW + wcslen(MailSenderW));
		sqlQueryString += L"'" + toIn2 + L"',";

		// timep point

		std::time_t timeC = std::chrono::system_clock::to_time_t(time);
		std::tm tm;
		gmtime_s(&tm, &timeC);// convert time to tm struct
		std::ostringstream oss;
		oss << std::put_time(&tm, "%Y-%m-%d");
		std::string timeStr = oss.str();

		SQLWCHAR* timeW = new SQLWCHAR[strlen(timeStr.c_str()) + 1];
		MultiByteToWideChar(CP_UTF8, 0, timeStr.c_str(), -1, timeW, strlen(timeStr.c_str()) + 1);

		std::wstring toIn3(timeW, timeW + wcslen(timeW));
		sqlQueryString += L"'" + toIn3 + L"',";

		// message
		SQLWCHAR* MessageW = new SQLWCHAR[strlen(text.c_str()) + 1];
		MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, MessageW, strlen(text.c_str()) + 1);

		std::wstring toIn4(MessageW, MessageW + wcslen(MessageW));
		sqlQueryString += L"'" + toIn4 + L"')";

		SQLWCHAR* queryLast = new SQLWCHAR[wcslen(sqlQueryString.c_str()) + 1];
		memcpy_s(queryLast, 2 * (wcslen(sqlQueryString.c_str()) + 1), sqlQueryString.c_str(), 2 * (wcslen(sqlQueryString.c_str()) + 1));
		std::wcout << queryLast << "\n";

		EnterCriticalSection(&criticalSection);
		SQLAllocHandle(SQL_HANDLE_STMT, hDbcEmail, &hStmtMail);
		
		retcode = SQLExecDirectW(hStmtMail, queryLast, SQL_NTS);
		if (SQL_SUCCEEDED(retcode)) {
			SQLFreeHandle(SQL_HANDLE_STMT, hStmtMail);
			LeaveCriticalSection(&criticalSection);
			std::cout << "Should insert into table Mails an new Email!\n";
			return;
		}
		SQLFreeHandle(SQL_HANDLE_STMT, hStmtMail);
		LeaveCriticalSection(&criticalSection);
		std::cout << "Insert into Mails does not happend!\n";
		return;
	}

	std::list< std::tuple<std::string, std::string, std::string, std::string> >* SQLConnection::RetMessaffeFromDbReceived(const std::string& receiver, int offset)
	{
		int bytesSize = strlen(receiver.c_str()) + 1;

		SQLWCHAR* receiverW = new SQLWCHAR[bytesSize];
		MultiByteToWideChar(CP_UTF8, 0, receiver.c_str(), -1, receiverW, strlen(receiver.c_str()) + 1);
		std::wstring receiverWString(receiverW, receiverW + wcslen(receiverW));
		delete[] receiverW;
		std::wstring offsetW = std::to_wstring(offset);

		std::wstring query = L"SELECT * FROM Mails WHERE MailRecepient = ";
		query += L"'" + receiverWString + L"' ";
		query += L"ORDER BY Date ASC OFFSET ";
		query += offsetW;
		query += L"ROWS FETCH NEXT 1 ROWS ONLY;";

		std::list< std::tuple<std::string, std::string, std::string, std::string> >* list = new std::list< std::tuple<std::string, std::string, std::string, std::string> >();

		SQLWCHAR* queryLast = new SQLWCHAR[wcslen(query.c_str()) + 1];
		memcpy_s(queryLast, 2 * (wcslen(query.c_str()) + 1), query.c_str(), 2 * (wcslen(query.c_str()) + 1));
		EnterCriticalSection(&criticalSection);
		SQLAllocHandle(SQL_HANDLE_STMT, hDbcEmail, &hStmtMail);

		retcode = SQLExecDirectW(hStmtMail, queryLast, SQL_NTS);
		delete[] queryLast;
		if (SQL_SUCCEEDED(retcode)) {
			int ret;

			SQLSMALLINT numCols;
			
			SQLCHAR mailSender[256];
			SQLCHAR mailReceiver[256];
			SQLCHAR Date[256];
			int size = 256;
			std::string completeMessage;
			SQLCHAR* Message = new SQLCHAR[size];
			
			while(SQL_SUCCEEDED(SQLFetch(hStmtMail))) {
				SQLGetData(hStmtMail, 1, SQL_C_CHAR, mailReceiver, sizeof(mailReceiver), nullptr);
				std::string mailRecp( reinterpret_cast<const char*>(mailReceiver) );
				
				SQLGetData(hStmtMail, 2, SQL_C_CHAR, mailSender, sizeof(mailSender), nullptr);
				std::string mailSndr(reinterpret_cast<const char*>(mailSender));
			
				SQLGetData(hStmtMail, 3, SQL_C_CHAR, Date, sizeof(Date), nullptr);
				std::string date(reinterpret_cast<const char*>(Date));
			
				while (true) {
					int ret = SQLGetData(hStmtMail, 4, SQL_C_CHAR, Message, size, nullptr);
					if (ret == SQL_SUCCESS_WITH_INFO || ret == SQL_SUCCESS) {
						char* tempChars = new char[256];
						memcpy_s(tempChars, 256, Message, 256);
						std::string temp(tempChars, tempChars + strlen(tempChars));
						completeMessage += temp;
						delete[] tempChars;
						ZeroMemory(Message, 256);
					}
					else {
						break;
					}
				}

				std::tuple<std::string, std::string, std::string, std::string> tupRes = std::make_tuple(mailRecp, mailSndr, date, completeMessage);
				list->push_back(tupRes);
			}
			delete[] Message;
		}

		SQLFreeHandle(SQL_HANDLE_STMT, hStmtMail);
		LeaveCriticalSection(&criticalSection);
		return list;
	}

	std::list<std::tuple<std::string, std::string, std::string, std::string>>* SQLConnection::RetMessaffeFromDbSend(const std::string& sender, int offset)
	{
		int bytesSize = strlen(sender.c_str()) + 1;

		SQLWCHAR* receiverW = new SQLWCHAR[bytesSize];
		MultiByteToWideChar(CP_UTF8, 0, sender.c_str(), -1, receiverW, strlen(sender.c_str()) + 1);
		std::wstring receiverWString(receiverW, receiverW + wcslen(receiverW));
		delete[] receiverW;
		std::wstring offsetW = std::to_wstring(offset);

		std::wstring query = L"SELECT * FROM Mails WHERE MailSender = ";
		query += L"'" + receiverWString + L"' ";
		query += L"ORDER BY Date ASC OFFSET ";
		query += offsetW;
		query += L"ROWS FETCH NEXT 1 ROWS ONLY;";

		std::list< std::tuple<std::string, std::string, std::string, std::string> >* list = new std::list< std::tuple<std::string, std::string, std::string, std::string> >();

		SQLWCHAR* queryLast = new SQLWCHAR[wcslen(query.c_str()) + 1];
		memcpy_s(queryLast, 2 * (wcslen(query.c_str()) + 1), query.c_str(), 2 * (wcslen(query.c_str()) + 1));
		EnterCriticalSection(&criticalSection);
		SQLAllocHandle(SQL_HANDLE_STMT, hDbcEmail, &hStmtMail);

		retcode = SQLExecDirectW(hStmtMail, queryLast, SQL_NTS);
		delete[] queryLast;
		if (SQL_SUCCEEDED(retcode)) {
			int ret;

			SQLSMALLINT numCols;

			SQLCHAR mailSender[256];
			SQLCHAR mailReceiver[256];
			SQLCHAR Date[256];
			int size = 256;
			std::string completeMessage;
			SQLCHAR* Message = new SQLCHAR[size];
			ZeroMemory(Message, 256);

			// potencial UB here
			while (SQL_SUCCEEDED(SQLFetch(hStmtMail))) {
				SQLGetData(hStmtMail, 1, SQL_C_CHAR, mailReceiver, sizeof(mailReceiver), nullptr);
				std::string mailRecp(reinterpret_cast<const char*>(mailReceiver));

				SQLGetData(hStmtMail, 2, SQL_C_CHAR, mailSender, sizeof(mailSender), nullptr);
				std::string mailSndr(reinterpret_cast<const char*>(mailSender));

				SQLGetData(hStmtMail, 3, SQL_C_CHAR, Date, sizeof(Date), nullptr);
				std::string date(reinterpret_cast<const char*>(Date));

				while (true) {
					int ret = SQLGetData(hStmtMail, 4, SQL_C_CHAR, Message, size, nullptr);
					if (ret == SQL_SUCCESS_WITH_INFO || ret == SQL_SUCCESS) {
						char* tempChars = new char[256];
						memcpy_s(tempChars, 256, Message, 256);
						std::string temp(tempChars, tempChars + strlen(tempChars));
						completeMessage += temp;
						delete[] tempChars;
						ZeroMemory(Message, 256);
					}
					else{
						break;
					}
				}
				

				std::tuple<std::string, std::string, std::string, std::string> tupRes = std::make_tuple(mailRecp, mailSndr, date, completeMessage);
				list->push_back(tupRes);
			}
			delete[] Message;
		}

		SQLFreeHandle(SQL_HANDLE_STMT, hStmtMail);
		LeaveCriticalSection(&criticalSection);
		return list;
	}

	bool SQLConnection::MailExist(const std::string& Username)
	{
		SQLWCHAR sqlQuery[] = L"SELECT * FROM Verification WHERE Username = ";
		std::wstring wstr(sqlQuery);
		int bytesSize = strlen(Username.c_str()) + 1;
		SQLWCHAR* usernameW = new SQLWCHAR[bytesSize];
		MultiByteToWideChar(CP_UTF8, 0, Username.c_str(), -1, usernameW, strlen(Username.c_str()) + 1);
		std::wstring wstrUsername(usernameW, usernameW + wcslen(usernameW));
		wstr += L"'" + wstrUsername + L"'";

		delete[] usernameW;

		EnterCriticalSection(&criticalSection);
		SQLAllocHandle(SQL_HANDLE_STMT, hDbcVerification, &hStmtVerification);

		retcode = SQLExecDirectW(hStmtVerification, const_cast<wchar_t*>(wstr.c_str()), SQL_NTS);
		if (SQL_SUCCEEDED(retcode)) {
			// char for buffer 
			SQLCHAR columnData[256];
			SQLCHAR columnData2[256];

			// length retrieved from sizeof which get
			SQLLEN columnDataLen;

			if (SQLFetch(hStmtVerification) == SQL_SUCCESS) {
				SQLFreeHandle(SQL_HANDLE_STMT, hStmtVerification);
				LeaveCriticalSection(&criticalSection);
				std::cout << "User with such a Username exist\n";
				return true;
			}
		}
		SQLFreeHandle(SQL_HANDLE_STMT, hStmtVerification);
		LeaveCriticalSection(&criticalSection);
		std::cout << "User with such a Username does not exist\n";
		return false;
	}

	SQLConnection::~SQLConnection()
	{
		DeleteCriticalSection(&criticalSection);
		//should close handles if throwed error 

		// disconnect from Db
		SQLDisconnect(hDbcVerification);
		// disconnect from second Db
		SQLDisconnect(hDbcEmail);

		//close handle for connection
		SQLFreeHandle(SQL_HANDLE_DBC, hDbcVerification);

		//close handle for connection
		SQLFreeHandle(SQL_HANDLE_DBC, hDbcEmail);

		// free environment handle
		SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
	}

}