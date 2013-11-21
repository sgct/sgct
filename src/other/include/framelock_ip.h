
/*
Modified from  Michael.Tirtasana@science-computing.de

*/


#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>


SOCKET server_socket;
SOCKET client_socket[16];

//A buffer to get error string
char localBuffer[1025];


//Return the error string from the error code
char* getLastErrorMessage(char* buffer, DWORD size, DWORD errorcode)
{
    memset(buffer, 0, size);
    if(FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorcode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)buffer,
        size, NULL) == 0){

            //failed in format message, let just do error code then
            sprintf(buffer,"Error code is %d", errorcode);
    }
    return buffer;
}

//http://www.tenouk.com/Winsock/Winsock2example4.html




int initWinsock()
{
    //initialize the winsock 2.2
    WSADATA  wsadata;
    if(WSAStartup(MAKEWORD(2,2), &wsadata)){
        printf("Failed to Startup Winsock\n");
        return -1; 
    }
    else
    {
        printf("Winsock started\n");
        return 1; 
    }

    ;
}


int startServer(unsigned int serverport)
{
    char serverportstr[64];
    struct addrinfo hints;
    struct addrinfo* addrs;
    SOCKET AcceptSocket;
    int tcpnodelay_flag = 1;
    /* set TCP_NODELAY to turn off Nagle's algorithm */

    sprintf(serverportstr,"%u",serverport);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if(getaddrinfo(NULL,(PCSTR) serverportstr, &hints, &addrs))
    {
        printf("Client: Error getaddrinfo %s\n", getLastErrorMessage(localBuffer, 1024, WSAGetLastError()));
        return 1;
    }

    //server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    server_socket = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);

    if (server_socket == INVALID_SOCKET)

    {
        printf("Server: Error socket(): %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
        WSACleanup();
        return -1;

    }
    else
    {
        printf("Server: socket() is OK!\n");
    }



    //if (bind(server_socket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
    if (bind(server_socket, addrs->ai_addr, (int)addrs->ai_addrlen) == SOCKET_ERROR)
    {
        printf("Server:  bind() failed: %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
        closesocket(server_socket);
        return -1;
    }
    else
    {
        printf("Server: bind() is OK!\n");
        printf("Server: Server IP: %s PORT: %u\n", inet_ntoa(((struct sockaddr_in*)addrs->ai_addr)->sin_addr),ntohs(((struct sockaddr_in*)addrs->ai_addr)->sin_port));
    }

    // Call the listen function, passing the created socket and the maximum number of allowed
    // connections to accept as parameters. Check for general errors.
    if (listen(server_socket, 10) == SOCKET_ERROR)
    {
        printf("Server: listen(): Error listening on socket  %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
        return -1;
    }
    else
    {
        printf("Server: listen() is OK, I'm waiting for connections...\n");
    }

    // Create a temporary SOCKET object called AcceptSocket for accepting connections.
    // Create a continuous loop that checks for connections requests. If a connection
    // request occurs, call the accept function to handle the request.
    printf("Server: Waiting for a client to connect...\n" );
    // Do some verification...
    while (1)
    {
        AcceptSocket = SOCKET_ERROR;
        while (AcceptSocket == SOCKET_ERROR)
        {
            AcceptSocket = accept(server_socket, NULL, NULL);
        }
        // else, accept the connection...
        // When the client connection has been accepted, transfer control from the
        // temporary socket to the original socket and stop checking for new connections.
        printf("Server: Client Connected!\n");
        server_socket = AcceptSocket;
        break;
    }

    if(setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &tcpnodelay_flag, sizeof(int)) == -1)
    {
        printf("Error setsockopt");
    }

    return 1;
}



int startClient(int numclient, char *client,unsigned int clientport)
{
    char clientportstr[64];
    struct addrinfo hints;
    struct addrinfo* addrs;
    int tcpnodelay_flag = 1;
    /* set TCP_NODELAY to turn off Nagle's algorithm */

    sprintf(clientportstr,"%u",clientport);


    //try to resolve the IP from hostname
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(client,(PCSTR) clientportstr, &hints, &addrs))
    {
        printf("Client: Error getaddrinfo %s\n", getLastErrorMessage(localBuffer, 1024, WSAGetLastError()));
        return 1;
    }
    else
    {
        struct addrinfo* paddr= addrs;
        while(paddr){
            printf("Client: Server IP: %s PORT: %u\n", inet_ntoa(((struct sockaddr_in*)paddr->ai_addr)->sin_addr),ntohs(((struct sockaddr_in*)paddr->ai_addr)->sin_port));
            paddr = paddr->ai_next;
        }
        printf("\n");
    }


    // Create a SOCKET for connecting to server
    client_socket[numclient] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket[numclient] == INVALID_SOCKET)
    {
        printf("Client: Error  socket():  %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
        WSACleanup();
        return -1;

    }
    else
    {
        printf("Client: socket() is OK.\n");
    }

    // Connect to server.
    if(connect(client_socket[numclient], addrs->ai_addr, sizeof(*(addrs->ai_addr))) == SOCKET_ERROR )
    {
        printf("Client: Error  connect():  %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
        WSACleanup();
        return -1;

    }
    else
    {
        printf("Client: connect() is OK.\n");
    }



    if(setsockopt(client_socket[numclient], IPPROTO_TCP, TCP_NODELAY, (char *) &tcpnodelay_flag, sizeof(int)) == -1)
    {
        printf("Error setsockopt");
    }




    return 1;
}


int sendToClientUint(int numclient,unsigned int data)
{
    if(send(client_socket[numclient] , (const char*) &data, sizeof(unsigned int), 0) == SOCKET_ERROR)
    {
        printf("Error sendToClientUint %d  reason %s\n", numclient ,getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
        Sleep(1000);
        return -1;
    }
    //printf("Client %d send() OK\n",numclient);

    return 1;
}
int recvFromClientUint(int numclient,unsigned int *data)
{
    if(recv(client_socket[numclient] , (char*) data, sizeof(unsigned int), 0) == SOCKET_ERROR)
    {
        printf("Error recvFromClientUint %d  reason %s\n", numclient ,getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
        Sleep(1000);
        return -1;
    }
    return 1;
}

int sendToServerUInt(unsigned int data)
{
    if(send(server_socket, (const char*) &data, sizeof(unsigned int), 0) == SOCKET_ERROR)
    {
        printf("Error sendToServerUInt reason %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
        Sleep(1000);
        return -1;
    }
    return 1;
}

int recvFromServerUInt(unsigned int *data)
{
    if(recv(server_socket, ( char*) data, sizeof(unsigned int), 0) == SOCKET_ERROR)
    {
        printf("Error recvFromServerUInt  reason %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
        Sleep(1000);
        return -1;
    }
    //printf("Client recv() OK\n");

    return 1;
}





