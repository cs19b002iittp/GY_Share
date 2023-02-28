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
char supernodeIp[] = "172.17.0.1";
int supernodePort = 5000;

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
void *receive_thread(void *server_fd);

int main(int argc, char const *argv[])
{
    int PORT_server;
    char IP_server[20];

    printf("Enter name:");
    scanf("%s", name);

    printf("Enter your port number:");
    scanf("%d", &PORT);
    
    printf("Enter your IP Address:");
    scanf("%s", IP);

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
        perror("listen");
        exit(EXIT_FAILURE);
    }
    int ch;
    pthread_t tid;
    pthread_create(&tid, NULL, &receive_thread, &server_fd); //Creating thread
    printf("\n*****Select the following:*****\n1.Get File\n0.Quit\n");
    printf("\nEnter choice: ");
    do
    {
        scanf("%d", &ch);
        switch (ch)
        {
        case 1:
            sending();
            break;
        case 0:
            printf("\nLeaving\n");
            break;
        default:
            printf("\nWrong choice\n");
        }
    } while (ch);

    close(server_fd);

    return 0;
}


void rpc(int PORT_server,char IP_server[20],struct node_info*inf,int fileId){ //Fetching port number
    printf("RPC called %d\n", fileId);
    char buffer[2000] = {0};
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char hello[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP_server); // gives an IP
    serv_addr.sin_port = htons(PORT_server);
    printf("rpc %d %s \n",PORT_server,IP_server);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }

    else{
        printf("Sending rpc..\n");
        sprintf(buffer, "%d",fileId);
        send(sock, buffer, sizeof(buffer), 0);
        recv(sock, inf, sizeof(struct node_info), 0);
        printf("Received rpc\n");
    }
    
    close(sock);
}

//Sending messages to port
void sending()
{
    int fileId;
    printf("Enter the file Id to be fetched : ");
    scanf("%d",&fileId);

    struct node_info result;
    rpc(supernodePort,supernodeIp,&result,fileId);

    char buffer[2000] = {0};
    //Fetching port number
    int PORT_server;
    char IP_server[20];
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char hello[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(result.IP); // gives an IP
    serv_addr.sin_port = htons(result.PORT);

    printf("Calling node with IP %s, PORT %d, for file %d\n", result.IP, result.PORT, fileId);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }

    sprintf(buffer, "%d",fileId);

    char dummy;
    
    send(sock, buffer, sizeof(buffer), 0);
    recv(sock,buffer,sizeof(buffer),0);

    // printf("Content of the file %d.txt is:\n\n%s\n",fileId, buffer);
    FILE *fptr;
    char path[50];
    sprintf(path, "%s/%d.txt",name, fileId);
    fptr = fopen(path,"w");

    if(fptr == NULL)
    {
        printf("Error!");   
        exit(1);             
    }

    fprintf(fptr,"%s",buffer);
    printf("File %d.txt is saved to folder %s\n",fileId, name);
    fclose(fptr);
    close(sock);
}

//Calling receiving every 2 seconds
void *receive_thread(void *server_fd)
{
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
    int valread;
    int value=0;
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
                    FD_SET(client_socket, &current_sockets);
                }
                else
                {
                    valread = recv(i, buffer, sizeof(buffer), 0);
                    printf("\nRequest for file buffer%s.txt\n", buffer);
                    char result[2000];
                    FILE    *textfile;
                    char    *text;
                    long    numbytes;
                    char path[100];
                    sprintf(path,"%s/%s.txt",name,buffer);
                
                    textfile = fopen(path, "r");
                    if(textfile == NULL)
                        return;
                    
                    fseek(textfile, 0L, SEEK_END);
                    numbytes = ftell(textfile);
                    fseek(textfile, 0L, SEEK_SET);  
                
                    text = (char*)calloc(numbytes, sizeof(char));   
                    if(text == NULL)
                        return;
                
                    fread(text, sizeof(char), numbytes, textfile);
                    fclose(textfile);
                    send(i, text, numbytes, 0);
                    FD_CLR(i, &current_sockets);
                }
            }
        }

        if (k == (FD_SETSIZE * 2))
            break;
    }
}