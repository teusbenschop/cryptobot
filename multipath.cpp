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


// Store the rates as container [exchange+market+coin] = rate.
unordered_map <string, float> exchange_market_coin_bids;
unordered_map <string, float> exchange_market_coin_asks;

// Store the coins as container [exchange+market] = coins.
unordered_map <string, vector <string> > exchange_market_coins;

mutex rates_mutex;

// The minimum trade size per exchange/market/coin.
map <string, float> minimum_trade_sizes;

// The pairs of exchange / market / coin that have their trading paused.
vector <string> paused_trading_exchanges_markets_coins;

// The multipaths already stored at the front bot.
vector <multipath> stored_multipaths;

// Tracker for simultaneous analyzers.
atomic <int> current_simultaneous_analyzer_count (0);


void multipath_get_rates (const string & exchange, const string & market)
{
  vector <string> coins = exchange_get_coins_per_market_cached (exchange, market);
  for (auto coin : coins) {
    float ask, bid;
    bool success = get_cached_approximate_rate (exchange, market, coin, ask, bid);
    if (success) {
      rates_mutex.lock ();
      string exchange_market_coin = exchange + market + coin;
      exchange_market_coin_bids [exchange_market_coin] = bid;
      exchange_market_coin_asks [exchange_market_coin] = ask;
      string exchange_market = exchange + market;
      exchange_market_coins [exchange_market].push_back (coin);
      rates_mutex.unlock ();
    }
  }
}


void multipath_analyze_one (multipath & path)
{
  // One more running multipath analyzer.
  current_simultaneous_analyzer_count++;

  // Flag whether the path is good to go.
  bool path_is_good = true;
  
  // Check that this path is not already stored.
  if (path_is_good) {
    bool already_stored = false;
    for (auto & storedpath : stored_multipaths) {
      // Do not check against multipaths that have a "done" status.
      // Because it could be that these paths can be executed again with profit.
      if (storedpath.status == multipath_status::done) continue;
      // Equal: Stored already.
      if (multipath_equal (path, storedpath)) already_stored = true;
    }
    if (already_stored) path_is_good = false;
  }

  // Check whether any of the market/coins in this multipath is paused.
  if (path_is_good) {
    bool paused = multipath_paused (&path, paused_trading_exchanges_markets_coins, false);
    if (paused) path_is_good = false;
  }
  
  // Storage for the approximate raw order books.
  vector <float> quantities1;
  vector <float> asks1;
  vector <float> quantities2;
  vector <float> bids2;
  vector <float> quantities3;
  vector <float> asks3;
  vector <float> quantities4;
  vector <float> bids4;
      
  // Step 1: Get prices for buying coin 1 at market 1, if the market differs from the coin.
  if (path_is_good) {
    if (path.market1name != path.coin1name) {
      path_is_good = get_cached_approximate_sellers (path.exchange, path.market1name, path.coin1name, quantities1, asks1);
    }
  }
      
  // Step 2: Get prices for selling coin 2 at market 2, if the market differs from the coin.
  if (path_is_good) {
    if (path.market2name != path.coin2name) {
      path_is_good = get_cached_approximate_buyers (path.exchange, path.market2name, path.coin2name, quantities2, bids2);
    }
  }
  
  // Step 3: Get prices for buying coin 3 at market 3, if the market differs from the coin.
  if (path_is_good) {
    if (path.market3name != path.coin3name) {
      path_is_good = get_cached_approximate_sellers (path.exchange, path.market3name, path.coin3name, quantities3, asks3);
    }
  }
  
  // Step 4: Get prices for selling coin 4 at market 4, if the market differs from the coin.
  if (path_is_good) {
    if (path.market4name != path.coin4name) {
      path_is_good = get_cached_approximate_buyers (path.exchange, path.market4name, path.coin4name, quantities4, bids4);
    }
  }
  
  // Process the multipath.
  if (path_is_good) {
    multipath_processor (nullptr, &path, minimum_trade_sizes,
                         quantities1, asks1, quantities2, bids2, quantities3, asks3, quantities4, bids4,
                         1);
  }
  
  // Store the investigated paths to the database.
  if (path_is_good) {
    if (path.status == multipath_status::profitable) {
      // Set the path status back to "bare", so the front bot will investigate it right before trade.
      path.status = multipath_status::bare;
      to_output output ("");
      output.add ({"The multipath analyzer will store the following path to the front bot"});
      multipath_output (&output, &path);
      SQL sql (front_bot_ip_address ());
      multipath_store (sql, path);
    }
  }
  
  // One multipath analyzer completed.
  current_simultaneous_analyzer_count--;
}


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  to_stdout ({"Multipath analyzer starting"});
  if (program_running (argv)) {
    to_failures (failures_type_api_error (), "", "", 0, {"Multipath analyzer delayed"});
  } else {

    
    // One of the purposes of this analyzer is to provide profitable multipaths in real time.
    // In a former configuration, the suggested multipaths were not processed for hours.
    // By the time they get processed, other traders had taken advantage of the opportunity already.
    // In the new configuration, it provides the profitable multipaths within one or two minutes.
    // Later it was found that providing the multipaths within one or two minutes was not soon enough.
    // The new goal is to process all multipaths within ten seconds.
    // This enables more timely multipath processing.
    // Not having timely multipaths gives other traders the opportunity to take advantage of a profitable path.
    // The bot will miss out on such a path then.
    // So timely multipaths are important for a profitable trade.

    
    // Read the maximum number of simultaneous analyzers to run.
    int maximum_simultaneous_analyzers = 0;
    if (argc >= 2) {
      maximum_simultaneous_analyzers = atoi (argv[1]);
    }
    // Fix invalid or non-numerical input.
    if (maximum_simultaneous_analyzers < 1) {
      maximum_simultaneous_analyzers = 1;
    }
    to_stdout ({"Maximum simultaneous detectors is", to_string (maximum_simultaneous_analyzers)});

    
    // The SQL server.
    SQL sql (front_bot_ip_address ());

    
    // The pairs of exchange / market / coin that have their trading paused.
    // Storage format: vector of exchange+market+coin, as one string.
    vector <string> paused_trading_exchanges_markets_coins;
    paused_trading_exchanges_markets_coins = pausetrade_get (sql);

    
    // Read the multipaths already stored at the front bot.
    stored_multipaths = multipath_get (sql);

    
    // Fetch the approximate rates from the exchanges in parallel threads.
    {
      vector <thread *> jobs;
      vector <string> exchanges = exchange_get_names ();
      for (auto exchange : exchanges) {
        vector <string> markets = exchange_get_supported_markets (exchange);
        for (auto market : markets) {
          thread * job = new thread (multipath_get_rates, exchange, market);
          jobs.push_back (job);
        }
      }
      for (auto job : jobs) job->join ();
    }
    to_stdout ({"Analyzing multipath trading in", to_string (exchange_market_coin_bids.size()), "exchange/market/coin combinations"});

    vector <multipath> bare_multipaths;
    
    {
      // Storage for feedback about exchanges and base markets.
      map <string, int> exchanges_markets_counts;
      
      // Iterate over all supported exchanges.
      vector <string> exchanges = exchange_get_names ();
      for (auto exchange : exchanges) {
        
        // Start with Bitcoins just now.
        // Because that's where most of the funds are kept just now.
        // Run the simulation with 0.01 Bitcoin to start with.
        // Update: It will now also start at other markets.
        // Starting at other markets has advantages.
        // One good thing of this is that other markets have lower dust trade values.
        // Thus this may lead to better order books down the multipath,
        // and this may lead to better profitability.
        vector <string> market1names = exchange_get_supported_markets (exchange);
        for (auto market1name : market1names) {
          float market1quantity = 0.01;
          
          // Iterate over all the first coins that can be bought at this first market.
          vector <string> coin1names = exchange_market_coins [exchange + market1name];
          for (auto coin1name : coin1names) {
            
            // Get the rate, and calculate the amount of first coins obtained through this simulated trade.
            // It does not yet take the exchange fees in account.
            // The logic does that later on.
            float ask1 = exchange_market_coin_asks [exchange + market1name + coin1name];
            float coin1quantity = market1quantity / ask1;
            
            // The coin1 is now taken as coin2 in the next step.
            string coin2name = coin1name;
            float coin2quantity = coin1quantity;
            
            // Iterate over all supported markets at this exchange.
            vector <string> market2names = exchange_get_supported_markets (exchange);
            for (auto market2name : market2names) {
              
              // Get the rate to sell the second coin at the second market.
              // Or in other words, to convert the second coin into value at the second market.
              // The proceeds will be a certain amount of that secoind market coin.
              float bid2 = exchange_market_coin_bids [exchange + market2name + coin2name];
              float market2quantity = bid2 * coin2quantity;
              
              // The market 2 is now taken as market 3 in the next step.
              string market3name = market2name;
              float market3quantity = market2quantity;
              
              // Iterate over all third coins at this third market.
              vector <string> coin3names = exchange_market_coins [exchange + market3name];
              for (auto coin3name : coin3names) {
                
                // Get the rate for buying a third coin at the third market.
                // And buy that coin there.
                float ask3 = exchange_market_coin_asks [exchange + market3name + coin3name];
                float coin3quantity = market3quantity / ask3;
                
                // In the next step, the third coin is taken as the fourth coin at the fourth market.
                string coin4name = coin3name;
                float coin4quantity = coin3quantity;
                
                // Get the rate for selling the fourth coin back on the primary market, called market 4 here.
                string market4name = market1name;
                float bid4 = exchange_market_coin_bids [exchange + market4name + coin4name];
                
                // Sell the fourth coin on the fourth market, that's the primary market.
                float market4quantity = bid4 * coin4quantity;
                
                // Do gains calculations in percentages. 
                float gain = (market4quantity - market1quantity) / market1quantity * 100;
                if (isnan (gain)) gain = 0;
                if (isinf (gain)) gain = 0;
                
                // Do further checks on whether the simulation produces real figures.
                if (isnan (coin1quantity)) continue;
                if (isinf (coin1quantity)) continue;
                if (isnan (market2quantity)) continue;
                if (isinf (market2quantity)) continue;
                if (isnan (coin3quantity)) continue;
                if (isinf (coin3quantity)) continue;
                if (isnan (market4quantity)) continue;
                if (isinf (market4quantity)) continue;
                
                // Record the trade path to take if there's gains.
                if ((gain > 2) && (gain < 90)) {
                  multipath path;
                  // The exchange.
                  path.exchange = exchange;
                  // Step 1: Buy the coin on the market.
                  path.market1name = market1name;
                  path.market1quantity = market1quantity;
                  path.ask1 = ask1;
                  path.coin1name = coin1name;
                  path.coin1quantity = coin1quantity;
                  // Step 2: Sell the coin, bought in the previous step, on the market.
                  path.coin2name = coin2name;
                  path.coin2quantity = coin2quantity;
                  path.bid2 = bid2;
                  path.market2name = market2name;
                  path.market2quantity = market2quantity;
                  // Step 3: Buy the coin on the market.
                  path.market3name = market3name;
                  path.market3quantity = market3quantity;
                  path.ask3 = ask3;
                  path.coin3name = coin3name;
                  path.coin3quantity = coin3quantity;
                  // Step 4: Sell the coin, bought in the previous step, on the initial market.
                  path.coin4name = coin4name;
                  path.coin4quantity = coin4quantity;
                  path.bid4 = bid4;
                  path.market4name = market4name;
                  path.market4quantity = market4quantity;
                  // The gain.
                  path.gain = gain;
                  // Check whether this path contains a coin/market/exchange that has trading paused.
                  bool paused = multipath_paused  (&path, paused_trading_exchanges_markets_coins, true);
                  if (!paused) {
                    // Path is not paused for trading.
                    // Store this path in the container.
                    bare_multipaths.push_back (path);
                    // Store feedback information.
                    exchanges_markets_counts [path.exchange + " " + market1name]++;
                  }
                }
              }
            }
          }
        }
      }

      // Feedback about suggested multipath opportunities, by exchange and base market.
      to_output output ("");
      output.add ({"Multipath analyzer will investigate the following bare results:"});
      for (auto & element : exchanges_markets_counts) {
        string exchange_market = element.first;
        int count = element.second;
        output.add ({to_string (count), exchange_market});
      }
    }

    
    // Investigate the bare multipaths.
    {

      // The minimum trade size per exchange/market/coin.
      // Storage format: container [exchange+market+coin] = minimum trade size.
      minimum_trade_sizes = mintrade_get (sql);
      
      // Read the pairs of exchange / market / coin that have their trading paused.
      // Storage format: vector of exchange+market+coin, as one string.
      paused_trading_exchanges_markets_coins = pausetrade_get (sql);
      
      // Investigate all the raw multipaths.
      for (auto & path : bare_multipaths) {
        
        // Wait till there's an available free slot for a new analyzer.
        while (current_simultaneous_analyzer_count >= maximum_simultaneous_analyzers) {
          this_thread::sleep_for (chrono::milliseconds (10));
        }

        // Start the analyzer thread.
        new thread (multipath_analyze_one, ref (path));

        // Wait shortly to give the started thread a chance to update the thread count.
        this_thread::sleep_for (chrono::milliseconds (1));
        
      }
      
      // Wait till all threads have completed.
      while (current_simultaneous_analyzer_count > 0) {
        this_thread::sleep_for (chrono::milliseconds (10));
      }

    }


    // Expire multipaths.
    multipath_expire (sql);
  }

  
  to_stdout ({"Multipath analyzer complete"});
  finalize_program ();
  return EXIT_SUCCESS;
}

