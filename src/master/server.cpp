#include "server.h"

Server::Server(string _name, int _application_id, string _location_point_of_access, string _location_zone)
{
	name = _name;
	application_id = _application_id;
	location_point_of_access = _location_point_of_access;
	location_zone = _location_zone;
}


Server::~Server()
{
}


string Server::get_name()
{
	return name;
}


int Server::get_application_id()
{
	return application_id;
}


string Server::get_location_point_of_access()
{
	return location_point_of_access;
}


string Server::get_location_zone()
{
	return location_zone;
}


int Server::assign_client(Client * client)
{
	active_clients.push_back(client);
	
	return 0;
}


int Server::remove_client(Client * client)
{
	for(auto client_iterator = active_clients.begin(); client_iterator != active_clients.end();)
	{
		if((*client_iterator)->get_name().compare(client->get_name()) == 0)
		{		
			client_iterator = active_clients.erase(client_iterator);
			
			return 0;
		}
		else
		{
			++client_iterator;
		}
	}
	
	return -1;
}


Client * Server::get_active_client(int index)
{
	return active_clients.at(index);
}


int Server::get_active_client_count()
{
	return active_clients.size();
}