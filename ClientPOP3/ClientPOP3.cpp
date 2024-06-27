#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "POP3Client.h"

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct sockaddr_in clientService;
    int iResult;
    const char* serverName = "127.0.0.1"; // Change to your server's IP address
    const unsigned short serverPort = 110; // Change to your server's port
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed with error: " << iResult << std::endl;
        return 1;
    }
    std::cout << "Enter how much mail you want to retrieve: ";
    int n; 
    std::cin >> n;
    std::string usernameEntered;
    std::string passwordEntered;
    std::cout << "Enter username: ";
    std::cin >> usernameEntered;
    std::cout << "Enter password: ";
    std::cin >> passwordEntered;
    bool authenticated = true;
    for (int count = 0; count < n; count++) {
        if (authenticated == false) {
            break;
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
        POP3Client client;
        client.currentEtap = POP3Client::UsernameSending;
        client.username = usernameEntered;
        client.password = passwordEntered;
        client.mailFrom = "140083@student.uni.opole.pl";
        client.mailTo = "HopeAndBelieve@mailfence.com";
        client.retrPos = count;
        bool continueSending = true;
        while (continueSending) {
            while (true) {
                if (WSAPoll(&listening, 1, 1) > 0) {
                    if (listening.revents & POLLRDNORM) {
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
                                if (client.currentEtap == POP3Client::Quit) {
                                    std::cout << "Current Etap: " << client.currentEtap << "\n";
                                    std::cout << "Got the mail:\n" << client.recvReplyFromServer << "\n";
                                }
                                break;
                            }
                        }
                        else {
                            break;
                        }
                    }
                }
            }
            client.ProcessFunction(buffer, continueSending);
            if (!continueSending) {
                break;
            }
            if (static_cast<int>(client.currentEtap) > static_cast<int>(POP3Client::RecvMailTo) && static_cast<int>(client.currentEtap) <= static_cast<int>(POP3Client::RecvDotRN)) {

            }
            else {
                iResult = send(ConnectSocket, buffer.buf, buffer.len, 0);
                if (iResult == SOCKET_ERROR) {
                    std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
                    break;
                }
            }

            Sleep(1000);
            if (client.currentEtap == POP3Client::End) {
                break;
            }
        }
        closesocket(ConnectSocket);
    }

    system("pause");
    WSACleanup();
    return 0;
}
