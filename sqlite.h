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


#ifndef INCLUDED_SQLITE_H
#define INCLUDED_SQLITE_H


#include "libraries.h"


class SqliteDatabase
{
public:
  SqliteDatabase (string database);
  ~SqliteDatabase ();
  static string path (string database);
  void clear ();
  void add (const char * fragment);
  void add (int value);
  void add (float value);
  void add (string value);
  string sql;
  void execute ();
  map <string, vector <string> > query ();
private:
  string name;
  static string suffix ();
  void * db;
  void diagnose (const string & prefix, char * error);
};


#endif
