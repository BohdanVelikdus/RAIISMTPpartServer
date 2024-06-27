#pragma once

#include <stdio.h>
#include <Windows.h>
#include <ctime>

#include <sql.h>
#include <sqlext.h>

#include <iostream>
#include <tuple>
#include <string>
#include <unordered_map>
#include <memory>
#include <exception>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <list>


namespace mSQL {

	class SQLConnection
	{
	private :
		static SQLConnection* connection;
		SQLConnection();

	public:
		static SQLConnection& getSQLConnection() {
			if (connection == nullptr) {
				try {
					connection = new SQLConnection();
					std::atexit(&SQLConnection::destroySingleton);
				}
				catch (std::exception ex) {
					std::cout << ex.what() << "\n";
				}
			}
			return *connection;
		}

		std::unordered_map<std::string, std::string>* getAllUsersCredentials(); 

		bool Login(const std::string& username, const std::string& password);
		bool Register(const std::string& username, const std::string& password);
		
		void SendMessageToDB(const std::string& MailRecp, const std::string& MailSender, const std::chrono::time_point<std::chrono::system_clock>& time, const std::string& text);
		
		std::list< std::tuple<std::string, std::string, std::string, std::string> >* RetMessaffeFromDbReceived(const std::string& receiver, int offset);
		std::list< std::tuple<std::string, std::string, std::string, std::string> >* RetMessaffeFromDbSend(const std::string& sender, int offset);

		bool MailExist(const std::string& Username);

		static void destroySingleton() {
			delete connection;
			connection = nullptr;
		}
		
		~SQLConnection();
	private:
		
		

		// handle for environment
		SQLHANDLE hEnv;
		// handle for connection
		SQLHANDLE hDbcVerification;
		SQLHANDLE hDbcEmail;
		// handle for quering
		SQLHANDLE hStmtVerification;
		SQLHANDLE hStmtMail;
	
		//retcode
		SQLRETURN retcode;

		// short connection string 	
		const SQLWCHAR* szConnStrInVerification = L"DRIVER={ODBC Driver 17 for SQL Server};SERVER=DESKTOP-T742L19;DATABASE=Verification;Trusted_Connection=Yes;";
		const SQLWCHAR* szConnStrInMails = L"DRIVER={ODBC Driver 17 for SQL Server};SERVER=DESKTOP-T742L19;DATABASE=Email;Trusted_Connection=Yes;";
		CRITICAL_SECTION criticalSection;
	};

}
