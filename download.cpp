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
#include "exchanges.h"
#include "controllers.h"
#include "proxy.h"


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  
  // Detect possible previous instance.
  if (program_running (argv)) {
    to_failures (failures_type_api_error (), "", "", 0, {"Delayed data downloader"});
  } else {
    
    SQL sql_front (front_bot_ip_address ());
    SQL sql_back (back_bot_ip_address ());

    to_tty ({"Downloading coin balances"});
    {
      // Read the most recent second of the already downloaded balances.
      int most_recent_downloaded_second = -1;
      {
        sql_back.clear ();
        sql_back.add ("SELECT MAX(seconds) FROM balances;");
        map <string, vector <string> > result = sql_back.query ();
        vector <string> seconds = result ["MAX(seconds)"];
        if (!seconds.empty ()) {
          most_recent_downloaded_second = str2int (seconds[0]);
        }
      }
      to_tty ({"Most recent downloaded second", to_string (most_recent_downloaded_second)});

      // Read all data from the balances at the front bot that is not older than the most recent downloaded second.
      vector <int> front_seconds;
      vector <string> front_stamp, front_exchange, front_coin;
      vector <float> front_total, front_btc, front_euro;
      if (most_recent_downloaded_second >= 0) {
        sql_front.clear ();
        sql_front.add ("SELECT * FROM balances WHERE seconds >=");
        sql_front.add (most_recent_downloaded_second);
        sql_front.add ("ORDER BY id;");
        map <string, vector <string> > result = sql_front.query ();
        vector <string> seconds = result ["seconds"];
        vector <string> stamp = result ["stamp"];
        vector <string> exchange = result ["exchange"];
        vector <string> coin = result ["coin"];
        vector <string> total = result ["total"];
        vector <string> btc = result ["btc"];
        vector <string> euro = result ["euro"];
        for (size_t i = 0; i < seconds.size(); i++) {
          front_seconds.push_back (str2int (seconds[i]));
          front_stamp.push_back (stamp[i]);
          front_exchange.push_back (exchange[i]);
          front_coin.push_back (coin[i]);
          front_total.push_back (str2float (total[i]));
          front_btc.push_back (str2float (btc[i]));
          front_euro.push_back (str2float (euro[i]));
        }
      }
      to_tty ({"Downloaded new balances", to_string (front_seconds.size())});

      // Read data from the balances at the back bot that is of the exact most recent downloaded second.
      // Store the exchanges plus coins for quick reference.
      vector <string> back_exchanges_plus_coins;
      if (most_recent_downloaded_second > 0) {
        sql_back.clear ();
        sql_back.add ("SELECT exchange, coin FROM balances WHERE seconds =");
        sql_back.add (most_recent_downloaded_second);
        sql_back.add (";");
        map <string, vector <string> > result = sql_back.query ();
        vector <string> exchange = result ["exchange"];
        vector <string> coin = result ["coin"];
        for (size_t i = 0; i < exchange.size(); i++) {
          back_exchanges_plus_coins.push_back (exchange[i] + coin[i]);
        }
      }
      to_tty ({"There are", to_string (back_exchanges_plus_coins.size()), "existing balances at the back bot at second", to_string (most_recent_downloaded_second)});

      // Iterate over the new balances at the front bot,
      // check if an item is already stored at the back bot,
      // and store the relevant items at the back bot.
      int stored_balances_count = 0;
      for (size_t i = 0; i < front_seconds.size(); i++) {
        int seconds = front_seconds [i];
        string stamp = front_stamp [i];
        string exchange = front_exchange[i];
        string coin = front_coin[i];
        float total = front_total[i];
        float btc = front_btc[i];
        float euro = front_euro [i];
        string key = exchange + coin;
        bool stored = (seconds == most_recent_downloaded_second) && in_array (key, back_exchanges_plus_coins);
        if (!stored) {
          sql_back.clear ();
          sql_back.add ("INSERT INTO balances (seconds, stamp, exchange, coin, total, btc, euro) VALUES (");
          sql_back.add (seconds);
          sql_back.add (",");
          sql_back.add (stamp);
          sql_back.add (",");
          sql_back.add (exchange);
          sql_back.add (",");
          sql_back.add (coin);
          sql_back.add (",");
          sql_back.add (total);
          sql_back.add (",");
          sql_back.add (btc);
          sql_back.add (",");
          sql_back.add (euro);
          sql_back.add (");");
          sql_back.execute ();
          stored_balances_count++;
        }
      }
      to_stdout ({"Stored", to_string (stored_balances_count), "new coin balances at the back bot"});
      
      // Remove the balances from the front bot which are older than the most recent downloaded second.
      if (most_recent_downloaded_second) {
        to_tty ({"Deleting records from balances at the front bot"});
        sql_front.clear ();
        sql_front.add ("DELETE FROM balances WHERE seconds <");
        sql_front.add (most_recent_downloaded_second);
        sql_front.add (";");
        sql_front.execute ();
      }
    }
    to_tty ({"Ready downloading the balances"});
    
    {
      to_tty ({"Downloading arbitrage quantities"});
      // Read all data from the arbitrage quantities at the front bot.
      vector <int> ids;
      vector <string> stamps;
      vector <int> seconds;
      vector <string> sellers, buyers, markets, coins;
      vector <float> bidsizes, asksizes, realizeds;
      availables_get (sql_front, ids, stamps, seconds, sellers, buyers, markets, coins, bidsizes, asksizes, realizeds);
      to_stdout ({"Downloaded", to_string (ids.size()), "new arbitrage quantities from the front bot"});
      // Store the downloaded data at the back bot and remove it from the front bot.
      for (size_t i = 0; i < ids.size(); i++) {
        availables_store (sql_back, stamps[i], seconds[i], sellers[i], buyers[i], markets[i], coins[i], bidsizes[i], asksizes[i], realizeds[i]);
        availables_delete (sql_front, ids[i]);
      }
      to_stdout ({"Stored", to_string (ids.size()), "new arbitrage quantities at the back bot"});
    }
  }
  
  finalize_program ();
  return EXIT_SUCCESS;
}
