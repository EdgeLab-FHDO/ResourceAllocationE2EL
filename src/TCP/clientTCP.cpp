#include "clientTCP.h"

using namespace std;

ClientTCP::ClientTCP()
{
	connected = false;
	last_error = "no error yet";
}


ClientTCP::~ClientTCP()
{
	disconnect();
}


int ClientTCP::connect_to(const char* address, int port, int _timeout)
{
	if(connected)
	{
		last_error = "ERROR socket already connected";
		return -1;
	}
	
	struct hostent* server;
	
	server = gethostbyname(address);
    if (server == NULL) {
		last_error = "no such host: ";
		last_error += address;
		return -1;
    }
	
	struct sockaddr_in socket_address;
	
	bzero((char *) &socket_address, sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&socket_address.sin_addr.s_addr, server->h_length);
    socket_address.sin_port = htons(port);
	
	socket_id = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_id < 0)
	{
		last_error = "ERROR opening socket: ";
		last_error += strerror(errno);
		return -1;
	}
	
	timeout = _timeout;
			
	/*struct timeval tv;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;
	if(setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0)
	{
		last_error = "ERROR setting timeout: ";
		last_error += strerror(errno);
		disconnect();
		return -1;
	}*/
	
	long socket_arguments = fcntl(socket_id, F_GETFL, NULL);
	if( socket_arguments < 0) 
	{ 
		last_error = "ERROR connecting: ";
		last_error += strerror(errno);
		return -1;
	} 
	
	socket_arguments |= O_NONBLOCK; 
	if(fcntl(socket_id, F_SETFL, socket_arguments) < 0)
	{ 
		last_error = "ERROR connecting: ";
		last_error += strerror(errno);
		return -1;
	} 
	
	if (connect(socket_id,(struct sockaddr *) &socket_address,sizeof(socket_address)) < 0) 
	{
		if(errno == EINPROGRESS)
		{
			struct timeval tv;
			tv.tv_sec = 5; 
			tv.tv_usec = 0; 
			
			fd_set write_fd_s;
			
			FD_ZERO(&write_fd_s); 
			FD_SET(socket_id, &write_fd_s); 
			
			if (select(socket_id + 1, NULL, &write_fd_s, NULL, &tv) <= 0)		// 0 if it is a timeout
			{ 
				last_error = "ERROR connecting: ";
				last_error += strerror(errno);
				return -1;
			} 

			int connection_error_number;
			socklen_t connection_error_number_length = sizeof(int);
			
			if (getsockopt(socket_id, SOL_SOCKET, SO_ERROR, (void*)(&connection_error_number), &connection_error_number_length) < 0)
			{ 
				last_error = "ERROR connecting: ";
				last_error += strerror(errno);
				return -1;
			} 
			
			if (connection_error_number != 0)
			{ 
				last_error = "ERROR connecting: ";
				last_error += strerror(connection_error_number);
				return -1;
			}
		}
		else
		{
			last_error = "ERROR connecting: ";
			last_error += strerror(errno);
			return -1;
		}
	}
	
	socket_arguments = fcntl(socket_id, F_GETFL, NULL);
	if( socket_arguments < 0) 
	{ 
		last_error = "ERROR connecting: ";
		last_error += strerror(errno);
		disconnect();
		return -1;
	} 
	
	socket_arguments &= ~O_NONBLOCK;
	if(fcntl(socket_id, F_SETFL, socket_arguments) < 0)
	{ 
		last_error = "ERROR connecting: ";
		last_error += strerror(errno);
		disconnect();
		return -1;
	} 
	
	connected = true;
	
	return 0;
}


int ClientTCP::send_data(const char* data, int size)
{
	if(!connected)
	{
		last_error = "ERROR socket not connected";
		return -1;
	}
	
	int n = send(socket_id, data, size, MSG_NOSIGNAL);
	if (n < 0) 
	{
		last_error = "ERROR sending data: ";
		last_error += strerror(errno);
		disconnect();
		return -1;
	}
	
	return 0;
}


int ClientTCP::receive_data(char* data, int& size)
{
	if(!connected)
	{
		last_error = "ERROR socket not connected";
		return -1;
	}
	
	time_point<system_clock, milliseconds> start_time = time_point_cast<milliseconds>(system_clock::now());
	
	int number_of_bytes_ready = 0;
	bool should_wait = true;
	while(should_wait)
	{
		usleep(5000);
		
		int new_value = 0;
		if(ioctl(socket_id, FIONREAD, &new_value) < 0)
		{
			last_error = "ERROR receiving data: ";
			last_error += strerror(errno);
			disconnect();
			return -1;
		}
		if(number_of_bytes_ready == 0)	// if no data is received at all then wait for a normal timeout
		{
			time_point<system_clock, milliseconds> end_time = time_point_cast<milliseconds>(system_clock::now());
			int wait_duration_ms = (end_time - start_time).count();
			if(wait_duration_ms >= timeout)
			{
				should_wait = false;
			}
		}
		else if(new_value == number_of_bytes_ready)	// if some data is received then wait only for 5ms without more data received
		{
			should_wait = false;
		}
		
		number_of_bytes_ready = new_value;
		
		if(number_of_bytes_ready >= size)
		{
			should_wait = false;
		}
	}
	
	int n = recv(socket_id, data, size, 0);
	if (n < 0)
	{
		last_error = "ERROR receiving data: ";
		last_error += strerror(errno);
		disconnect();
		return -1;
	}
	else if(n == 0)	// connection lost without error
	{
		last_error = "ERROR no data received";
		disconnect();
		return -1;
	}
	
	size = n;
	
	return 0;
}


bool ClientTCP::is_connected()
{
	return connected;
}


void ClientTCP::disconnect()
{
	connected = false;
	close(socket_id);
}


string ClientTCP::get_last_error()
{
	return last_error;
}