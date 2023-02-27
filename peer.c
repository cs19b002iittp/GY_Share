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

#pragma pack(1)   // this helps to pack the struct to 5-bytes

struct system_info
{
  char sysname[50];
  char nodename[50];
  char version[50];
  char release[50];
  char machine[50];
};

#pragma pack(0)   // turn packing off

void sending();
void rpc();
struct system_info info();
void receiving(int server_fd);
void *receive_thread(void *server_fd);

int main(int argc, char const *argv[])
{
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
    pthread_create(&tid, NULL, &receive_thread, &server_fd); //Creating thread to keep receiving message in real time
    printf("\n*****Select the following:*****\n1.Send message\n2.Get system info\n0.Quit\n");
    printf("\nEnter choice: ");
    do
    {

        scanf("%d", &ch);
        switch (ch)
        {
        case 1:
            sending();
            break;
        case 2:
            rpc();
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

struct system_info info(){
    struct utsname buf1;
    errno =0;
    if(uname(&buf1)!=0)
    {
      perror("uname doesn't return 0, so there is an error");
      exit(EXIT_FAILURE);
    }
    struct system_info s1;
    strcpy(s1.sysname,buf1.sysname);
    strcpy(s1.nodename,buf1.nodename);
    strcpy(s1.version,buf1.version);
    strcpy(s1.release,buf1.release);
    strcpy(s1.machine,buf1.machine);
    return s1;
}

void rpc(){
    char buffer[2000] = {0};
    //Fetching port number
    int PORT_server;
    char IP_server[20];

    //IN PEER WE TRUST
    printf("Enter the port to send message:"); //Considering each peer will enter different port
    scanf("%d", &PORT_server);

    printf("Enter the IP address:");
    scanf("%s", IP_server);

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

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }

    sprintf(buffer, "Getting system info");
    send(sock, buffer, sizeof(buffer), 0);

    struct system_info result;
    recv(sock, &result, sizeof(struct system_info), 0);
    printf("\nSystem Info\n");
    printf("System Name = %s\n",result.sysname);
    printf("Node Name = %s\n",result.nodename);
    printf("Version = %s\n",result.version);
    printf("Release = %s\n",result.release);
    printf("Machine = %s\n",result.machine);
    close(sock);
    // sprintf("Getting system info");
}

//Sending messages to port
void sending()
{
    char buffer[2000] = {0};
    //Fetching port number
    int PORT_server;
    char IP_server[20];

    //IN PEER WE TRUST
    printf("Enter the port to send message:"); //Considering each peer will enter different port
    scanf("%d", &PORT_server);

    printf("Enter the IP address:");
    scanf("%s", IP_server);

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

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return;
    }

    char dummy;
    printf("Enter your message:");
    scanf("%c", &dummy); //The buffer is our enemy
    scanf("%[^\n]s", hello);
    sprintf(buffer, "%s[IP:%s][PORT:%d] says: %s", name, IP, PORT, hello);
    send(sock, buffer, sizeof(buffer), 0);
    printf("\nMessage sent\n");
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
    int value;
    struct system_info s2;
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
                    value=strcmp(buffer,"Getting system info");  
                    if(value==0){
                        s2 = info();
                        send(i, &s2, sizeof(s2), 0);
                    }
                    else{
                        printf("\n%s\n", buffer);
                    }
                    FD_CLR(i, &current_sockets);
                }
            }
        }

        if (k == (FD_SETSIZE * 2))
            break;
    }
}