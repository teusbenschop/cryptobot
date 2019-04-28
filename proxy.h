/*
 Copyright (©) 2013-2017 Teus Benschop.
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef INCLUDED_PROXY_H
#define INCLUDED_PROXY_H


#include "libraries.h"


vector <string> satellite_hosts ();
void proxy_satellite_register_live_ones ();
string proxy_get_ipv4 (const string & exchange);
string proxy_get_ipv46 (const string & exchange);
string satellite_get (const string & exchange);
void register_ip_interfaces ();
string get_ipv46_interface (const string & host);
string front_bot_ip_address ();
string back_bot_ip_address ();
string front_bot_name ();
string back_bot_name ();
void satellite_register_known_hosts ();
string satellite_get_interface (string url);


#endif

