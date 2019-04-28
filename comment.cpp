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


#include "libraries.h"
#include "models.h"
#include "bittrex.h"
#include "cryptopia.h"
#include "bl3p.h"
#include "shared.h"
#include "exchanges.h"
#include "controllers.h"
#include "controllers.h"
#include "sql.h"
#include "proxy.h"


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  
  
  // Gather all the arguments passed to this program.
  vector <string> arguments;
  for (int i = 1; i < argc; i++) {
    arguments.push_back (argv[i]);
  }

  
  SQL sql (front_bot_ip_address ());
  
  
  bool healthy = true;
  
  
  to_tty ({"Usage:"});
  to_tty ({"./comment"});
  to_tty ({"./comment [coin] [comment ...]"});
  to_tty ({"./comment id"});

  
  // Check whether to delete a comment.
  if (arguments.size () == 1) {
    string argument = arguments [0];
    int id = str2int (argument);
    if (argument == to_string (id)) {
      comments_delete (sql, id);
      to_tty ({"The comment was deleted"});
      healthy = false;
    }
  }

  
  // Check that the coin is known at exchange(s).
  string coin;
  if (healthy && !arguments.empty ()) {
    coin = arguments [0];
    to_tty ({"Coin:", coin});
    arguments.erase (arguments.begin());
    bool coin_known = false;
    vector <string> exchanges = exchange_get_names ();
    for (auto exchange : exchanges) {
      if (coin_known) continue;
      vector <string> coins = exchange_get_coin_ids (exchange);
      if (in_array (coin, coins)) {
        coin_known = true;
      }
    }
    if (!coin_known) {
      to_tty ({"The coin is not known at any exchange"});
      healthy = false;
    }
  }
  
  
  // Get the comment.
  string comment;
  if (healthy) {
    comment = implode (arguments, " ");
    to_tty ({"Comment:", comment});
  }


  // If no comment given, display the comments from the database.
  if (healthy && comment.empty ()) {
    vector <int> ids;
    vector <string> stamps, coins, comments;
    comments_get (sql, coin, ids, stamps, coins, comments);
    for (size_t i = 0; i < ids.size (); i++) {
      to_tty ({to_string (ids[i]), stamps[i], coins[i], comments[i]});
    }
  }


  // If coin and comment given, write it to the database.
  if (healthy && (!coin.empty ()) && (!comment.empty ())) {
    comments_store (sql, coin, comment);
    to_tty ({"Comment was recorded in the database"});
  }
  
  
  finalize_program ();
  return EXIT_SUCCESS;
}
