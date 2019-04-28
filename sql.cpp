/*
Copyright (©) 2003-2017 Teus Benschop.

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


#include <sql.h>
#include <my_global.h>
#include <mysql.h>


SQL::SQL (string ip_address)
{
  // Initialize the MySQL structure.
  MYSQL * con = mysql_init (NULL);
  
  // Initialize the object's variables.
  connection = con;
  connected = true;
  
  if (con != NULL) {

    // The architecture of all programs should be such that it does not make too many connections
    // in too short of a time.
    // Else this error comes up:
    // ERROR 1226 (42000): User 'u940645557_store' has exceeded the 'max_connections_per_hour' resource (current value: 500)
    
    // The IP address of the MySQL server.
    // If an IP address is given, take that.
    // Else it connects to localhost.
    if (ip_address.empty ()) ip_address = "127.0.0.1";
    
    // Use another MySQL structure for the result of connecting.
    // It should not overwrite the earlier initialized structure.
    // Because in case of returning NULL, and putting that in the main MySQL structure,
    // no error message can be extracted.
    MYSQL * result = mysql_real_connect (con, ip_address.c_str(), "root", "yourpasswordhere",  "store", 0, NULL, 0);
    // MYSQL * result = mysql_real_connect (con, "sql124.main-hosting.eu", "u940645557_store", "pRHs6Dgra4fxB3R9l",  "u940645557_store", 0, NULL, 0);
    
    if (result != NULL) return;
  }

  // Error information.
  // Since this may not be a logical error in the SQL, it it not considered very important.
  to_stdout ({mysql_error (con)});
  
  // Not connected.
  connected = false;
}


SQL::~SQL ()
{
  if (connection) {
    MYSQL * con = (MYSQL *) connection;
    mysql_close(con);
  }
}


void SQL::clear ()
{
  sql.clear ();
}


void SQL::add (const char * fragment)
{
  sql.append (" ");
  sql.append (fragment);
  sql.append (" ");
}


void SQL::add (int value)
{
  sql.append (" ");
  sql.append (to_string (value));
  sql.append (" ");
}


void SQL::add (float value)
{
  sql.append (" ");
  stringstream ss;
  ss << setprecision (numeric_limits<float>::digits10+1);
  ss << value;
  sql.append (ss.str());
  sql.append (" ");
}


void SQL::add (string value)
{
  sql.append (" '");

  /*
   This is the old code.
   It crashed the bot multiple times per day.
   
   MYSQL * con = (MYSQL *) connection;
   const char * from = value.c_str();
   char * to = new char [(strlen(from) * 2) + 1];
   mysql_real_escape_string (con, to, from, strlen (from));
   value = to;

   The new code below has not yet crashed the bot even once.
   */

  // Put an extra backslash before the following characters:
  // \x00, \n, \r, \, ', " and \x1a
  string escaped;
  for (unsigned int i = 0; i < value.size (); i++) {
    string character = value.substr (i, 1);
    bool escape = false;
    if (character == "\x00") escape = true;
    if (character == "\n") escape = true;
    if (character == "\r") escape = true;
    if (character == "\\") escape = true;
    if (character == "'") escape = true;
    if (character == "\"") escape = true;
    if (character == "\x1a") escape = true;
    if (escape) escaped.append ("\\");
    escaped.append (character);
  }
  
  sql.append (escaped);
  sql.append ("' ");
}


void SQL::add (const SQL & value)
{
  sql.append (" (");
  sql.append (value.sql);
  sql.append (") ");
}


void SQL::execute ()
{
  if (!connected) return;
  MYSQL * con = (MYSQL *) connection;
  int result = mysql_query (con, sql.c_str());
  if (result) {
    to_stderr ({sql, mysql_error (con)});
  }
}


map <string, vector <string> > SQL::query ()
{
  if (!connected) return {};
  
  MYSQL * con = (MYSQL *) connection;

  int query_result = mysql_query (con, sql.c_str());
  if (query_result) {
    to_stderr ({sql, mysql_error (con)});
    return {};
  }
  
  MYSQL_RES * result = mysql_store_result (con);
  if (result == NULL)
  {
    to_stderr ({sql, mysql_error (con)});
    return {};
  }

  map <int, string> field_names;
  MYSQL_FIELD *field;
  for(unsigned int i = 0; (field = mysql_fetch_field(result)); i++) {
    field_names [i] = field->name;
  }

  map <string, vector <string> > values;
  
  int num_fields = mysql_num_fields (result);
  MYSQL_ROW row;
  while ((row = mysql_fetch_row (result)))
  {
    for (int i = 0; i < num_fields; i++) {
      string value;
      if (row[i]) {
        value = row[i];
      }
      values [field_names[i]].push_back (value);
    }
  }
  
  mysql_free_result(result);

  return values;
}
