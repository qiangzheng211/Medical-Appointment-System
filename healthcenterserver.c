
//
//  healthcenterserver.c
//  healthcenter
//
//  Created by Qiang Zheng on 11/2/14.
//  Copyright (c) 2014 Qiang Zheng. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define healthcenterserverport "21157"
#define MAXDATASIZE 50


void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

/* concat two string */
char* concat(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);/* +1 for the zero-terminator */
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int main()
{
    int lines_allocated = 20;
    int max_line_len = 50;
    char **userstr = (char **)malloc(sizeof(char*)*lines_allocated); /* Allocate lines of text */
    char **avaistr = (char **)malloc(sizeof(char*)*lines_allocated); /* Allocate lines of text */
    int i; int j;
    int userlines; int availines;
    int shmid;
    int* avaiflag;
    struct hostent * localHost;
    char* healthcenterserverip;
    
    /* use shared memory to record available appointments */
    shmid = shmget(IPC_PRIVATE, 10*sizeof(int), IPC_CREAT | 0666);
    if (shmid < 0) {
        printf("shmget error\n");
        exit(1);
    }
    avaiflag = shmat(shmid,0,0);
    
    /* opening users.txt */
    FILE *fp1 = fopen("users.txt", "r");
    if (fp1 == NULL)
    {
        fprintf(stderr,"Error opening file.\n");
        exit(1);
    }
    
    for (i=0;1;i++)
    {
        /* Allocate space for the next line */
        userstr[i] = malloc(max_line_len);
        if (fgets(userstr[i],max_line_len-1,fp1)==NULL)
            break;
        
        /* Get rid of CR or LF at end of line */
        for (j=(int)strlen(userstr[i])-1;j>=0 && (userstr[i][j]=='\n' || userstr[i][j]=='\r');j--);
        userstr[i][j+1]='\0';
    }
    userlines = i;
    
    /* opening availabilities.txt */
    FILE *fp2 = fopen("availabilities.txt", "r");
    if (fp2 == NULL)
    {
        fprintf(stderr,"Error opening file.\n");
        exit(1);
    }
    
    for (i=0;1;i++)
    {
        /* Allocate space for the next line */
        avaistr[i] = malloc(max_line_len);
        if (fgets(avaistr[i],max_line_len-1,fp2)==NULL)
            break;
    }
    availines = i;
    
    /* Some of codes below are modifed from Beej’s socket programming tutorial */
    /* Get address information: port number and IP address of Health Center Server */
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;  /* will point to the results */
    memset(&hints, 0, sizeof hints); /* make sure the struct is empty */
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; /* TCP stream sockets */
    hints.ai_protocol = IPPROTO_TCP;
    localHost = gethostbyname("localhost");
    healthcenterserverip = inet_ntoa(*(struct in_addr *)*localHost->h_addr_list);
    if ((status = getaddrinfo(healthcenterserverip, healthcenterserverport , &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    
    /* Creat a socket */
    int sockfd;
    if((sockfd = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol)) == -1)
    {   perror("server: socket build fail");
        exit(1);
    }
    
    /* bind it to the port we passed in to getaddrinfo(): */
    if(bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {   close(sockfd);
        perror("server: bind fail");
        exit(1);
    }
    
    freeaddrinfo(servinfo);   /* free the linked-list */
    
    /* Listen to connections from clients */
    if(listen(sockfd, 2) == -1)
    {   perror("listen fail");
        exit(1);
    }
    
    /* reap all dead processes */
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1); }
    
    /* Accept connections from clients */
    char user_data[MAXDATASIZE];
    int user_bytes;
    char* usefuldata;
    char user_choice[10];
    char askavai[20];
    char* username;
    char* password;
    char* choice;
    char linestr[75];
    char docstr[MAXDATASIZE];
    int k;
    
    while(1)
    {
        struct sockaddr_storage client_addr;
        socklen_t addr_size;
        int new_fd;
        addr_size = sizeof client_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
        if (new_fd == -1) {
            continue; }
        
        /* get the ip address and port number of clients */
        char ipstr[50];
        char portstr[50];
        int rc = getnameinfo((struct sockaddr *)&client_addr, addr_size, ipstr, sizeof(ipstr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
        if (rc != 0)
        {   perror("cannot get client's address information");
            exit(1);
        }
        portstr[strlen(portstr)] = '\0';
        printf( "\nPhase 1: The Health Center Server has port number %s and IP address %s.\n", healthcenterserverport, healthcenterserverip);
        
        if(!fork())
        {
            send(new_fd,portstr,strlen(portstr), 0);
            close(sockfd); /* child doesn't need the listener */
            /* get users' username and password */
            user_bytes = (int) recv(new_fd,user_data,MAXDATASIZE-1,0);
            user_data[user_bytes]='\0';
            const char s[2] = " ";
            char* authstr;
            authstr = strtok(user_data, s);
            
            /* Authentication */
            if(strcmp(authstr,"authenticate") == 0)
            {   username = strtok(NULL, s);
                password = strtok(NULL, s);
                printf( "\nPhase 1: The Health Center Server has received request from a patient with username %s and password %s.\n", username, password);
                usefuldata = concat(username," ");
                usefuldata = concat(usefuldata, password);
            }
            else { perror("authenticate error");
                exit(1);
            }
            
            /* check if username and password match */
            int resultflag = 0;
            for (j=0; j<userlines; j++)
            {
                if (strcmp(userstr[j],usefuldata) == 0)
                {
                    resultflag = 1;
                    break;
                }
            }
            if( resultflag == 1 )
            {   send(new_fd,"success",strlen("success"), 0);
                printf( "\nPhase 1: The Health Center Server sends the response success to patient with usernam %s.\n", username);
                /* Appointment Booking */
                recv(new_fd,askavai,MAXDATASIZE-1,0);
                askavai[9]='\0';
                if(strcmp(askavai,"available") != 0)
                {   perror("ask availability fail");
                    exit(1);
                }
                printf( "\nPhase 2: The Health Center Server, receives a request for available time slots from patients with port number %s and IP address %s.\n", portstr, ipstr);
                k=0;
                memset(linestr, 0, sizeof(linestr));
                for(j=0; j<availines; j++)
                {   
                    if(avaiflag[atoi(&avaistr[j][0])] == 1)
                        continue;
                    for(i=0; i<10; i++)
                    {   linestr[k] = avaistr[j][i];
                        k++;
                    }
                    linestr[k]='\n';
                    k++;
                }
                linestr[k]='\0';
                send(new_fd,linestr,strlen(linestr),0);
                printf( "\nPhase 2: The Health Center Server sends available time slots to patient with username %s.\n", username);
                
                /* receive user's selection */
                recv(new_fd,user_choice,MAXDATASIZE-1,0);
                if(strcmp(strtok(user_choice,s),"selection") != 0)
                {   perror("choose selection fail");
                    exit(1);
                }
                else
                {   choice = strtok(NULL,s);
                    choice[strlen(choice)]='\0';
                    printf("\nPhase 2: The Health Center Server receives a request for appointment %d from patient with port number %s and username %s.\n", atoi(choice), portstr, username);
                    for(j=0; j<availines; j++)
                    {
                        if ( avaiflag[atoi(choice)] == 1 )
                        {
                            send(new_fd,"notavailable",strlen("notavailable"), 0);
                            printf("\nPhase 2: The Health Center rejects the following appointment %d to patient with username %s.\n", atoi(choice), username);
                            exit(1);
                        }
                    }
                    avaiflag[atoi(choice)] = 1;
                    for(j=0; j<availines; j++)
                    {   if(j == atoi(choice)-1 )
                        {
                            for(i=11; i<21; i++)
                            {
                                docstr[i-11] = avaistr[j][i];
                            }
                            break;
                        }
                    }
                    docstr[10]='\0';
                    send(new_fd,docstr,strlen(docstr), 0);
                    /* confirmation */
                    printf("\nPhase 2: The Health Center confirms the following appointment %d to patient with username %s.\n", atoi(choice), username);
                    close(new_fd);
                    exit(0);
                }
            }
            else {  send(new_fd,"failure",strlen("failure"), 0);
                printf( "\nPhase 1: The Health Center Server sends the response failure to patient with usernam %s.\n", username);
                exit(1);
            }
        }
        close(new_fd);
    }
    return 0;
}

