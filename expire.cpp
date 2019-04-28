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


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  to_tty ({"Expiring"});

  
  // Gather the arguments passed.
  vector <string> arguments;
  for (int i = 1; i < argc; i++) {
    arguments.push_back (argv[i]);
  }
  
  
  // Possible commands.
  string arbitrage = "arbitrage";
  string patterns = "patterns";
  string rates = "rates";
  string sqlite = "sqlite";
  string sqlitesmall = "sqlitesmall";
  string sqlitelarge = "sqlitelarge";
  string balances = "balances";
  string availables = "availables";
  vector <string> commands = {arbitrage, patterns, rates, sqlite, sqlitesmall, sqlitelarge, balances, availables};

  
  // Whether it executed a command.
  bool ran_command = false;
  
  
  // Expire arbitrage data.
  // The arbitrage data is kept for some time.
  if (in_array (arbitrage, arguments)) {
    to_stdout ({"Removing expired arbitrage data"});
    SQL sql (back_bot_ip_address ());
    sql.clear ();
    sql.add ("DELETE FROM arbitrage WHERE stamp < NOW() - INTERVAL 15 DAY;");
    sql.execute ();
    ran_command = true;
  }
  
  
  // Expire all sorts of patterns databases.
  if (in_array (patterns, arguments)) {
    to_stdout ({"Removing expired pattern data"});
    SQL sql (front_bot_ip_address ());
    sql.clear ();
    //sql.add ("DELETE FROM live WHERE stamp < NOW() - INTERVAL 60 DAY;");
    //sql.execute ();
    ran_command = true;
  }
  
  
  // Expire SQLite databases.
  // Set the number of days after which data should be expired.
  // At the front bot, it requires far fewer days than at the back bots.
  // Because the analysis is done at the back bots, so that needs big data.
  int sqlite_expiry_days = 0;
  if (in_array (sqlitesmall, arguments)) sqlite_expiry_days = 20;
  if (in_array (sqlite, arguments)) sqlite_expiry_days = 60;
  if (in_array (sqlitelarge, arguments)) sqlite_expiry_days = 120;
  if (sqlite_expiry_days) {
    to_stdout ({"Removing older than", to_string (sqlite_expiry_days), "days from the SQLite databases"});
    vector <string> exchanges = exchange_get_names ();
    for (auto exchange : exchanges) {
      vector <string> markets = exchange_get_supported_markets (exchange);
      for (auto market : markets) {
        vector <string> coins = exchange_get_coin_ids (exchange);
        for (auto coin : coins) {
          allrates_expire (exchange, market, coin, sqlite_expiry_days);
        }
      }
    }
    ran_command = true;
  }

  
  // Expire rates.
  if (in_array (rates, arguments)) {
    to_stdout ({"Removing expired rates"});
    SQL sql (front_bot_ip_address ());
    sql.clear ();
    // Retrieve the row identifier, plus exchange/market/coin information.
    // The rates are not relevant here for expiry purposes.
    // Because it will expire duplicate exchange/market/coin records,
    // regardless of their rates values.
    sql.add ("SELECT id, exchange, market, currency FROM rates ORDER BY id;");
    map <string, vector <string> > result = sql.query ();
    vector <string> id = result ["id"];
    vector <string> exchange = result ["exchange"];
    vector <string> market = result ["market"];
    vector <string> coin = result ["currency"];
    // The row identifiers to delete from the database.
    vector <int> ids_to_delete;
    // Store distinct values for exchange/market/coin.
    unordered_map <string, int> distinctvalues;
    for (unsigned int i = 0; i < id.size(); i++) {
      string key = exchange[i] + market[i] + coin[i];
      int value = stoi (id[i]);
      // If the exchange/market/coin was stored already,
      // this is an old entry that can be deleted,
      // so store its row identifier.
      int id_to_delete = distinctvalues [key];
      if (id_to_delete) ids_to_delete.push_back (id_to_delete);
      // Store the new distinct exchange/markete/coin.
      distinctvalues [key] = value;
    }
    // It was tried to delete 100k+ of values in one go,
    // but this took a very long time.
    // So the code below deletes the row identifiers
    // in chunks of 1k a time.
    // That gives very good performance.
    while (!ids_to_delete.empty ()) {
      sql.clear ();
      sql.add ("DELETE FROM rates WHERE");
      for (unsigned int i = 0; i < 1000; i++) {
        if (ids_to_delete.empty ()) continue;
        if (i) sql.add ("OR");
        sql.add ("id =");
        sql.add (ids_to_delete[0]);
        ids_to_delete.erase (ids_to_delete.begin());
      }
      sql.add (";");
      sql.execute ();
    }
    // Finally delete entries older than a few weeks.
    sql.clear ();
    sql.add ("DELETE FROM rates WHERE stamp < NOW() - INTERVAL 15 DAY;");
    sql.execute ();
    ran_command = true;
  }

  
  // Expire the balances database.
  if (in_array (balances, arguments)) {
    to_stdout ({"Expiring balances"});
    SQL sql (back_bot_ip_address ());
    // Remove older balances from the back bot.
    // The current balances graphs do not exceed one month.
    sql.add ("DELETE FROM balances WHERE stamp < NOW() - INTERVAL 40 DAY;");
    sql.execute ();
    ran_command = true;
  }


  // Expire the availables database.
  if (in_array (availables, arguments)) {
    to_stdout ({"Expiring available trades"});
    SQL sql (back_bot_ip_address ());
    // Remove older balances from the back bot.
    // The current relevant graphs read and display all available data.
    sql.add ("DELETE FROM availables WHERE stamp < NOW() - INTERVAL 5 DAY;");
    sql.execute ();
    ran_command = true;
  }


  // Help information.
  if (!ran_command) {
    commands.insert (commands.begin(), "Usage: ./expire");
    to_stdout (commands);
  }
  
  
  finalize_program ();
  to_tty ({"Ready expiring"});
  return EXIT_SUCCESS;
}
