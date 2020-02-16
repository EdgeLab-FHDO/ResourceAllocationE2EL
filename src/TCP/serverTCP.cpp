#include "serverTCP.h"

using namespace std;

ServerTCP::ServerTCP()
{
	connected = false;
	last_error = "no error yet";
}


ServerTCP::~ServerTCP()
{
	//cout << "destroying connection of socket number " << socket_id << endl;
	
	disconnect();
}


int ServerTCP::listen_on(int port, int listen_timeout, int connection_timeout)
{
	if(connected)
	{
		last_error = "ERROR socket already connected";
		return -1;
	}
		
	struct sockaddr_in socket_address;
	
	bzero((char *) &socket_address, sizeof(socket_address));
	socket_address.sin_family = AF_INET;
	socket_address.sin_addr.s_addr = INADDR_ANY;
	socket_address.sin_port = htons(port);
	
	int socket_server = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_server < 0)
	{
		last_error = "ERROR opening socket: ";
		last_error += strerror(errno);
		return -1;
	}
	
	int address_reuse = 1;
	
	if(setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &address_reuse, sizeof(address_reuse)) < 0)
	{
		last_error = "ERROR setting address_reuse: ";
		last_error += strerror(errno);
		close(socket_server);
		return -1;
	}
	
	if (bind(socket_server, (struct sockaddr *) &socket_address,sizeof(socket_address)) < 0) 
	{
		last_error = "ERROR on binding: ";
		last_error += strerror(errno);
		close(socket_server);
		return -1;
	}
	
	if(listen(socket_server,1) < 0)
	{
		last_error = "ERROR on listen: ";
		last_error += strerror(errno);
		close(socket_server);
		return -1;
	}
	
	struct timeval tv;
	tv.tv_sec = listen_timeout / 1000;
	tv.tv_usec = (listen_timeout % 1000) * 1000;
	
	fd_set read_fd_s;
	
	FD_ZERO(&read_fd_s);
	FD_SET(socket_server, &read_fd_s);

	int n = select(socket_server + 1, &read_fd_s, NULL, NULL, &tv);
	if (n < 0)
	{
		last_error = "ERROR on select: ";
		last_error += strerror(errno);
		close(socket_server);
		return -1;
	}
	else if(n == 0)	// connection lost without error
	{
		last_error = "ERROR no connection received";
		close(socket_server);
		return -1;
	}
	
	struct sockaddr_in client_address;
	socklen_t client_address_length = sizeof(client_address);
			
	socket_id = accept(socket_server, (struct sockaddr *) &client_address, &client_address_length);
	if (socket_id < 0) 
	{
		last_error = "ERROR on accept: ";
		last_error += strerror(errno);
		close(socket_server);
		return -1;
	}
	
	this->connection_timeout = connection_timeout;
	
	/*tv.tv_sec = connection_timeout / 1000;
	tv.tv_usec = (connection_timeout % 1000) * 1000;
	if(setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0)
	{
		last_error = "ERROR setting timeout: ";
		last_error += strerror(errno);
		close(socket_server);
		disconnect();
		return -1;
	}*/
	
	close(socket_server);	// so that no more clients can connect
	
	connected = true;
	
	return 0;
}


int ServerTCP::multithread_listen_on(int port, int connection_timeout, void (* thread_function)(ServerTCP * TCP_connection))
{
	if(connected)
	{
		last_error = "ERROR socket already connected";
		return -1;
	}
		
	struct sockaddr_in socket_address;
	
	bzero((char *) &socket_address, sizeof(socket_address));
	socket_address.sin_family = AF_INET;
	socket_address.sin_addr.s_addr = INADDR_ANY;
	socket_address.sin_port = htons(port);
	
	int socket_server = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_server < 0)
	{
		last_error = "ERROR opening socket: ";
		last_error += strerror(errno);
		return -1;
	}
	
	int address_reuse = 1;
	
	if(setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &address_reuse, sizeof(address_reuse)) < 0)
	{
		last_error = "ERROR setting address_reuse: ";
		last_error += strerror(errno);
		close(socket_server);
		return -1;
	}
	
	if (bind(socket_server, (struct sockaddr *) &socket_address,sizeof(socket_address)) < 0) 
	{
		last_error = "ERROR on binding: ";
		last_error += strerror(errno);
		close(socket_server);
		return -1;
	}
	
	if(listen(socket_server, 50) < 0)
	{
		last_error = "ERROR on listen: ";
		last_error += strerror(errno);
		close(socket_server);
		return -1;
	}
	
	struct sockaddr_in client_address;
	socklen_t client_address_length = sizeof(client_address);
	
	while(true)
	{
		socket_id = accept(socket_server, (struct sockaddr *) &client_address, &client_address_length);
		if (socket_id < 0) 
		{
			last_error = "ERROR on accept: ";
			last_error += strerror(errno);
			close(socket_server);
			return -1;
		}
		
		cout << "client connected to socket " << socket_id << endl;
		
		ServerTCP * new_connection = new ServerTCP();
		new_connection->set_socket_id(socket_id, connection_timeout);
		
		threads_connection_list.push_back(new_connection);
		
		threads_list.push_back(thread(thread_function, new_connection));
	}
	
	close(socket_server);
		
	return 0;
}


// TODO: test this function
void ServerTCP::join_threads()
{
	for(auto thread_iterator = threads_list.begin(); thread_iterator != threads_list.end(); ++thread_iterator)
	{
		(*thread_iterator).join();
	}
}


void ServerTCP::set_socket_id(int socket_id, int connection_timeout)
{
	this->socket_id = socket_id;
	this->connection_timeout = connection_timeout;
	connected = true;
}


int ServerTCP::send_data(const char* data, int size)
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


int ServerTCP::receive_data(char* data, int& size)
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
			if(wait_duration_ms >= connection_timeout)
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


int ServerTCP::receive_data_exact_size_timeout_without_error(char* data, int& size)
{
	if(!connected)
	{
		last_error = "ERROR socket not connected";
		return -1;
	}
	
	time_point<system_clock, milliseconds> start_time = time_point_cast<milliseconds>(system_clock::now());
	
	int number_of_bytes_ready = 0;

	while(true)
	{
		if(ioctl(socket_id, FIONREAD, &number_of_bytes_ready) < 0)
		{
			last_error = "ERROR receiving data: ";
			last_error += strerror(errno);
			disconnect();
			return -1;
		}
		
		if(number_of_bytes_ready >= size)
		{
			break;
		}

		time_point<system_clock, milliseconds> end_time = time_point_cast<milliseconds>(system_clock::now());
		int wait_duration_ms = (end_time - start_time).count();
		if(wait_duration_ms >= connection_timeout)
		{
			size = 0;
			return -2;
		}
		
		usleep(5000);
	}
	
	int n = recv(socket_id, data, size, 0);
	if (n < 0)
	{
		last_error = "ERROR receiving data: ";
		last_error += strerror(errno);
		disconnect();
		return -1;
	}
	
	size = n;
	
	return 0;
}


int ServerTCP::receive_data_exact_size(char* data, int& size)
{
	if(!connected)
	{
		last_error = "ERROR socket not connected";
		return -1;
	}
	
	time_point<system_clock, milliseconds> start_time = time_point_cast<milliseconds>(system_clock::now());
	
	int number_of_bytes_ready = 0;

	while(true)
	{
		if(ioctl(socket_id, FIONREAD, &number_of_bytes_ready) < 0)
		{
			last_error = "ERROR receiving data: ";
			last_error += strerror(errno);
			disconnect();
			return -1;
		}
		
		if(number_of_bytes_ready >= size)
		{
			break;
		}

		time_point<system_clock, milliseconds> end_time = time_point_cast<milliseconds>(system_clock::now());
		int wait_duration_ms = (end_time - start_time).count();
		if(wait_duration_ms >= connection_timeout)
		{
			last_error = "ERROR data received is less than expected";
			disconnect();
			return -1;
		}
		
		usleep(5000);
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


int ServerTCP::receive_data_exact_size_no_timeout(char* data, int& size)
{
	if(!connected)
	{
		last_error = "ERROR socket not connected";
		return -1;
	}
		
	int number_of_bytes_ready = 0;

	while(true)
	{
		if (recv(socket_id, data, 1, MSG_PEEK) < 0)
		{
			last_error = "ERROR receiving data: ";
			last_error += strerror(errno);
			disconnect();
			return -1;
		}

		if(ioctl(socket_id, FIONREAD, &number_of_bytes_ready) < 0)
		{
			last_error = "ERROR receiving data: ";
			last_error += strerror(errno);
			disconnect();
			return -1;
		}
		
		if(number_of_bytes_ready >= size)
		{
			break;
		}

		usleep(5000);
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


bool ServerTCP::is_connected()
{
	return connected;
}


void ServerTCP::disconnect()
{
	//cout << "disconnecting socket number " << socket_id << endl;
	
	connected = false;
	close(socket_id);
	
	for(auto& thread_connection : threads_connection_list)
	{
		thread_connection->disconnect();
		
		delete thread_connection;
	}
}


std::string ServerTCP::get_last_error()
{
	return last_error;
}