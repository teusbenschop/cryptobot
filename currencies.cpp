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
#include "bittrex.h"
#include "cryptopia.h"
#include "bl3p.h"
#include "yobit.h"
#include "exchanges.h"
#include "models.h"
#include "proxy.h"


// It is sufficient if the crontab runs this program once a week.
// It collects the names, the abbreviations, and the identifiers of all coins
// at all supported markets at all supported exchanges.
// It compares this with the coins the code has.
// It outputs the code for adding new coins.
// It outputs the coins no longer supported.
// It updates the database of coins supported per market,
// since not all coins are supported at all markets at all exchanges.
// Example:
// At Cryptopia it has been seen that certain coins, like "CCB",
// are available at the Litecoin and Dogecoin markets,
// but not at the Bitcoin and USDT and NZD markets.


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);

  to_tty ({"Getting coin names"});
  
  SQL sql (front_bot_ip_address ());

  // Iterate over all the known exchanges.
  vector <string> exchanges = exchange_get_names ();
  for (auto exchange : exchanges) {

    // Flag for API call error on this exchange.
    bool api_call_error_encountered = false;
    
    // Storage for all distinct accumulated coin abbreviations at all markets.
    set <string> all_distinct_abbreviations;

    // Storage for all identifiers, names, and exchange abbreviations,
    // in this format: container [abbreviation] = value.
    map <string, string> all_identifiers;
    map <string, string> all_names;
    map <string, string> all_exchange_abbreviations;

    // Iterate over the markets the exchange supports.
    vector <string> markets = exchange_get_supported_markets (exchange);
    for (auto market : markets) {

      // Get the coins at the market at the exchange.
      vector <string> abbreviations;
      vector <string> identifiers;
      vector <string> names;
      vector <string> exchange_abbreviations;
      string error;
      exchange_get_coins (exchange, market, abbreviations, identifiers, names, exchange_abbreviations, error);
      
      // API call error handler.
      if (!error.empty ()) {
        api_call_error_encountered = true;
        to_stderr ({error});
      }

      // Store the coin data in the accumulating containers.
      for (unsigned int i = 0; i < abbreviations.size(); i++) {
        string abbreviation = abbreviations[i];
        all_distinct_abbreviations.insert (abbreviation);
        if (!identifiers.empty ()) {
          string identifier = identifiers[i];
          all_identifiers [abbreviation] = identifier;
        }
        if (!names.empty ()) {
          string name = names[i];
          all_names [abbreviation] = name;
        }
        if (!exchange_abbreviations.empty ()) {
          string exchange_abbreviation = exchange_abbreviations[i];
          all_exchange_abbreviations [abbreviation] = exchange_abbreviation;
        }
      }
      
      // Update the database with the coins supported at each market.
      // If there's no coins at a certain market, likely there's a communication error,
      // so skip that market for just now.
      if (!abbreviations.empty ()) {
        vector <string> coin_ids;
        for (auto abbreviation : abbreviations) {
          string identifier = exchange_get_coin_id (exchange, abbreviation);
          if (!identifier.empty ()) {
            coin_ids.push_back (identifier);
          }
        }
        coins_market_set (sql, exchange, market, implode (coin_ids, " "));
      }
      
    }

    // If there was an API call error on this exchange, don't output anything,
    // because the coins it lists may not be accurate.
    if (api_call_error_encountered) continue;
    
    // Look for undefined coins and output them.
    bool new_coins_found = false;
    for (auto abbreviation : all_distinct_abbreviations) {
      string identifier = exchange_get_coin_id (exchange, abbreviation);
      if (identifier.empty ()) {
        if (!new_coins_found) {
          new_coins_found = true;
          to_stderr ({"New coins at", exchange});
        }
        identifier = all_names [abbreviation];
        string name = all_names [abbreviation];
        string exchange_abbreviation = all_exchange_abbreviations [abbreviation];
        identifier = str2lower (str_replace (" ", "", identifier));
        identifier = str2lower (str_replace ("/", "", identifier));
        string exchange_coin_abbreviation_part;
        if (!exchange_abbreviation.empty ()) {
          exchange_coin_abbreviation_part = ", \"" + exchange_abbreviation + "\"";
        }
        to_stderr ({"{ \"" + identifier + "\", \"" + abbreviation + "\", \"" + name + "\"" + exchange_coin_abbreviation_part + " },"}, false);
      }
    }
    
    // Look for defined coins no longer available at the exchange, and output them.
    vector <string> defined_coin_ids = exchange_get_coin_ids (exchange);
    for (auto coin_id : defined_coin_ids) {
      string coin_abbrev = exchange_get_coin_abbrev (exchange, coin_id);
      if (all_distinct_abbreviations.find (coin_abbrev) == all_distinct_abbreviations.end ()) {
        // If a coin is a supported market, don't report it as non-available.
        if (!in_array (coin_id, markets)) {
          to_stderr ({"Not available at", exchange, "coin id", coin_id, "coin abbrev", coin_abbrev});
        }
      }
    }
  }

  finalize_program ();
  
  to_tty ({"Ready doing coin names"});
  
  return EXIT_SUCCESS;
}

/*

 Question:
 Will it not be better to make automatic coin management for the exchanges that have hundreds of coins?
 That would save the weekly hassle of coin updates.
 Suggestion would be to include the age of "last seen" of the coin.
 If there's coins that have last been seen at some time,
 and there's coins that's older as to their 'last seen" status, these older ones definitely can be removed.
 This is to cater for exchange communiation erors.
 Tis applies to Bittrex, Cryptopia and TradeSatoshi and Yobit.
 Answer:
 It would have made sense in the past more than now.
 Because now the bot does arbitrage on the best of coins.
 So new coins are very unlikely to soon rise to the status of being among the best of coins.
 So no longer it is needed to do weekly coin updates.
 Once in a while, that would be enough.
 
 */
