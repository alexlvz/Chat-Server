#include<stdio.h> 
#include<unistd.h> 
#include<string.h>
#include<stdlib.h> 
#include<string.h>    
#include<sys/socket.h>   
#include<arpa/inet.h> 
#include <netdb.h>
#include <ctype.h>
#include <time.h>
#define PREFIX 7
#define NO_PORT_PROVIDED 0
#define URL_PREFIX "http://"
#define USAGE_ERROR "Usage: [-h] [-d <time-Interval>] <URL>\n"
#define DEFAULT_PORT 80
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define GET_REQUEST 'g'
#define HEAD_REQUEST 'h'
#define TIME_REQUEST "-d"
#define HEAD_REQUEST_STR "-h"
#define SUCCESS 1
#define FAILURE -1
#define EQUAL 0
#define EXIT_ERROR 1
#define READ_BUF_SIZE 1024
#define GET "GET /"
#define HEAD "HEAD /"
#define INPUT_ERROR "wrong input\n"
#define TIME_BUF_SIZE 128
#define NO_PORT 1
#define HTTP_REQUEST_PRINT "HTTP request =\n%s\nLEN = %d\n"

//CHECK IF PORT IS NEGATIVE OR NOT A NUMBER - > WRONG INPUT
//===========================================================================================================================================//
//struct that holds the request parameters
struct requestParams
{
	char *host;
	char *port;
	char *urlDir;
	char *time;
	char *gethostbyname; //for gethostbyname function
	char type;

}typedef requestParams;
//===========================================================================================================================================//
void printUsageError() //function to print the usage error
{
	printf(USAGE_ERROR);
	exit(EXIT_ERROR);
}
//===========================================================================================================================================//
int checkIfStringIsNum(char *token) //function that checks if a string contains numbers only
{
	for(int i=0 ; i < strlen(token) ; i++)
		if(isdigit(token[i]) == 0)
			return FAILURE;
	return SUCCESS; 
}
//===========================================================================================================================================//
int checkTimeParams(char *argv[], int n) //checks if the given time parameters (-d and time) are valid
{
	char s[2] = ":";
	char *token;
	char *str;
	int numOfDeviders = 0; //should be 3 parts

	str = strdup(argv[n]);
	//slice the string to check its part
	token = strtok(str, s); 
	while( token != NULL ) 
	{
		numOfDeviders ++;
		if(checkIfStringIsNum(token) == -1)
		{
			free(str);
			str = NULL;
			printf(INPUT_ERROR);
			exit(EXIT_ERROR);
		}         
		token = strtok(NULL, s);
	}

	if(numOfDeviders != 3) //wrong input
	{
		free(str);
		str = NULL;
		printf(INPUT_ERROR);
		exit(EXIT_ERROR);
	}
	free(str);
	str = NULL;
	return SUCCESS; 
}
//===========================================================================================================================================//
void setTimeParams(requestParams *request)//sets the time parameters by user's -d input. called after checking that the input is correct
{
	char *token, *str;
	int day, hour, min;
    //slice the time string into parameters
	str = strdup(request->time);
	token = strtok(str, ":"); 
	day = atoi(token);    
	token = strtok(NULL, ":");
	hour = atoi(token);
	token = strtok(NULL, ":");
	min = atoi(token);
    // given code
	time_t now;
	char timebuf[TIME_BUF_SIZE];
	now = time(NULL);
	now=now-(day*24*3600+hour*3600+min*60);
	strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
	free(request->time);
	request->time = strdup(timebuf);    
	free(str);
	str = NULL;
}
//===========================================================================================================================================//
int checkUrl(char *argv[], int n)//checks if the given url is valid
{
	char *temp = strchr(argv[n] + PREFIX,'/'); //checks if contanins '/'
	if(temp == NULL)
	{
		printf(INPUT_ERROR);
		exit(EXIT_ERROR);
	}
	
	int indexOfSlash = strchr(argv[n] + PREFIX,'/')-argv[1];

	if((strncmp(URL_PREFIX , argv[n] , PREFIX) != EQUAL) || ((argv[n][strlen(argv[n])-1] != '/')   && (indexOfSlash == strlen(argv[n]-1)))) //checks if ends with '/' if without path or without http://
	{
		printf(INPUT_ERROR);
		exit(EXIT_ERROR);
	}
	return SUCCESS;
}
//===========================================================================================================================================//
void setRequestParams(requestParams *request, char *userString, char *timeParam, char msgType) //functions that sets the struct parameters accrding to users input. will be called only if input is correct
{
	request->type = msgType;
	if(timeParam != NULL) //if -d was given      
		request->time = strdup(timeParam);


		int portLength;
		char *newStr = strndup(userString + PREFIX, strlen(userString)-PREFIX);  //url without http://
		int indexOfPort = strchr(newStr,':')-newStr;
		if(indexOfPort >= 0)
		{
		    char *check = strndup(newStr+indexOfPort+1,1);
		    if(checkIfStringIsNum(check) ==  FAILURE)
		    {
		        free(newStr);
		        free(check);
		    	newStr = NULL;
		    	check = NULL;
		    	if(request->time != NULL)
		    	{
		    	    free(request->time);
		    	    request->time = NULL;
		    	}
		    	printf(INPUT_ERROR);
		    	exit(EXIT_ERROR);
		    }
		    free(check);
		    check = NULL; 
		}
		int indexOfRoot = strchr(newStr,'/')-newStr; //find the end of host , if not found its minus
		int rootSize = strlen(newStr) - indexOfRoot; //size of path 

		char *rootDir = strndup(newStr + indexOfRoot+1, rootSize);  //its the path
		char *host;
		char *gethostbyname;

		if(indexOfPort<0) //no port given
			portLength = NO_PORT;
		else    
			portLength = indexOfRoot - indexOfPort;

		char *port = strndup(newStr+indexOfPort+1,portLength-1);;
	
		if(portLength == NO_PORT)
		{
		    host = strndup(newStr, indexOfRoot -portLength+1);
		    gethostbyname = strdup(host);
		}
			
		else
		{
		    host = strndup(newStr, indexOfRoot);
		    gethostbyname = strndup(newStr, indexOfRoot -portLength);
		}
        //set parameters of request
		request->host = host;
		request->gethostbyname = gethostbyname;
		request->port = port;
		request->urlDir = rootDir;  
		free(newStr);
		newStr = NULL; 
}
//===========================================================================================================================================//
char* buildRequest(requestParams *request, char requestType) //function that builds a string request from the parameters that were given from the user
{
	char *messageHeader;

	if(requestType == GET_REQUEST)
		messageHeader  = GET;
	else
		messageHeader  = HEAD;

	char *message; 
	char *messagePartThree = " HTTP/1.1\r\nHost: ";
	char *messagePartTwo; 
	char *messagePartSuffix;
	if(request->time == NULL) //request without -d
	{
		messagePartTwo = "\r\nConnection: close\r\n\r\n";
		message=(char*)malloc(sizeof(char)*(strlen(request->host) + strlen(messageHeader) + strlen(messagePartTwo) + strlen(request->urlDir) + strlen(messagePartThree) +1));
		if(message == NULL)
		{
			perror("malloc");
			exit(EXIT_ERROR);
		}
		// counstructing the message from the parametetrs
		strcpy(message,messageHeader);
		strcat(message,request->urlDir);
		strcat(message,messagePartThree);
		strcat(message,request->host);
		strcat(message,messagePartTwo);        
	}

	else //request with -d .more parameters, need to reallocate memory for the new parts.
	{
		messagePartTwo = "\r\nConnection: close\r\nIf-Modified-Since: ";
		messagePartSuffix = "\r\n\r\n";  
		setTimeParams(request);      
		message=(char*)malloc(sizeof(char)*(strlen(request->host) + strlen(messageHeader) + strlen(messagePartTwo) + strlen(request->urlDir) + strlen(messagePartThree) + strlen(messagePartSuffix)+ strlen(request->time)+1));
		if(message == NULL)
		{
			perror("malloc");
			exit(EXIT_ERROR);
		}
		strcpy(message,messageHeader);
		strcat(message,request->urlDir);
		strcat(message,messagePartThree);
		strcat(message,request->host);
		strcat(message,messagePartTwo);
		strcat(message,request->time);
		strcat(message,messagePartSuffix);      
	}
	message[strlen(message)] = '\0';

	return message;
}
//===========================================================================================================================================//
//checks the input from argv and if it is ok - sets the parameters of the request
void checkAndSetInputString(int argc, char *argv[] , requestParams *request)
{
    if((argc == 2) && ((strcmp(argv[1],HEAD_REQUEST_STR) == 0) || (strcmp(argv[1],TIME_REQUEST) == 0))) //only if given -h -d or -d -h
        printUsageError();
        
    else if((argc == 3) && ((((strcmp(argv[1],HEAD_REQUEST_STR) == 0) && (strcmp(argv[2],TIME_REQUEST) == 0))) || (((strcmp(argv[1],TIME_REQUEST) == 0) && (strcmp(argv[2],HEAD_REQUEST_STR) == 0))))) // if given -h -d TIME or -d TIME -h
        printUsageError();      

	else if((argc == 2) && checkUrl(argv,1) == SUCCESS) //only url
		setRequestParams(request, argv[1], NULL, GET_REQUEST);            

	else if(argc == 3) //url with header
	{
		if((strcmp(argv[1] , HEAD_REQUEST_STR) == EQUAL) && (checkUrl(argv,2) == SUCCESS)) // -h url
			setRequestParams(request, argv[2], NULL,HEAD_REQUEST);  

		else if((strcmp(argv[2] , HEAD_REQUEST_STR) == EQUAL) && (checkUrl(argv,1) == SUCCESS)) // url -h
			setRequestParams(request, argv[1], NULL ,HEAD_REQUEST);  

		else
			printUsageError();
	}

	else if(argc == 4) //url with time 
	{
        if((strcmp(argv[2] , TIME_REQUEST) == EQUAL) && (checkTimeParams(argv,3) == SUCCESS) && (strcmp(argv[1] , HEAD_REQUEST_STR) == EQUAL)) // if given only -h -d
			printUsageError(); 
	       
        if((strcmp(argv[3] , HEAD_REQUEST_STR) == EQUAL) && (checkTimeParams(argv,2) == SUCCESS) && (strcmp(argv[1] , TIME_REQUEST) == EQUAL)) // if given only -d -h
			printUsageError();   	   	   
	   
		else if((strcmp(argv[1] , TIME_REQUEST) == EQUAL) && (checkTimeParams(argv,2) == SUCCESS) && (checkUrl(argv,3) == SUCCESS)) // -d TIME url
			setRequestParams(request, argv[3], argv[2] ,GET_REQUEST);  

		else if((strcmp(argv[2] , TIME_REQUEST) == EQUAL) && (checkTimeParams(argv,3) == SUCCESS) && (checkUrl(argv,1) == SUCCESS)) // url -d TIME
			setRequestParams(request, argv[1], argv[3], GET_REQUEST); 

		else
			printUsageError();
	}
	else if(argc == 5) //url with header and time (has 6 options)
	{
		if(((strcmp(argv[1] , HEAD_REQUEST_STR) == EQUAL)) && (strcmp(argv[2] , TIME_REQUEST) == EQUAL) && (checkTimeParams(argv,3) == SUCCESS) && (checkUrl(argv,4) == SUCCESS))
			setRequestParams(request, argv[4], argv[3], HEAD_REQUEST);  

		else if(((strcmp(argv[1] , TIME_REQUEST) == EQUAL)) && (strcmp(argv[3] , HEAD_REQUEST_STR) == EQUAL) && (checkTimeParams(argv,2) == SUCCESS) && (checkUrl(argv,4) == SUCCESS))
			setRequestParams(request, argv[4], argv[2], HEAD_REQUEST);  

		else if(((strcmp(argv[3] , TIME_REQUEST) == EQUAL)) && (strcmp(argv[1] , HEAD_REQUEST_STR) == EQUAL) && (checkTimeParams(argv,4) == SUCCESS) && (checkUrl(argv,2) == SUCCESS))
			setRequestParams(request, argv[2], argv[4], HEAD_REQUEST); 

		else if(((strcmp(argv[2] , TIME_REQUEST) == EQUAL)) && (strcmp(argv[4] , HEAD_REQUEST_STR) == EQUAL) && (checkTimeParams(argv,3) == SUCCESS) && (checkUrl(argv,1) == SUCCESS))
			setRequestParams(request, argv[1], argv[3], HEAD_REQUEST);

		else if(((strcmp(argv[3] , TIME_REQUEST) == EQUAL)) && (strcmp(argv[2] , HEAD_REQUEST_STR) == EQUAL) && (checkTimeParams(argv,4) == SUCCESS) && (checkUrl(argv,1) == SUCCESS))
			setRequestParams(request, argv[1], argv[4], HEAD_REQUEST);

		else if(((strcmp(argv[1] , TIME_REQUEST) == EQUAL)) && (strcmp(argv[4] , HEAD_REQUEST_STR) == EQUAL) && (checkTimeParams(argv,2) == SUCCESS) && (checkUrl(argv,3) == SUCCESS))
			setRequestParams(request, argv[3], argv[2], HEAD_REQUEST);

		else
			printUsageError();            
	}
	else
		printUsageError();
}
//===========================================================================================================================================//
//Frees up all memory that was allocated during the run time
void freeMemory(char *message, requestParams *request)
{
    free(message); //the request
	free(request->host); //request params
	free(request->port);
	free(request->urlDir);
	free(request->time);
	free(request->gethostbyname);
	request->host = NULL;
	request->urlDir = NULL;
	request->time = NULL;
	request->port = NULL;
	request->gethostbyname = NULL;
	message = NULL;
}
//===========================================================================================================================================//
//main holds the socket creation and is responsible for sending the HTTP request and getting the answer
int main(int argc , char *argv[])
{
	requestParams request[1]; //struct for the params
	request->time = NULL;  //sets time to null. will be changed it -d is given...
	checkAndSetInputString(argc,argv,request);      
	char *message = buildRequest(request, request->type);
	printf(HTTP_REQUEST_PRINT,message,(int)strlen(message));

	int sock ,nbytes;
	struct sockaddr_in server;
	struct hostent *hp;
	unsigned char server_reply[READ_BUF_SIZE]; //will hold the response
	sock = socket(AF_INET , SOCK_STREAM , 0); //creates the socket
	if (sock == FAILURE)
	{
	    freeMemory(message, request);
		perror("socket");
		exit(EXIT_ERROR);
	}
	hp = gethostbyname(request->gethostbyname); //gets host ip
	if(hp == NULL)
	{   
	    freeMemory(message, request);
	    close(sock);
		herror("gethostbyname");
		exit(EXIT_ERROR);
	}
	//setting socket parameters 
	server.sin_addr.s_addr = ((struct in_addr*)(hp->h_addr))->s_addr;
	server.sin_family = AF_INET;   
	
	if(strlen(request->port) == NO_PORT_PROVIDED) //if no port provided , set it to 80
		server.sin_port = htons(DEFAULT_PORT);
	else
		server.sin_port = htons(atoi(request->port)); //sets the given port

	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) // tryies to connect
	{
	    freeMemory(message, request);
	    close(sock);
		perror("connect");
		exit(EXIT_ERROR);
	}

	if((nbytes = write(sock , message, strlen(message))) < 0) //sends the request
	{
	    freeMemory(message, request);
	    close(sock);
		perror("write");
		exit(EXIT_ERROR);
	}
	nbytes=1; //num of readden bytes
	int countNbytes=0; //total bytes recieved
	while(nbytes>0)
	{
		if((nbytes = read(sock , server_reply , sizeof(server_reply))) < 0) //recieve the response
		{
		    freeMemory(message, request);
		    close(sock);
			perror("read");
			exit(EXIT_ERROR);
		}
		server_reply[nbytes]='\0';
		countNbytes+=nbytes;
		if(nbytes>0)
		{
			if(write(1,server_reply,nbytes) != nbytes)//print the reply
			{
				perror("write");
				exit(1);
			}
		}		
	}

	printf("\n Total received response bytes: %d\n",countNbytes);
    freeMemory(message, request);
	if(close(sock)<0)
	{
	    perror("close");
	    exit(EXIT_ERROR);
	}
	return 0;
}
