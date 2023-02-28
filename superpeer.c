// C program to demonstrate peer to peer chat using Socket Programming
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include<errno.h>
#include<sys/utsname.h>

char name[20];
int PORT;
char IP[20];

struct st{
    int PORT;
    char IP[20];
    int nfiles;
    int files[10];
};

#pragma pack(1)   // this helps to pack the struct to 5-bytes

struct node_info
{
    int PORT;
    char IP[20];
};

#pragma pack(0)   // turn packing off

void sending();
void rpc();
void receiving(int server_fd);
void receive_thread(void *server_fd);
void *connection_handler(void *socket_desc,fd_set current_sockets);
struct st nodes[100];
int n;

struct node_info getnode_info(int fileId){ 
    for(int z=0; z<n; z++){
        for(int y=0; y<nodes[z].nfiles; y++){
            if(fileId == nodes[z].files[y]){
                struct node_info inf;
                strcpy(inf.IP,nodes[z].IP);
                inf.PORT=nodes[z].PORT;
                printf("hi");
                return inf;
            }
        }
    }
    struct node_info ginf;
    printf("hello");
    return ginf;
}

int main(int argc, char const *argv[])
{
    printf("Enter the IP address of super node : ");
    scanf("%s",IP);

    printf("Enter port number of super node:");
    scanf("%d", &PORT);
   
    printf("Number of nodes: ");
    scanf("%d",&n);

    
    for(int i=0;i<n;i++){
        printf("Enter IP address of node %d :",i);
        scanf("%s",nodes[i].IP);

        printf("Enter port number of node %d :",i);
        scanf("%d",&nodes[i].PORT);

        printf("Enter the number of files owned by the node %d :",i);
        int nf=0;
        scanf("%d", &nf);
        nodes[i].nfiles = nf;
        for(int k=0;k<nf;k++){
            printf("Enter the file Id: ");
            scanf("%d",&nodes[i].files[k]);
        }
    }
    

    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int k = 0;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(IP);
    address.sin_port = htons(PORT);

    //Printed the server socket addr and port
    printf("IP address is: %s\n", inet_ntoa(address.sin_addr));
    printf("port is: %d\n", (int)ntohs(address.sin_port));

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 5) < 0)
    {
        printf("Listen Error");
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    receive_thread(&server_fd);
    close(server_fd);

    return 0;
}



//Calling receiving every 2 seconds
void receive_thread(void *server_fd)
{
    printf("Receive thread...\n");
    int s_fd = *((int *)server_fd);
    while (1)
    {
        sleep(2);
        receiving(s_fd);
    }
}

//Receiving messages on our port
void receiving(int server_fd)
{
    struct sockaddr_in address;
    int value;
    char buffer[2000] = {0};
    int addrlen = sizeof(address);
    fd_set current_sockets, ready_sockets;
    //Initialize my current set
    FD_ZERO(&current_sockets);
    FD_SET(server_fd, &current_sockets);
    int k = 0;
    while (1)
    {
        k++;
        ready_sockets = current_sockets;

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0)
        {
            perror("Error");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &ready_sockets))
            {

                if (i == server_fd)
                {
                    int client_socket;
                    if ((client_socket = accept(server_fd, (struct sockaddr *)&address,
                                                (socklen_t *)&addrlen)) < 0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    else{
                        pthread_t thread_id;
                        pthread_create( &thread_id , NULL , connection_handler , (void*) &client_socket);
                    }
                    FD_SET(client_socket, &current_sockets);
                }
            }
        }

        if (k == (FD_SETSIZE * 2))
            break;
    }
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc,fd_set current_sockets)
{
    int i = *(int*)socket_desc;
    int valread;
    char buffer[2000] = {0};
    valread = recv(i, buffer, sizeof(buffer), 0);
    int fileId = atoi(buffer);              

    struct node_info inf;
    inf=getnode_info(fileId);
    send(i, &inf, sizeof(inf), 0);
            
    FD_CLR(i, &current_sockets);
} 
