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
#define MESSAGE_CONTENT 12
#define EMPTY_LIST 0
#define NO_SOCKET -1
#define NUM_OF_SD_DIGITS 2
#define TRUE 1
#define NO_DATA 0

typedef struct sd_node{
	int data;
	struct sd_node *next;
}sd_node;

typedef struct sd_list{
	sd_node *head;
	int size;
}sd_list;

sd_list *initList()
{
    sd_list *list = (sd_list*)malloc(sizeof(sd_list));
    list -> head = NULL;
    list -> size = EMPTY_LIST;
    return list;
}

void addSocket(sd_list *list, int data)
{
    sd_node *node = (sd_node*)malloc(sizeof(sd_node));
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
  //  printf("socket %d added successfuly\n", data);
}
 
void removeSocket(sd_list *list, int data)
{
    sd_node *temp;

    if(list -> size == EMPTY_LIST)
        return;
    
    if(list->size == 1)
    {
        free(list->head);
        list->head = NULL;
        list->size--;
     //   printf("socket %d removed successfuly\n", data);
        return;
    }      

    if(list->head->data == data)
    {
        temp = list->head;
        list->head = list->head->next;
        list->size--;
        free(temp);
        temp = NULL;
      //  printf("socket %d removed successfuly\n", data);
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
 //   printf("socket %d removed successfuly\n", data);
}

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

void freeMemory(sd_list *list)
{
    for(sd_node *curr = list->head; curr != NULL ; curr = curr->next)
        removeSocket(list,curr->data);
    free(list);
    list = NULL;
}

sd_list *list;

void sigHandler(int val) {
    free(list);
    exit(EXIT_SUCCESS);
}

int main(int argc , char *argv[])
{
    if(argc < 2)
    {
        perror("No port given");
        exit(EXIT_FAILURE);
    }
    list = initList();
    signal(SIGINT, sigHandler);

    int master_socket , addrlen , new_socket , activity, i , valread , sd , char_index = 0 , max_sd;
    char buffer[BUFFER_LENGTH] , *guest_header = "Guest" , guest_id[NUM_OF_SD_DIGITS] , single_line[BUFFER_LENGTH] , guestMessage[BUFFER_LENGTH + MESSAGE_CONTENT];
	struct sockaddr_in address; 
    fd_set readfds , writefds;

    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
  
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( atoi(argv[1]) );
      
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
     
    if (listen(master_socket, 5) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    addrlen = sizeof(address);
   // puts("Waiting for connections ...");
    while(TRUE) 
    {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(master_socket, &readfds);
        FD_SET(master_socket, &writefds);
     
        max_sd = master_socket;
         
        for ( i = 0 ; i < list->size ; i++) 
        {
            sd = getSocketDesc(list,i+1);
            FD_SET( sd , &readfds);
            FD_SET( sd , &writefds);
            if(sd > max_sd)
                max_sd = sd;
        }
		
        activity = select( max_sd + 1 , &readfds , &writefds , NULL , NULL);
    
        if ((activity < 0) && (errno!=EINTR)) 
        {
            perror("select error");
        }
          
        if (FD_ISSET(master_socket, &readfds)) 
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            addSocket(list,new_socket);
        }
        for (i = 0; i < list->size; i++) 
        {
            sd = getSocketDesc(list,i+1);
            if (FD_ISSET( sd , &readfds)) 
            {
                char_index = 0;
                printf("Server is ready to read from socket %d\r\n",sd);
                while((valread = read( sd , buffer, 1)) != 0 && char_index < BUFFER_LENGTH) //read from client
                {
                    if(buffer[0] == '\n')
                        break;

                    single_line[char_index] = buffer[0];
                    char_index++;
                }
                if(valread == NO_DATA)
                {
                    //printf("Guest %d closed session\r\n", sd);
                    close(sd);
                    removeSocket(list,sd);
                }
                else
                {
			        sprintf(guest_id, "%d", sd);
	    		    strcpy(guestMessage,guest_header);
	       		    strcat(guestMessage,guest_id);
		    	    strcat(guestMessage,": ");
		    	    strncat(guestMessage,single_line,char_index-1);
	    		    guestMessage[strlen(guestMessage)]='\0';
	    		    for(i = 0 ; i < list->size ; i++)
	    		    {
                        if (FD_ISSET( getSocketDesc(list,i+1) , &writefds)) 
                        {
                            printf("Server is ready to write to socket %d\r\n", getSocketDesc(list,i+1));
                            write(getSocketDesc(list,i+1) , guestMessage , strlen(guestMessage));	
                        }
		    	    }
                }  
            }      
        }
    }
    freeMemory(list);
    return EXIT_SUCCESS;
} 

