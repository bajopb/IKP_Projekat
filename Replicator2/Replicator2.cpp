#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <string.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <winsock.h>
#include <tchar.h>
#include "..\Common\ReplicatorList.h"
#include "..\Common\ProcessList.h"

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27017"
#define DEFAULT_PORT_R2 27017
#define GUID_FORMAT "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX"
#define GUID_ARG(guid) guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]

bool InitializeWindowsSockets();

DWORD WINAPI handleSocket(LPVOID lpParam);
DWORD WINAPI handleConnectSocket(LPVOID lpParam);
DWORD WINAPI handleData(LPVOID lpParam);


char* guidToString(const GUID* id, char* out);
GUID stringToGUID(const std::string& guid);

char recvbuf[DEFAULT_BUFLEN];

NODE_REPLICATOR* head;
NODE_PROCESS* headProcessReceive;
NODE_PROCESS* headProcessSend;
SOCKET replicatorSocket = INVALID_SOCKET;
// Socket used for communication with client
SOCKET acceptedSockets[10];

NODE_PROCESS* processList;

int  main(void)
{
    int numOfProcess = 0;
    // Socket used for listening for new clients 
    SOCKET listenSocket = INVALID_SOCKET;
   
    InitProcessList(&processList);
   
    
    // variable used to store function return value
    int iResult;
    // Buffer used for storing incoming data
    

    if (InitializeWindowsSockets() == false)
    {
        // we won't log anything since it will be logged
        // by InitializeWindowsSockets() function
        return 1;
    }

    // Prepare address information structures
    addrinfo* resultingAddress = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // 

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    listenSocket = socket(AF_INET,      // IPv4 address famly
        SOCK_STREAM,  // stream socket
        IPPROTO_TCP); // TCP

    if (listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket - bind port number and local address 
    // to socket
    iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddress);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }



    // Since we don't need resultingAddress any more, free it
    //freeaddrinfo(resultingAddress);

    // Set listenSocket in listening mode
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    SOCKET connectSocket = INVALID_SOCKET;
    // variable used to store function return value
    //int iResult;
    // message to send
    //char* messageToSend = "";

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

    // create and initialize address structure
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(27016);
    // connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
    }

    DWORD funId;

    CreateThread(NULL, 0, &handleConnectSocket, &connectSocket, 0, &funId);

#pragma endregion 

    printf("Waiting connection with Replicator1...\n");
    int numberOfClients = 0;

    replicatorSocket = accept(listenSocket, NULL, NULL);

    if (replicatorSocket == INVALID_SOCKET)
    {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    else
    {
        printf("Connection with Replicator1 established.\n");
        printf("Server initialized, waiting for clients.\n");
    }
    do
    {
        // Wait for clients and accept client connections.
        // Returning value is acceptedSocket used for further
        // Client<->Server communication. This version of
        // server will handle only one client.
        acceptedSockets[numberOfClients] = accept(listenSocket, NULL, NULL);

        if (acceptedSockets[numberOfClients] == INVALID_SOCKET)
        {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        //POKRETANJE NITI ZA SVAKOG KLIJENTA(PROCES)

        DWORD funId[10];
        HANDLE handle[10];

        PROCESS processAdd[10];
        GUID Id;
        CoCreateGuid(&Id);
        processAdd[numberOfClients] = InitProcess(Id, acceptedSockets[numberOfClients], numOfProcess);
        numOfProcess++;
        //nit koja radi samo sa ovim procesom
        handle[numberOfClients] = CreateThread(NULL, 0, &handleSocket, &processAdd[numberOfClients], 0, &funId[numberOfClients]);
        CloseHandle(handle[numberOfClients]);

        numberOfClients++;

        // here is where server shutdown loguc could be placed

    } while (1);

    closesocket(listenSocket);

    for (int i = 0; i < 10; i++)
    {
        iResult = shutdown(acceptedSockets[i], SD_SEND);
        if (iResult == SOCKET_ERROR)
        {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(acceptedSockets[i]);
            WSACleanup();
            return 1;
        }

        closesocket(acceptedSockets[i]);
    }
    WSACleanup();

    return 0;
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

/*
    FUNKCIJA: handleSocket
    FUNKCIONALNOST: Stavlja soket u non-blocking mode, stavlja proces u red, registruje proces, salje podatke
    POVRATNA VREDNOST: U zavisnosti od odabira akcije salje podatke, registruje proces...
*/
DWORD WINAPI handleSocket(LPVOID lpParam)
{
    PROCESS* process = (PROCESS*)lpParam;
    SOCKET acceptedSocket = process->acceptedSocket;
    GUID Id = process->processId;
    int iResult;
    char recvbuf[512];

    if (IsSocketNull(&head))
    {
        *process = InitProcess(Id, acceptedSocket, 0);
        AddSocketToID(&head, &process);
    }

    unsigned long mode = 1; //non-blocking mode
    iResult = ioctlsocket(acceptedSocket, FIONBIO, &mode);
    if (iResult != NO_ERROR)
        printf("ioctlsocket failed with error: %ld\n", iResult);

    fd_set readfds;
    FD_ZERO(&readfds);
    char poruka[] = "Primili smo vasu poruku";
    do {
        fd_set readfds;
        FD_ZERO(&readfds);
        // Receive data until the client shuts down the connection
        FD_SET(acceptedSocket, &readfds);
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

        else if (FD_ISSET(acceptedSocket, &readfds)) {
            char output[DEFAULT_BUFLEN];
            iResult = recv(acceptedSocket, recvbuf, DEFAULT_BUFLEN, 0);
            if (iResult > 0) {
                if (recvbuf[0] == '+') {
                    printf("Primljena poruka: %s\n", recvbuf);
                  /*  char temp[DEFAULT_BUFLEN];
                    memcpy(temp, &recvbuf[1], (int)strlen(recvbuf)+1);*/

                    send(acceptedSockets[process->index], recvbuf, (int)strlen(recvbuf)+1, 0);
                }
                else {
                    guidToString(&process->processId, output);
                    printf("Primljena poruka: %s, a tvoj indeks je %d\n", recvbuf, process->index);
                    DATA data = InitData(recvbuf, 1);
                    PushProcess(&processList, data);

                    PrintAllData(&processList);
                    char temp[DEFAULT_BUFLEN];
                    memcpy(&temp[1], recvbuf, (int)strlen(recvbuf));
                    temp[0] = '+';
                    send(replicatorSocket, recvbuf, (int)strlen(recvbuf) + 1, 0);
                }
            }
           
        }
    } while (1);

}

DWORD WINAPI handleConnectSocket(LPVOID lpParam)
{
    SOCKET* acceptedSocket = (SOCKET*)lpParam;
    while (1) {
        int iResult = recv(*acceptedSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {

            printf("Primljena poruka: %s\n", recvbuf);
            send(acceptedSockets[0], recvbuf, (int)strlen(recvbuf) + 1, 0);
        }
    }
    return 0;

}

char* guidToString(const GUID* id, char* out) {
    int i;
    char* ret = out;
    out += sprintf(out, "%.8lX-%.4hX-%.4hX-", id->Data1, id->Data2, id->Data3);
    for (i = 0; i < sizeof(id->Data4); ++i) {
        out += sprintf(out, "%.2hhX", id->Data4[i]);
        if (i == 1) *(out++) = '-';
    }
    return ret;
}
