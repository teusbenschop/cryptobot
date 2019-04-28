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
#include "bittrex.h"
#include "cryptopia.h"


// To copy the databases from the bot to a machine:
// Disable rates collection in the crontab.
// $ rsync -av --delete root@185.87.186.173:cryptodata/databases ../cryptodata
// $ ./rates back
// * Enable rates collection in the crontab again.


// Store the exchange/markets/coins considered to be good coins for certain patterns.
vector <string> profitable_exchanges_markets_coins_reasons;

// Tracker for simultaneous detectors.
atomic <int> current_simultaneous_detector_count (0);

void patterns_detector (const string & exchange, const string & market, const string & coin)
{
  // One more running pattern detector.
  current_simultaneous_detector_count++;
  
  to_output output ("Patterns for " + coin + " @ " + market + " @ " + exchange);
  
  // Storage for super fast access to the minutely rates and the average hourly rates.
  unordered_map <int, float> minute_asks, minute_bids, hour_asks, hour_bids;
  pattern_prepare (&output, exchange, market, coin, minute_asks, minute_bids, hour_asks, hour_bids);
  
  // If there's insufficient rates, skip the analysis.
  if (hour_asks.size () >= 10) {

    // Detect the specified patterns.
    string reason = pattern_type_increase_day_week ();
    bool coin_good = pattern_increase_day_week_simulator (&output, hour_asks, hour_bids, reason);
    
    // Whether to store this coin plus details in the database, or to remove it.
    string signature = exchange + market + coin + reason;
    bool good_coin_stored = in_array (signature, profitable_exchanges_markets_coins_reasons);
    // Only connect to the database if it's needed.
    if (good_coin_stored || coin_good) {
      SQL sql (front_bot_ip_address ());
      // In all cases remove the coin, if stored.
      if (good_coin_stored) patterns_delete (sql, exchange, market, coin, reason);
      if (coin_good) {
        // Store good coins afresh, so the date stamp is set.
        // When the bot reads them for trading, it only reads the recent ones.
        // If the pattern analyzer fails to produce timely data,
        // then the trading bot knows that the data is old, so it does not use it.
        patterns_store (sql, exchange, market, coin, reason);
      }
    }
    
    // To save system resources, only plot the rates for development purposes.
    // For normal production use, there is no good reason to keep plotting the rates,
    // and nobody is using that information.
    bool plot_rates = false;
    if (coin_good) plot_rates = true;
    plot_rates = false;
    
    if (plot_rates) {
      vector <pair <float, float> > plotdata;
      string picturepath;
      string basename = exchange + "-" + market + "-" + coin;
      for (int hour = -300; hour <= 0; hour++) {
        float rate = (hour_asks [hour] + hour_bids [hour]) / 2;
        plotdata.push_back (make_pair (hour, rate));
      }
      plot (plotdata, {}, basename, "", picturepath);
    }
    
  }

  // One pattern detector completed.
  current_simultaneous_detector_count--;
}


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  to_stdout ({"Patterns analyzer start"});
  if (program_running (argv)) {
    to_failures (failures_type_api_error (), "", "", 0, {"Patterns analyzer delayed"});
  } else {
    
    // Read the maximum number of simultaneous detectors to run.
    int maximum_simultaneous_detectors = 0;
    if (argc >= 2) {
      maximum_simultaneous_detectors = atoi (argv[1]);
    }
    // Fix invalid or non-numerical input.
    if (maximum_simultaneous_detectors < 1) {
      maximum_simultaneous_detectors = 1;
    }
    to_tty ({"Maximum simultaneous detectors is", to_string (maximum_simultaneous_detectors)});
    
    SQL sql (front_bot_ip_address ());
    
    // Get the exchange/markets/coins considered to be good coins for certain patterns.
    // These are the potentially profitable coins.
    // Read them from the database.
    {
      vector <string> exchanges, markets, coins, reasons;
      patterns_get (sql, exchanges, markets, coins, reasons, false);
      for (unsigned int i = 0; i < exchanges.size (); i++) {
        string signature = exchanges [i] + markets [i] + coins [i] + reasons[i];
        profitable_exchanges_markets_coins_reasons.push_back (signature);
      }
    }
    
    // Get the exchanges where arbitrage is taking place.
    // Skip pattern trading on those exchanges for just now.
    vector <string> arbitraging_exchanges;
    {
      vector <string> exchanges1, exchanges2, markets, coins;
      vector <int> days;
      pairs_get (sql, exchanges1, exchanges2, markets, coins, days);
      for (unsigned int i = 0; i < exchanges1.size (); i++) {
        // If there's coins trading on these exchange, skip those.
        // The reason for this is to enable arbitrage profitable detection still to be on-going.
        // Store the exchanges.
        arbitraging_exchanges.push_back (exchanges1[i]);
        arbitraging_exchanges.push_back (exchanges2[i]);
      }
    }
    
    // Iterate over all exchanges.
    vector <string> exchanges = exchange_get_names ();
    for (auto exchange : exchanges) {

      // If there's arbitrage trade here, skip this exchange.
      if (in_array (exchange, arbitraging_exchanges)) continue;
      // Including this exchange would lead to unclear gain statistics.
      // This exclusion can be left in place,
      // till there is a good way to do combined arbitrage and pattern trading together.

      // Iterate over all markets at this exchange.
      vector <string> markets = exchange_get_supported_markets (exchange);
      for (auto market : markets) {
        vector <string> coins = exchange_get_coins_per_market_cached (exchange, market);
        for (auto coin : coins) {
          //if (exchange != "bitfinex") continue;
          if (market != "bitcoin") continue;
          //if (coin != "bitbay") continue;
          //continue;
          // Wait till there's an available slot for this new detection thread.
          while (current_simultaneous_detector_count >= maximum_simultaneous_detectors) {
            this_thread::sleep_for (chrono::milliseconds (10));
          }
          // Start the detection thread.
          new thread (patterns_detector, exchange, market, coin);
          // Wait shortly to give the previously started thread to update the thread count.
          this_thread::sleep_for (chrono::milliseconds (1));
        }
      }
    }
    
    // Wait till all threads have completed.
    while (current_simultaneous_detector_count > 0) {
      this_thread::sleep_for (chrono::milliseconds (10));
    }

  }
  finalize_program ();
  to_stdout ({"Patterns analyzer ready"});
  return EXIT_SUCCESS;
}

