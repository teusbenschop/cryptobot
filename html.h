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


#ifndef INCLUDED_HTML_H
#define INCLUDED_HTML_H


#include "libraries.h"
#include "pugixml.hpp"


using namespace pugi;


class Html
{
public:
  Html (string title, string javascript);
  void p ();
  void div (string id);
  void txt (string text);
  string get ();
  string inner ();
  void h (int level, string text, string id = "");
  void a (string reference, string text);
  void img (string image);
  void buy (string coin);
  void save (string name);
private:
  xml_document htmlDom;
  xml_node currentPDomElement;
  xml_node headDomNode;
  xml_node bodyDomNode;
  bool current_p_node_open = false;
  void newNamedHeading (string style, string text, bool hide = false);
};


#endif
