#include "node.h"

Node::Node(string _name, int _application_id, string _location_point_of_access, string _location_zone)
{
	name = _name;
	application_id = _application_id;
	location_point_of_access = _location_point_of_access;
	location_zone = _location_zone;
}


Node::~Node()
{
}


string Node::get_name()
{
	return name;
}


int Node::get_application_id()
{
	return application_id;
}


string Node::get_location_point_of_access()
{
	return location_point_of_access;
}


string Node::get_location_zone()
{
	return location_zone;
}

void Node::add_client(Client * client)
{
	clients.push_back(client);
}

void Node::remove_client(Client * client)
{
	for(auto client_iterator = clients.begin(); client_iterator != clients.end();)
	{
		if((*client_iterator) == client)
		{		
			client_iterator = clients.erase(client_iterator);
		}
		else
		{
			++client_iterator;
		}
	}
}

Client * Node::get_client_at(int index)
{
	return clients.at(index);
}

int Node::get_clients_count()
{
	return clients.size();
}

/*
int Node::assign_server(Server * server)
{
	for(auto server_iterator = servers.begin(); server_iterator != servers.end(); ++server_iterator)
	{
		if((*server_iterator)->get_name().compare(server->get_name()) == 0)
		{		
			return -1;	// server already exists
		}
	}
	
	servers.push_back(server);
	
	return 0;
}
*/

/*
int Node::remove_server(Server * server)
{
	for(auto server_iterator = servers.begin(); server_iterator != servers.end();)
	{
		if((*server_iterator)->get_name().compare(server->get_name()) == 0)
		{		
			server_iterator = servers.erase(server_iterator);
			
			return 0;
		}
		else
		{
			++server_iterator;
		}
	}
	
	return -1;
}
*/

/*
Server * Node::get_next_free_server()
{
	for(auto server_iterator = servers.begin(); server_iterator != servers.end(); ++server_iterator)
	{
		if((*server_iterator)->get_active_client_count() == 0)
		{
			return (*server_iterator);
		}
	}
	
	return nullptr;
}
*/

/*
int Node::get_free_server_count()
{
	int free_servers_count = 0;
	
	for(auto server_iterator = servers.begin(); server_iterator != servers.end(); ++server_iterator)
	{
		if((*server_iterator)->get_active_client_count() == 0)
		{
			free_servers_count++;
		}
	}
	
	return free_servers_count;
}
*/

/*
Server * Node::get_server(int index)
{
	return servers.at(index);
}
*/

/*
int Node::get_server_count()
{
	return servers.size();
}
*/
