#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <conio.h>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <combaseapi.h>
#include "..\Common\ProcessList.h"
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27016
#define DEFAULT_PORT_R2 27017

#define SERVER_IP_ADDERESS "127.0.0.1"
#define OUTGOING_BUFFER_SIZE 1024

bool InitializeWindowsSockets();
void RegisterProcess(SOCKET connectSocket, int i);
void SendData(SOCKET connectSocket, char* i);

char* guidToString(const GUID* id, char* out);
GUID stringToGUID(const std::string& guid);

DWORD WINAPI handleIncomingData(LPVOID lpParam);

NODE_PROCESS* headProcess;

int __cdecl main(int argc, char** argv)
{
    // socket used to communicate with server
    SOCKET connectSocket = INVALID_SOCKET;
    // variable used to store function return value
    int iResult;
    // message to send
    char messageToSend[OUTGOING_BUFFER_SIZE];

    // Validate the parameters
   /* if (argc != 2)
    {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }*/

    if (InitializeWindowsSockets() == false)
    {
        // we won't log anything since it will be logged
        // by InitializeWindowsSockets() function
        return 1;
    }

    // create a socket
    connectSocket = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("Da li zelite replikator 1 ili 2: ");
    int serNum = 0;
    int port;
    scanf("%d", &serNum);
    if (serNum == 1)
        port = DEFAULT_PORT;
    else
        port = DEFAULT_PORT_R2;

    gets_s(messageToSend, OUTGOING_BUFFER_SIZE);
    // create and initialize address structure
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP_ADDERESS);
    serverAddress.sin_port = htons(port);
    // connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
    }
    printf("Povezano na server %d\n", serNum);
    // Send an prepared message with null terminator included
    HANDLE handle;

    handle = CreateThread(NULL, 0, &handleIncomingData, &connectSocket, 0, NULL);

    while (1) {

        printf("Mogu se slati poruke: ");
        gets_s(messageToSend, OUTGOING_BUFFER_SIZE);

        iResult = send(connectSocket, messageToSend, (int)strlen(messageToSend) + 1, 0);

        if (iResult == SOCKET_ERROR)
        {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }
        memset(messageToSend, 0, strlen(messageToSend));
        //iResult = recv(connectSocket, messageToSend, DEFAULT_BUFLEN, 0);
        /*if (iResult > 0) {
            printf("Primljena poruka: %s\n", messageToSend);
        }*/

    }
    printf("Bytes Sent: %ld\n", iResult);

    // cleanup
    closesocket(connectSocket);
    WSACleanup();

    return 0;
}

DWORD WINAPI handleIncomingData(LPVOID lpParam) {


    SOCKET* connectSocket = (SOCKET*)lpParam;

    int iResult;
    char messageBuffer[DEFAULT_BUFLEN];

    while (true)
    {
        fd_set readfds;
        FD_ZERO(&readfds);

        FD_SET(*connectSocket, &readfds);
        timeval timeVal;
        timeVal.tv_sec = 2;
        timeVal.tv_usec = 0;
        int result = select(0, &readfds, NULL, NULL, &timeVal);

        if (result == 0)
        {
            // vreme za cekanje je isteklo
        }
        else if (result == SOCKET_ERROR)
        {
            //desila se greska prilikom poziva funkcije
        }
        else if (FD_ISSET(*connectSocket, &readfds))
        {
            // rezultat je jednak broju soketa koji su zadovoljili uslov
            iResult = recv(*connectSocket, messageBuffer, DEFAULT_BUFLEN, 0);
            if (iResult > 0)
            {
                printf("\nReplikator poslao: %s\n", messageBuffer);

            }
        }
        else if (iResult == 0)
        {
            // connection was closed gracefully
            printf("Connection with Replicator closed.\n");

            closesocket(*connectSocket);
        }
        else
        {
            // there was an error during recv
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(*connectSocket);
        }
    }
}

bool InitializeWindowsSockets()
{
    WSADATA wsaData;
    // Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}
