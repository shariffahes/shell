#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/mman.h>
//#include <wait.h>
#include <fcntl.h>
#include <signal.h>

#define SERV_PORT 8880
#define COMMAND_SIZE 100
#define BUFFER_SIZE 100
#define NB_OF_COMMANDS 15
#define NB_OF_OPTIONS 3
//#define USER "guest\0"
#define OS "Deadpool\0"
#define VERSION "0.3"
#define AUTHORS "Jehad Oumer, Heba Rachid, Sharif Fahes"
#define YEAR "2020"
#define BACKLOG 10

char *deadpoolCommands[NB_OF_COMMANDS] = {"listdr", "printcdr", "createdr", "remove", "return", "create", "move", "clear", "compare", "read", "permission", "wordC", "copy", "ipinfo", "psinfo"};
char *sysCommands[NB_OF_COMMANDS] = {"ls", "pwd", "mkdir", "rm", "echo", "touch", "mv", "clear", "diff", "cat", "chmod", "wc", "cp", "ifconfig", "ps"};
char *deadpoolOptions[NB_OF_OPTIONS] = {"-all", "-F", "-details"};
char *sysOptions[NB_OF_OPTIONS] = {"-l", "-rf", "aux"};

void onePipe(char **, char **);
void twoPipe(char **, char **, char **);
int identifyCommand(char *);
int identifyOption(char *);
void *handle_client(void *);

char osLogo[] = " ____                 _                   _    ___  ____\n|  _ \\  ___  __ _  __| |_ __   ___   ___ | |  / _ \\/ ___|\n| | | |/ _ \\/ _` |/ _` | '_ \\ / _ \\ / _ \\| | | | | \\___ \\\n| |_| |  __/ (_| | (_| | |_) | (_) | (_) | | | |_| |___) |\n|____/ \\___|\\__,_|\\__,_| .__/ \\___/ \\___/|_|  \\___/|____/\n                       |_|                  \n";

char welcomeMessage[] = "***Server Version of the OS, Now it supports multiple clients!!!***\nWelcome to the Beta version of Deadpool OS. This is the third version of the system. More updates will follow.\n\nfor help use \"help\"\n";

typedef struct ClientS
{
    int id;
    int clientSocket;
    char *clientIP;
} clientS;
int main()
{
    //printf("%s %s server\nWaiting for connection....\n", OS, VERSION);
    int serverSocket = 0;
    int clientSocket = 0;
    struct sockaddr_in address;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        printf("error server socket");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Server socket %d Initalized\n", SERV_PORT);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(SERV_PORT);

    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    int k = listen(serverSocket, BACKLOG);
    if (k < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Listening at port %d for incoming connection....\n", SERV_PORT);
    }

    int ClientCounter = 0;
    while (1)
    {
        int addrlen = sizeof(address);
        clientS client;

        clientSocket = accept(serverSocket, (struct sockaddr *)&address, (socklen_t *)&addrlen);

        client.clientSocket = clientSocket;

        if (client.clientSocket < 0)
        {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        else
        {
            ClientCounter++;
            client.id = ClientCounter;
            client.clientIP = inet_ntoa(address.sin_addr);
            printf("Connected with Client #%d %s.\n", client.id, client.clientIP);
            if (k >= 0)
                printf("Listening at port 8880 for incoming connection....\n");
        }
        if (fork() == 0)
        {
            pthread_t th;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_create(&th, &attr, handle_client, &client);
        }

        //printf("I am here");
    }

    close(serverSocket);
    return 0;
}
void *handle_client(void *c)
{
    clientS *client = (clientS *)c;

    int s = (int)client->clientSocket;
    char *clientIP = (char *)client->clientIP;
    int clientID = (int)client->id;
    char clientIDString[2];
    sprintf(clientIDString, "%d", clientID);
    char USER[7] = "guest";

    strcat(USER, clientIDString);

    char t[10];
    send(s, osLogo, strlen(osLogo), 0);
    recv(s, t, sizeof(t), 0);
    send(s, welcomeMessage, strlen(welcomeMessage) + 1, 0);
    while (1)
    {

        int fd[2];
        pipe(fd);
        char buffer[BUFFER_SIZE] = {0};
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("Error forking child\n");
        }
        else if (pid == 0)
        {
            close(fd[0]);

            send(s, USER, strlen(USER), 0);
            recv(s, t, sizeof(t), 0);
            send(s, OS, strlen(OS), 0);
            recv(s, buffer, sizeof(buffer), 0);
            printf("Client #%d %s, Recieved: %s\n", clientID, clientIP, buffer);

            write(fd[1], buffer, strlen(buffer) + 1);

            close(fd[1]);
            exit(EXIT_SUCCESS);
        }
        else
        {
            wait(NULL);
            close(fd[1]);

            char buffer[BUFFER_SIZE];
            read(fd[0], buffer, BUFFER_SIZE);
            if (strcmp(buffer, "disconnect") == 0)
            {

                printf("Client #%d %s disconnected\n", clientID, clientIP);
                close(s);
                //
                break;
            }
            //printf("Recieved: %s\n", buffer);
            char *command[COMMAND_SIZE] = {0};
            char *token = strtok(buffer, " ");

            int i = 0;
            while (token != NULL)
            {
                command[i++] = token;
                token = strtok(NULL, " ");
            }
            command[i++] = NULL;

            char line[1024] = {0};
            char bufferFile[4000] = {0};

            int fd2[2];
            pipe(fd2);
            pid_t pid2 = fork();

            if (pid2 < 0)
            {
                perror("Error forking child2\n");
            }
            else if (pid2 == 0)
            {
                close(fd2[0]);
                dup2(fd2[1], 1);
                dup2(fd2[1], 2);
                if (strcmp(command[0], "help") == 0)
                {
                    printf("\n%s OS Version %s offers the following commands:\n\t1) listdr : list the content of the directory, equivalent to ls and it uses the same set of options.\n\t2) printcdr : prints the current working directory, equivalent to pwd and it uses the same set of options.\n\t3) createdr : creates a new directory, equivalent to mkdir and it uses the same set of options.\n\t4) remove: removes file or directory, =rm, Usage: remove file, remove -F directory\n\t5) return: returns a string to the shell, =echo, Usage: return text.\n\t6) create: creates a new file, =touch, Usage: create filename\n\t7) move: moves a file or directory to another directory, =mv , Usage: move file dir.\n\t8) clear: clears the content on the shell, =clear\n\t9) compare: compares the difference between two files, equivalent to diff, Usage: compare file1 file2\n\t10) help: shows the list of commands allowed in the OS and their description\n\t11) premission : modifies the access premissions of a file, equivalent to chmod and it uses the same set of options\n\t12) findAvenger: search inside avengers.txt file and locate a certain avenger by name or letter, example: findAvenger Hulk, findAvenger a ...\n\t13) disconnect: kills connection with the connected client\n\t14) countF: counts the number of files and directories in the directory\n\t15)count ...:count the number of files with specified name, =ls | grep key | wc -l, Usage: count key\n\t16) details: get info about the OS.\n\t17)listSorted: list the files and directories in a sorted manner, =ls | sort | less\n\t18)copy : creates a copy of a file or directory, =cp,  Usage: copy file1 file2\n\t19)ipinfo : prints network interfaces information, =ifconfig\n\t20)read:display content of file =cat\n\t21)wordC: count the frequency of word = wc\n\t22)psinfo: print information about current running processes, =ps.\n\n\tSupported Options: -all listSorted and -F for remove, -details for psinfo\n", OS, VERSION);

                    close(fd2[1]);
                    exit(0);
                }
                if (strcmp(command[0], "details") == 0)
                {

                    printf("\n%s OS Version %s\nCSC326 Project - Phase 3\n\nLebenese American University\n%s, %s\n", OS, VERSION, AUTHORS, YEAR);

                    close(fd2[1]);
                    exit(0);
                }
                else if (strcmp(command[0], "countF") == 0)
                {
                    char *firstpart[5] = {"ls", NULL};
                    char *secondpart[10] = {"wc", "-l", NULL};
                    onePipe(firstpart, secondpart);
                }
                else if (strcmp(command[0], "findAvenger") == 0)
                {
                    char *firstpart[5] = {"cat", "avengers.txt", NULL};
                    char *secondpart[10] = {"grep", command[1], NULL};
                    onePipe(firstpart, secondpart);
                }
                else if (strcmp(command[0], "count") == 0)
                {
                    char *firstpart[5] = {"ls", NULL};
                    char *secondpart[10] = {"grep", command[1], NULL};
                    char *thirdpart[10] = {"wc", "-l", NULL};
                    twoPipe(firstpart, secondpart, thirdpart);
                }
                else if (strcmp(command[0], "listSorted") == 0)
                {
                    char *firstpart[5] = {"ls", NULL};
                    char *secondpart[10] = {"sort", NULL};
                    char *thirdpart[10] = {"less", NULL};
                    twoPipe(firstpart, secondpart, thirdpart);
                }
                else if (command[0][0] == '.' && command[0][1] == '/')
                {
                    execlp(command[0], command[0], NULL);
                }
                else
                {
                    int value = identifyCommand(command[0]);

                    if (value == -1)
                    {
                        printf("%s command not found\n", command[0]);

                        close(fd2[1]);
                        exit(0);
                    }
                    else
                    {
                        command[0] = sysCommands[value];
                        if (i > 2)
                        {

                            int k = 1;
                            while (k < (i - 1) && command[k][0] == '-')
                            {
                                int v = identifyOption(command[k]);
                                if (v == -1)
                                {

                                    printf("%s invalid option for command %s\n", command[k], command[0]);

                                    close(fd2[1]);
                                    exit(0);
                                }
                                else
                                    command[k] = sysOptions[v];
                                k++;
                            }
                        }
                        execvp(command[0], command);
                    }
                }
            }
            else
            {
                wait(NULL);
                //close(1);
                close(fd2[1]);
                int n = read(fd2[0], bufferFile, sizeof(bufferFile));

                send(s, bufferFile, strlen(bufferFile) + 1, 0);
                recv(s, t, strlen(t), 0);
                close(fd2[0]);
            }
        }
        close(fd[0]);
        close(fd[1]);
    }
    close(s);
}

int identifyCommand(char *command)
{
    for (int i = 0; i < NB_OF_COMMANDS; i++)
    {
        if (strcmp(command, deadpoolCommands[i]) == 0)
        {
            return i;
        }
    }

    return -1;
}

int identifyOption(char *option)
{
    for (int i = 0; i < NB_OF_OPTIONS; i++)
        if (strcmp(option, deadpoolOptions[i]) == 0)
            return i;
    return -1;
}
void onePipe(char **command, char **command1)
{

    int fd[2];
    pipe(fd);
    pid_t child = fork();
    if (child == 0)
    {
        close(1);
        dup(fd[1]);
        close(fd[1]);
        close(fd[0]);
        execvp(command[0], command);
    }
    else
    {
        wait(NULL);
        close(0);
        dup(fd[0]);
        close(fd[0]);
        close(fd[1]);
        execvp(command1[0], command1);
    }
}

void twoPipe(char **command, char **command2, char **command3)
{
    int fd[2];
    int fd1[2];
    pipe(fd1);
    pipe(fd);
    pid_t child = fork();
    if (child == 0)
    {
        close(1);
        dup(fd[1]);

        close(fd1[1]);
        close(fd1[0]);
        close(fd[1]);
        close(fd[0]);
        execvp(command[0], command);
    }
    else
    {
        pid_t child1 = fork();
        if (child1 == 0)
        {
            close(0);
            dup(fd[0]);

            close(1);
            dup(fd1[1]);

            close(fd[1]);
            close(fd[0]);
            close(fd1[0]);
            close(fd1[1]);

            execvp(command2[0], command2);
        }
        else
        {

            close(0);
            dup(fd1[0]);

            close(fd[0]);
            close(fd[1]);
            close(fd1[0]);
            close(fd1[1]);
            execvp(command3[0], command3);
        }
        wait(NULL);
        close(fd[0]);
        close(fd[1]);
        close(fd1[0]);
        close(fd1[1]);
    }
}
