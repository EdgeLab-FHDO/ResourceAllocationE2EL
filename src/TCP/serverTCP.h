#ifndef SERVER_TCP_H
#define SERVER_TCP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
#include <errno.h>
#include <chrono>
#include <vector>
#include <thread>

using namespace std;
using namespace std::chrono;

class ServerTCP {
	
	private:
		
		int socket_id;
		bool connected;
		std::string last_error;
		int connection_timeout;
		vector<ServerTCP *> threads_connection_list;
		vector<thread> threads_list;
	
	public:
		
		ServerTCP();
		~ServerTCP();
		ServerTCP(ServerTCP &&) = default;	// required to get threads working
		int listen_on(int port, int listen_timeout, int connection_timeout);
		int multithread_listen_on(int port, int connection_timeout, void (* thread_function)(ServerTCP * TCP_connection));
		void join_threads();
		void set_socket_id(int socket_id, int connection_timeout);
		int send_data(const char* data, int size);
		int receive_data(char* data, int& size);
		int receive_data_exact_size_timeout_without_error(char* data, int& size);
		int receive_data_exact_size(char* data, int& size);
		int receive_data_exact_size_no_timeout(char* data, int& size);
		void disconnect();
		bool is_connected();
		std::string get_last_error();
};

#endif	//SERVER_TCP_H