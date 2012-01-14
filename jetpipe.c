/*
  jetpipe.c -- a minimal lp_server replacement.

  jetpipe.c is copyright 2012 Eric Belanger <bilange@hotmail.com>
  Original jetpipe (https://code.launchpad.net/~ogra/ltsp/feisty-ltsp-jetpipe)
  is copyright 2006, Oliver Grawert <orga@ubuntu.com>, Canonical Ltd.

  This file ports the jetpipe.py script from the LTSP project to the C 
  programming language.

  This port has been created in order to get rid of the Python dependency (>20Mb)
  in my static, non-LTSP ultra lightweight thin client solution. It is however a 
  bit more verbose than the original python script for diagnostic purposes.

  Although this port is complete feature-wise compared to jetpipe.py, I do not
  affirm that it has no known or otherwise bugs. Go easy, this is my 
  baby footsteps in C :-)

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation; either version 2, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include <stdlib.h>    //exit(), atoi
#include <string.h>    //memset
#include <sys/types.h> //fork, pid_t, umask, open, socket
#include <sys/socket.h>//socket
#include <netinet/in.h>//sockaddr_in
#include <sys/stat.h>  //umask, open
#include <unistd.h>    //fork, _exit, chdir
#include <errno.h>     //errno
#include <fcntl.h>     //open
#include <syslog.h>    //syslog
#include <stdio.h>     //syslog


#define MAXDATASIZE 300

void main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: %s <printerdevice> <port>\n", argv[0]);
    exit(0);
  }
  int port = atoi(argv[2]);

  pid_t pid1;
  pid_t pid2;

  pid1 = fork();
  if (pid1 == -1) { 
    syslog(LOG_USER, "can't fork #1, error %d, exiting.\n", errno);
    exit(EXIT_FAILURE);
  }
  if (pid1 == 0) { 
    setsid();
    umask(0);
    chdir("/");
    pid2 = fork();
    if (pid2 == -1) {
      syslog(LOG_USER, "can't fork #1, error %d, exiting.\n", errno);
      exit(EXIT_FAILURE);
    }
    if (pid2 == 0) { 
      //Closing all standards FDs, and re-opening them all to point to /dev/null
      fcloseall();
      open("/dev/null",O_RDWR);
      dup2(0,1);
      dup2(0,2);
      int port = atoi(argv[2]);

      struct sockaddr_in addr, client_addr;
      socklen_t client_addr_size;
      int client;
      int devfd,recvbytes,writtenbytes;
      char buf[MAXDATASIZE];

      memset(&addr, 0, sizeof(struct sockaddr_in));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = INADDR_ANY;

      int serverfd = socket(AF_INET, SOCK_STREAM,0);
      if (bind(serverfd,(struct sockaddr *) &addr, sizeof(struct sockaddr)) == -1) {
        syslog(LOG_USER,"bind() failed, exiting.");
        exit(1);
      }
      if (listen(serverfd, 2) == -1) {
        syslog(LOG_USER,"listen() failed, exiting.");
        exit(1);
      }
      syslog(LOG_USER,"Started. Device: %s, TCP Port: %s",argv[1],argv[2]);
      while (1) {
        while ((client = accept(serverfd,NULL,NULL)) != -1) {
          syslog(LOG_USER,"Connection accepted, client sockfd #%d",client);
          devfd = open(argv[1],O_WRONLY, 0777);

          recvbytes = recv(client, buf, MAXDATASIZE-1,0);
          while (recvbytes > 0) {
            writtenbytes = write(devfd,buf,recvbytes);
            recvbytes = recv(client, buf, MAXDATASIZE-1,0);
          }
          if (errno > 0) syslog(LOG_USER,"Error: errno %d: %s",errno, strerror(errno));
          fflush(NULL); 
          close(devfd);
          close(client);
          syslog(LOG_USER,"Terminated for sockfd #%d.",client);
        }
      }
    }
  }
}
