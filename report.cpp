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
#include "generators.h"
#include "bl3p.h"


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);

  // Gather the arguments passed.
  vector <string> arguments;
  for (int i = 1; i < argc; i++) {
    arguments.push_back (argv[i]);
  }

  // Whether to email the output.
  bool email  = false;
  {
    string needle = "email";
    if (in_array (needle, arguments)) {
      email = true;
      array_remove (needle, arguments);
    }
  }
  
  // Possible commands.
  string gains = "gains";
  string fails = "fails";
  string orders = "orders";
  string performance = "performance";
  string delays = "delays";
  string bl3p = "bl3p";
  vector <string> commands = {
    gains,
    fails,
    orders,
    performance,
    delays,
    bl3p,
  };
  
  // Actual commands.
  vector <string> command = array_intersect (arguments, commands);

  if (in_array (gains, command)) {
    daily_book_keeper (email);
  }
  
  if (in_array (fails, command)) {
    failures_reporter (email);
  }
  
  if (in_array (orders, command)) {
    orders_reporter ();
  }
  
  if (in_array (performance, command)) {
    performance_reporter (email);
  }
  
  if (in_array (delays, command)) {
    delayed_transfers_reporter (email);
  }

  if (in_array (bl3p, command)) {
    vector <string> coin_abbrevs, coins;
    vector <float> total, reserved, unconfirmed;
    exchange_get_balances (bl3p_get_exchange (), coin_abbrevs, coins, total, reserved, unconfirmed);
    for (unsigned int i = 0; i < coins.size(); i++) {
      to_stderr ({coins[i], "total", float2string (total[i]), "reserved", float2string (reserved[i]), "unconfirmed", float2string (unconfirmed[i])});
    }
  }

  if (command.empty ()) {
    commands.insert (commands.begin(), "Usage: ./report [email]");
    to_stdout (commands);
  }
  
  
  finalize_program ();
  return EXIT_SUCCESS;
}
