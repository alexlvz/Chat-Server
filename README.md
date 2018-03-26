This program is an HTTP chat server. It creates a chat room and all the clients that connect to it
can chat with everyone that is connected. It uses the C select method, wich replaces the need of
multythreaded server. New clients are added to a linked list, and according to write/read avaliability, 
the server gets and sends messages to everyone in the room

The file server.c is the main file of the program. 
List implementation is in the same file, all comments are inside the code.
In order to use the program, you have to compile server.c file
with this command:
gcc -Wall server.c -o server
After that, run the server in this format

./server <port> 

Now that the server is up and running , you can connect to it using any client that supports
read/write operations.
If you are using telnet, run it like this:
telnet localhost <server-port-number>
after the connection, simply start chatting. 
To quit the server program, use ^c.
To quit telnet. use ctrl+] and then type quit and enter. 

This was a really cool excercise! 

Thank you! :)
