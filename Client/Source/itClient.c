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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "itClient.h"

#define 	PORT 27232

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

int BuffSetup(unsigned int *len, char *buf, const char *pProg, const char *pOpt, const char *pDir)
{
	ER_RV			Rval 		= ER_OK;
	MsgHeader_t 	header;

	memset(&header, 0, sizeof(MsgHeader_t));
	memcpy(&header.szProg[0], &pProg[0], sizeof(header.szProg));
	memcpy(&header.szOpt[0], &pOpt[0], sizeof(header.szOpt));
	memcpy(&header.szDir[0], &pDir[0], sizeof(header.szDir));

	memcpy(&buf[0], &header, sizeof(MsgHeader_t));
	*len = sizeof(MsgHeader_t);

	ERR_END

	return Rval;
}

int Connector(int iCliSocket, struct sockaddr_in server_addr, socklen_t iAddrLen, const char *args[])
{
	ER_RV 			Rval			= ER_OK;
	char			*pBuff 			= NULL;
	char			cDone			= '1';
	unsigned int 	uiMaxBytes		= 80;
	unsigned int 	uiMsgLen		= 0;
	int 			iRead 			= 0;

	if((pBuff = (char *)malloc(uiMaxBytes)) == NULL)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "Memory allocation failed!\n");
	}
	memset(pBuff, 0, uiMaxBytes);

	/*setting up buffer to send to server */
	Rval = BuffSetup(&uiMsgLen, &pBuff[0], args[2], args[3], args[4]);
	if(Rval != ER_OK)
	{
		ERR_MSG(MSG_ERROR, "BuffSetup() failed!\n");
	}

	printf("Attempting to connect to server at %s and port %d\n", args[1], PORT);
	/* attempting to connect to server and error checking */
	iAddrLen = sizeof(struct sockaddr);
	if(connect(iCliSocket, (struct sockaddr *)&server_addr, iAddrLen) < 0)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "connect() failed! with error %s\n", strerror(errno));
	}
	printf("client is now connected to server\n");

	/* writing to socket */
	if(write(iCliSocket, pBuff, uiMsgLen) == -1)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "write() failed! with error %s\n", strerror(errno));
	}
	memset(pBuff, 0, uiMaxBytes);

	/* getting data from socket */
	while(cDone == '1')
	{
		iRead = read(iCliSocket, pBuff, uiMaxBytes);
		if(iRead < 0)
		{
			Rval = ER_FAIL;
			ERR_MSG(MSG_ERROR, "read() failed! with error %s\n", strerror(errno));
		}
		else if(iRead > 0)
		{
			printf("%s", pBuff);
			memset(pBuff, 0, uiMaxBytes);
		}
		else
		{
			cDone = '\0';
		}
	}

	ERR_END

	if(pBuff != NULL)
	{
		free(pBuff);
	}

	return Rval;
}

int main(int argc, char const *argv[])
{
	ER_RV					Rval 				= 0;
	int 					iCliSocket			= 0;
	socklen_t				iAddrLen			= 0;
	struct sockaddr_in		server_addr;
	struct hostent			*pHostInfo			= NULL;

	if((pHostInfo = (struct hostent *)malloc(sizeof(struct hostent))) == NULL)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR,"Memory allocation failed!\n");
	}
	memset(pHostInfo, 0, sizeof(struct hostent));

	memset(&server_addr, 0, sizeof(sockaddr_in));
	/* checking for enough arguments */
	if(argc < 4)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "Not enough arguments!! Input goes as follows: hostname command directory\n");
	}

	/* setting host name  and error checking */
	if((pHostInfo = gethostbyname(argv[1])) == NULL)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "gethostbyname() failed! for host name %s and the error was %s\n", argv[1], strerror(errno));
	}
	printf("Hostname is: %s\n", argv[1]);
	/* setting socket and error checking */
	iCliSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(iCliSocket < 0)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "socket() failed! with error %s", strerror(errno));
	}
	printf("Client socket is set\n");

	/* setting up addr struct with server info */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	memcpy(&server_addr.sin_addr, pHostInfo->h_addr_list[0], pHostInfo->h_length);

	Rval = Connector(iCliSocket, server_addr, iAddrLen, argv);
	if(Rval != ER_OK)
	{
		ERR_MSG(MSG_ERROR, "Connector() failed!\n");
	}
	
	ERR_END

	if(iCliSocket != 0)
	{
		close(iCliSocket);
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