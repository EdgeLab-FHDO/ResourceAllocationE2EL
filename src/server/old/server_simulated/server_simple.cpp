/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

//using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
	
	//sleep(60);
	
	//::cout << "after sleep\n";
	
	auto fileStream = std::make_shared<ostream>();

    // Open stream to output file.
    pplx::task<void> requestTask = fstream::open_ostream(U("results.html")).then([=](ostream outFile)
    {
        *fileStream = outFile;

        // Create http_client to send the request.
        http_client client(U("http://meep-mg-manager"));
		
        // Build request URI and start the request.
        uri_builder builder(U("/v1/mg/mysrv/app/server"));
        //builder.append_query(U("q"), U("cpprestsdk github"));
        return client.request(methods::POST, builder.to_string());
    })

    // Handle response headers arriving.
    .then([=](http_response response)
    {
        //printf("Received response status code:%u\n", response.status_code());

		std::cout << "Received response status code: " << response.status_code() << std::endl;

        // Write response body into the file.
        return response.body().read_to_end(fileStream->streambuf());
    })

    // Close the file stream.
    .then([=](size_t)
    {
        return fileStream->close();
    });

    // Wait for all the outstanding I/O to complete and handle any exceptions
    try
    {
        requestTask.wait();
    }
    catch (const std::exception &e)
    {
        //printf("Error exception:%s\n", e.what());
		std::cout << "Error exception: " << e.what() << std::endl;
    }
	
	int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
	 bool connected = false;
	 
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
	 
	 while(true)
	 {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			std::cout << "ERROR opening socket: " << strerror(errno) << std::endl;
		}
		
		if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		{
			std::cout << "ERROR on binding: " << strerror(errno) << std::endl;
		}
		
		std::cout << "listening on port: " << portno << std::endl;
		listen(sockfd,5);
		clilen = sizeof(cli_addr);
		
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) 
		{
			std::cout << "ERROR on accept: " << strerror(errno) << std::endl;
		}
		
		close(sockfd);	// so that no one else can connect until the current connection is closed
		connected = true;
		
		
		struct timeval tv;
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		setsockopt(newsockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
		
		while(connected)
		{
			bzero(buffer,256);
			n = recv(newsockfd,buffer,1,0);
			if (n < 0) 
			{
				std::cout << "ERROR reading from socket: " << strerror(errno) << std::endl;
				connected = false;
			}
			
			std::cout << "count: "<< (int)buffer[0] << std::endl;
			buffer[0]++;

			n = send(newsockfd,buffer,1,MSG_NOSIGNAL);
			if (n < 0) 
			{
				std::cout << "ERROR writing to socket: " << strerror(errno) << std::endl;
				connected = false;
			}
		}
	 }
     close(newsockfd);
     close(sockfd);
     return 0; 
}