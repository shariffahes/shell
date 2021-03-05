#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8880
#define DEFAULT_IP "127.0.0.1"
char User[20];
char Os[20];
int clientSocket = 0;
void sigintHandler(int);
int main(int argc, char **argv)
{
  signal(SIGINT, sigintHandler);

  if (argc > 2)
  {
    printf("Too many arguments.\n");
  }
  else
  {
    char ip[15];
    if (argc == 2)
    {
      strcpy(ip, argv[1]);
    }
    else
    {
      strcpy(ip, DEFAULT_IP);
    }

    struct sockaddr_in address;
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
      perror("Client Socket failed to Initalize.\n");
      exit(EXIT_FAILURE);
    }
    else
    {
      printf("Client Socket Initalized.\n");
    }
    sleep(1);
    printf("Connecting to %s at %d....\n", ip, PORT);
    sleep(1);
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ip, &address.sin_addr) <= 0)
    {
      printf("\nInvalid address, %s address not supported\n", ip);
      exit(EXIT_FAILURE);
    }
    if (connect(clientSocket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
      printf("Connection Failed, %s refused to connect.\nDefault server IP address is %s, to specify a different address please include it as argument\n", ip, DEFAULT_IP);
      exit(EXIT_FAILURE);
    }
    else
    {
      printf("Connected successfully with %s at %d.\n", ip, PORT);
    }
    sleep(1);

    char osLogo[336];
    char welcomeMessage[350];

    char *t = "recieved";

    recv(clientSocket, osLogo, sizeof(osLogo), 0);
    send(clientSocket, t, strlen(t), 0);
    recv(clientSocket, welcomeMessage, sizeof(welcomeMessage), 0);

    printf("%s", osLogo);
    printf("%s", welcomeMessage);

    while (1)
    {
      fflush(stdin);

      recv(clientSocket, User, sizeof(User), 0);
      send(clientSocket, t, strlen(t), 0);
      recv(clientSocket, Os, sizeof(Os), 0);

      char command[100];

      printf("\033[0;32m");
      printf("%s@%s ~: ", User, Os);
      printf("\033[0m");

      fgets(command, sizeof(command), stdin);
      strtok(command, "\n");

      send(clientSocket, command, strlen(command) + 1, 0);

      if (strcmp(command, "disconnect") == 0)
      {
        break;
      }

      char bufferFile[4000];
      int n = recv(clientSocket, bufferFile, sizeof(bufferFile), 0);
      send(clientSocket, t, strlen(t), 0);
      printf("%s", bufferFile);
    }
  }

  return 0;
}
void sigintHandler(int sig_num)
{
  signal(SIGINT, sigintHandler);
  printf("\n");
  send(clientSocket, "disconnect", strlen("disconnect") + 1, 0);
  exit(0);
}
