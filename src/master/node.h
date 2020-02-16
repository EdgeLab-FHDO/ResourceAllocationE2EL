#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>

#include "server.h"
#include "client.h"

class Server;
class Client;

using namespace std;

class Node
{
	private:
	
	string name;
	
	int application_id;
	
	string location_point_of_access;
	
	string location_zone;
	
	//vector<Server *> servers;
	
	vector<Client *> clients;
	
	public:
	
	Node(string _name, int _application_id, string _location_point_of_access, string _location_zone);
	~Node();
	
	string get_name();
	
	int get_application_id();
	
	string get_location_point_of_access();
	string get_location_zone();
	
	void add_client(Client * client);
	
	void remove_client(Client * client);
	
	Client * get_client_at(int index);
	int get_clients_count();
	
	//int assign_server(Server * server);
	
	//int remove_server(Server * server);
	
	//Server * get_next_free_server();
	//int get_free_server_count();
	
	//Server * get_server(int index);
	//int get_server_count();
};

#endif //NODE_H