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
#include "simulators.h"


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  to_stdout ({"Optimizer started"});
  
  if (program_running (argv)) {
    to_failures (failures_type_api_error (), "", "", 0, {"Delayed optimizer"});
  } else {
    
    SQL sql_front (front_bot_ip_address ());
    SQL sql_back (back_bot_ip_address ());

    // Read all coin rates and projections from the database.
    // They were stored during last run.
    // Store them like this: container [exchange+coin] = value;
    unordered_map <string, int> optimizer_id;
    unordered_map <string, float> optimizer_start;
    unordered_map <string, float> optimizer_forecast;
    {
      vector <int> ids;
      vector <string> exchanges, coins;
      vector <float> starts, forecasts, realizeds;
      optimizer_get (sql_front, ids, exchanges, coins, starts, forecasts, realizeds);
      for (unsigned int i = 0; i < exchanges.size(); i++) {
        string key = exchanges[i] + coins[i];
        optimizer_id [key] = ids [i];
        optimizer_start [key] = starts [i];
        optimizer_forecast [key] = forecasts [i];
      }
    }
    
    // Read the most recent recorded total balances from the database.
    // It does not read the balances from the exchanges,
    // because this optimizer will run separately from the trading bot,
    // so private calls to the exchanges will result in nonce errors, at times.
    // Store the balances as container [exchange][coin] = balance.
    unordered_map <string, unordered_map <string, float> > exchange_coin_total_balances;
    // Store the available coins with balances per exchange.
    unordered_map <string, vector <string> > exchange_coin_names;
    {
      vector <string> exchanges, coins;
      vector <float> totals;
      balances_get_current (sql_back, exchanges, coins, totals);
      for (unsigned int i = 0; i < exchanges.size(); i++) {
        string exchange = exchanges [i];
        string coin = coins [i];
        float total = totals [i];
        //to_tty ({exchange, coin, float2string (total)});
        exchange_coin_total_balances [exchange][coin] = total;
        exchange_coin_names [exchange].push_back (coin);
      }
    }
    
    vector <string> exchanges = exchange_get_names ();
    for (auto exchange : exchanges) {
      //if (exchange != "bittrex") continue;
      vector <string> coins = exchange_coin_names [exchange];
      for (auto coin : coins) {
        //if (coin != "goldcoin") continue;
        float total_balance = exchange_coin_total_balances [exchange][coin];
        float starting_rate = optimizer_start [exchange + coin];
        float forecast_change_percentage = optimizer_forecast [exchange + coin];
        int database_record_id = optimizer_id [exchange + coin];
        bool update_database = true;
        bool plot_rates = true; plot_rates = false;
        rate_forecast_validator (exchange, coin,
                                 total_balance,
                                 starting_rate, forecast_change_percentage,
                                 update_database, database_record_id,
                                 plot_rates);
      }
    }
  }
  
  finalize_program ();
  to_stdout ({"Optimizer completed"});
  return EXIT_SUCCESS;
}
