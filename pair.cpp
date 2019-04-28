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
  bool executed = false;

  // Read the existing pairs from the database.
  vector <string> exchange1s, exchange2s, markets, coins;
  vector <int> days;
  pairs_get (sql, exchange1s, exchange2s, markets, coins, days);

  // No arguments: List the pairs from the database.
  if (arguments.empty ()) {
    for (size_t i = 0; i < coins.size (); i++) {
      to_tty ({exchange1s[i], exchange2s[i], markets[i], coins[i], to_string (days[i])});
    }
    executed = true;
  }

  // Four arguments given: add or remove the arguments from the database.
  if (arguments.size () == 5) {
    string exchange1 = arguments [0];
    string exchange2 = arguments [1];
    string market = arguments [2];
    string coin = arguments [3];
    int days = str2int (arguments [4]);

    bool healthy = true;

    // Do the exchanges exist?
    vector <string> exchanges = exchange_get_names ();
    if (healthy) {
      if (!in_array (exchange1, exchanges)) {
        to_tty ({"Unknown exchange1", exchange1});
        healthy = false;
      }
    }
    if (healthy) {
      if (!in_array (exchange2, exchanges)) {
        to_tty ({"Unknown exchange2", exchange2});
        healthy = false;
      }
    }

    // Does the market exist on both exchanges?
    if (healthy) {
      vector <string> markets = exchange_get_supported_markets (exchange1);
      if (!in_array (market, markets)) {
        to_tty ({"Market", market, "is not supported at", exchange1});
        healthy = false;
      }
    }
    if (healthy) {
      vector <string> markets = exchange_get_supported_markets (exchange2);
      if (!in_array (market, markets)) {
        to_tty ({"Market", market, "is not supported at", exchange2});
        healthy = false;
      }
    }

    // Does the coin exist on both exchanges and markets?
    if (healthy) {
      vector <string> coins = exchange_get_coins_per_market_cached (exchange1, market);
      if (!in_array (coin, coins)) {
        to_tty ({"Coin", coin, "is not supported at market", market, "at exchange", exchange1});
        healthy = false;
      }
    }
    if (healthy) {
      vector <string> coins = exchange_get_coins_per_market_cached (exchange2, market);
      if (!in_array (coin, coins)) {
        to_tty ({"Coin", coin, "is not supported at market", market, "at exchange", exchange2});
        healthy = false;
      }
    }

    // Check if the current data is already present in the database.
    bool already_present = false;
    if (healthy) {
      for (size_t i = 0; i < coins.size (); i++) {
        if ((exchange1s[i] != exchange1) && (exchange1s[i] != exchange2)) continue;
        if ((exchange2s[i] != exchange1) && (exchange2s[i] != exchange2)) continue;
        if (markets[i] != market) continue;
        if (coins[i] != coin) continue;
        already_present = true;
      }
    }

    // Toggle the information in the database.
    if (healthy) {
      if (already_present) {
        pairs_remove (sql, exchange1, exchange2, market, coin);
        to_tty ({"Removed:", exchange1, exchange2, market, coin, to_string (days)});
      } else {
        pairs_add (sql, exchange1, exchange2, market, coin, days);
        to_tty ({"Added:", exchange1, exchange2, market, coin, to_string (days)});
      }
      executed = true;
    }
  }
  
  // Possible usage information.
  if (!executed) {
    to_tty ({"Usage: ./pair [exchange1 exchange2 market coin days]"});
  }
  
  finalize_program ();
  return EXIT_SUCCESS;
}
