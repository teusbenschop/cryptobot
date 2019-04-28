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
#include "sql.h"
#include "models.h"
#include "bittrex.h"
#include "cryptopia.h"
#include "bl3p.h"
#include "shared.h"
#include "exchanges.h"
#include "controllers.h"
#include "proxy.h"


int main (int argc, char *argv[])
{
  // Start up.
  initialize_program (argc, argv);
  
  
  SQL sql (front_bot_ip_address ());
  
  
  // Gather all the arguments passed to this program.
  vector <string> arguments;
  for (int i = 1; i < argc; i++) {
    arguments.push_back (argv[i]);
  }
  
  
  // Find an existing exchange in the arguments.
  vector <string> exchanges = exchange_get_names ();
  vector <string> intersection = array_intersect (arguments, exchanges);
  if (!intersection.empty ()) {
    string exchange = intersection [0];
    to_tty ({"Exchange:", exchange});

    
    // Read all available currencies at this exchange.
    // Find the currency identifier passed on the command line.
    vector <string> coin_identifiers = exchange_get_coin_ids (exchange);
    intersection = array_intersect (arguments, coin_identifiers);
    if (!intersection.empty ()) {
      string coin_id = intersection [0];
      to_tty ({"Coin identifier:", coin_id});

      
      // Read the coin name from the data.
      string coin_name = exchange_get_coin_name (exchange, coin_id);
      if (!coin_name.empty ()) {
        to_tty ({"Coin name:", coin_name});


        // Obtain the coin abbreviation.
        string coin_abbreviation = exchange_get_coin_abbrev (exchange, coin_id);
        if (!coin_abbreviation.empty ()) {
          to_tty ({"Coin abbreviation:", coin_abbreviation});
          
          
          // Read the wallet address from the exchange.
          string address_exchange;
          string payment_id_exchange;
          exchange_get_wallet_details (exchange, coin_id, address_exchange, payment_id_exchange);
          if (!address_exchange.empty ()) {
            to_tty ({"Address at the exchange:", address_exchange});
            to_tty ({"Payment id at the exchange:", payment_id_exchange});


            // Check whether to 'save' this wallet address to the database.
            if (in_array (string ("save"), arguments)) {
              wallets_set_deposit_details (sql, exchange, coin_id, address_exchange, payment_id_exchange);
              to_tty ({"The address and the payment ID were saved to the database"});
            }
            
            
            // Check whether to update this wallet's parameters in the database.
            if (in_array (string ("trading"), arguments)) {
              wallets_set_trading (sql, exchange, coin_id, true);
              to_tty ({"Set trading on in the database"});
            }
            if (in_array (string ("notrading"), arguments)) {
              wallets_set_trading (sql, exchange, coin_id, false);
              to_tty ({"Set trading off in the database"});
            }
            if (in_array (string ("balancing"), arguments)) {
              wallets_set_balancing (sql, exchange, coin_id, true);
              to_tty ({"Set balancing on in the database"});
            }
            if (in_array (string ("nobalancing"), arguments)) {
              wallets_set_balancing (sql, exchange, coin_id, false);
              to_tty ({"Set balancing off in the database"});
            }

            
            // Information whether the wallet is trading right now or whether it was disabled temporarily.
            bool can_trade = wallets_get_trading (sql, exchange, coin_id);
            vector <string> trading_exchanges = wallets_get_trading_exchanges (sql, coin_id);
            bool trading_now = in_array (exchange, trading_exchanges);


            to_tty ({"Address in the database:", wallets_get_address (sql, exchange, coin_id)});
            to_tty ({"Payment id in the database:", wallets_get_payment_id (sql, exchange, coin_id)});
            to_tty ({"Wallet can trade:", to_string (can_trade)});
            if (can_trade && !trading_now) {
              to_tty ({"Wallet is temporarily not trading but will resume trading within minutes"});
            }
            to_tty ({"Wallet can balance:", to_string (wallets_get_balancing (sql, exchange, coin_id))});

            
          } else {
            to_tty ({"The exchange is still generating the wallet address..."});
            to_tty ({"Please try again shortly"});
          }
        } else {
          to_tty ({"Cannot find the abbreviation for this coin in the database"});
        }
      } else {
        to_tty ({"Cannot find the name for this coin in the database"});
      }
    } else {
      coin_identifiers.insert (coin_identifiers.begin(), "Please pass one of the known coins at this exchange:");
      to_tty (coin_identifiers);
    }
  } else {
    to_tty ({"Options: ./wallet exchange coin [save | [no]trading | [no]balancing]"});
    exchanges.insert (exchanges.begin(), "Please pass one of the known exchanges:");
    to_tty (exchanges);
  }

  
  // Done.
  finalize_program ();
  return EXIT_SUCCESS;
}

