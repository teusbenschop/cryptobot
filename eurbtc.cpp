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
#include "exchanges.h"
#include "bl3p.h"
#include "models.h"
#include "proxy.h"
#include "sqlite.h"


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  to_stdout ({"Rates: Start fetching and recording them"});
  if (program_running (argv)) {
    to_stderr ({"Rates: Delayed fetching and recording them"});
  } else {
    
    // The current number of seconds since the Unix Epoch.
    // Should be the same throughout this recording session.
    // This enables SQL grouping of values per session.
    int second = seconds_since_epoch ();

    // Fetch Bitcoin prices in Euros from BL3P.
    string exchange = bl3p_get_exchange ();
    string market = euro_id ();

    // Get the rates from the exchange.
    vector <string> coins;
    vector <float> bids;
    vector <float> asks;
    string error;
    exchange_get_market_summaries (exchange, market, coins, bids, asks, &error);
    
    // Store the rates in memory.
    for (unsigned int i = 0; i < coins.size (); i++) {

      // Skip coins that have spaces in their name:
      // It's an error.
      if (coins[i].find (" ") != string::npos) continue;

      // There has been cases that the coin looks like this:
      // </div><!--
      // This is clearly an error.
      // The SQLite database logic crashed on that.
      if (coins[i].find ("<") != string::npos) continue;

      // Go on storing this rate.
      allrates_add (exchange, market, coins[i], second, bids[i], asks[i]);

      // Feedback.
      to_tty ({exchange, market, coins[i], "bid", float2string (bids[i]), "ask", float2string (asks[i]), error});
    }
  }

  finalize_program ();
  to_stdout ({"Rates: Ready fetching and recording them"});
  return EXIT_SUCCESS;
}
