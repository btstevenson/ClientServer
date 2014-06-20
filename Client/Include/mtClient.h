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

#include <stdarg.h>

#define 		DIRDATA			0
#define			FILEDATA		1
#define 		PORT			27232


#define		ER_RV		unsigned int
#define		ER_OK		0		/* No error reported */
#define		ER_FAIL		1		/* General fail */
#define		ER_MEM		2		/* memory allocation failed */

void WriteMsg(int code, const char *Message, ...);

/* error msg macro */
#define ERR_MSG(type, message, ...) \
        {                                \
           WriteMsg(type, message, ##__VA_ARGS__); \
           goto exit;                             \
        } 

#define		MSG_ERROR			1

#define		ERR_END			exit:

struct MsgHeader
{
	unsigned int 	uiDataType;
	unsigned int 	uiFileLength;
	char 			szFileName[256];
	char 			szDirName[1024];
};
typedef struct MsgHeader MsgHeader_t;

struct ClientDescriptor
{
	struct sockaddr_in *serveraddr;
	char 	szDirName[1024];
	int 	threadID;
};
typedef struct ClientDescriptor CliDesc_t;
