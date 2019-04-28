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
  // Start up.
  initialize_program (argc, argv);
  
  
  // Gather all the arguments passed to this program.
  vector <string> arguments;
  for (int i = 1; i < argc; i++) {
    arguments.push_back (argv[i]);
  }

  
  // Check the correct number of arguments.
  if (arguments.size() == 3) {
    
    
    // Check that the exchange is known.
    string exchange = arguments [0];
    vector <string> exchanges = exchange_get_names ();
    if (in_array (exchange, exchanges)) {
      to_tty ({"Exchange:", exchange});
      
      
      // Check that the market is known at the exchange.
      string market = arguments [1];
      vector <string> markets = exchange_get_supported_markets (exchange);
      if (in_array (market, markets)) {
        to_tty ({"Market:", market});
        
        
        // Check that the coin is known at the exchange.
        string coin = arguments [2];
        vector <string> coins = exchange_get_coins_per_market_cached (exchange, market);
        if (in_array (coin, coins)) {
          string coin_abbreviation = exchange_get_coin_abbrev (exchange, coin);
          string coin_name = exchange_get_coin_name (exchange, coin);
          to_tty ({"Coin:", coin, coin_abbreviation, coin_name});
          

          // Read and display the sell orders for this coin.
          vector <float> quantity;
          vector <float> rate;
          exchange_get_sellers_via_satellite (exchange, market, coin, quantity, rate, false, nullptr);
          for (unsigned int i = 0; i < quantity.size(); i++) {
            to_tty ({"Sell", float2string (quantity[i]), coin, "at rate", float2string (rate[i]), "total", to_string (quantity[i] * rate[i]), market});
          }

          
        } else {
          to_tty ({"Please pass one of the known coins at this exchange:"});
          to_tty (coins);
        }
      } else {
        to_tty ({"Please pass one of the known markets at this exchange:"});
        to_tty (markets);
      }
    } else {
      to_tty ({"Please pass one of the known exchanges:"});
      to_tty (exchanges);
    }
  } else {
    to_tty ({"Usage: ./sellers exchange market coin"});
  }


  // Done.
  finalize_program ();
  return EXIT_SUCCESS;
}
