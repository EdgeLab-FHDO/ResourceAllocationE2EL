#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
#include <errno.h>

using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	
	bool connected = false;

    char buffer[256];
	bzero(buffer,256);
	
    if (argc < 3) {
       //fprintf(stderr,"usage %s hostname port\n", argv[0]);
	   cout << "usage " << argv[0] << " hostname port" << endl;
       exit(0);
    }
    portno = atoi(argv[2]);
	
	//sleep(20);
	
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        //fprintf(stderr,"ERROR, no such host\n");
		cout << "ERROR, no such host" << endl;
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
	while(true)
	{
		close(sockfd);
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			cout << "ERROR opening socket: " << strerror(errno) << endl;
		}
		
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	
		while(!connected)
		{
			if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
			{
				cout << "ERROR connecting: " << strerror(errno) << endl;
				sleep(1);
				connected = false;
			}
			else
			{
				connected = true;
			}
		}
			
		while(connected)
		{
			cout << "counter: " << (int)buffer[0] << endl;
			
			n = send(sockfd,buffer,1,MSG_NOSIGNAL);
			if (n < 0) 
			{
				cout << "ERROR writing to socket: " << strerror(errno) << endl;
				connected = false;
			}
			
			bzero(buffer,256);
			n = recv(sockfd,buffer,1,0);
			if (n < 0) 
			{
				cout << "ERROR reading from socket: " << strerror(errno) << endl;
				connected = false;
			}
			 sleep(1);
		}
	}
	cout << "cleaning up" << endl;
    close(sockfd);
    return 0;
}