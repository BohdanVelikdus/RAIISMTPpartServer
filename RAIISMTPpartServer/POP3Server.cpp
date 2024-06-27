#include "POP3Server.h"


PSOCKET_CONNECTED_POP POP3Server::sockets = nullptr;
CRITICAL_SECTION POP3Server::criticalSection;


POP3Server::POP3Server(const std::string& str, const short port, mSQL::SQLConnection& connection) : connectionSQL(connection)
{
	listenSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (listenSocket == INVALID_SOCKET) {
		std::cout << "Error in creating a lsitening socket: " << WSAGetLastError() << "\n";
		throw std::exception("Error in creating a socket for listening\n");
	}
	InitializeCriticalSection(&criticalSection);
	serverAddrV4.sin_port = htons(port);
	serverAddrV4.sin_family = AF_INET;
	if (inet_pton(AF_INET, str.c_str(), &serverAddrV4.sin_addr) != 1) {
		std::cout << "Error parsing string to IPv4 addres\n";
		throw std::exception("Error in passing a std::string& str argument\n");
	}
	iResult = bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddrV4), sizeof(sockaddr_in));
	if (iResult == SOCKET_ERROR) {
		std::cout << "Error in binding: " << WSAGetLastError() << "\n";
		throw std::exception("Error in passing a std::string& str argument\n");
	}
	if (listen(listenSocket, 5) == SOCKET_ERROR) {
		std::cout << "Error in listening: " << WSAGetLastError() << "\n";
		throw std::exception("Error in listening\n");
	}
	acceptSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (acceptSocket == INVALID_SOCKET) {
		std::cout << "Error in creating a lsitening socket: " << WSAGetLastError() << "\n";
		throw std::exception("Error in creating a socket for listening\n");
	}
	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (completionPort == INVALID_HANDLE_VALUE) {
		std::cout << "Error in creating a completion port: " << GetLastError() << "\n";
		throw std::exception("Error in creating a completion port\n");
	}
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	for (DWORD count = 0; count < systemInfo.dwNumberOfProcessors; count++) {
		HANDLE hThread = INVALID_HANDLE_VALUE;
		DWORD dwThreadId;
		hThread = CreateThread(nullptr, 0, WorkerThread, completionPort, 0, &dwThreadId);
		if (hThread == INVALID_HANDLE_VALUE) {
			std::cerr << "CreateThread failed, error: " << GetLastError() << std::endl;
		}
		else {
			threads.push_back(hThread);
		}
	}
}

void POP3Server::StartServer()
{
	int nRet;
	DWORD dwSendBytes;
	DWORD dwFlags = 0;
	while (TRUE) {
		acceptSocket = WSAAccept(listenSocket, nullptr, NULL, nullptr, 0);
		if (acceptSocket == SOCKET_ERROR) {
			std::cout << "Error in accepting socket\n" << WSAGetLastError();
			continue;
		}
		std::cout << "Done with accepting socket\n";
		if (!UpdateCompletionPort(acceptSocket, IOSendPOP)) {
			closesocket(acceptSocket);
		}
		else {
			EnterCriticalSection(&criticalSection);
			nRet = WSASend(sockets->socket, &sockets->io->srv->dummyWSA, 1, &dwSendBytes, dwFlags, &sockets->io->overlapped, nullptr);
			if (nRet == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING) {
				CloseClient(sockets, true);
			}
			LeaveCriticalSection(&criticalSection);
		}
	}
}

bool POP3Server::UpdateCompletionPort(SOCKET socket, IOOperationPOP operation)
{
	PSOCKET_CONNECTED_POP newConnection = reinterpret_cast<PSOCKET_CONNECTED_POP>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SOCKET_CONNECTED_POP)));
	if (newConnection != NULL) {
		newConnection->socket = socket;
		newConnection->nextSocket = nullptr;
		newConnection->prevSocket = nullptr;
		PIO_INFO_POP pIOIonfo = reinterpret_cast<PIO_INFO_POP>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IO_INFO_POP)));
		if (pIOIonfo != NULL) {
			pIOIonfo->socket = socket;
			pIOIonfo->wsaBuf.buf = pIOIonfo->buffer;
			pIOIonfo->wsaBuf.len = 8192;

			pIOIonfo->ioOperation = operation;

			pIOIonfo->bytesRecv = 0;
			pIOIonfo->bytesSend = 0;
			pIOIonfo->srv = new POP3Part(connectionSQL);
			newConnection->io = pIOIonfo;
			completionPort = CreateIoCompletionPort((HANDLE)newConnection->socket, completionPort, (DWORD_PTR)newConnection, 0);
			if (completionPort != NULL) {
				EnterCriticalSection(&criticalSection);
				if (sockets == nullptr) {
					sockets = newConnection;
				}
				else {
					PSOCKET_CONNECTED_POP temp = sockets;
					temp->nextSocket = newConnection;
					newConnection->prevSocket = temp;
					sockets = newConnection;
				}
				LeaveCriticalSection(&criticalSection);
				return true;
			}
		}
		HeapFree(GetProcessHeap(), 0, newConnection);
	}
	return false;

}

// possible refactoring 
DWORD __stdcall POP3Server::WorkerThread(LPVOID WorkThreadContext)
{

	HANDLE hIOCP = (HANDLE)WorkThreadContext;
	PSOCKET_CONNECTED_POP lpConnectedSocket = nullptr;
	PIO_INFO_POP lpIOInfo = nullptr;
	BOOL bSuccess = FALSE;
	LPWSAOVERLAPPED lpOverlappped = nullptr;
	DWORD dwFlags = 0;
	DWORD ioSize = 0;
	DWORD dwSendNumBytes;
	DWORD dwRecvNumBytes;
	int nRet;
	int prevIoSize;
	std::string sstream;
	WSABUF buffRecv;
	std::string tempStr;
	BOOL clientAlive = true;

	// need to refactor the most
	while (clientAlive) {
		bSuccess = GetQueuedCompletionStatus(hIOCP, &ioSize, (PDWORD_PTR)&lpConnectedSocket, (LPOVERLAPPED*)&lpOverlappped, INFINITE);
		std::cout << "Thread Id: " << GetCurrentThreadId() << "~~~~~~~~!\n";
		if (!bSuccess) {
			std::cout << "Error in GetQueuedCompletionStatues() " << GetLastError() << "\n";
			break;
		}
		lpIOInfo = (PIO_INFO_POP)lpOverlappped;
		if (lpConnectedSocket == NULL)
			return 0;
		if (!bSuccess || (bSuccess && (ioSize == 0))) {
			CloseClient(lpConnectedSocket, FALSE);
			continue;
		}
		if (WSAGetOverlappedResult(lpConnectedSocket->socket, lpOverlappped, &ioSize, TRUE, &dwFlags) == FALSE) {
			std::cout << "error in WSAGetOverlappedResult " << WSAGetLastError();
			continue;
		}
		// here the problem 
		switch (lpIOInfo->ioOperation) {
		case IORecvPOP:
			ZeroMemory(lpIOInfo->srv->dummyWSA.buf, lpIOInfo->srv->dummyWSA.len);
			if (!lpIOInfo->srv->authenticated) {
				CloseClient(lpConnectedSocket, FALSE);
				std::cout << "Not graceful end\n";
				clientAlive = false;
				break;
			}
			if (!(lpIOInfo->srv->ProcessFunction(lpIOInfo->srv->dummyWSA, lpIOInfo->wsaBuf))) {
				CloseClient(lpConnectedSocket, TRUE);
				clientAlive = false;
				break;
			}
			if ( static_cast<int>(lpIOInfo->srv->currentEtap) > static_cast<int>(POP3Part::Retriving) && static_cast<int>(lpIOInfo->srv->currentEtap) <= static_cast<int>(POP3Part::DotRN)) {
				lpIOInfo->ioOperation = IORecvPOP;
			}else
				lpIOInfo->ioOperation = IOSendPOP;
			if (WSASend(lpIOInfo->socket, &lpIOInfo->srv->dummyWSA, 1, &dwRecvNumBytes, dwFlags, &lpIOInfo->overlapped, NULL) == SOCKET_ERROR) {
				if (WSAGetLastError() != ERROR_IO_PENDING) {
					std::cout << "Error in Send " << WSAGetLastError() << "\n";
					return 1;
				}
			}
			break;
		case IOSendPOP:
			lpIOInfo->ioOperation = IORecvPOP;
			ZeroMemory(lpIOInfo->wsaBuf.buf, lpIOInfo->wsaBuf.len);
			if (WSARecv(lpIOInfo->socket, &lpIOInfo->wsaBuf, 1, &dwRecvNumBytes, &dwFlags, &lpIOInfo->overlapped, NULL) == SOCKET_ERROR) {
				if (WSAGetLastError() != ERROR_IO_PENDING) {
					std::cout << "Error in WSARecv " << WSAGetLastError() << "\n";
					return 1;
				}
			}		
			break;
		}
	}
	std::cout << "End with it\n";
	return 0;
}

VOID POP3Server::CloseClient(PSOCKET_CONNECTED_POP lpSocketConnected, BOOL graceful)
{
	EnterCriticalSection(&criticalSection);
	if (!graceful) {
		LINGER  lingerStruct;

		lingerStruct.l_onoff = 1;
		lingerStruct.l_linger = 0;
		setsockopt(lpSocketConnected->socket, SOL_SOCKET, SO_LINGER,
			(char*)&lingerStruct, sizeof(lingerStruct));
	}
	if (!lpSocketConnected->socket)
	{
		LeaveCriticalSection(&criticalSection);
		return;
	}
	closesocket(lpSocketConnected->socket);
	lpSocketConnected->socket = NULL;

	if (lpSocketConnected->nextSocket == NULL && lpSocketConnected->prevSocket == NULL) {

		while (!HasOverlappedIoCompleted(&lpSocketConnected->io->overlapped))
			Sleep(0);
		delete lpSocketConnected->io->srv;
		HeapFree(GetProcessHeap(), 0, lpSocketConnected->io);
		HeapFree(GetProcessHeap(), 0, lpSocketConnected);
		sockets = nullptr;
	}
	else if (lpSocketConnected->nextSocket == NULL && lpSocketConnected->prevSocket != NULL) {
		sockets = lpSocketConnected->prevSocket;

		while (!HasOverlappedIoCompleted(&lpSocketConnected->io->overlapped))
			Sleep(0);
		delete lpSocketConnected->io->srv;
		HeapFree(GetProcessHeap(), 0, lpSocketConnected->io);
		HeapFree(GetProcessHeap(), 0, lpSocketConnected);
	}
	else if (lpSocketConnected->nextSocket != NULL && lpSocketConnected->prevSocket == NULL) {
		lpSocketConnected->nextSocket->prevSocket = nullptr;

		while (!HasOverlappedIoCompleted(&lpSocketConnected->io->overlapped))
			Sleep(0);
		delete lpSocketConnected->io->srv;
		HeapFree(GetProcessHeap(), 0, lpSocketConnected->io);
		HeapFree(GetProcessHeap(), 0, lpSocketConnected);
	}
	else {
		lpSocketConnected->nextSocket->prevSocket = lpSocketConnected->prevSocket;
		lpSocketConnected->prevSocket->nextSocket = lpSocketConnected->nextSocket;

		while (!HasOverlappedIoCompleted(&lpSocketConnected->io->overlapped))
			Sleep(0);
		delete lpSocketConnected->io->srv;
		HeapFree(GetProcessHeap(), 0, lpSocketConnected->io);
		HeapFree(GetProcessHeap(), 0, lpSocketConnected);
	}
	LeaveCriticalSection(&criticalSection);
	return;
}
