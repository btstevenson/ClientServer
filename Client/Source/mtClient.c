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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include "mtClient.h"

#define 	MAX_BYTES 	4096
#define 	DIR_MODE 	0777
#define 	NUMTHREADS	10

char		szWorkingPath[MAX_BYTES];
pthread_mutex_t count_mutex;

/* error message handler */
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

/* copies user inputed directory for server */
int BuffSetup(unsigned int *len, char *buf, const char *arg)
{
	ER_RV			Rval 		= ER_OK;
	unsigned int 	uiBuflen 	= 0;
	unsigned int 	uiIdx 		= 0;
	MsgHeader_t		header;				/* message header that is sent on each transaction */

	memset(&header, 0, sizeof(MsgHeader_t));

	while(arg[uiBuflen] != '\0')
	{
		uiBuflen++;
	}
	/* setting up message header */
	memcpy(&header.szDirName[0], &arg[0], uiBuflen);
	*len = sizeof(MsgHeader_t);
	memcpy(buf, &header, sizeof(MsgHeader_t));

	ERR_END

	return Rval;
}

/* creates a new directory in current work directory */
int  DirInfo(char *pInfo)
{
	ER_RV 			Rval 	 			= ER_OK;
	char			szPath[MAX_BYTES];
	MsgHeader_t 	header;
	char 			*pTemp				= NULL;

	memset(&szPath[0], 0, MAX_BYTES);
	memset(&header, 0, sizeof(MsgHeader_t));

	/* copying working path */
	strcpy(szPath, szWorkingPath);
	memcpy(&header, &pInfo[0], sizeof(MsgHeader_t));
	pTemp = header.szDirName;
	/* adding path for new directory to working */
	strcat(szPath, pTemp);
	if(mkdir(szPath, DIR_MODE) < 0)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "mkdir() failed! for path %s with error %s\n", szPath, strerror(errno));
	}
	printf("created directory %s\n", szPath);
	

	ERR_END

	return Rval;
}

/* reads data from memory into a file */
int FileInfo(int sockID, char *pInfo, unsigned int uiInfoLen)
{
	ER_RV 			Rval 					= ER_OK;
	char 			szPath[MAX_BYTES];
	char 			*pHold					= NULL;
	MsgHeader_t 	header;
	unsigned int 	uiBytesRead 			= 0;
	unsigned int 	uiBytesLeft 			= 0;
	int 			iRead 					= 0;
	unsigned int 	uiWrote 				= 0;
	FILE 			*pFile 					= NULL;

	memset(&szPath[0], 0, MAX_BYTES);
	memset(&header, 0, sizeof(MsgHeader_t));
	memcpy(&header, &pInfo[0], sizeof(MsgHeader_t));

	/* adding file path to working path */
	strcpy(szPath, szWorkingPath);
	strcat(szPath, header.szFileName);
	/* do create file stuff here and write to file */
	printf("File is: %s\n", header.szFileName);

	/* opening file for writing */
	pFile = fopen(szPath, "w");
	if(pFile == NULL)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "could not create file %s\n", header.szFileName);
	}

	/* allocating memory for reading data */
	if((pHold = (char *)malloc(header.uiFileLength)) == NULL)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "malloc() failed!\n");
	}
	/* reading from socket up to designate filelength */
	uiBytesLeft = header.uiFileLength;
	while(uiBytesLeft > 0)
	{
		iRead = read(sockID, &pHold[uiBytesRead], uiBytesLeft);
		if(iRead < 0)
		{
			Rval = ER_FAIL;
			ERR_MSG(MSG_ERROR, "read() failed!\n");
		}
		uiBytesLeft -= iRead;
		uiBytesRead += iRead;
	}
	/* writing data to file */
	uiWrote = fwrite(pHold, sizeof(char), header.uiFileLength, pFile);
	if(uiWrote != header.uiFileLength)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "fwrite() failed!\n");
	}
	fclose(pFile);

	ERR_END

	if(pHold != NULL)
	{
		free(pHold);
	}

	return Rval;
}

/* handles messages from server and sends content to appropriate call */
int Messenger(int iSocket, char *pBuff, unsigned int uiMsgLen)
{
	ER_RV 			Rval 			= ER_OK;
	int 			iRead 			= 0;
	MsgHeader_t		header;
	char 			cDone 			= '1';
	unsigned int 	uiIdx 			= 0;

	/*modify to read in msgheader first */

	while(uiMsgLen > 0)
	{
		iRead = write(iSocket, pBuff, uiMsgLen);
		if(iRead < 0)
		{
			Rval = ER_FAIL;
			ERR_MSG(MSG_ERROR, "write() failed!");
		}
		uiMsgLen -= iRead;
		pBuff += iRead;
	}
	memset(pBuff, 0, MAX_BYTES);

	while(cDone == '1')
	{
		/* reading from socket */
		iRead = read(iSocket, pBuff, sizeof(MsgHeader_t));
		if(iRead < 0)
		{
			Rval = ER_FAIL;
			ERR_MSG(MSG_ERROR, "read() failed! with error %s\n", strerror(errno));
		}
		else
		{	
			/* only wanted message header for now */
			if(iRead > 0 && iRead == sizeof(MsgHeader_t))
			{
				memcpy(&uiIdx, pBuff, sizeof(unsigned int));
				/* checking for dir data or file data */
				if(uiIdx == DIRDATA)
				{
					Rval = DirInfo(pBuff);
				}
				else
				{
					Rval = FileInfo(iSocket, pBuff, uiIdx);
				}
				/* will need to zero everything as new block is coming */
				uiIdx = 0;
				memset(pBuff, 0, MAX_BYTES);
			}
			else
			{
				cDone = '\0';
			}
		}
	}

	ERR_END

	return Rval;
}


void *Connect(void *clidescrip)
{
	pthread_mutex_lock(&count_mutex);
	#define ADDRPTR ((CliDesc_t *) clidescrip)
	ER_RV			Rval 			= ER_OK;
	int 			iCliSocket		= 0; 
	socklen_t 		iAddrLen 		= 0;
	char 			szThreadID[5];
	char 			*pBuff 			= NULL;

	if((pBuff = (char *)malloc(MAX_BYTES)) == NULL)
	{
		ERR_MSG(MSG_ERROR, "malloc() failed!\n");
	}
	memset(pBuff, 0,MAX_BYTES);

	/* intializing socket */
	iCliSocket = socket(PF_INET, SOCK_STREAM, 0);
	if(iCliSocket < 0)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "socket() failed! with error %s\n", strerror(errno));
	}
	printf("Socket is set.\n");

	iAddrLen = sizeof(struct sockaddr);
	if(connect(iCliSocket, (struct sockaddr *)ADDRPTR->serveraddr, iAddrLen) < 0)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "connect() failed! with error %s\n", strerror(errno));
	}
	printf("client is now connected to server\n");

	/* get cwd and assign thread label */
	/* getting current working directory */
	getcwd(&szWorkingPath[0], MAX_BYTES);
	strcat(szWorkingPath, "/");
	sprintf(szThreadID, "%d", ADDRPTR->threadID);
	strcat(szWorkingPath, szThreadID);
	mkdir(szWorkingPath, DIR_MODE);
	memcpy(&pBuff[0], &ADDRPTR->szDirName[0], strlen(ADDRPTR->szDirName));

	/* receive data from server */
	Rval = Messenger(iCliSocket, &pBuff[0], strlen(ADDRPTR->szDirName));
	if(Rval != ER_OK)
	{
		ERR_MSG(MSG_ERROR, "Messenger() failed!\n");
	}

	ERR_END

	if(pBuff != NULL)
	{
		free(pBuff);
	}
	pthread_mutex_unlock(&count_mutex);
	close(iCliSocket);
	pthread_exit(0);
}
int main(int argc, char const *argv[])
{
	ER_RV 					Rval 					= ER_OK;
	char 					*pBuff 					= NULL;
	struct sockaddr_in		server_addr;
	struct hostent 			*pHostInfo 				= NULL;
	unsigned int 			uiMsgLen 				= 0;
	unsigned int 			uiIdx 					= 0;
	pthread_t 				threads[NUMTHREADS];
	CliDesc_t				*clidescrip; 			

	/* intializing data structures */
	if((pHostInfo = (struct hostent *)malloc(sizeof(struct hostent))) == NULL)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "malloc failed!\n");
	}
	memset(pHostInfo, 0, sizeof(struct hostent));

	if((pBuff = (char *)malloc(MAX_BYTES)) == NULL)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "malloc failed!\n");
	}
	memset(pBuff, 0, MAX_BYTES);
	memset(&server_addr, 0, sizeof(sockaddr_in));
	memset(&szWorkingPath[0], 0, MAX_BYTES);
	
	pthread_mutex_init(&count_mutex, NULL);

	/* checking for enough arguments */
	if(argc < 3)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "Not enough arguments! Format is: hostname directory \n");
	}

	/* setting up msg header */
	Rval = BuffSetup(&uiMsgLen, &pBuff[0], argv[2]);
	if(Rval != ER_OK)
	{
		ERR_MSG(MSG_ERROR, "BuffSetup() failed!\n");
	}

	/* getting host from arguments */
	if((pHostInfo = gethostbyname(argv[1])) == NULL)
	{
		Rval = ER_FAIL;
		ERR_MSG(MSG_ERROR, "gethostbyname() failed! for hostname %s and the error was %s\n", argv[1], strerror(errno));
	}

	/* setting up addr */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	memcpy(&server_addr.sin_addr, pHostInfo->h_addr_list[0], pHostInfo->h_length);

	for(int i = 0; i < NUMTHREADS; i++)
	{
		clidescrip = (CliDesc_t *)malloc(sizeof(CliDesc_t));
		memset(clidescrip, 0, sizeof(CliDesc_t));
		clidescrip->threadID = i + 1;
		memcpy(&clidescrip->szDirName[0], argv[2], strlen(argv[2]));
		clidescrip->serveraddr = &server_addr;
		if(pthread_create(&threads[i], NULL, Connect, clidescrip) != 0)
		{
			printf("pthread_create() failed!\n");
		}
	}

	for(int i = 0; i < NUMTHREADS; i++)
	{
		pthread_join(threads[i], NULL);
	}
	ERR_END

	pthread_mutex_destroy(&count_mutex);

	if(Rval != ER_OK)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}