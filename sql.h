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


#ifndef INCLUDED_SQL_H
#define INCLUDED_SQL_H


#include "libraries.h"


class SQL
{
public:
  SQL (string ip_address = "");
  ~SQL ();
  void clear ();
  void add (const char * fragment);
  void add (int value);
  void add (float value);
  void add (string value);
  void add (const SQL & value);
  string sql;
  void execute ();
  map <string, vector <string> > query ();
private:
  void * connection;
  bool connected;
};


#endif
