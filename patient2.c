//
//  patient2.c
//  healthcenter
//
//  Created by Qiang Zheng on 11/3/14.
//  Copyright (c) 2014 Qiang Zheng. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAXDATASIZE 50

/* concat two string */
char* concat(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int main()
{
    FILE *fp1;
    char patient2str[MAXDATASIZE];
    FILE *fp2;
    char insurancestr[MAXDATASIZE];
    int lines_allocated = 20;
    int max_line_len = 50;
    struct hostent * localHost;
    char* localip;
    
    /* opening patient2.txt */
    fp1 = fopen("patient2.txt", "r");
    if (fp1 == NULL)
    {
        fprintf(stderr,"Error opening file.\n");
        exit(1);
    }
    
    fgets(patient2str,MAXDATASIZE,fp1);
    patient2str[20]='\0';
    
    /* opening patient2insurance.txt */
    fp2 = fopen("patient2insurance.txt", "r");
    if (fp2 == NULL)
    {
        fprintf(stderr,"Error opening file.\n");
        exit(1);
    }
    
    fgets(insurancestr,MAXDATASIZE,fp2);
    insurancestr[10]='\0';
    
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;  /* will point to the results */
    memset(&hints, 0, sizeof hints); /* make sure the struct is empty */
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; /* TCP stream sockets */
    localHost = gethostbyname("localhost");
    localip = inet_ntoa(*(struct in_addr *)*localHost->h_addr_list);
    if ((status = getaddrinfo(localip, "21157", &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    /* Creat a socket */
    int sockfd;
    if((sockfd = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol)) == -1)
    {   perror("server: socket build fail");
        exit(1);
    }
    
    if(connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {   close(sockfd);
        perror("client: connect fail");
        exit(1);
    }
    
    freeaddrinfo(servinfo); /* all done with this structure */
    
    char portstr[MAXDATASIZE];
    char auth_data[MAXDATASIZE];
    char confires[MAXDATASIZE];
    char avai_data[200];
    char* username;
    char* password;
    char* authstr;
    char* docport;
    char choice[10];
    const char s[2] = " ";
    
    /* get port number */
    recv(sockfd,portstr,MAXDATASIZE,0);
    portstr[5] = '\0';
    printf("\nPhase 1: Patient2 has TCP port number %s and IP address %s.\n", portstr,localip);
    
    /* Authentication */
    authstr = concat("authenticate ", patient2str);
    send(sockfd,authstr,strlen(authstr), 0);
    username = strtok(patient2str, s);
    password = strtok(NULL, s);
    
    printf("\nPhase 1: Authentication request from Patient 2 with username %s and password %s has been sent to the Health Center Server.\n", username, password);
    
    /* get authentciation result */
    recv(sockfd,auth_data,MAXDATASIZE-1,0);
    auth_data[7] = '\0';
    printf("\nPhase 1: Patient 2 authentication result: %s.\n", auth_data);
    if(strcmp(auth_data,"failure") == 0)
        exit(1);
    printf("\nPhase 1: End of Phase 1 for patient2.\n");
    
    /* Appointment Booking */
    send(sockfd,"available",strlen("available"), 0);
    
    int m = recv(sockfd,avai_data,100,0);
    avai_data[m] = '\0';
    
    printf("\nPhase 2: The following appointments are available for patient2: \n%s", avai_data);
    /* deal with the received data */
    char **linedata = (char **)malloc(sizeof(char*)*lines_allocated); /* Allocate lines of text */
    int k=0,i ,j;
    int lines, flag=0;
    for (i=0;1;i++)
    {
        linedata[i] = malloc(max_line_len);
        for (j=0; 1; j++)
        {
            linedata[i][j] = avai_data[k];
            k++;
            if ( avai_data[k-1] =='\n' || avai_data[k] =='\0')
                break;
        }
        if ( avai_data[k]=='\0' )
            break;
    }
    lines = i;
    
    /* Choose available time */
    printf("\nPlease enter the preferred appointment index and press enter:");
    fgets(choice,10,stdin);
    choice[strlen(choice)-1]='\0';
    /* check the choosed time */
    while (flag == 0)
    {
        for (i=0; i<lines+1; i++)
        {
            if(strcmp(strtok(linedata[i],s),choice) == 0)
            {   flag = 1;
                break;
            }
        }
        if (flag==0)
        {
            printf("\nYour choice does not match with the time indices displayed. Please re-enter a correct time index:");
            fgets(choice,10,stdin);
            choice[strlen(choice)-1]='\0';
        }
    }
    
    /* send request */
    send(sockfd,concat("selection ", choice),strlen(concat("selection ", choice)), 0);
    
    /* receive appointment confirmation */
    recv(sockfd,confires,MAXDATASIZE-1,0);
    if( strcmp(confires,"notavailable") == 0)
    {
        printf("\nPhase 2: The requested appointment from Patient 2 is not available. Exiting...\n");
        exit(1);
    }
    strtok(confires, s);
    docport = strtok(NULL, s);
    printf("\nPhase 2: The requested appointment from Patient 2 is available and reserved to Patient 1. The assigned doctor number is %s.\n", docport);
    close(sockfd);
    
    
    
    /* use UDP to communnicate with doctor */
    struct addrinfo udphints;
    struct addrinfo *udpservinfo;  /* will point to the results */
    memset(&udphints, 0, sizeof udphints); /* make sure the struct is empty */
    udphints.ai_family = AF_UNSPEC;
    udphints.ai_socktype = SOCK_DGRAM; /* UDP stream sockets */
    if ((status = getaddrinfo(localip, docport, &udphints, &udpservinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    
    /* Creat a socket */
    int udpsockfd;
    if((udpsockfd = socket(udpservinfo->ai_family,udpservinfo->ai_socktype,udpservinfo->ai_protocol)) == -1)
    {   perror("server: socket build fail");
        exit(1);
    }
    
    sendto(udpsockfd, "patient2", strlen("patient2"), 0, udpservinfo->ai_addr, udpservinfo->ai_addrlen);
    
    char udpport[MAXDATASIZE];
    char esticost[MAXDATASIZE];
    
    recvfrom(udpsockfd, udpport, MAXDATASIZE, 0, udpservinfo->ai_addr, &udpservinfo->ai_addrlen);
    
    printf("\nPhase 3: Patient 2 has a dynamic UDP port number %s and IP address %s.\n", udpport, localip);
    
    /* send estimation request to doctor */
    sendto(udpsockfd, insurancestr, strlen(insurancestr), 0, udpservinfo->ai_addr, udpservinfo->ai_addrlen);
    
    printf("\nPhase 3: The cost estimation request from Patient 2 with insurance plan %s has been sent to the doctor with port number %s and IP address %s.\n", insurancestr, docport, localip);
    
    recvfrom(udpsockfd, esticost, MAXDATASIZE, 0, udpservinfo->ai_addr, &udpservinfo->ai_addrlen);
    esticost[strlen(esticost)-3] = '\0';
    
    /* receive estimation cost from doctor */
    if (strcmp(docport, "41157") == 0)
    {
        printf("\nPhase 3: Patient 2 has received %s$ estimation cost from doctor with portnumber %s and name doc1.\n", esticost, docport);
    }
    else {
        printf("\nPhase 3: Patient 2 has received %s$ estimation cost from doctor with portnumber %s and name doc2.\n", esticost, docport);
    }
    
    printf("\nPhase 3: End of Phase 3 for Patient 2.\n");
    
    close(udpsockfd);
    
    return 0;
}
