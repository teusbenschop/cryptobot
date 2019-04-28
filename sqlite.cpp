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


#include "sqlite.h"
#include <sqlite3.h>


// Stores values collected during a reading session of SQLite3.
class SqliteReader
{
public:
  map <string, vector <string> > result;
  static int callback (void *userdata, int argc, char **argv, char **column_names);
private:
};


int SqliteReader::callback (void *userdata, int argc, char **argv, char **column_names)
{
  for (int i = 0; i < argc; i++) {
    // Handle NULL field.
    if (argv [i] == NULL) ((SqliteReader *) userdata)->result [column_names [i]].push_back ("");
    else ((SqliteReader *) userdata)->result [column_names [i]].push_back (argv[i]);
  }
  return 0;
}


SqliteDatabase::SqliteDatabase (string database)
{
  db = NULL;
  name = database;
  string filename = path (database);
  sqlite3 *db3;
  int rc = sqlite3_open (filename.c_str(), &db3);
  if (rc) {
    const char * error = sqlite3_errmsg (db3);
    diagnose ("Database " + filename, (char *) error);
    db3 = NULL;
  }
  if (db3) sqlite3_busy_timeout (db3, 1000);
  if (db3) db = db3;
}


SqliteDatabase::~SqliteDatabase ()
{
  if (db) {
    sqlite3 * database = (sqlite3 *) db;
    sqlite3_close (database);
  }
}


void SqliteDatabase::clear ()
{
  sql.clear ();
}


void SqliteDatabase::add (const char * fragment)
{
  sql.append (" ");
  sql.append (fragment);
  sql.append (" ");
}


void SqliteDatabase::add (int value)
{
  sql.append (" ");
  sql.append (to_string (value));
  sql.append (" ");
}


void SqliteDatabase::add (float value)
{
  sql.append (" ");
  sql.append (float2string (value));
  sql.append (" ");
}


void SqliteDatabase::add (string value)
{
  sql.append (" '");
  value = str_replace ("'", "''", value);
  sql.append (value);
  sql.append ("' ");
}


void SqliteDatabase::execute ()
{
  if (db) {
    char *error = NULL;
    sqlite3 * database = (sqlite3 *) db;
    if (database) {
      int rc = sqlite3_exec (database, sql.c_str(), NULL, NULL, &error);
      if (rc != SQLITE_OK) diagnose (sql, error);
    } else {
      diagnose (sql, error);
    }
    if (error) sqlite3_free (error);
  }
}


map <string, vector <string> > SqliteDatabase::query ()
{
  if (db) {
    sqlite3 * database = (sqlite3 *) db;
    char * error = NULL;
    SqliteReader reader;
    if (database) {
      int rc = sqlite3_exec (database, sql.c_str(), reader.callback, &reader, &error);
      if (rc != SQLITE_OK) diagnose (sql, error);
    } else {
      diagnose (sql, error);
    }
    if (error) sqlite3_free (error);
    return reader.result;
  }
  return {};
}


// The function provides the path fo the database in the default database folder.
string SqliteDatabase::path (string database)
{
  // Get the path to the program, e.g. /home/hoe/cryptobot/lab
  string path = program_path;
  // Get the path to the databases folder, e.g. /home/hoe/cryptodata/databases
  path = dirname (dirname (path));
  path.append ("/cryptodata/databases");
  // Get the path to the database file, e.g.  /home/hoe/cryptodata/databases/data.sqlite
  // Only if a database is given.
  // If no database is given, this returns the database folder.
  if (!database.empty ()) {
    path.append ("/");
    path.append (database);
    path.append (suffix ());
  }
  // Done.
  return path;
}


string SqliteDatabase::suffix ()
{
  return ".sqlite";
}


// Logs any error on the database connection.
// The error will be prefixed by $prefix.
void SqliteDatabase::diagnose (const string & prefix, char * error)
{
  string message = prefix;
  if (error) {
    message.append (" - ");
    message.append (error);
  }
  if (db) {
    sqlite3 * database = (sqlite3 *) db;
    int errcode = sqlite3_errcode (database);
    const char * errstr = sqlite3_errstr (errcode);
    string error_string;
    if (errstr) error_string.assign (errstr);
    int x_errcode = sqlite3_extended_errcode (database);
    const char * x_errstr = sqlite3_errstr (x_errcode);
    string extended_error_string;
    if (x_errstr) extended_error_string.assign (x_errstr);
    if (!error_string.empty ()) {
      message.append (" - ");
      message.append (error_string);
    }
    if (extended_error_string != error_string) {
      message.append (" - ");
      message.append (extended_error_string);
    }
    const char * filename = sqlite3_db_filename (database, "main");
    if (filename) {
      message.append (" - ");
      message.append (filename);
    }
  } else {
    if (!message.empty ()) message.append (" - ");
    message.append ("No database connection");
  }
  to_stdout ({message});
}
