#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "SMTPClient.h"

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct sockaddr_in clientService;
    int iResult;
    const char* serverName = "127.0.0.1"; // Change to your server's IP address
    const unsigned short serverPort = 25; // Change to your server's port
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed with error: " << iResult << std::endl;
        return 1;
    }
    ConnectSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (ConnectSocket == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    clientService.sin_family = AF_INET;
    clientService.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverName, &clientService.sin_addr);
    WSAPOLLFD listening = {};
    listening.fd = ConnectSocket;
    listening.events = POLLRDNORM;
    listening.revents = 0;
    iResult = connect(ConnectSocket, (sockaddr*)&clientService, sizeof(clientService));
    if (iResult == SOCKET_ERROR) {
        std::cerr << "Unable to connect to server: " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Connected to server!" << std::endl;
    WSABUF buffer;
    char data[8192];
    buffer.buf = data;
    buffer.len = 8000;
    SMTPClient client;
    client.currentEtap = SMTPClient::EHLO;
    std::cout << "Enter the username: ";
    std::cin >> client.username;
    
    std::cout << "Enter the password: ";
    std::cin >> client.password;
    
    client.mailFrom = client.username;
    std::cout << "Mail to: ";
    std::cin >> client.mailTo;
    
    std::cout << "Enter the mail:\n";
    std::cin >> client.mail;
    bool continueSending = true;
    while (continueSending) {
        while (true) {  
            if (WSAPoll(&listening, 1, 1) > 0) {
                if (listening.revents & POLLRDNORM) {
                    std::cout << "Read occured...\n";
                    iResult = recv(ConnectSocket, buffer.buf, buffer.len, 0);
                    if (iResult > 0) {
                        char* toPush = new char[iResult];
                        memcpy_s(toPush, iResult, buffer.buf, iResult);
                        std::string str(toPush, iResult);
                        str += "\0";
                        char* newBuffer = new char[strlen(client.recvReplyFromServer) + strlen(str.c_str()) + 1];
                        strcpy_s(newBuffer, strlen(client.recvReplyFromServer) + strlen(str.c_str()) + 1, client.recvReplyFromServer);
                        strcat_s(newBuffer, strlen(client.recvReplyFromServer) + strlen(str.c_str()) + 1, str.c_str());
                        delete[] client.recvReplyFromServer;
                        client.recvReplyFromServer = newBuffer;
                        //check for 3 last chars
                        char* twoChars = new char[2 + 1];
                        strncpy_s(twoChars, 4, client.recvReplyFromServer + strlen(client.recvReplyFromServer) - 2, 2);
                        if (strcmp(twoChars, "\r\n") == 0) {
                            std::cout << "Got the full buffer: " << client.recvReplyFromServer << "\n";
                            break;
                        }
                    }
                    else {
                        break;
                    }
                }
            }
        }
        if (client.currentEtap == SMTPClient::Sending) {
            int res;
            WSABUF forSendData;
            char* mutable_c_str = new char[strlen(client.mail.c_str())];
            memcpy_s(mutable_c_str, strlen(client.mail.c_str()), client.mail.c_str(), strlen(client.mail.c_str()));
            int df = strlen(client.mail.c_str());
            int size = strlen(client.mail.c_str());
            BOOL continueSending = TRUE;
            while (continueSending) {
                Sleep(2000);
                ZeroMemory(buffer.buf, buffer.len);
                forSendData.buf = mutable_c_str + (df - size);
                std::cout << "while sending: " << forSendData.buf << "\n=====\n";
                forSendData.len = size - (df - size);
                res = send(ConnectSocket, forSendData.buf, forSendData.len, 0);
                std::cout << "res " << res << "\n";
                size -= res;
                if (size <= 0) {
                    client.currentEtap = SMTPClient::PointAtTheEnd;
                    delete[] mutable_c_str;
                    continueSending = FALSE;
                }
            }
        }
        client.ProcessFunction(buffer, continueSending);
        if (!continueSending) {
            break;
        }
        iResult = send(ConnectSocket, buffer.buf, buffer.len, 0);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
            break;
        }
        std::cout << "Send the buffer: " << buffer.buf << "\n";
        Sleep(1000);
        if (client.currentEtap == SMTPClient::End) {
            break;
        }
    }
    system("pause");
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}
