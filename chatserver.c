#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>  
#include <arpa/inet.h>   
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> 
#include <signal.h>

#define BUFFER_LENGTH 4096
#define MESSAGE_CONTENT 20
#define EMPTY_LIST 0
#define NO_SOCKET -1
#define NUM_OF_SD_DIGITS 10
#define TRUE 1
#define DISCONNECT_REQUEST 0
#define SINGLE_CHAR 1
//holds one sd info
typedef struct sd_node
{
	int data;
	struct sd_node *next;
}sd_node;
//list of sds
typedef struct sd_list
{
	sd_node *head;
	int size;
}sd_list;
//================INITIALIZES-SOCKETS-LIST======================//
sd_list *initList()
{
    sd_list *list = (sd_list*)malloc(sizeof(sd_list));
    if(list == NULL)
    {
        perror("malloc error");
        exit(EXIT_FAILURE);

    }
    list -> head = NULL;
    list -> size = EMPTY_LIST;
    return list;
}
//===============ADDS-NEW-SOCKET-TO-LIST===========================//
void addSocket(sd_list *list, int data)
{
    sd_node *node = (sd_node*)malloc(sizeof(sd_node));
    if(node == NULL)
    {
        perror("malloc error");
        exit(EXIT_FAILURE);

    }
    node->data = data;
    node->next = NULL;

    if(list->head == NULL)
    {
        list->head = node;
        list->size++;
    }
    else
    {
        sd_node *curr = list->head;

        while( curr->next != NULL)
        {
            curr = curr->next;
        }
        curr->next = node;
        list->size++;
    }
}
//===================REMOVES-A-SPECIFIC-SOCKET-FROM-LIST=====================//
void removeSocket(sd_list *list, int data)
{
    sd_node *temp;

    if(list -> size == EMPTY_LIST)
        return;
    
    if(list->size == 1) //only one socket
    {
        free(list->head);
        list->head = NULL;
        list->size--;
        return;
    }      

    if(list->head->data == data) //like pop first
    {
        temp = list->head;
        list->head = list->head->next;
        list->size--;
        free(temp);
        temp = NULL;
        return;
    }
        
    for(sd_node *curr = list->head ; curr != NULL ; curr = curr->next)
    {
        if(curr->next->data == data)
        {
            temp = curr->next;
            curr->next = curr->next->next;
            list->size--;
            break;
        }
    }
    if(temp != NULL)
    {
        free(temp);
        temp = NULL;
    }  
}
//=============RETURNS-SOCKET-DESCRIPTOR-BY-GIVEN-INDEX=====================//
int getSocketDesc(sd_list *list, int index)
{
    if(list->size == EMPTY_LIST)
        return NO_SOCKET;

    int i = 1;
    for(sd_node *curr = list->head ; curr != NULL ; curr = curr-> next)   
    {
        if(i == index)
            return curr->data;
        i++;
    }
        
 return NO_SOCKET;       
}
//========FREES-MEMORY-AND-CLOSES-SOCKETS================//
void freeMemory(sd_list *list)
{
    while(list->head != NULL)
    {
        close(list->head->data);
        removeSocket(list,list->head->data);
    }
    free(list);
    list = NULL;
}

sd_list *list;
//=======HANDLES-QUIT-PROGRAM-SIGNAL===============//
void sigHandler(int val) 
{
	freeMemory(list);
    exit(EXIT_SUCCESS);
}
//==========================MAIN-CHAT-SERVER-PROGRAM===============================//
int main(int argc , char *argv[])
{
    if(argc < 2) 
    {
        perror("No port provided");
        exit(EXIT_FAILURE);
    }
    list = initList();
    signal(SIGINT, sigHandler); //takes care of ^c signal 

    int master_socket , addrlen , new_socket , activity, i , j, k , valread , sd , max_sd , line_length;
    char buffer[BUFFER_LENGTH] , *guest_header = "Guest" , guest_id[NUM_OF_SD_DIGITS] , guestMessage[BUFFER_LENGTH + MESSAGE_CONTENT];
	struct sockaddr_in address; 
    fd_set readfds , writefds;
     //=============================START-OF-TCP-SERVER-SETUP===============================================//
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
        freeMemory(list);
        exit(EXIT_FAILURE);
    }
  
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( atoi(argv[1]));
      
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("bind failed");
        freeMemory(list);
        exit(EXIT_FAILURE);
    }
     
    if (listen(master_socket, 5) < 0)
    {
        perror("listen");
        freeMemory(list);
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    //=============================END-OF-TCP-SERVER-SETUP===============================================//
    while(TRUE) 
    {
        FD_ZERO(&readfds);  //initialize FD_SET
        FD_ZERO(&writefds);
        FD_SET(master_socket, &readfds);
        FD_SET(master_socket, &writefds);
     
        max_sd = master_socket;
         
        for ( i = 0 ; i < list->size ; i++)  //add all sockets to read and write sets
        {
            sd = getSocketDesc(list,i+1);
            FD_SET( sd , &readfds);
            FD_SET( sd , &writefds);
            if(sd > max_sd)
                max_sd = sd;
        }
		
        activity = select( max_sd + 1 , &readfds , &writefds , NULL , NULL); //initialize the select function 
    
        if ((activity < 0) && (errno!=EINTR)) 
        {
            perror("select error");
            freeMemory(list);
            exit(EXIT_FAILURE);
        }
          
        if (FD_ISSET(master_socket, &readfds)) ///add new socket to the list
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                freeMemory(list);
                exit(EXIT_FAILURE);
            }
            addSocket(list,new_socket);
        }
        for (j = 0; j < list->size; j++) 
        {
            sd = getSocketDesc(list,j+1); //get socket id
            if (FD_ISSET( sd , &readfds)) 
            {
                printf("Server is ready to read from socket %d\r\n",sd);
                if((valread = recv( sd , buffer, BUFFER_LENGTH, MSG_PEEK) < 0)) //read without taking the data from client in order to find the first line
                { 
                    perror("read");
                    close(sd);
                    removeSocket(list,sd);

                }
                line_length = strstr(buffer, "\r\n")-buffer+2;
                if((valread = recv( sd , buffer, line_length , 0)) < 0) //read one line from client
                { 
                    perror("read");
                    close(sd);
                    removeSocket(list,sd);
                }
                if(valread == DISCONNECT_REQUEST) //client wants to disconnect
                {
                    close(sd);
                    removeSocket(list,sd);
                }
                else //constructs client message with its details and send it to all other avaliable clients
                {
			        sprintf(guest_id, "%d", sd);
	    		    strcpy(guestMessage,guest_header);
	       		    strcat(guestMessage,guest_id);
		    	    strcat(guestMessage,": ");
		    	    strncat(guestMessage,buffer,valread);
	    		    guestMessage[strlen(guestMessage)]='\0';
                    
	    		    for(k = 0 ; k < list->size ; k++) //loop throufh all avaliable clients and sent the message
	    		    {
                        if (FD_ISSET( getSocketDesc(list,k+1) , &writefds)) 
                        {
                            printf("Server is ready to write to socket %d\r\n", getSocketDesc(list,k+1));
                            write(getSocketDesc(list,k+1) , guestMessage , strlen(guestMessage));	
                        }
		    	    }
                }  
            }      
        }
    }
    return EXIT_SUCCESS;
} 