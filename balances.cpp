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
    to_failures (failures_type_api_error (), "", "", 0, {"Delayed balances processing"});
  }
  
  else {

    to_tty ({"Recording coin balances"});

    // Get all balances for all coins in all wallets on all exchanges.
    get_all_balances ();
    
    record_all_balances ();
    
    // Storage for the withdrawals made: 0: exchange | 1: coin_id | 2: withdrawn amount.
    vector <tuple <string, string, float> > withdrawals;
    redistribute_all_coins (withdrawals);
    
    to_tty ({"Ready dealing with the balances"});
    
  }
  
  finalize_program ();
  return EXIT_SUCCESS;
}
