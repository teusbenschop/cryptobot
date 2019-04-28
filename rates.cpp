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
#include "sqlite.h"


class rate
{
public:
  string exchange;
  string market;
  string coin;
  float bid;
  float ask;
};


mutex ratesmutex;
vector <rate> rates;
unordered_map <string, float> fastbids;
unordered_map <string, float> fastasks;


void fetch_rates_front (string exchange, string market)
{
  // Storage.
  vector <string> coins;
  vector <float> bids;
  vector <float> asks;
  
  // Get the rates from the exchange.
  exchange_get_market_summaries_via_satellite (exchange, market, coins, bids, asks);

  // Store the rates in memory.
  for (unsigned int i = 0; i < coins.size (); i++) {
    rate detail;
    detail.exchange = exchange;
    detail.market = market;
    detail.coin = coins [i];
    // Skip coins that have spaces in their name:
    // It's an error.
    if (detail.coin.find (" ") != string::npos) continue;
    // There has been cases that the coin looks like this:
    // </div><!--
    // This is clearly an error.
    // The SQLite database logic crashed on that.
    if (detail.coin.find ("<") != string::npos) continue;
    // Go on storing this rate.
    detail.bid = bids[i];
    detail.ask = asks [i];
    string key = exchange + market + coins[i];
    ratesmutex.lock ();
    rates.push_back (detail);
    fastbids [key] = bids[i];
    fastasks [key] = asks[i];
    ratesmutex.unlock ();
  }
}


// Store the rates, not in parallel threads, but in one single thread,
// for a lower load on the MySQL server.
// $seconds: The second since Epoch to store with the rates.
void store_rates_to_rates_database (int seconds)
{
  // Nothing to store? Done.
  if (rates.empty ()) return;

  SQL sql (front_bot_ip_address ());

  // Iterate over the data and store the rates in the database.
  // Use one large multiple insert statement.
  // That works much faster than doing multiple single insert statements.
  // The result of this is that every minute there's going to be plenty of rates added.
  // So the database is going to have multiple duplicate rates.
  // The bot deals in two ways with this, elsewhere:
  // 1. When getting the rates, it only gets the newest distinct values.
  // 2. It regularly expires and removes duplicates.
  sql.add ("INSERT INTO rates (second, exchange, market, currency, bid, ask, rate) VALUES");
  for (unsigned int i = 0; i < rates.size(); i++) {
    string exchange = rates[i].exchange;
    string market = rates[i].market;
    string coin = rates[i].coin;
    float bid = rates[i].bid;
    float ask = rates[i].ask;
    if (i) sql.add (",");
    sql.add ("(");
    sql.add (seconds);
    sql.add (",");
    sql.add (exchange);
    sql.add (",");
    sql.add (market);
    sql.add (",");
    sql.add (coin);
    sql.add (",");
    sql.add (bid);
    sql.add (",");
    sql.add (ask);
    sql.add (",");
    sql.add ((bid + ask) / 2);
    sql.add (")");
  }
  sql.add (";");
  sql.execute ();
}


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  to_stdout ({"Rates: Start fetching and recording them"});
  if (program_running (argv)) {
    to_failures (failures_type_api_error (), "", "", 0, {"Rates: Delayed fetching and recording them"});
  } else {
    
    // Gather all the arguments passed to this program.
    // This is to see whether it runs at the front bot,
    // or on one of the back bots.
    vector <string> arguments;
    for (int i = 1; i < argc; i++) {
      arguments.push_back (argv[i]);
    }
    bool run_at_front = in_array (string ("front"), arguments);
    bool run_at_back = in_array (string ("back"), arguments);
    
    string setting = "last_rates_second";
    string settings_path = SqliteDatabase::path (setting);

    SQL sql (front_bot_ip_address ());
    
    // The current number of seconds since the Unix Epoch.
    // Should be the same throughout this recording session.
    // This enables SQL grouping of values per session.
    int second = seconds_since_epoch ();

    // The front bot, rather than the back bot(s),
    // fetches the rates from the exchanges,
    // and stores them all in various databases.
    // The front bot is better for this for these reasons:
    // * Housed in datacenter and will be always on.
    // * Faster connection to the network.
    // The back bots experience interruptions far more often,
    // since they run at home.
    if (run_at_front) {
      to_stdout ({"Rates: Start getting from all exchanges"});

      // Go over all known exchanges.
      vector <string> exchanges = exchange_get_names ();
      vector <thread *> jobs;
      for (auto exchange : exchanges) {
        vector <string> markets = exchange_get_supported_markets (exchange);
        for (auto market : markets) {
          to_tty ({"exchange", exchange, "market", market});
          // Fetch the rates from the exchanges.
          thread * job = new thread (fetch_rates_front, exchange, market);
          jobs.push_back (job);
        }
      }
      // Wait till all jobs are ready.
      for (auto job : jobs) {
        job->join ();
      }

      to_stdout ({"Rates: Ready getting from all exchanges"});

      to_stdout ({"Rates: Start storing them to MySQL"});
      store_rates_to_rates_database (second);
      to_stdout ({"Rates: Ready storing them to MySQL"});

    }
    
    if (run_at_front) {
      
      // Assemble containers suitable for storing them in the databases.
      vector <string> exchanges, markets, coins;
      vector <float> bids, asks;
      for (unsigned int i = 0; i < rates.size(); i++) {
        exchanges.push_back (rates[i].exchange);
        markets.push_back (rates[i].market);
        coins.push_back (rates[i].coin);
        bids.push_back (rates[i].bid);
        asks.push_back (rates[i].ask);
      }
      
      // The front bot stores all the rates for this minute in one dedicated SQLite database.
      // This is to service the back bots so they can download these rates in one go,
      // and add them to their local store.
      // Rather than to have to query 5000++ database,
      // having this single database store all rates for one second,
      // speeds querying up a huge lot.
      to_stdout ({"Rates: Storing", to_string (exchanges.size()), "rates for clients"});
      allrates_add (second, exchanges, markets, coins, bids, asks); 
      to_stdout ({"Rates: Ready storing the rates for clients"});
      
      // Once all rates have been stored for the client,
      // the front bot writes its second to the file,
      // so it can know that this second is now available for download.
      file_put_contents (settings_path, to_string (second));
      settings_set (sql, front_bot_name (), setting, to_string (second));
      // When back bots copy all these database with rsync,
      // the good thing is that it will also copy the most recent second these databases contain.
    }
    
    // The back bots downloads the rates it needs from the front bot and stores them locally.
    if (run_at_back) {

      to_stdout ({"Rates: Back bot requesting available newer rates from front bot"});

      // Get the most recent second that the back bot has downloaded rates for.
      string value = file_get_contents (settings_path);
      if (value.empty ()) value = "0";
      int last_rates_second = stoi (value);
      // Increase it by one.
      last_rates_second++;
      string second_from = to_string (last_rates_second);
      second_from = string_fill (second_from, 10, '0');

      // Get the most recent rate the front bot has stored rates for.
      string second_to = settings_get (sql, front_bot_name (), setting);
      if (second_to.empty ()) second_to = "0";
      second_to = string_fill (second_to, 10, '0');
      
      // Ask all seconds the front bot has rates for.
      // These will be newer than the back bot has.
      string command = "rates between " + second_from + " and " + second_to;
      string error;
      string result = satellite_request (front_bot_ip_address (), command, false, error);
      // Error handler.
      if (!error.empty ()) {
        to_stdout ({"Rates:", "Error:", error});
      } else {

        // Iterate over all available seconds with newer rates.
        vector <string> seconds = explode (result, '\n');
        for (auto second : seconds) {
          if (second.size() != 10) continue;
          to_stdout ({"Rates:", "Downloading rates for second", second, to_string (seconds_since_epoch () - stoi (second)), "seconds old"});
          command = "get rates " + second;
          result = satellite_request (front_bot_ip_address (), command, false, error);
          if (!error.empty ()) {
            to_stdout ({"Rates:", "Error:", error});
          } else {

            // Store the rates for this second in a known database.
            string database = "downloadedrates";
            string path = SqliteDatabase::path (database);
            file_put_contents (path, result);

            // Read the contents of the downloaded database.
            vector <string> exchanges, markets, coins;
            vector <float> bids, asks;
            allrates_get (database, exchanges, markets, coins, bids, asks);

            // Store all rates in that database locally in multiple databases.
            to_stdout ({"Rates:", "Storing", to_string (exchanges.size()), "downloaded rates"});
            for (unsigned int i = 0; i < exchanges.size(); i++) {
              allrates_add (exchanges[i], markets[i], coins[i], stoi (second), bids[i], asks[i]);
            }

            // Update the setting for the back bot's most recently downloaded second.
            // Write it to two places.
            settings_set (sql, hostname (), setting, second);
            file_put_contents (settings_path, second);
          }
        }
      }
    }
    
  }
  finalize_program ();
  to_stdout ({"Rates: Ready fetching and recording them"});
  return EXIT_SUCCESS;
}
