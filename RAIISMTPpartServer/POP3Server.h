#pragma once

#include <WinSock2.h>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <WS2tcpip.h>
#include <Windows.h>

#include "POP3Part.h"

//general for memory checking and SQL connection
#include "SQLConnection.h"
#include "MemoryCheckpoint.h"
#include "HeapCheck.h"

class POP3Server
{
public:
	POP3Server(const std::string& str, const short port, mSQL::SQLConnection& connection);
	void StartServer();

	~POP3Server() {
		closesocket(listenSocket);
		closesocket(acceptSocket);

		for (DWORD i = 0; i < 4; i++)
			PostQueuedCompletionStatus(completionPort, 0, 0, NULL);
		if (WAIT_OBJECT_0 != WaitForMultipleObjects(threads.size(), threads.data(), TRUE, 1000))
			std::cout << "WaitForMultipleObjects() failed: " << GetLastError();
		while (true) {
			CloseClient(sockets, true);
			if (sockets == nullptr)
				break;
		}
	}

private:
	class VectorForThreads {
	public:
		VectorForThreads() = default;

		void push_back(HANDLE handle) {
			threads.push_back(handle);
		}

		int size() {
			return threads.size();
		}

		HANDLE* data() {
			return threads.data();
		}

		// possible UB
		~VectorForThreads() {
			for (HANDLE handle : threads) {
				if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
					CloseHandle(handle);
				}
			}
		}
	private:
		std::vector<HANDLE> threads;
	};
	mSQL::SQLConnection& connectionSQL;

	int iResult = 0;
	sockaddr_in serverAddrV4;
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET acceptSocket = INVALID_SOCKET;
	VectorForThreads threads;
	HANDLE completionPort = INVALID_HANDLE_VALUE;
	static CRITICAL_SECTION criticalSection;
	static PSOCKET_CONNECTED_POP sockets;

	bool UpdateCompletionPort(SOCKET socket, IOOperationPOP operation);
	static DWORD WINAPI WorkerThread(LPVOID WorkThreadContext);
	static VOID CloseClient(PSOCKET_CONNECTED_POP lpSocketConnected, BOOL graceful);
};

