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


#include "sql.h"
#include "exchanges.h"
#include "models.h"
#include "controllers.h"
#include "proxy.h"
#include "bittrex.h"
#include "cryptopia.h"
#include "simulators.h"
#include "html.h"
#include "generators.h"


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  to_stdout ({"Website: Start"});
  
  if (program_running (argv)) {
    to_failures (failures_type_api_error (), "", "", 0, {"Website: Previous instance delayed"});
  } else {

    // Read the maximum number of simultaneous threads to run.
    int max_threads = 1;
    if (argc >= 2) {
      max_threads = str2int (argv[1]);
    }
    // Fix invalid or non-numerical input.
    if (max_threads < 1) {
      max_threads = 1;
    }
    to_tty ({"Maximum simultaneous threads is", to_string (max_threads)});

    // Load data from the database.
    generator_load_data ();
    
    // Load prices coins were bought for.
    load_bought_coins_details ();
    
    // Generate all pages for the website.
    website_index_generator ();
    //prospective_coin_page_generator (max_threads);
    //asset_performance_page_generator (max_threads);
    balances_page_generator (max_threads);
    arbitrage_performance_page_generator (max_threads);
    arbitrage_opportunities_page_generator (max_threads);
    website_consistency_generator ();
    website_notes_generator ();
    website_javascript_copier ();
  }

  finalize_program ();
  to_stdout ({"Website: Ready"});
  return EXIT_SUCCESS;
}
