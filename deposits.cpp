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


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  
  // Gather all the arguments passed to this program.
  vector <string> arguments;
  for (int i = 1; i < argc; i++) {
    arguments.push_back (argv[i]);
  }
  
  // Check the correct number of arguments.
  if (arguments.size() == 2) {
    
    // Check that the exchange is known.
    string exchange = arguments [0];
    vector <string> exchanges = exchange_get_names ();
    if (in_array (exchange, exchanges)) {
      cout << "Exchange: " << exchange << endl;

      // Check that the coin is known at the exchange.
      string coin = arguments [1];
      vector <string> coins = exchange_get_coin_ids (exchange);
      if (in_array (coin, coins)) {
        string coin_abbreviation = exchange_get_coin_abbrev (exchange, coin);
        string coin_name = exchange_get_coin_name (exchange, coin);
        cout << "Coin: " << coin << " " << coin_abbreviation << " " << coin_name << endl;
        
        vector <string> order_ids;
        vector <string> coin_abbreviations;
        vector <string> coin_ids;
        vector <float> quantities;
        vector <string> addresses;
        vector <string> transaction_ids;
        exchange_deposithistory (exchange, coin_abbreviation, order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids);
        for (unsigned int i = 0; i < order_ids.size(); i++) {
          if (coin_abbreviations[i] != coin_abbreviation) continue;
          cout << "order " << order_ids[i] << " deposit " << quantities[i] << " " << coin_ids[i] << " to " << addresses[i] << " txid " << transaction_ids[i] << endl;
        }
        
      } else {
        to_tty ({"Please pass one of the known coins at this exchange:"});
        to_tty (coins);
      }
    } else {
      cout << "Please pass one of the known exchanges:";
      for (auto exchange : exchanges) cout << " " << exchange;
      cout << endl;
    }
  } else {
    cout << "Usage: ./deposits exchange coin" << endl;
  }
  
  finalize_program ();
  return EXIT_SUCCESS;
}
