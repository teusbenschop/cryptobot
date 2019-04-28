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
#include "simulators.h"


// Tracker for simultaneous deep learners.
atomic <int> current_simultaneous_learner_count (0);


// Main function for coin rates deep learning.
// The function took 30 minutes on a Proliant DL360e.
void learn_exchange_coin_processor (const string & exchange, const string & coin)
{
  // One more running learner.
  current_simultaneous_learner_count++;

  to_output output (exchange + " " + coin);
  output.to_stderr (true);
  
  // Storage for the prepared rates for looking at the patterns.
  unordered_map <int, float> minute_asks, minute_bids, hour_asks, hour_bids;
  // The change in a coin's value is measured against the Euro.
  // Measuring change against the Bitcoin is not always a good indicator of
  // how much the value of the coin changes.
  // The reason is that the rate of the Bitcoin itself fluctuates a lot too.
  // Thus the Bitcoin is no longer considered the standard to measure everything against.
  // Rather the Euro is now taken instead.
  // This is more realistic.
  // It is expected to lead to better and more stable profitability.
  string market = euro_id ();
  // The number of hours passed to the preparer is for many days.
  // There should be sufficient days to have enough data to learn from.
  pattern_prepare (&output, exchange, market, coin, 61 * 24, minute_asks, minute_bids, hour_asks, hour_bids);
  
  // Container for the forecast properties for all simulated hours.
  vector <forecast_properties> all_forecast_properties;
  
  // Overall weights for all factors taken in account for making a forecast.
  forecast_weights weights;
  
  learn_rates_process_observations (&output, exchange, market, coin, minute_asks, minute_bids, hour_asks, hour_bids, all_forecast_properties, weights);
  
  // After gathering all forecast properties, start optimizing the weights,
  // so as to get the most accurate forecast.
  optimize_forecast_weights (&output, all_forecast_properties, weights);
  {
    vector <string> msg = {
      "Weights:",
      "hourly", float2string (weights.hourly_change_weight),
      "daily", float2string (weights.daily_change_weight),
      "weekly", float2string (weights.weekly_change_weight),
    };
    for (int hours : deep_learning_hours () ) {
      msg.push_back (to_string (hours));
      msg.push_back (float2string (weights.hours_curve_weights [hours]));
    }
    output.add (msg);
  }

  // The final residuals calculation, with the optimized weights.
  float sum_absolute_residuals, maximum_residual_percentage, average_residual_percentage;
  get_forecast_residuals (&output, all_forecast_properties, weights, sum_absolute_residuals, maximum_residual_percentage, average_residual_percentage);
  // Fix things like: Unknown column 'nan' in 'field list' below.
  if (isnan (maximum_residual_percentage)) maximum_residual_percentage = 100;
  if (isnan (average_residual_percentage)) average_residual_percentage = 100;
  if (isinf (maximum_residual_percentage)) maximum_residual_percentage = 100;
  if (isinf (average_residual_percentage)) average_residual_percentage = 100;
  {
    vector <string> msg = {
      "Sum residuals", float2string (sum_absolute_residuals),
      "Maximum residual", float2visual (maximum_residual_percentage), "%",
      "Average residual", float2visual (average_residual_percentage), "%",
    };
    output.add (msg);
  }

  // Store the weights and residuals in the database.
  string json = weights_encode (weights.hourly_change_weight,
                                weights.daily_change_weight,
                                weights.weekly_change_weight,
                                weights.hours_curve_weights);
  SQL sql (front_bot_ip_address ());
  weights_save (sql, exchange, market, coin, json,
                sum_absolute_residuals, maximum_residual_percentage, average_residual_percentage);

  // One deep learner has completed.
  current_simultaneous_learner_count--;
}


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  to_stdout ({"Learn: Start deep learning"});
  
  if (program_running (argv)) {
    to_failures (failures_type_api_error (), "", "", 0, {"Learn: Deep learning delayed"});
  } else {

    // Read the maximum number of simultaneous deep learners to run.
    int maximum_simultaneous_learners = 0;
    if (argc >= 2) {
      maximum_simultaneous_learners = str2int (argv[1]);
    }
    // Fix invalid or non-numerical input.
    if (maximum_simultaneous_learners < 1) {
      maximum_simultaneous_learners = 1;
    }
    to_tty ({"Maximum simultaneous deep learners is", to_string (maximum_simultaneous_learners)});

    // Iterate over all exchanges and their coins.
    vector <string> exchanges = exchange_get_names ();
    for (auto exchange : exchanges) {
      //if (exchange != "bittrex") continue;
      vector <string> coins = exchange_get_coin_ids (exchange);
      for (auto coin : coins) {
        //if (coin != "goldcoin") continue;
        
        // Wait till there's an available slot for this new deep learning thread.
        while (current_simultaneous_learner_count >= maximum_simultaneous_learners) {
          this_thread::sleep_for (chrono::milliseconds (10));
        }

        // Start the deep learning thread.
        new thread (learn_exchange_coin_processor, exchange, coin);
        // Wait shortly to give the previously started thread time to update the thread count.
        this_thread::sleep_for (chrono::milliseconds (1));
      }
    }
    
    // Wait till all threads have completed.
    while (current_simultaneous_learner_count > 0) {
      this_thread::sleep_for (chrono::milliseconds (10));
    }
  }
  
  finalize_program ();
  to_stdout ({"Learn: Deep learning ready"});
  return EXIT_SUCCESS;
}
