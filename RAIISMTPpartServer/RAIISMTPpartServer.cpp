#include <iostream>
#include <WinSock2.h>
#include <Windows.h>

#include <thread>

#include "Server.h"
#include "POP3Server.h"

#include "SQLConnection.h"
#include "HeapCheck.h"
#include "MemoryCheckpoint.h"

void POP() {
    POP3Server srv("127.0.0.1", 110, mSQL::SQLConnection::getSQLConnection());
    srv.StartServer();
}

void createPOPThread() {
    std::thread t(POP);
    t.detach();
}

int main()
{
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
        std::cout << "WSADATA not initialized " << WSAGetLastError() << "\n";
        return 0;
    }
    try {
        createPOPThread();
        Server srv("127.0.0.1", 25, mSQL::SQLConnection::getSQLConnection());
        srv.StartServer(); 
    }
    catch (std::exception e) {
        std::cout << e.what() << "\n";
        mSQL::SQLConnection::destroySingleton();
    }

    WSACleanup();

}

