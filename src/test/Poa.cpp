#include "Poa.h"



Poa::Poa(string _location_point_of_access , string _location_zone)
{
	point_of_access_name = _location_point_of_access;
	location_zone = _location_zone;
}


string Poa::get_poa_name()
{
	return point_of_access_name;
}


string Poa :: get_zone_of_poa()
{
	return location_zone;
}
