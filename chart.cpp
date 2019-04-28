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
#include "exchanges.h"
#include "shared.h"
#include "proxy.h"
#include "controllers.h"


int main (int argc, char *argv[])
{
  // Start up.
  initialize_program (argc, argv);
  
  
  // Gather all the arguments passed to this program.
  vector <string> arguments;
  for (int i = 1; i < argc; i++) {
    arguments.push_back (argv[i]);
  }
  
  
  // The possible commands to run.
  string balances = "balances";
  string rates = "rates";
  vector <string> commands = {balances, rates};
  // Find a command in the arguments.
  string command;
  if (in_array (balances, arguments)) {
    command = balances;
  }
  if (in_array (rates, arguments)) {
    command = rates;
  }
  if (!command.empty ()) {
    to_tty ({"Command:", command});
    array_remove (command, arguments);
    
    
    // Find known exchanges in the arguments.
    vector <string> known_exchanges = exchange_get_names ();
    vector <string> exchanges = array_intersect (arguments, known_exchanges);
    for (auto & exchange : exchanges) array_remove (exchange, arguments);
    if (exchanges.empty ()) {
      to_tty ({"Using all known exchanges (optionally pass one or more exchanges)"});
    } else {
      vector <string> msg = exchanges;
      msg.insert (msg.begin (), "Exchange(s):");
      to_tty (msg);
    }
    
    
    // Read all possible coin identifiers at all exchanges.
    // Find the coin identifier passed on the command line.
    string coin;
    {
      vector <string> allcoins = exchange_get_coin_ids ("");
      vector <string> ourcoins = array_intersect (allcoins, arguments);
      for (auto & ourcoin : ourcoins) {
        coin = ourcoin;
        array_remove (ourcoin, arguments);
      }
    }
    if (coin.empty ()) {
      to_tty ({"Using all coins (optionally pass a coin identifier)"});
    } else {
      to_tty ({"Coin:", coin});
    }
    

    // Sets of plot data plus their titles.
    vector <vector <pair <float, float> > > plotdatas;
    vector <string> titles;


    // The different periods to chart, in days.
    vector <int> periods = { 1, 7, 30, 180, 365, 1095 };

    
    // Do the balances.
    if (command == balances) {
      // Iterate over the different periods to chart.
      for (auto days : periods) {
        // Generate the data for the chart: The balances expressed in the coin passed.
        vector <pair <float, float> > plotdata;
        plotdata = chart_balances (exchanges, coin, days, false, false);
        plotdatas.push_back (plotdata);
        // The title for this chart.
        string title = "balance " + implode (exchanges, " ");
        if (!coin.empty ()) title.append (" " + coin);
        title.append (" " + to_string (days) + " days");
        titles.push_back (title);
        // Generate the same data but with the balances converted to Bitcoins.
        plotdata = chart_balances (exchanges, coin, days, true, false);
        plotdatas.push_back (plotdata);
        titles.push_back (title + " bitcoins");
        // Generate the same data but with the balances converted to Euros.
        plotdata = chart_balances (exchanges, coin, days, false, true);
        plotdatas.push_back (plotdata);
        titles.push_back (title + " euros");
      }
    }

    
    // Do the rates.
    if (command == rates) {
      // Get the input.
      string exchange = argv [2];
      string market = argv [3];
      string coin = argv [4];
      // Iterate over the different periods to chart.
      for (auto days : periods) {
        to_tty ({"Days:", to_string (days)});
        vector <pair <float, float> > plotdata;
        plotdata = chart_rates (exchange, market, coin, days);
        plotdatas.push_back (plotdata);
        string title = "rates " + exchange + " " + market + " " + coin + " " + to_string (days);
        titles.push_back (title);
      }
    }


    // Do the plotting.
    for (unsigned int i = 0; i < plotdatas.size(); i++) {
      to_tty ({"Plotting", to_string (i)});
      auto plotdata = plotdatas[i];
      string title = titles[i];
      string picturepath;
      plot (plotdata, {}, title, "", picturepath);
    }
    
    
    // Open the folder with the graphs.
    if (isatty (fileno (stdout))) {
      string folder = dirname (dirname (program_path)) + "/cryptodata/graphs";
      string command = "open " + folder;
      system (command.c_str());
    }
    
    
  } else {
    to_tty ({"Usage:"});
    to_tty ({"./chart balances [exchange[s]] [coin]"});
    to_tty ({"./chart rates exchange market coin"});
  }

  
  // Done.
  finalize_program ();
  return EXIT_SUCCESS;
}


