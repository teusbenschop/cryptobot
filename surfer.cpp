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
#include "shared.h"
#include "controllers.h"
#include "exchanges.h"
#include "proxy.h"


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  
  // Gather all the arguments passed to this program.
  vector <string> arguments;
  for (int i = 1; i < argc; i++) {
    arguments.push_back (argv[i]);
  }
  
  bool healthy = true;

  // Check on correct number of arguments.
  if (healthy) {
    if (arguments.size() != 2) {
      to_tty ({"Usage: ./surfer coin", surfer_buy () + "|" + surfer_sell ()});
      healthy = false;
    }
  }
  
  // Check that the coin is known at the exchanges.
  string coin;
  if (healthy) {
    coin = arguments [0];
    set <string> allcoins;
    vector <string> exchanges = exchange_get_names ();
    for (auto exchange : exchanges) {
      vector <string> coins = exchange_get_coin_ids (exchange);
      for (auto coin : coins) allcoins.insert (coin);
    }
    vector <string> coins (allcoins.begin (), allcoins.end());
    if (in_array (coin, coins)) {
      to_tty ({"Coin:", coin});
    } else {
      to_tty ({"Please pass one of the known coins:"});
      to_tty (coins);
      healthy = false;
    }
  }
  
  // Check the type of trade to do.
  string trade;
  if (healthy) {
    trade = arguments [1];
    if ((trade != surfer_buy ()) && (trade != surfer_sell ())) {
      to_tty ({"Please pass '", surfer_buy (), "' or '", surfer_sell (), "'"});
      healthy = false;
    } else {
      to_tty ({"Operation:", trade});
      if (trade == surfer_sell ()) {
        to_tty ({"The bot will sell all balances of this coin at all exchanges"});
      }
      if (trade == surfer_buy ()) {
        to_tty ({"The bot will buy optimal amounts of this coin at all exchanges"});
        to_tty ({"The bot will sell it again after it detects a few percents gain"});
      }
    }
  }

  SQL sql (front_bot_ip_address ());

  // Store the surfer command in the database.
  if (healthy) {
    surfer_add (sql, coin, trade, surfer_status::entered);
    to_tty ({"Status:", surfer_status::entered});
  }
  
  finalize_program ();
  return EXIT_SUCCESS;
}

