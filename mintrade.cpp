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
#include "exchanges.h"
#include "models.h"
#include "controllers.h"
#include "proxy.h"


// Some pages with minimum trade sizes:
// https://support.kraken.com/hc/en-us/articles/205893708-What-is-the-minimum-order-size-


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  to_tty ({"Fetching minimum trade sizes"});
  SQL sql (front_bot_ip_address ());

  map <string, float> minimum_trade_amounts;
  // Get the minimum trade amounts now recorded in the database.
  // The purpose is to not update them if they're correct already.
  // Store the minimum trade sizes in the following format:
  // container [exchange+market+coin] = minimum trade size.
  minimum_trade_amounts = mintrade_get (sql);
  
  // Iterate over the exchanges.
  vector <string> exchanges = exchange_get_names ();
  for (auto exchange : exchanges) {
    
    // Iterate over the supported markets per exchange.
    vector <string> markets = exchange_get_supported_markets (exchange);
    for (auto market : markets) {
      to_tty ({"market", market, "@", exchange});

      // Get the coins this market at this exchange support.
      vector <string> coins = exchange_get_coins_per_market_cached (exchange, market);
      
      // If there's any coins here, proceed.
      if (!coins.empty ()) {

        // Memory storage.
        map <string, float> market_amounts;
        map <string, float> coin_amounts;
        
        // Call the exchange API.
        exchange_get_minimum_trade_amounts (exchange, market, coins, market_amounts, coin_amounts);
        
        // Store the minimum trade amounts in the database.
        // One important reason for storing the amounts in the database is that,
        // if there's a communication error with the exchange,
        // the bot can still safely fetch the values from the database.
        // This robustness will lead to fewer failed arbitrage trades.
        for (auto coin : coins) {
          float mintrade = coin_amounts [coin];
          float market_amount = market_amounts [coin];
          if (market_amount > 0) {
            float rate = rate_get_cached (exchange, market, coin);
            if (rate > 0) {
              float amount = market_amount / rate;
              if (amount > mintrade) mintrade = amount;
            }
          }
          if (mintrade > 0) {
            // If the value in the database is good already, then all is well.
            float stored_mintrade = minimum_trade_amounts [exchange + market + coin];
            if (mintrade != stored_mintrade) {
              
              // Use a transaction for atomic operations.
              // Reason: There's never any time the bot cannot get a minimum trade amount from the database.
              // It used to use one transaction per market.
              // But that led to deadlocks and to lock timeouts.
              // Using one transaction per coin solves this.
              sql.clear ();
              sql.add ("START TRANSACTION;");
              sql.execute ();

              // Remove the old value and insert the new one.
              sql.clear ();
              sql.add ("DELETE FROM mintrade where exchange =");
              sql.add (exchange);
              sql.add ("AND market =");
              sql.add (market);
              sql.add ("AND coin =");
              sql.add (coin);
              sql.add (";");
              sql.execute ();
              sql.clear ();
              sql.add ("INSERT INTO mintrade (exchange, market, coin, quantity) VALUES (");
              sql.add (exchange);
              sql.add (",");
              sql.add (market);
              sql.add (",");
              sql.add (coin);
              sql.add (",");
              sql.add (mintrade);
              sql.add (");");
              sql.execute ();
              
              // Commit the transaction.
              sql.clear ();
              sql.add ("COMMIT;");
              sql.execute ();
            }
          }
        }
      }
    }
  }
  
  finalize_program ();
  to_tty ({"Ready fetching minimum trade sizes"});
  return EXIT_SUCCESS;
}
