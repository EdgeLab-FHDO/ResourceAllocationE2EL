#ifndef CLIENT_TCP_H
#define CLIENT_TCP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
#include <errno.h>
#include <atomic>
#include <thread>
#include <chrono>

using namespace std;
using namespace std::chrono;

class ClientTCP {
	
	private:
		
		int socket_id;
		bool connected;
		string last_error;
		int timeout;
	
	public:
		
		ClientTCP();
		~ClientTCP();
		int connect_to(const char* address, int port, int _timeout);
		int send_data(const char* data, int size);
		int receive_data(char* data, int& size);
		bool is_connected();
		void disconnect();
		string get_last_error();
};

#endif //CLIENT_TCP_H