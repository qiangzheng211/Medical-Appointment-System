//
//  doctor.c
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
#include <unistd.h>
#include <sys/wait.h>

#define doc1port "41157"
#define doc2port "42157"

int main()
{
    int lines_allocated = 20;
    int max_line_len = 50;
    char **doc1str = (char **)malloc(sizeof(char*)*lines_allocated); /* Allocate lines of text */
    char **doc2str = (char **)malloc(sizeof(char*)*lines_allocated); /* Allocate lines of text */
    int i;
    int doc1lines; int doc2lines;
    struct hostent * localHost;
    char* docip;
    
    /* opening doc1.txt */
    FILE *fp1 = fopen("doc1.txt", "r");
    if (fp1 == NULL)
    {
        fprintf(stderr,"Error opening file.\n");
        exit(1);
    }
    
    for (i=0;1;i++)
    {
        /* Allocate space for the next line */
        doc1str[i] = malloc(max_line_len);
        if (fgets(doc1str[i],max_line_len-1,fp1)==NULL)
            break;
    }
    doc1lines = i;
    
    /* opening doc2.txt */
    FILE *fp2 = fopen("doc2.txt", "r");
    if (fp2 == NULL)
    {
        fprintf(stderr,"Error opening file.\n");
        exit(1);
    }
    
    for (i=0;1;i++)
    {
        /* Allocate space for the next line */
        doc2str[i] = malloc(max_line_len);
        if (fgets(doc2str[i],max_line_len-1,fp2)==NULL)
            break;
    }
    doc2lines = i;
    
    /* Some of codes below are modifed from Beej’s socket programming tutorial */
    /* Get address information: port number and IP address of doctor 1*/
    int status;
    struct addrinfo hints1;
    struct addrinfo *servinfo1;  /* will point to the results */
    memset(&hints1, 0, sizeof hints1); /* make sure the struct is empty */
    hints1.ai_family = AF_UNSPEC;
    hints1.ai_socktype = SOCK_DGRAM; /* UDP datagram sockets */
    localHost = gethostbyname("localhost");
    docip = inet_ntoa(*(struct in_addr *)*localHost->h_addr_list);
    if ((status = getaddrinfo(docip, doc1port , &hints1, &servinfo1)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    
    /* Creat a socket */
    int sockfd1;
    if((sockfd1 = socket(servinfo1->ai_family,servinfo1->ai_socktype,servinfo1->ai_protocol)) == -1)
    {   perror("doctor 1: socket build fail");
        exit(1);
    }
    
    /* bind it to the port we passed in to getaddrinfo(): */
    if(bind(sockfd1, servinfo1->ai_addr, servinfo1->ai_addrlen) == -1)
    {   close(sockfd1);
        perror("doctor 1: bind fail");
        exit(1);
    }
    
    freeaddrinfo(servinfo1);   /* free the linked-list */
    
    struct addrinfo hints2;
    struct addrinfo *servinfo2;  /* will point to the results */
    memset(&hints2, 0, sizeof hints2); /* make sure the struct is empty */
    hints2.ai_family = AF_UNSPEC;
    hints2.ai_socktype = SOCK_DGRAM; /* UDP datagram sockets */
    if ((status = getaddrinfo(docip, doc2port , &hints2, &servinfo2)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    
    /* Creat a socket */
    int sockfd2;
    if((sockfd2 = socket(servinfo2->ai_family,servinfo2->ai_socktype,servinfo2->ai_protocol)) == -1)
    {   perror("doctor 2: socket build fail");
        exit(2);
    }
    
    /* bind it to the port we passed in to getaddrinfo(): */
    if(bind(sockfd2, servinfo2->ai_addr, servinfo2->ai_addrlen) == -1)
    {   close(sockfd2);
        perror("doctor 2: bind fail");
        exit(1);
    }
    
    int max_sd;
    if(sockfd1 > sockfd2)
        max_sd = (sockfd1 + 1);
    else if(sockfd2 > sockfd1)
        max_sd = (sockfd2 + 1);
    
    freeaddrinfo(servinfo2);   /* free the linked-list */
    
    struct sockaddr_storage client_addr;
    socklen_t addr_size;
    addr_size = sizeof client_addr;
    char ipstr[50];
    char portstr[50];
    char username[50];
    char insuplan[50];
    char* money;
    char docstr[50];
    fd_set readfds;
    
    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(sockfd1, &readfds);
        FD_SET(sockfd2, &readfds);
        /* don't care about writefds and exceptfds */
        int ret = select(max_sd, &readfds, NULL, NULL,NULL);
        
        if(ret <0)
        {
            printf("Select thrown an exception\n");
            exit(1);
        }
        /* doctor1 deal with patient if choosed */
        else if(FD_ISSET(sockfd1, &readfds))
        {
            recvfrom(sockfd1, username, 50, 0, (struct sockaddr *) &client_addr, &addr_size);
            printf( "\nPhase 3: Doctor 1 has a static UDP port %s and IP address %s.\n", doc1port, docip);
            
            int rc = getnameinfo((struct sockaddr *)&client_addr, addr_size, ipstr, sizeof(ipstr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
            if (rc != 0)
            {   perror("cannot get client's address information");
                exit(1);
            }
            portstr[strlen(portstr)] = '\0';
            
            sendto(sockfd1, portstr, strlen(portstr), 0, (struct sockaddr *) &client_addr, addr_size);
            
            /* receive estimation request from patient */
            recvfrom(sockfd1, insuplan, 50, 0, (struct sockaddr *) &client_addr, &addr_size);
            
            printf("\nPhase 3: Doctor 1 receives the request from the patient with port number %s and name %s with the insurance plan %s.\n", portstr, username, insuplan);
            for (i=0; i<doc1lines; i++)
            {   strcpy(docstr,doc1str[i]);
                if (strcmp(insuplan, strtok(docstr, " ")) == 0)
                {
                    money =strtok(NULL, " ");
                    break;
                }
            }
            money[strlen(money)-1] = '\0';
            
            /* send estimation cost to patient */
            sendto(sockfd1, money, strlen(money), 0, (struct sockaddr *) &client_addr, addr_size);
            printf("\nPhase 3: Doctor 1 has sent estimated price %s$ to patient with port number %s.\n", money, portstr);
        }
        
        /* doctor2 deal with patient if choosed */
        else if(FD_ISSET(sockfd2, &readfds))
        {
            recvfrom(sockfd2, username, 50, 0, (struct sockaddr *) &client_addr, &addr_size);
            printf( "\nPhase 3: Doctor 2 has a static UDP port %s and IP address %s.\n", doc2port, docip);
            
            int rc = getnameinfo((struct sockaddr *)&client_addr, addr_size, ipstr, sizeof(ipstr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
            if (rc != 0)
            {   perror("cannot get client's address information");
                exit(1);
            }
            portstr[strlen(portstr)] = '\0';
            
            sendto(sockfd2, portstr, strlen(portstr), 0, (struct sockaddr *) &client_addr, addr_size);
            
            /* receive estimation request from patient */
            recvfrom(sockfd2, insuplan, 50, 0, (struct sockaddr *) &client_addr, &addr_size);
            
            printf("\nPhase 3: Doctor 2 receives the request from the patient with port number %s and name %s with the insurance plan %s.\n", portstr, username, insuplan);
            for (i=0; i<doc2lines; i++)
            {   strcpy(docstr,doc2str[i]);
                if (strcmp(insuplan, strtok(docstr, " ")) == 0)
                {
                    money =strtok(NULL, " ");
                    break;
                }
            }
            money[strlen(money)-1] = '\0';
            
            /* send estimation cost to patient */
            sendto(sockfd2, money, strlen(money), 0, (struct sockaddr *) &client_addr, addr_size);
            
            printf("\nPhase 3: Doctor 2 has sent estimated price %s$ to patient with port number %s.\n", money, portstr);
        }
    }
    
    return 0;
}

