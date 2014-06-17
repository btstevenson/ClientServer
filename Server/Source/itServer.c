/*
* Course: CS 100 Winter 2014
*
* First Name: Brandon
* Last Name: Stevenson
* Username: bstev002
* email address: bstev002@ucr.edu
*
*
* Assignment: Homework #8 Iterative
*
* I hereby certify that the contents of this file represent
* my own original individual work. Nowhere herein is there
* code from any outside resources such as another individual,
* a website, or publishings unless specifically designated as
* permissible by the instructor or TA. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include "itServer.h"

#define PORT	27232
#define SPORT PORT
#define MAXDATA	300
#define BACKLOG 5

/***********************************
 *Function: WriteMsg()
 *Programmer: Brandon Stevenson
 *Date: 3/5/14
 *
 *Purpose: Handles error messages
 *		   with extra arguments.
 *		   
 *		   
 * ********************************/
void WriteMsg(int iCode, const char *szMessage, ...)
{
	va_list args;
	va_start(args, szMessage);

   /* checking for error message and printing if needed */
   if(iCode == MSG_ERROR)
   {
         vprintf(szMessage, args);
   }

   va_end(args);
}

/***********************************
 *Function: Messenger()
 *Programmer: Brandon Stevenson
 *Date: 3/5/14
 *
 *Purpose: Reads input from socket,
 *		   then executes the command
 *		   and returns the output to
 *		   client.
 * ********************************/
int Messenger(int iCliSocket)
{
	ER_RV 				Rval 					= ER_OK;
	char 				*pBuff					= NULL;
	char 				*pArgs[4];
	unsigned int 		uiMaxBytes				= 80;
	MsgHeader_t 		header;
	int  				iRead 					= 0;
	int 				iPid 					= 0;
	int 				status 					= 0;

	/* allocating memory for buffer */
	if((pBuff = (char *)malloc(uiMaxBytes)) == NULL)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "Memory allocation failed!\n");
	}
	memset(pBuff, 0, uiMaxBytes);

	/* initializing msg header */
	memset(&header, 0, sizeof(MsgHeader_t));

	/* reading from socket and error checking */
	iRead = read(iCliSocket, pBuff, sizeof(MsgHeader_t));
	if(iRead > 0 && iRead == sizeof(MsgHeader_t))
	{
		memcpy(&header, &pBuff[0], sizeof(MsgHeader_t));
	}
	else
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "read() failed! with error %s\n", strerror(errno));
	}
	/* setting up arg parameters */
	pArgs[0] = header.szProg;
	pArgs[1] = header.szOpt;
	pArgs[2] = header.szDir;
	pArgs[3] = '\0';
	/* calling fork to run the command on the directory */
	iPid = fork();
	switch(iPid)
	{
		case -1:
			Rval = ER_FAIL;
			ERR_MSG(MSG_ERROR,"fork() failed! with error %s\n", strerror(errno));
		case  0:
			dup2(iCliSocket, 0);
			dup2(iCliSocket, 1);
			dup2(iCliSocket, 2);
			if(execvp(pArgs[0], pArgs) == -1)
			{
				exit(1);
			}
			exit(0);
		default:
			close(iCliSocket);
			wait(&status);
	}

	ERR_END

	if(pBuff != NULL)
	{
		free(pBuff);
	}

	return Rval;
}

/***********************************
 *Function: main()
 *Programmer: Brandon Stevenson
 *Date: 3/5/14
 *
 *Purpose: Intializes socket for
 *		   connections. Will handle
 *		   each connection one at a
 * 		   time.
 * ********************************/
int main()
{
	ER_RV				Rval					= 0;
	int					iServSocket				= 0;
	int					iCliSocket				= 0;
	int 				iOpt 					= 1;
	socklen_t			iCliSize				= 0;
	struct sockaddr_in	server_addr;
	struct sockaddr_in	client_addr;

	memset(&server_addr, 0, sizeof(sockaddr_in));
	memset(&client_addr, 0, sizeof(sockaddr_in));

	/* creating socket and checking for error */
	iServSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(iServSocket == -1)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "Socket() failed! with error %s \n", strerror(errno));
	}
	printf("Server Socket initialized.\n");

	/* giving permission for socket to be reused */
	if(setsockopt(iServSocket, SOL_SOCKET, SO_REUSEADDR, &iOpt, sizeof(int)) < 0)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "setsockopt() failed! with error %s\n", strerror(errno));
	}
	/* setting up addr struct */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SPORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	/* binding socket and checking for error */
	if(bind(iServSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "bind() failed! with error %s \n", strerror(errno));
	}
	printf("bind() completed successfully.\n");
	
	/* setting server to listen and checking for error */
	if(listen(iServSocket, BACKLOG) == -1)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "listen() failed! with error %s \n", strerror(errno));
	}
	printf("Server is patiently waiting....\n");

    while(1)
    {
		/* accepting connection */
		iCliSize = sizeof(client_addr);
		iCliSocket = accept(iServSocket, (struct sockaddr *)&client_addr, &iCliSize);
		if(iCliSocket == -1)
		{
			Rval = ER_FAIL;
			ERR_MSG(MSG_ERROR, "accept() failed! with error %s \n", strerror(errno));
		}
		printf("server has accepted a client\n");
		/* handling client request */
		Rval = Messenger(iCliSocket);
		if(Rval != ER_OK)
		{
			printf("Messenger() failed!\n");
		}
		printf("Server is patiently waiting......\n");
	}

	ERR_END

	if(iCliSocket != 0)
	{
		close(iCliSocket);
	}
	if(iServSocket != 0)
	{
		close(iServSocket);
	}

	if(Rval != ER_OK)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

