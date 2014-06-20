/*
* Course: CS 100 Winter 2014
*
* First Name: Brandon
* Last Name: Stevenson
* Username: bstev002
* email address: bstev002@ucr.edu
*
*
* Assignment: Homework #8 Concurrent
*
* I hereby certify that the contents of this file represent
* my own original individual work. Nowhere herein is there
* code from any outside resources such as another individual,
* a website, or publishings unless specifically designated as
* permissible by the instructor or TA. */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "conServer.h"

#define			NUM_THREADS		10
#define 		BACKLOG 		10
#define 		MAX_BYTES 		4096

pthread_mutex_t 	count_mutex;

/***********************************
 *Function: WriteMsg()
 *Programmer: Brandon Stevenson
 *Date: 3/7/14
 *
 *Purpose: Handles error messages
 *		   and will print additional
 *		   arguments
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
 *Function: DirSend()
 *Programmer: Brandon Stevenson
 *Date: 3/7/14
 *
 *Purpose: Initalizes message 
 *		   header for a directory
 *		   and sends to client
 *
 * ********************************/
void DirSend(int sockID, const char *pDir)
{
	unsigned int 	uiLength 			= 0;
	unsigned int 	uiBytesWrote 		= 0;
	int 			iWrite 				= 0;
	MsgHeader_t		*header 			= NULL;

	if((header = (MsgHeader_t *)malloc(sizeof(MsgHeader_t))) == NULL)
	{
		ERR_MSG(MSG_ERROR, "malloc() failed!\n");
	}
	memset(&header[0], 0, sizeof(MsgHeader_t));

	/* getting length of directory path */
	while(pDir[uiLength] != '\0')
	{
		uiLength++;
	}
	/* setting up message header */
	memcpy(&header->szDirName[0], &pDir[0], uiLength);
	header->uiDataType = DIRDATA;
	uiLength = sizeof(MsgHeader_t);
	while(uiLength > 0)
	{
		/* writing to socket */
		iWrite = write(sockID, &header[uiBytesWrote], uiLength);
		if(iWrite < 0)
		{
			ERR_MSG(MSG_ERROR, "write() failed! directory not sent!");
		}
		uiLength -= iWrite;
		uiBytesWrote += iWrite;
	}

	ERR_END;

	if(header != NULL)
	{
		free(header);
	}

}

/***********************************
 *Function: FileSend()
 *Programmer: Brandon Stevenson
 *Date: 3/7/14
 *
 *Purpose: Opens file and reads
 *		   data into memory.
 *		   Creates message header
 *		   for a file and sends to
 * 		   client
 *
 * ********************************/
void FileSend(int sockID, unsigned int uiFileSize, const char *pPath)
{	
	FILE 			*pFile 			= NULL;
	char 			*pHold  		= NULL;
	char 			*pMsg 			= NULL;
	long			lFileLength 	= 0;
	int 			iRead 			= 0;
	unsigned int 	uiIdx 			= 0;
	unsigned int 	uiNameLength	= 0;
	unsigned int 	uiPathLength 	= 0;
	unsigned int 	uiBytesRead 	= 0;
	MsgHeader_t 	*header;

	if((header = (MsgHeader_t *)malloc(sizeof(MsgHeader_t))) == NULL)
	{
		ERR_MSG(MSG_ERROR, "malloc() failed!\n");
	}
	memset(&header[0], 0, sizeof(MsgHeader_t));

	/* getting path length */
	while(pPath[uiPathLength] != '\0')
	{
		uiPathLength++;
	}
	/* opening file for reading */
	pFile = fopen(pPath, "rb");
	if(pFile == NULL)
	{
		ERR_MSG(MSG_ERROR, "could not open file %s and error is %s\n", pPath, strerror(errno));
	}
	/* allocating a buffer to hold file data */
	if((pHold = (char *)malloc(uiFileSize)) == NULL)
	{
		ERR_MSG(MSG_ERROR, "malloc failed!, file will not be sent\n");
	}
	memset(pHold, 0, lFileLength);
	/* setting up header */
	memcpy(&header->szFileName[0], &pPath[0], uiPathLength);
	header->uiDataType = FILEDATA;
	header->uiFileLength = uiFileSize;

	/* reading data from file into memory */
	iRead = fread(pHold, sizeof(char), uiFileSize, pFile);
	if(iRead == 0)
	{
		ERR_MSG(MSG_ERROR, "error reading file %s file will not be sent\n", pPath);
	}
	fclose(pFile);

	/* allocating memory for full message to be send */
	if((pMsg = (char *)malloc(sizeof(MsgHeader_t) + uiFileSize)) == NULL)
	{
		ERR_MSG(MSG_ERROR, "malloc failed! file will not be sent");
	}
	memset(pMsg, 0, sizeof(MsgHeader_t) + uiFileSize);

	/* setting up message buffer */
	memcpy(pMsg + uiIdx, &header[0], sizeof(MsgHeader_t));
	uiIdx += sizeof(MsgHeader_t);
	memcpy(pMsg + uiIdx, &pHold[0], uiFileSize);
	uiIdx += uiFileSize;

	while(uiIdx > 0)
	{
		/* writing data to socket */
		iRead = write(sockID, &pMsg[uiBytesRead], uiIdx);
		if(iRead < 0)
		{
			ERR_MSG(MSG_ERROR, "write() failed!, file will not be sent\n");
		}
		uiIdx -= iRead;
		uiBytesRead += iRead;
	}

	ERR_END

	if(pHold != NULL)
	{
		free(pHold);
	}
	if(pMsg != NULL)
	{
		free(pMsg);
	}
}

/***********************************
 *Function: lsRun()
 *Programmer: Brandon Stevenson
 *Date: 3/7/14
 *
 *Purpose: Will open file or directory.
 *		   If a directory with subdirectory
 *		   will call those recursively.
 *
 * ********************************/
int lsRun(const char *szOperand, int iLevel, int sockID)
{
	ER_RV 			Rval 		= ER_OK;
	DIR 			*pDir		= NULL;
	char 			*pBuff 		= NULL;
	struct dirent 	*pFile 		= NULL;
	struct stat 	Stat;
	bool			fFile		= false;
	static int 		iSwitch 	= 0;

	if((pBuff = (char*)malloc(strlen(szOperand) + NAME_MAX + 4)) == NULL)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "Memory allocation failed!\n");
	}
	memset(&pBuff[0], 0, strlen(szOperand) + NAME_MAX + 4);

	/* checking for recursive call and checking if file is operand */
	if(iSwitch == 0 || iLevel == 0)
	{
		memcpy(&pBuff[0], &szOperand[0], strlen(szOperand));
		if(stat(pBuff, &Stat) == -1)
		{
			Rval = ER_FAIL;
			ERR_MSG(MSG_ERROR, "stat() failed! with error %s\n", strerror(errno));
		}
		if(S_ISREG(Stat.st_mode))
		{
			fFile = true;
		}
	}

	if(!fFile)
	{
		//if(iSwitch == 0)
		//{
		//	DirSend(sockID, szOperand);
		//}
		iSwitch = 1;
		/* opening stream to directory */
		if((pDir = opendir(szOperand)) == NULL)
		{
			if(errno != EACCES)
			{
				Rval = ER_FAIL;
				ERR_MSG(MSG_ERROR, "opendir() failed! with error %s\n", strerror(errno));
			}
			printf("could not open directory %s, access denied!!\n", szOperand);
		}
		else
		{
			while((pFile = readdir(pDir)) != NULL)
			{
				if(pFile->d_type == DT_DIR)
				{
					char	szPath[1024];
					int iPathLen = snprintf(szPath, sizeof(szPath)-1, "%s/%s", szOperand, pFile->d_name);
					szPath[iPathLen] = 0;
					if(strcmp(pFile->d_name, ".") == 0 || strcmp(pFile->d_name, "..") == 0)
						continue;
					/* sending dir path to client */
					DirSend(sockID, szPath);

					/* recursive call for directory */
					Rval = lsRun(szPath, iLevel+1, sockID);
					if(Rval != ER_OK)
					{
						ERR_MSG(MSG_ERROR, "lsRun() failed!\n");
					}
				}

				if(pFile->d_type == DT_REG)
				{
					sprintf(pBuff, "%s/%s", szOperand, pFile->d_name);
					if((stat(pBuff, &Stat)) == -1)
					{
						Rval = ER_FAIL;
						ERR_MSG(MSG_ERROR, "stat() failed! with error %s\n", strerror(errno));
					}
					/* sending file path and size to client */
					FileSend(sockID, Stat.st_size, pBuff);
				}
			}
			Rval = closedir(pDir);
			if(Rval != ER_OK)
			{
				ERR_MSG(MSG_ERROR, "closedir() failed! with error %s", strerror(errno));
			}
		}
	}

	ERR_END

	if(iLevel == 0)
	{
		iSwitch = 0;
	}
	if(pBuff != NULL)
	{
		free(pBuff);
	}
	return Rval;
}

/***********************************
 *Function: ParseBuff()
 *Programmer: Brandon Stevenson
 *Date: 3/7/14
 *
 *Purpose: Parses client input into
 *		   pieces for use in finding
 *		   directory
 *
 * ********************************/
int ParseBuff(char *pBuff, char *pDirHold, unsigned int uiBuffLen)
{
	ER_RV 				Rval 		= ER_OK;

	memcpy(&pDirHold[0], &pBuff[0], uiBuffLen);

	ERR_END

	return Rval;
}

/***********************************
 *Function: ThreadWork()
 *Programmer: Brandon Stevenson
 *Date: 3/7/14
 *
 *Purpose: Thread does reading of
 *		   initial client input
 *		   and then performs directory
 *		   find.
 *
 * ********************************/
void *ThreadWork(void *socketdesc)
{
	#define SOCKPTR ((SockDesc_t *) socketdesc)
	ER_RV			Rval 			= ER_OK;
	char 			*pBuff 			= NULL;
	char 			*pHold 			= NULL;
	char			cDone			= '1';
	int 			iRead 			= 0;
	unsigned int 	uiIdx 			= 0;
	unsigned int 	uiBytesRead 	= 0;
	unsigned int 	uiMsgLength 	= 0;


	printf("new thread created!!!\n");
	if((pBuff = (char *)malloc(MAX_BYTES + 1)) == NULL)
	{
		ERR_MSG(MSG_ERROR, "Malloc failed! and thread will exit with no work done\n");
	}
	memset(pBuff, 0, MAX_BYTES + 1);

	if((pHold = (char *)malloc(MAX_BYTES *2)) == NULL)
	{
		ERR_MSG(MSG_ERROR, "Malloc failed! and thread will exit with no work done\n");
	}
	memset(pHold, 0, MAX_BYTES * 2);

	/* reading intial message from client */

	while(cDone == '1')
	{
		iRead = read(SOCKPTR->CliSockDesc, pBuff, MAX_BYTES);
		if(iRead < 0)
		{
			ERR_MSG(MSG_ERROR, "read() failed! with error %s\n", strerror(errno));
		}
		if(iRead > 0 && iRead == MAX_BYTES)
		{
			memcpy(&pHold[uiBytesRead], pBuff, iRead);
			uiBytesRead += MAX_BYTES;
		}
		else
		{
			if(iRead > 0)
			{
				memcpy(&pHold[uiBytesRead], pBuff, iRead);
				uiBytesRead += iRead;
			}
			cDone = '\0';

		}
	}
	memset(pBuff, 0, MAX_BYTES);
	/* setting up buffer to send requested directory */
	Rval = ParseBuff(&pHold[0], &pBuff[0], uiBytesRead);
	/* getting reuqested directory and all subcontents */
	DirSend(SOCKPTR->CliSockDesc, pBuff);
	Rval = lsRun(pBuff, 0, SOCKPTR->CliSockDesc);
	ERR_END
	/* cleaning up */
	if(pBuff != NULL)
	{
		free(pBuff);
	}
	close(SOCKPTR->CliSockDesc);
	free(SOCKPTR);
	pthread_exit(0);


}
/***********************************
 *Function: main()
 *Programmer: Brandon Stevenson
 *Date: 3/7/14
 *
 *Purpose: Intializes socket for
 *		   connections. Will allow
 *		   up to 10 connections at
 *		   at a time. 
 * ********************************/
int main()
{
	ER_RV 				Rval 			= ER_OK;
	int 				iOpt 			= 1;
	int 				i 				= 0;
	int 				iServSocket		= 0;
	socklen_t			iCliSize 		= 0;
	int 				iCliSocket		= 0;
	pthread_t 			threads;
	SockDesc_t			*infosock;
	struct sockaddr_in	server_addr;
	struct sockaddr_in	client_addr;

	pthread_attr_t 		attr;

	memset(&server_addr, 0, sizeof(sockaddr_in));
	memset(&client_addr, 0, sizeof(sockaddr_in));
	memset(&infosock, 0, sizeof(SockDesc_t));

	pthread_mutex_init(&count_mutex, NULL);
	pthread_attr_init(&attr);
	/* want threads to detach when done */
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	/* setting up socket */
	if((iServSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "socket() failed! with error %s\n", strerror(errno));
	}
	printf("socket initalized.\n");

	/* giving permission for socket to be reused */
	if(setsockopt(iServSocket, SOL_SOCKET, SO_REUSEADDR, &iOpt, sizeof(int)) < 0)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "setsockopt() failed! with error %s\n", strerror(errno));
	}

	/* setting up addr */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	/* binding socket */
	if(bind(iServSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "bind() failed! with error %s\n", strerror(errno));
	}
	printf("bind completed.\n");
		/* server is listening */
	if(listen(iServSocket, BACKLOG) < 0)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "listen() failed! with error %s\n", strerror(errno));
	}
	printf("server is patiently waiting.....\n");

	while(1)
	{
		/* accepting client connection */
		iCliSocket = accept(iServSocket, NULL, NULL);
		if(iCliSocket > 0)
		{
			infosock = (SockDesc_t *)malloc(sizeof(SockDesc_t));
			infosock->CliSockDesc = iCliSocket;
			if(pthread_create(&threads, &attr, ThreadWork, infosock) != 0)
			{
				printf("pthread_create() failed!\n");
			}
		}
		printf("server is patiently waiting......\n");
	}

	ERR_END

	if(Rval != ER_OK)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}