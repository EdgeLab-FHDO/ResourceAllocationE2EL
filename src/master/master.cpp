#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <chrono>
#include <float.h>
#include <limits>
#include <ctime>

#include "serverTCP.h"
#include "clientTCP.h"
#include "serializer.h"

#include "client.h"
#include "server.h"
#include "node.h"
#include "Poa.h"


#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
//using namespace concurrency::streams;       // Asynchronous streams

using namespace std;
using namespace std::chrono;

enum Event_type
{
	MOVE
};

enum Score_calculation_algorithm_type
{
	RANDOM,
	NEAREST,
	SCORE
};

struct Event
{
	milliseconds duration;
	Event_type type;
	string client_name;
	string point_of_access_name;
};

struct History_score
{
	Client * client;
	Node * node;
	double score;
	time_point<system_clock, milliseconds> last_update_time;
};

struct Node_with_score
{
	Node * node;
	double score;
	Poa * poa;
};

vector<Client *> clients;
vector<Server *> servers;
vector<Node *> nodes;
vector<Event> events;
vector<History_score *> history_scores;
vector<Poa *> poas;//b:


Score_calculation_algorithm_type score_calculation_algorithm;

double history_score_factor = -1;
double hardware_score_factor = -1;
double network_score_factor = -1;
double distance_score_factor = -1;
double history_decay_factor = 1 / 60 / 1000;	// decrease of number of disconnects per ms

fstream log_file;
fstream output_file;

bool no_limit = false;

void read_limits_file(string file_name);
void read_simulation_file(string file_name);;
void read_parameters_file(string file_name);
void read_poa_configuration(string file_name);
void read_poa_client_configuration(string file_name);
void write_number_of_connected_clients_to_output_file(time_point<system_clock, milliseconds> time);
void clients_number_writer_thread();
void servers_reply_thread(int port);
//Server * select_best_server(Client * client);
//Server * select_random_server(Client * client);
vector<Node_with_score> calculate_node_random(Client * client);
vector<Node_with_score> calculate_node_nearest(Client * client);
vector<Node_with_score> calculate_node_score(Client * client);
Node_with_score select_node(vector<Node_with_score> scores_list);
Server * get_or_create_server(string server_name, uint16_t application_id, string point_of_access, string zone);
Client * get_or_create_client(string client_name, uint16_t application_id, string user_equipment_name);
History_score * get_or_create_history_score(Client * client, Node * node);
void get_client_location(Client * client, string& point_of_access, string& zone);
void move_client(Client * client, string location);
int get_distance(Client * client, Server * server);
int get_distance(Client * client, Node * node);
//int get_distance_client_poa(Client * client,Node * node);
void simulation_thread(time_point<system_clock, milliseconds> simulation_start_time);



//vector<Node_with_score> calculate_node_possible_poa_score(Client * client ,vector<Poa*>possbile_poa_client);
void split(const string& str,const string& delim, vector<string>&  parts);
int main(int argc, char *argv[])
{	
	if (argc < 5) {
		cout << "usage: " << argv[0] << " client_port server_port no_limit parameters_file_name" << endl;
		return -1;
	}
	
	int port = atoi(argv[1]);//b:getting port number from argv command line what is shared
	int server_port = atoi(argv[2]);
	no_limit = atoi(argv[3]);
	string parameters_file_name = argv[4];
	
	/*
	try
	{
		http_client client(U("http://meep-mg-manager"));
		uri_builder builder(U("/v1/mg/master-srv/app/master"));
		http_response response = client.request(methods::POST, builder.to_string()).get();
		
		cout << "Received response status code: " << response.status_code() << endl;
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
	*/
	
	system_clock::time_point time_stamp_time_point = system_clock::now();// b:saving current time in a variable
	time_t time_stamp_time_t = system_clock::to_time_t(time_stamp_time_point);// b:converting to seconds epoch time
	string time_stamp_string = ctime(&time_stamp_time_t);//b: convert to local time is characters
	
	// replace spaces with underscores
	while(true)
	{
		size_t position = time_stamp_string.find(" ");	// find next space
		
		if(position == string::npos) break;	// if no more spaces are found
		
		time_stamp_string.replace(position, 1, "_");	// replace with underscore
	}
	
	log_file.open("log_" + time_stamp_string + ".txt", fstream::out);//b: create a file in write mode to write logs may be
	if(!log_file.is_open())
	{
		cout << "ERROR can not open file " << "log.txt" << " for writing" << endl;
		return -1;
	}
	
	output_file.open("output_" + time_stamp_string + ".csv", fstream::out);//b:create a file to output
	if(!output_file.is_open())
	{
		cout << "ERROR can not open file " << "output.csv" << " for writing" << endl;
		return -1;
	}
	
	read_limits_file("limits.txt");//b:limits of node and there association with the other layers of network
	
	read_simulation_file("simulation.txt");
	
	read_parameters_file(parameters_file_name);//
	
	read_poa_configuration("poa_configuration.txt");

	read_poa_client_configuration("poa_client_configuration.txt");

	auto time_now = time_point_cast<milliseconds>(system_clock::now());// b:understanding is required
	
	thread simulation(simulation_thread, time_now);// b:new thread is created to simulate client movement
	
	thread clients_number_writer(clients_number_writer_thread);// b:what is achieved by this
	
	thread servers_reply(servers_reply_thread, server_port);// b:what is achieved by this any check is being done ??
	
	//write_number_of_connected_clients_to_output_file(time_now);//b:client are created later
	
	log_file << endl << endl << time_now.time_since_epoch().count() << " start" << endl;
	
	ServerTCP connection_TCP;
	
	const int max_buffer_size = 256;
	char buffer[max_buffer_size];
	int buffer_size;
	
	Serializer serializer;
	
	while(true)
	{
		if(connection_TCP.listen_on(port, 120000, 2000) < 0)//b:is the client requesting for node in this port ??
		{
			std::cout << connection_TCP.get_last_error() << std::endl;
			sleep(1);
		}
		
		if(connection_TCP.is_connected())
		{
			buffer_size = max_buffer_size;
			if(connection_TCP.receive_data(buffer, buffer_size) < 0)
			{
				cout << connection_TCP.get_last_error() << endl;
				continue;
			}
			
			string name;
			char type;
			uint16_t application_id;
			string user_equipment_name;
			
			serializer.deserialize_request(buffer, name, application_id, type, user_equipment_name);// b:could not find the deserialize function
			
			//cout << "name: " << name << ", id: " << application_id << ", type: " << (int)type << ", user_equipment_name: " << user_equipment_name << endl;
			
			string server_hostname;
			uint16_t server_port;
			
			switch(type)
			{
				case 0:	// client
				{
					Client * client = get_or_create_client(name, application_id, user_equipment_name);
					
					Node * previous_node = client->get_node();
					
					client->set_node(nullptr);
					
					if(previous_node != nullptr)
					{
						auto time_now = time_point_cast<milliseconds>(system_clock::now());
						
						previous_node->remove_client(client);// b:still connection is not established y are we removig it
						
						History_score * history_score = get_or_create_history_score(client, previous_node);		// history of disconnects
						
						history_score->score++;
						
						cout << client->get_name() << " disconnected from " << previous_node->get_name() << endl;
						
						//write_number_of_connected_clients_to_output_file(time_now);
						
						log_file << time_now.time_since_epoch().count() << " " << client->get_name() << " disconnected from " << previous_node->get_name() << endl;

					}
					
					string point_of_access;
					string zone;
					
					get_client_location(client, point_of_access, zone);
					
					client->set_location_point_of_access(point_of_access);
					client->set_location_zone(zone);
				//	cout<<"test1"<<endl ;

					vector<Node_with_score> scores_list;
					
					switch(score_calculation_algorithm)

					{
						case RANDOM:
							scores_list = calculate_node_random(client);
						break;
						
						case NEAREST:
							scores_list = calculate_node_nearest(client);
						break;
						
						case SCORE:
							scores_list = calculate_node_score(client);
						break;
						
						default:
							scores_list = calculate_node_nearest(client);
							
							cout << "Score calculation algorithm not specified, defaulting to NEAREST" << endl << endl;
					}

					Node_with_score selected_node = select_node(scores_list);
					
					if(selected_node.score == -numeric_limits<double>::max())
					{
						server_hostname = "";
						server_port = 0;
						
						cout << "no nodes available for " << client->get_name() << endl << endl;
						
						log_file << time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count() << " no nodes available for " << client->get_name() << endl;
					}
					else
					{
						//move client should be here (new change)
						move_client(client, selected_node.poa->get_poa_name());//new change
						auto time_now = time_point_cast<milliseconds>(system_clock::now());
						
						selected_node.node->add_client(client);//new_change
						client->set_node(selected_node.node);//new_change
						
						cout << client->get_name() << " connected to " << selected_node.node->get_name() << endl;
						
						//write_number_of_connected_clients_to_output_file(time_now);
						
						log_file << time_now.time_since_epoch().count() << " " << client->get_name() << " connected to " << selected_node.node->get_name()<<"   " <<"coonected poa"<< "   " <<selected_node.poa->get_poa_name()<< "  "<< endl;
					
						server_hostname = "";
						server_hostname += "server";
						server_hostname += '-';
						server_hostname += selected_node.node->get_location_zone();//(new_change)
						
						if(selected_node.node->get_location_point_of_access() != "null")
						{
							server_hostname += '-';
							server_hostname += selected_node.node->get_location_point_of_access();//new change
						}
						
						if(application_id == 1)
						{
							server_port = 2500;
						}
						else if(application_id == 2)
						{
							server_port = 2501;
						}
					}
				
					serializer.serialize_response(buffer, buffer_size, server_hostname, server_port);
				
					if(connection_TCP.send_data(buffer, buffer_size) < 0)
					{
						cout << connection_TCP.get_last_error() << endl;
					}
				}
				break;
				
				//(should no longer occur, since we are using one multi-threaded server)
				case 1:	// server
				{
					connection_TCP.disconnect();
					/*buffer_size = max_buffer_size;
					if(connection_TCP.receive_data(buffer, buffer_size) < 0)
					{
						cout << connection_TCP.get_last_error() << endl;
						continue;
					}
					
					string point_of_access;
					string zone;
					
					serializer.deserialize_location(buffer, point_of_access, zone);
					
					cout << "server name: " << name << ", application id: " << application_id << ", point of access: " << point_of_access << ", zone: " << zone << endl << endl;
					
					Server * server = get_or_create_server(name, application_id, point_of_access, zone);
					
					for(auto node_iterator = nodes.begin(); node_iterator != nodes.end(); ++node_iterator)
					{
						if((*node_iterator)->get_application_id() == server->get_application_id()									// same application id
						&& (*node_iterator)->get_location_zone().compare(server->get_location_zone()) == 0 							// same zone
						&& (*node_iterator)->get_location_point_of_access().compare(server->get_location_point_of_access()) == 0)	// same point of access
						{
							(*node_iterator)->assign_server(server);
							break;
						}
					}

					while(server->get_active_client_count() != 0)
					{
						Client * client = server->get_active_client(0);
						client->remove_server(server);
						server->remove_client(client);
					}*/
				}
				break;
				
				default:
				
				cout << "type: " << (int)type << " does not specify an entity" << endl;
			}
			
			connection_TCP.disconnect();
		}
	}
	
	log_file.close();
	
	connection_TCP.disconnect();
	
	return 0;
}


void read_limits_file(string file_name)
{
	ifstream file(file_name);
	if(file.is_open())
	{
		string node_name;
		string zone_name;
		string point_of_access_name;
		int application_id;
		long cpu_allocated_time;
		long cpu_allocation_period;
		int application_limit;
		
		while(file >> node_name >> zone_name >> point_of_access_name >> application_id >> cpu_allocated_time >> cpu_allocation_period >> application_limit)
		{
			cout << "node_name: " << node_name << ", zone_name: " << zone_name << ", point_of_access_name: " << point_of_access_name  << ", application_id: " << application_id << ", cpu_allocated_time: " << cpu_allocated_time << ", cpu_allocation_period: " << cpu_allocation_period << ", application_limit: " << application_limit << endl;
			
			log_file << "node_name: " << node_name << ", zone_name: " << zone_name << ", point_of_access_name: " << point_of_access_name  << ", application_id: " << application_id << ", cpu_allocated_time: " << cpu_allocated_time << ", cpu_allocation_period: " << cpu_allocation_period << ", application_limit: " << application_limit << endl;
			
			Node * new_node = new Node(node_name, application_id, point_of_access_name, zone_name, application_limit);
			

			nodes.push_back(new_node);

		}
	}
	else
	{
		cout << "cannot open file " << file_name << endl;
	}
	
	file.close();
}

void read_poa_configuration(string file_name)
{
	ifstream file(file_name);
	string point_of_access_name;
	string zone_name;
	if(file.is_open())
	{
		while(file >> point_of_access_name >> zone_name)
		{
			if(point_of_access_name.compare("null")!=0)
			{
				cout << "poa: " << point_of_access_name << ", zone: " << zone_name << endl;
				Poa * new_poa = new Poa(point_of_access_name, zone_name);
				poas.push_back(new_poa);
			}
		}

	}
	else
	{
		cout << "cannot open file " << file_name << endl;
	}

	file.close();

}

void read_poa_client_configuration(string file_name)
{
	ifstream file(file_name);
	std:: string str;
	string client_name;
	uint16_t application_id;
	string user_equipment_name;
	string delimit =" ";
	string zone_name =" ";
	vector<Poa*> *possible_poas_to_client;
	int poas_count=0;
	static int count;


	vector<string> list_poas_to_client;
	vector<string>::iterator it;
	if(file.is_open())
	{

	   while(!file.eof())
	   {
		   while (std::getline(file, str))
		   {

			//   cout<< str << endl;
			   list_poas_to_client.clear();
			   split(str,delimit,list_poas_to_client);
			   count++;
			 //  cout<< "the size of vector received"  << list_poas_to_client.size()<< "count is" << count << endl ;
			   possible_poas_to_client = new vector<Poa*>();
			   poas_count=0;
			   for (it = list_poas_to_client.begin(); it != list_poas_to_client.end(); it++)
			   {
				   if(it==list_poas_to_client.begin())
				   {
					   client_name = *it;
				   }
				   else if(it ==(list_poas_to_client.begin()+1))
				   {
					   application_id = std::stoi(*it);
				   }
				   else if(it ==(list_poas_to_client.begin()+2))
				   {
					   user_equipment_name =*it;

				   }
				   else
				   {

				   }
				   if(it>list_poas_to_client.begin()+2)
				   {
					   Poa * new_poa = new Poa(*it,zone_name);
					   possible_poas_to_client->push_back(new_poa);
					   poas_count++;

				   }


			   }
			   Client * client = get_or_create_client(client_name, application_id, user_equipment_name);
			   client->set_possible_poas_client(possible_poas_to_client);
			   cout<<"number of Poas for " << client_name << "are" << poas_count << endl ;

		   }
		   cout << "done with config file"<< endl ;
	   }


	}


	else{
		cout << "Unable to open file";
	}


	file.close();


   cout<< "out of reading"<< endl;
}

void  split(const string& str, const string& delim, vector<string>& parts)
{

	//cout << "inside split" ;
   size_t start, end = 0;
  while (end < str.size()) {
    start = end;
    while (start < str.size() && (delim.find(str[start]) != string::npos)) {
      start++;  // skip initial whitespace
    }
    end = start;
    while (end < str.size() && (delim.find(str[end]) == string::npos)) {
      end++; // skip to end of word
    }
    if (end-start != 0) {  // just ignore zero-length strings.
      parts.push_back(string(str, start, end-start));
    }
  }



}



void read_simulation_file(string file_name)
{
	ifstream file(file_name);
	if(file.is_open())
	{
		long duration;
		string event_name;
		string client_name;
		string point_of_access_name;
		
		while(file >> duration >> event_name >> client_name >> point_of_access_name)
		{
			cout << "duration: " << duration << ", event_type: " << event_name << ", client_name: " << client_name << ", point_of_access_name: " << point_of_access_name << endl;
			
			log_file << "duration: " << duration << ", event_type: " << event_name << ", client_name: " << client_name << ", point_of_access_name: " << point_of_access_name << endl;
			
			Event new_event;
			
			new_event.duration = milliseconds(duration);
			if(event_name.compare("move") == 0) new_event.type = MOVE;
			new_event.client_name = client_name;
			new_event.point_of_access_name = point_of_access_name;
			
			events.push_back(new_event);
		}
	}
	else
	{
		cout << "cannot open file " << file_name << endl;
	}
	
	file.close();
}

void read_parameters_file(string file_name)
{
	ifstream file(file_name);
	if(file.is_open())
	{
		string score_calculation_algorithm_name;
		
		file >> score_calculation_algorithm_name
			>> history_score_factor
			>> hardware_score_factor
			>> network_score_factor
			>> distance_score_factor
			>> history_decay_factor;
			
		if(score_calculation_algorithm_name.compare("RANDOM") == 0) score_calculation_algorithm = RANDOM;
		else if(score_calculation_algorithm_name.compare("NEAREST") == 0) score_calculation_algorithm = NEAREST;
		else if(score_calculation_algorithm_name.compare("SCORE") == 0) score_calculation_algorithm = SCORE;
		else
		{
			score_calculation_algorithm = NEAREST;
			cout << "Score calculation algorithm not specified, defaulting to NEAREST" << endl << endl;
		}
		
		cout << "score_calculation_algorithm_name: " << score_calculation_algorithm_name << endl
			<< "history_score_factor: " << history_score_factor << endl
			<< "hardware_score_factor: " << hardware_score_factor << endl
			<< "network_score_factor: " << network_score_factor << endl
			<< "distance_score_factor: " << distance_score_factor << endl
			<< "history_decay_factor: " << history_decay_factor << endl
			<< endl;
			
		log_file << "score_calculation_algorithm_name: " << score_calculation_algorithm_name << endl
			<< "history_score_factor: " << history_score_factor << endl
			<< "hardware_score_factor: " << hardware_score_factor << endl
			<< "network_score_factor: " << network_score_factor << endl
			<< "distance_score_factor: " << distance_score_factor << endl
			<< "history_decay_factor: " << history_decay_factor << endl
			<< endl;
	}
	else
	{
		cout << "cannot open file " << file_name << endl;
	}
	
	file.close();
}

void write_number_of_connected_clients_to_output_file(time_point<system_clock, milliseconds> time)
{
	int number_of_connected_clients = 0;
	for(auto client_iterator = clients.begin(); client_iterator != clients.end(); ++client_iterator)
	{
		if((*client_iterator)->get_node() != nullptr)
		{
			number_of_connected_clients++;
		}
	}
	
	output_file << time.time_since_epoch().count() << ',' << number_of_connected_clients << endl;
}

Server * select_best_server(Client * client)
{
	Server * selected_server = 0;
	int selected_server_distance = 100;
	int distance;
	
	for(auto server_iterator = servers.begin(); server_iterator != servers.end(); ++server_iterator)
	{
		//cout << "select_best_server debug: " << "server name: " << (*server_iterator)->get_name() << ", server application id: " << (*server_iterator)->get_application_id() << ", server active clients count: " << (*server_iterator)->get_active_client_count() << ", client application id: " << client->get_application_id() << endl;
		
		if((*server_iterator)->get_application_id() == client->get_application_id() && (*server_iterator)->get_active_client_count() == 0)	// same application id and no clients are connected to it
		{
			//cout << "select_best_server debug: " << "selected server: " << (*server_iterator)->get_name() << endl;
			distance = get_distance(client, (*server_iterator));
			if(distance < selected_server_distance)
			{
				selected_server_distance = distance;
				selected_server = (*server_iterator);
			}
		}
	}
	return selected_server;
}


Server * select_random_server(Client * client)
{
	Server * selected_server = 0;
	int selected_server_distance = 100;
	int distance;
	
	for(auto server_iterator = servers.begin(); server_iterator != servers.end(); ++server_iterator)
	{
		//cout << "select_best_server debug: " << "server name: " << (*server_iterator)->get_name() << ", server application id: " << (*server_iterator)->get_application_id() << ", server active clients count: " << (*server_iterator)->get_active_client_count() << ", client application id: " << client->get_application_id() << endl;
		
		if((*server_iterator)->get_application_id() == client->get_application_id() && (*server_iterator)->get_active_client_count() == 0)	// same application id and no clients are connected to it
		{
			//cout << "select_best_server debug: " << "selected server: " << (*server_iterator)->get_name() << endl;
			distance = rand() % 100;
			if(distance < selected_server_distance)
			{
				selected_server_distance = distance;
				selected_server = (*server_iterator);
			}
		}
	}
	return selected_server;
}


vector<Node_with_score> calculate_node_random(Client * client)
{
	vector<Node_with_score> scores_list;
	
	Node_with_score score_entry;
	for(auto poa_iterator = poas.begin();poa_iterator!= poas.end(); ++poa_iterator)
	{
		 client->set_location_point_of_access((*poa_iterator)->get_poa_name());
		 for(auto node_iterator = nodes.begin(); node_iterator != nodes.end(); ++node_iterator)
		 {
			 if((*node_iterator)->get_application_id() == client->get_application_id())	// same application id
			 {
				 score_entry.score = rand() % 100;
			
				 score_entry.node = (*node_iterator);
			
				 scores_list.push_back(score_entry);
			 }
		 }
	}
	return scores_list;
}


vector<Node_with_score> calculate_node_nearest(Client * client)
{
	vector<Node_with_score> scores_list;
	
	Node_with_score score_entry;
	
	int distance;
	
	int distance_poa;//b:
	for(auto poa_iterator = poas.begin();poa_iterator!= poas.end(); ++poa_iterator)
	{
		 client->set_location_point_of_access((*poa_iterator)->get_poa_name());

			for(auto node_iterator = nodes.begin(); node_iterator != nodes.end(); ++node_iterator)
			{
				if((*node_iterator)->get_application_id() == client->get_application_id())	// same application id
				{
					distance = get_distance(client, (*node_iterator));
			
					score_entry.score = distance * -1;	// -1 to make the least distance node has the highest score
			
					score_entry.node = (*node_iterator);

					score_entry.poa = (*poa_iterator);
			
					scores_list.push_back(score_entry);
				}
			}
	}

	return scores_list;
}


vector<Node_with_score> calculate_node_score(Client * client)
{
	vector<Node_with_score> scores_list;
	
	Node_with_score score_entry;
	
	double score = 0;
	
	auto time_now = time_point_cast<milliseconds>(system_clock::now());

	log_file << time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count() << " score-based node selecetion for: " << client->get_name() << endl;
	for(auto poa_iterator = client->get_possible_poas_client()->begin();poa_iterator!= client->get_possible_poas_client()->end(); ++poa_iterator)
	{
		cout<<(*poa_iterator)->get_poa_name()<< endl;

     //   cout<<"test2"<<endl;

		client->set_location_point_of_access((*poa_iterator)->get_poa_name());


		for(auto node_iterator = nodes.begin(); node_iterator != nodes.end(); ++node_iterator)
		{
			if((*node_iterator)->get_application_id() == client->get_application_id())	// same application id
			{


					log_file << "node: " << (*node_iterator)->get_name() <<"  " << "poa"<< (*poa_iterator)->get_poa_name() << "Zone _Poa" << (*poa_iterator)->get_zone_of_poa() << "Zone_Client"
							<< client->get_location_zone() <<endl;//new changes1
			
					History_score * history_score = get_or_create_history_score(client, (*node_iterator));		// history of disconnects
			
					// apply decay over time to history score
					double history_score_decay_over_time = (time_now - history_score->last_update_time).count() * history_decay_factor;
					history_score->score -= history_score_decay_over_time;
			
					if(history_score->score < 0) history_score->score = 0;
			
					history_score->last_update_time = time_now;
			
					double hardware_score = (*node_iterator)->get_clients_count() / (*node_iterator)->get_application_limit();	// number of running servers (connected clients) relative to the max number of connected clients
			
					double distance_score = get_distance(client, (*node_iterator));	// distance between node and client
			
					double network_score = 0;	// number of connections crossing this node (not necessary directly connected)
			
					for(auto client_iterator = clients.begin(); client_iterator != clients.end(); ++client_iterator)
					{
						if((*client_iterator)->get_node() != nullptr)		// client is sending data
						{
							if((*client_iterator)->get_node() == (*node_iterator))	// client connected directly to node
							{
								network_score++;
							}
							else if((*client_iterator)->get_location_point_of_access().compare(client->get_location_point_of_access()) == 0) // clinet in the same point of access as the client to be evaluated
							{
								network_score++;
							}
							else if((*node_iterator)->get_location_point_of_access() == "null") 	// node is a zone node
							{
								if((*client_iterator)->get_location_zone().compare((*node_iterator)->get_location_zone()) == 0)	// client in same zone
								{
									if(get_distance((*client_iterator), (*client_iterator)->get_node()) > 1)	// client connected to a node outside of its point of access (thus crossing the zone)
									{
										network_score++;
									}
								}
								else // client not in same zone
								{
									if((*client_iterator)->get_node()->get_location_zone().compare((*node_iterator)->get_location_zone()) == 0) // the node which the client is connected to is in the same zone as the node to be evaluated
									{
										network_score++;
									}
								}
							}
							else 	// node is a point of access node
							{
								if((*client_iterator)->get_location_point_of_access().compare((*node_iterator)->get_location_point_of_access()) == 0)	// client in same point of access
								{
									network_score++;
								}
							}
						}
					}
			
					score = history_score_factor * history_score->score
						+ hardware_score_factor * hardware_score
						+ network_score_factor * network_score
						+ distance_score_factor * distance_score;
			
				log_file << "history_score: " << history_score->score
						<< ", hardware_score: " << hardware_score
						<< ", network_score: " << network_score
						<< ", distance_score: " << distance_score
						<< ", score: " << score << endl;
			
				score_entry.score = score;
			
				score_entry.node = (*node_iterator);
				
				score_entry.poa = (*poa_iterator);
			
				scores_list.push_back(score_entry);

			}
		}

	}
	
	return scores_list;


}


Node_with_score select_node(vector<Node_with_score> scores_list)//(new change)
{
	Node_with_score selected_score ;
	
	selected_score.node = nullptr;
	selected_score.poa = nullptr;
	selected_score.score = -numeric_limits<double>::max();
	
	for(auto score_entry_iterator = scores_list.begin(); score_entry_iterator != scores_list.end(); ++score_entry_iterator)
	{
		if(score_entry_iterator->node->is_full() == false || no_limit == true)// use limit only if no_limit is false
		{
			if(selected_score.score < score_entry_iterator->score)
			{
				selected_score = *score_entry_iterator;
			}

		}
		//cout << "node: " << (*score_entry_iterator).node->get_name() << ", score: " << (*score_entry_iterator).score << endl;
	}
	
	//return selected_score.node; (new change)
	return selected_score;//(new change)
}

Server * get_or_create_server(string server_name, uint16_t application_id, string point_of_access, string zone)
{
	for(auto server_iterator = servers.begin(); server_iterator != servers.end(); ++server_iterator)
	{
		if((*server_iterator)->get_name().compare(server_name) == 0)	// server already exists
		{
			return (*server_iterator);
		}
	}
	
	// server does not exist
	
	Server * new_server = new Server(server_name, application_id, point_of_access, zone);
	
	servers.push_back(new_server);
	
	return new_server;
}


Client * get_or_create_client(string client_name, uint16_t application_id, string user_equipment_name)
{
	for(auto client_iterator = clients.begin(); client_iterator != clients.end(); ++client_iterator)//b: from where is
	{
		if((*client_iterator)->get_name().compare(client_name) == 0)	// client already exists
		{
			return (*client_iterator);
		}
	}
	
	// client does not exist
	
	Client * new_client = new Client(client_name, application_id, user_equipment_name);
	
	clients.push_back(new_client);
	
	return new_client;
}


History_score * get_or_create_history_score(Client * client, Node * node)
{
	for(auto history_score_iterator = history_scores.begin(); history_score_iterator != history_scores.end(); ++history_score_iterator)
	{
		if((*history_score_iterator)->client == client && (*history_score_iterator)->node == node)
		{
			return (*history_score_iterator);
		}
	}
	
	History_score * new_history_score = new History_score();
	new_history_score->client = client;
	new_history_score->node = node;
	new_history_score->score = 0;
	new_history_score->last_update_time = time_point_cast<milliseconds>(system_clock::now());
	
	history_scores.push_back(new_history_score);
	
	return new_history_score;
}


void get_client_location(Client * client, string& point_of_access, string& zone)
{
	try
	{
		//http_client http_client_object(U("http://meep-loc-serv"));
		http_client http_client_object(U("http://localhost:30007"));
		uri_builder builder(U("/etsi-013/location/v1/users/" + client->get_location_user_equipment()));
		http_response response = http_client_object.request(methods::GET, builder.to_string()).get();
		
		json::value response_value = response.extract_json().get();
		
		json::value user_info_json = response_value.as_object().at("userInfo");
		
		json::value point_of_access_json = user_info_json.as_object().at("accessPointId");
		json::value zone_json = user_info_json.as_object().at("zoneId");
		
		point_of_access = point_of_access_json.as_string();
		zone = zone_json.as_string();
		
		//cout << "point_of_access: " << point_of_access << ", zone: " << zone << endl;
		
		return;
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}

void move_client(Client * client, string point_of_access)
{
	try
	{
		string json_string = "{  \"name\": \"name\",  \"type\": \"MOBILITY\",  \"eventMobility\": {    \"elementName\": \"" + client->get_location_user_equipment() + "\",    \"dest\": \"" + point_of_access + "\"  }}";
		// b:create json value after parsing
		json::value json_value = json::value::parse(json_string);
		http_client http_client_object(U("http://127.0.0.1:30000"));//b:create an http client
		uri_builder builder(U("/v1/events/MOBILITY"));//b:what is done here ?
		http_response response = http_client_object.request(methods::POST, builder.to_string(), json_value).get();//b:what is done here ?
		
		//cout << "Received response status code: " << response.status_code() << endl;
		
		string new_point_of_access;
		string new_zone;
		
		get_client_location(client, new_point_of_access, new_zone);
		
		log_file << time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count() << " " << client->get_name() << " moved from " << client->get_location_point_of_access() << " " << client->get_location_zone() << " to " << new_point_of_access << " " << new_zone << endl;
		
		client->set_location_point_of_access(new_point_of_access);
		client->set_location_zone(new_zone);
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}

int get_distance(Client * client, Server * server)
{
	if(client->get_location_point_of_access().compare(server->get_location_point_of_access()) == 0)	// server in the same point of access and zone
	{
		return 1;
	}
	else if(client->get_location_zone().compare(server->get_location_zone()) == 0)	// server in the same zone but different point of access
	{
		if(server->get_location_point_of_access().compare("null") == 0)	// edge server
		{
			return 2;
		}
		else	// fog server
		{
			return 3;
		}
	}
	else	// server in different zone
	{
		if(server->get_location_point_of_access().compare("null") == 0)	// edge server
		{
			return 9;
		}
		else	// fog server
		{
			return 10;
		}
	}	
}

int get_distance(Client * client, Node * node)
{
	if(client->get_location_point_of_access().compare(node->get_location_point_of_access()) == 0)	// node in the same point of access and zone
	{
		return 1;
	}
	else if(client->get_location_zone().compare(node->get_location_zone()) == 0)	// node in the same zone but different point of access
	{
		if(node->get_location_point_of_access().compare("null") == 0)	// edge node
		{
			return 2;
		}
		else	// fog node //b: connected to other PoA
		{
			return 3;
		}
	}
	else
	{
		if(node->get_location_point_of_access().compare("null") == 0)	// edge node
		{
			return 4;
		}
		else	// fog node
		{
			return 5;
		}
	}	
}


void simulation_thread(time_point<system_clock, milliseconds> simulation_start_time)
{
	auto last_event_time = simulation_start_time;
	auto current_event_time = last_event_time;	// not really needed
	
	Client * client = nullptr;
	
	while(true)
	{
		for(auto event_iterator = events.begin(); event_iterator != events.end(); ++event_iterator)
		{
			auto current_event_time = last_event_time + (*event_iterator).duration;
			
			this_thread::sleep_until(current_event_time);// b:what is done here ??
			
			last_event_time = current_event_time;
			
			switch ((*event_iterator).type)
			{
				case MOVE:
				
				client = nullptr;
				
				for(auto client_iterator = clients.begin(); client_iterator != clients.end(); ++client_iterator)
				{
					if((*client_iterator)->get_name().compare((*event_iterator).client_name) == 0)
					{
						client = (*client_iterator);
					}
				}
				
				if(client == nullptr)
				{
					cout << "client name not found " << (*event_iterator).client_name << endl;
				}
				else
				{
					move_client(client, (*event_iterator).point_of_access_name);
					cout << (*event_iterator).client_name << " moved to " << (*event_iterator).point_of_access_name << endl;
				}
				
				break;
				
				default:
				cout << (*event_iterator).type << " Type not defined" << endl;
			}
		}
	}
}

void clients_number_writer_thread()
{
	while(true)
	{
		time_point<system_clock, milliseconds> time_now = time_point_cast<milliseconds>(system_clock::now());
		//b:why is this required
		write_number_of_connected_clients_to_output_file(time_now);
		time_now = time_now + milliseconds(1000);
		this_thread::sleep_until(time_now);
	}
}

void servers_reply_thread(int port)
{
	ServerTCP server_reply_TCP;
	while(true)
	{
		server_reply_TCP.listen_on(port, 120000, 2000);// b: what are the arguments(max que may be one )?
		server_reply_TCP.disconnect();
	}
}
