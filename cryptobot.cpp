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
#include "controllers.h"
#include "traders.h"
#include "proxy.h"


// Mutex for updating any of the shared containers.
mutex update_mutex;


// Store the minimum trade size per exchange/market/coin.
// Storage format: container [exchange+market+coin] = minimum trade size.
map <string, float> minimum_trade_sizes;


// Store the pairs of exchange / market / coin that have their trading paused.
// Storage format: vector of exchange+market+coin, as one string.
vector <string> paused_trading_exchanges_markets_coins;


// The non-executed withdrawals used to be read from the database
// during each coin/market trade iteration.
// Since the database of withdrawals keeps growing,
// accessing it will become slower over time.
// So it becomes more important to store this in memory for fast access.
// Store the non-executed withdrawal amounts per exchange/coin.
// Storage format:
// container [exchange + coin_abbrev] = amount.
map <string, float> non_executed_withdrawals;


// Storage for monitoring threads.
// The monitoring jobs used to run as a separate binary parallel to this trading bot.
// But that would lead to nonce errors, as the nonces were not guaranteed to be strictly increasing.
// So now the monitoring jobs are started by this trading bot.
// So they share the mutexes that guarantee strictly increasing nonces.
vector <thread *> monitoring_jobs;


// Storage for any active surfing commands.
vector <int> surfing_ids, surfing_stamps;
vector <string> surfing_coins, surfing_trades, surfing_statuses, surfing_remarks;
vector <float> surfing_rates, surfing_amounts;


void cryptobot_coin_arbitrage_execute (string exchange1, string exchange2, string coin, string market, int days)
{
  bool verbose = false;
  //if (market != bitcoin_id ()) verbose = true;
  
  
  // If the coin is the same as the market, skip it, since there's no possible arbitrage there.
  if (coin == market) return;

  
  // Get the coin abbreviations per exchange.
  map <string, string> coin_abbreviations;
  coin_abbreviations [exchange1] = exchange_get_coin_abbrev (exchange1, coin);
  coin_abbreviations [exchange2] = exchange_get_coin_abbrev (exchange2, coin);

  
  vector <string> exchanges = {exchange1, exchange2};
  
  
  // How many trading iterations done from now on.
  int trading_iteration_counter = 0;

  
  // Database connection.
  SQL sql (front_bot_ip_address ());

  
  // The arbitrage investigation and optional trading will be done from now on.
  // This will be done multiple times within the current running minute.
  do {

    
    // Arbitrage needs at two exchanges.
    // During trading, exchanges can be removed.
    // Therefore it checks on that.
    if (exchanges.size () != 2) return;
    
    
    // New trade iteration.
    trading_iteration_counter++;
    // On the first iteration, don't wait, but on subsequent iterations, do wait.
    // It used to wait at the end of the trading routine.
    // It was moved here for two reasons:
    // * The five seconds's wait would not be included in the bundled output for this trade iteration.
    // * It's faster to end the bot in case it has reached the end of the minute, rather than wait for no reason.
    if (trading_iteration_counter > 1) {
      // Wait a while so the exchanges can update their orders and do their trades.
      // Orders usually get fulfilled straightaway.
      // But at times they take more time,
      this_thread::sleep_for (chrono::seconds (5));
    }
    
    
    // Feedback.
    to_output output ("Arbitrage " + coin + " @ " + market + " @ " + exchange1 + " and " + exchange2);

    
    // Record the current time.
    // The purpose is to detect that getting the order books from the exchanges does not take too long.
    // For arbitrage trading, it's very important that prices are as recent as possible.
    int start_getting_orderbook_time = seconds_since_epoch ();

    
    // Available and realized quantities statistics.
    float bid_size = 0, ask_size = 0, realized_size = 0;

    
    // Get the situation on the markets for this coin.
    // It used to get the situation on the markets in sequence, that is,
    // getting buyers on an exchange, then getting the sellers on that same exchange,
    // then doing the same things on the second exchange, and so on.
    // This took too much time in case of network issues, or in case of overloaded exchanges.
    // So now it gets the market situation in parallel threads, to speed things up.
    // Long delays means that the market situation changes,
    // so this explains the many open orders on the exchanges,
    // whereas one would expect that favourable orders would be fulfilled right away.

    string bidding_exchange;
    float maximum_bid = 0;
    float bid_quantity = 0;
    string asking_exchange;
    float minimum_ask = 0;
    float ask_quantity = 0;
    vector <float> buyer_quantities;
    vector <float> buyer_rates;
    vector <float> seller_quantities;
    vector <float> seller_rates;
    
    {
      map <string, vector <float> > all_buyer_quantities;
      map <string, vector <float> > all_buyer_rates;
      map <string, vector <float> > all_seller_quantities;
      map <string, vector <float> > all_seller_rates;
      get_market_summaries_hurry ({exchange1, exchange2}, market, coin, all_buyer_quantities, all_buyer_rates, all_seller_quantities, all_seller_rates, &output);
      
      {
        // Get the buyers at the first exchange.
        vector <float> quantities = all_buyer_quantities[exchange1];
        vector <float> rates = all_buyer_rates[exchange1];
        // Filter dust trades out.
        fix_dust_trades (market, quantities, rates);
        // Filter too low trades out.
        fix_too_low_trades (minimum_trade_sizes, exchange1, market, coin, quantities, rates);
        if (!rates.empty ()) {
          maximum_bid = rates [0];
          bidding_exchange = exchange1;
          bid_quantity = quantities [0];
          buyer_quantities = all_buyer_quantities[exchange1];
          buyer_rates = all_buyer_rates[exchange1];
        }
      }
      
      {
        // Get the sellers at the second exchange.
        vector <float> quantities = all_seller_quantities[exchange2];
        vector <float> rates = all_seller_rates[exchange2];
        // Filter dust trades out.
        fix_dust_trades (market, quantities, rates);
        // Filter too low trades out.
        fix_too_low_trades (minimum_trade_sizes, exchange2, market, coin, quantities, rates);
        if (!rates.empty ()) {
          minimum_ask = rates [0];
          asking_exchange = exchange2;
          ask_quantity = quantities [0];
          seller_quantities = all_seller_quantities[exchange2];
          seller_rates = all_seller_rates[exchange2];
        }
      }
      
      // Check if the highest bid on the coin is more than the lowest ask price for the same coin.
      // If this is not the case, then swap the exchanges, and try again.
      bool good = (maximum_bid > minimum_ask);
      if (!good) {
        // Get the buyers at the second exchange.
        vector <float> quantities = all_buyer_quantities[exchange2];
        vector <float> rates = all_buyer_rates[exchange2];
        // Filter dust trades out.
        fix_dust_trades (market, quantities, rates);
        // Filter too low trades out.
        fix_too_low_trades (minimum_trade_sizes, exchange2, market, coin, quantities, rates);
        if (!rates.empty ()) {
          maximum_bid = rates [0];
          bidding_exchange = exchange2;
          bid_quantity = quantities [0];
          buyer_quantities = all_buyer_quantities[exchange2];
          buyer_rates = all_buyer_rates[exchange2];
        }
      }
      if (!good) {
        // Get the sellers at the first exchange.
        vector <float> quantities = all_seller_quantities[exchange1];
        vector <float> rates = all_seller_rates[exchange1];
        // Filter dust trades out.
        fix_dust_trades (market, quantities, rates);
        // Filter too low trades out.
        fix_too_low_trades (minimum_trade_sizes, exchange1, market, coin, quantities, rates);
        if (!rates.empty ()) {
          minimum_ask = rates [0];
          asking_exchange = exchange1;
          ask_quantity = quantities [0];
          seller_quantities = all_seller_quantities[exchange1];
          seller_rates = all_seller_rates[exchange1];
        }
      }
      
    }
    
    
    // Feedback.
    output.add ({bidding_exchange, "bids", float2string (maximum_bid), "for", float2string (bid_quantity), coin, "@", market});
    output.add ({asking_exchange, "asks", float2string (minimum_ask), "for", float2string (ask_quantity), coin, "@", market});

    
    // Flags for whether balances are too low to trade.
    // Reset them each iteration,
    // because there can be new exchanges providing the market balance or the coin balances.
    // And the balances are different on each exchange.
    bool market_balance_too_low = false;
    bool coin_balance_too_low = false;
    

    // Check if there's a favourable deal that can be made.
    bool favourable_deal = false;
    if (maximum_bid > minimum_ask) {
      if (bidding_exchange != asking_exchange) {
        float percentage = (100 * maximum_bid / minimum_ask) - 100;
        output.add ({"Difference:", to_string (percentage), "%"});
        if (percentage >= arbitrage_difference_percentage (bidding_exchange, asking_exchange)) {
          favourable_deal = true;
          // Store available quantities for trade, for the record.
          bid_size = bid_quantity;
          ask_size = ask_quantity;
          // Check that the order book was obtained fast enough.
          // There's a timeout on the REST API calls of so many seconds.
          // But order books obtained through a WebSocket connection don't have those same short timeouts.
          // So it needs to be checked here if the orderbook was obtained in a timely fashion.
          bool order_book_in_time = (seconds_since_epoch () < (start_getting_orderbook_time + 7));
          if (!order_book_in_time) output.add ({"The order books were not obtained in time"});
          if (order_book_in_time) {
            
            // Lock balances starting from here.
            // The goal is that the arbitrage processor does the right thing,
            // in the face of parallel trading of the same coin at different markets.
            // Trading multiple coins at the same market simultaneously
            // at times leads to insufficient funds errors at the exchanges.
            // Locking this area prevents that.
            update_mutex.lock ();
            
            // Get the available balances on both exchanges, from memory.
            float asking_exchange_balance_before_trading = 0;
            exchange_get_balance_cached (asking_exchange, market, NULL, &asking_exchange_balance_before_trading, NULL, NULL);
            float bidding_exchange_balance_before_trading = 0;
            exchange_get_balance_cached (bidding_exchange, coin, NULL, &bidding_exchange_balance_before_trading, NULL, NULL);
            
            // Subtract the amounts of the non-executed orders for the two involved coins on both exchanges.
            // The reasons are as follows:
            // * If the exchange behaves correctly, it would do this correction when providing the available balances.
            // * Not all exchanges do this in a correct way in all circumstances.
            // * Manually correcting for non-executed withdrawals order can work around this bug.
            string market_coin_abbreviation = exchange_get_coin_abbrev (asking_exchange, market);
            float market_correction = non_executed_withdrawals [asking_exchange + market_coin_abbreviation];
            if (market_correction > 0) {
              float before = asking_exchange_balance_before_trading;
              asking_exchange_balance_before_trading -= market_correction;
              float after = asking_exchange_balance_before_trading;
              output.add ({"Because of a pending withdrawal order there is a correction on available", market, "for trading:", "before", to_string (before), "after", to_string (after)});
            }
            float coin_correction = non_executed_withdrawals [bidding_exchange + coin_abbreviations [bidding_exchange]];
            if (coin_correction > 0) {
              float before = bidding_exchange_balance_before_trading;
              bidding_exchange_balance_before_trading -= coin_correction;
              float after = bidding_exchange_balance_before_trading;
              output.add ({"Because of a pending withdrawal order there is a correction on available", coin, "for trading:", "before", to_string (before), "after", to_string (after)});
            }

            
            // The quantity to trade.
            float quantity = 0;
            
            // Run the core trade processor.
            arbitrage_processor (&output, market, coin, quantity,
                                 ask_quantity, bid_quantity,
                                 minimum_ask, maximum_bid,
                                 asking_exchange_balance_before_trading,
                                 bidding_exchange_balance_before_trading,
                                 asking_exchange, bidding_exchange,
                                 market_balance_too_low, coin_balance_too_low,
                                 minimum_trade_sizes,
                                 buyer_quantities, buyer_rates,
                                 seller_quantities, seller_rates);
            
            float base_market_coins_gain = 0;
            
            // If it's not the correct weekday, skip it.
            // The purpose of this is as follows:
            // It has been observed that when doinr arbitrage for a couple of weeks,
            // somehow other traders adjust their parameters.
            // This results in our bot no longer making a gain.
            // The idea now is to introduce irregular trading days.
            // So the other bots will get a bit confused and can't adjust their parameters properly.
            // The expected result will be that our bot makes gain again, also in the long term.
            bool days_good = (days == day_within_week ()); // Todo
            days_good = true; // Todo

            // If the quantity is larger than zero, it means that the arbitrage processor sees a possible trade.
            if (quantity > 0) {
              
              if (days_good) {
                
                
                // Calculate the base market's coins to spend on the asking exchange where to buy the coin.
                float base_market_coins_spent = quantity * minimum_ask;
                float fee = base_market_coins_spent * exchange_get_trade_fee (asking_exchange);
                base_market_coins_spent += fee;
                
                // Calculate the proceeds in base market coins on the bidding exchange where to sell the coin.
                float base_market_coins_earned = quantity * maximum_bid;
                fee = base_market_coins_earned * exchange_get_trade_fee (bidding_exchange);
                base_market_coins_earned -= fee;
                
                // The realistic Bitcoins, or base market coins, gained.
                base_market_coins_gain = base_market_coins_earned - base_market_coins_spent;
                output.add ({"Will buy", float2string (quantity), coin, "@", market, "on", asking_exchange, "at rate", float2string (minimum_ask), "and sell it on", bidding_exchange, "at rate", float2string (maximum_bid), "gain", to_string (base_market_coins_gain), market});
                
                // Output balances for feedback and possible error monitoring.
                output.add ({"Balances before trade:"});
                show_balances (&output, {asking_exchange, bidding_exchange}, {market, coin});
                
                // Update the balances in memory with the payment made to buy the coins.
                // The coins themselves won't be considered to have been bought,
                // because the buy limit order may not be filled right away.
                // Even if the buy order failed, still update the balances.
                // Because there have been cases that the API said that order placement failed, yet it placed the order.
                {
                  float base_market_coins = quantity * minimum_ask;
                  float current_total_balance = 0, current_available_balance = 0;
                  exchange_get_balance_cached (asking_exchange, market, &current_total_balance, &current_available_balance, NULL, NULL);
                  exchange_set_balance_cache (asking_exchange, market, current_total_balance - base_market_coins, current_available_balance - base_market_coins, 0, 0);
                }
                
                // Update the balances with the sold coins.
                // It won't update the balances with the income of this sale yet
                // because the sale might not have gone through right away.
                // Even if the sell order failed, still update the balances.
                // Because there have been cases that the API said that order placement failed, yet it placed the order.
                {
                  float current_total_balance = 0, current_available_balance = 0;
                  exchange_get_balance_cached (bidding_exchange, coin, &current_total_balance, &current_available_balance, NULL, NULL);
                  exchange_set_balance_cache (bidding_exchange, coin, current_total_balance - quantity, current_available_balance - quantity, 0, 0);
                }
                
                // Output balances for feedback and possible error monitoring.
                output.add ({"Balances after trade:"});
                show_balances (&output, {asking_exchange, bidding_exchange}, {market, coin});
              }
            }
            
            // Unlock the balances here again, making them ready for other parallel traders.
            update_mutex.unlock ();
            
            // If the quantity is larger than zero, it means that the arbitrage processor sees a possible trade.
            if (quantity > 0) {
              
              if (days_good) {
                
                // The arbitrage processor already has checked that there's enough coin balance to proceed.
                // So place the limit trade orders right away, and lose no time.
                // If time is lost, the order book may change, and the arbitrage won't work so well, or fails more or less.
                vector <thread *> trade_jobs;
                
                // The bot just places the orders on the exchanges.
                // The bids and asks may have changed slightly during the past few seconds.
                // The orders may or may not get fulfilled immediately at the requested rate.
                // The bot just leaves the orders to get fulfilled in their own time.
                // The buy and sale trade orders are placed on the exchanges simulteneously rather than in sequence.
                // If doing it simultaneously, it has a better change of getting fulfilled,
                // as opposed to doing it in sequence, which will postpone the second order till the first is ready,
                // so prices may have changed in the mean time.
                string buy_error, buy_json, buy_order_id;
                thread * buy_job = new thread (exchange_limit_buy, asking_exchange, market, coin, quantity, minimum_ask, ref (buy_error), ref (buy_json), ref (buy_order_id), &output);
                trade_jobs.push_back (buy_job);
                string sell_error, sell_json, sell_order_id;
                thread * sell_job = new thread (exchange_limit_sell, bidding_exchange, market, coin, quantity, maximum_bid, ref (sell_error), ref (sell_json), ref (sell_order_id), &output);
                trade_jobs.push_back (sell_job);
                
                // Store the trade for profitability reporting.
                trades_store (sql, market, coin, asking_exchange, bidding_exchange, quantity, base_market_coins_gain);
                
                // Conditionally update the database with prices the coin was bought for.
                redistribute_coin_bought_price (asking_exchange, bidding_exchange, market, coin, minimum_ask);
                
                // Wait till both trade threads have completed.
                for (auto trade_job : trade_jobs) {
                  trade_job->join ();
                }
                
                // Follow-up on the two limit trades, give feedback, and optionally adjust some process parameters.
                thread * buy_follow_up_job = new thread (limit_trade_follow_up, &output, asking_exchange, market, coin, buy_error, buy_json, buy_order_id, quantity, minimum_ask, "buy", ref (exchanges));
                thread * sell_follow_up_job = new thread (limit_trade_follow_up, &output, bidding_exchange, market, coin, sell_error, sell_json, sell_order_id, quantity, maximum_bid, "sell", ref (exchanges));
                buy_follow_up_job->join ();
                sell_follow_up_job->join ();
              }
              
              // Set the realized quantity for the record.
              realized_size = quantity;
            }
            
            // If the base market coin balance is too low,
            // it means that the coin could not be bought,
            // so remove the asking exchange (where the coin was supposed to be bought).
            // Once this exchange has been removed,
            // the next iteration continues with the remaining exchanges,
            // perhaps a good arbitrage deal is possible with those,
            // likely with a lower percentage difference, but still good enough to be profitable.
            if (market_balance_too_low) {
              // Remove the exchange for the rest of this minute for this coin/market.
              array_remove (asking_exchange, exchanges);
              output.add ({"Removing exchange due to too low a balance:", coin, "@", market, "@", asking_exchange});
            }
            // If the trading coin balance is too low, it means that the coin could not be sold,
            // so remove the bidding exchange (where the coin was supposed to be sold).
            // This is no longer done on multipath trading for the same reasons as above.
            if (coin_balance_too_low) {
              // Remove the exchange for the rest of this minute for this coin/market.
              array_remove (bidding_exchange, exchanges);
              output.add ({"Removing exchange due to too low a balance:", coin, "@", market, "@", bidding_exchange});
            }
          }
        } else {
          // One liner with reason why there was no arbitrage this time.
          if (verbose) to_stdout ({"No arbitrage", coin, "@", market, "@", implode (exchanges, " and "), "due to only", float2visual (percentage), "%"});
        }
      }
    }

    
    // Record the available qantities and the realized quantity.
    availables_store (sql, "", seconds_since_epoch (), asking_exchange, bidding_exchange, market, coin, bid_size, ask_size, realized_size);

    
    // If no deal was possible, erase the bundled output.
    // So there's going to be less unnecessary output.
    if (!verbose) if (!favourable_deal) output.clear ();

    
    // After the trade orders were placed on the exchanges,
    // it used to obtain the new balances from all exchanges
    // But this is no longer done, and is no longer needed.
    // The trade functions now update the copy of the balances kept in memory.
    // This saves API calls, plus it is more robust and accurate
    // in view of API call failures that occur now and then.

    
    // Keep running if there's still enough time left within the current minute.
  } while (running_within_minute ());
}


void cryptobot_multipath_execute (multipath path)
{
  // Bundled feedback for in the logbook.
  to_output output ("Executing multipath " + path.exchange + " " + path.market1name + " > " + path.coin2name + " > " + path.market3name + " > " + path.coin4name + " > " + path.market4name);

  // Initially email the output for development purposes.
  output.to_stderr (true);
  
  bool keep_going = true;
  
  int timer = 0;

  SQL sql (front_bot_ip_address ());

  // Set the path as being executed now.
  // The reason for this is to prevent the following situation:
  // It has happened that in case the exchange timed out,
  // that the next instance of the bot was already starting on the same path,
  // while the previous instance was still waiting for the timeout to clear.
  // This led to confusing results, at times some steps were executed more than once.
  // When the path is set as being executed, the next instance sees this,
  // and won't execute this right away.
  // It will executed this path as soon as this 'executing' flag is cleared.
  path.executing = true;
  multipath_update (sql, path);
  
  // Process multipaths in multiple iterations.
  do {

    output.add ({"Status", path.status});

    if (path.status == multipath_status::bare)
    {
      // Write information about this multipath to the logbook.
      multipath_output (&output, &path);
      // Investigate the feasibility of this trade path.
      bool investigated = multipath_investigate (&output, minimum_trade_sizes, &path);
      if (investigated) {
        // Write updated information about this multipath to the logbook.
        multipath_output (&output, &path);
      } else {
        // Error while investigating the multipath.
        path.status = multipath_status::unrecoverable;
      }
    }

    else if (path.status == multipath_status::profitable)
    {
      // The path is profitable.
      output.add ({"This path is profitable as its calculated gain is", float2visual (path.gain), "%"});
      // Assign the next status to the path.
      path.status = multipath_status::start;
    }

    // The status "start" is normally set after the multipath is profitable.
    // It can also be set manually in the database for enforcing a trade,
    // even if it were not profitable,
    // for debugging purposes.
    else if (path.status == multipath_status::start)
    {
      path.status = multipath_status::buy1place;
    }

    else if (path.status == multipath_status::buy1place)
    {
      int step = 1;
      multipath_place_limit_order (&output, &path, step, update_mutex, minimum_trade_sizes, paused_trading_exchanges_markets_coins);
      // No need to update the status here.
      // The function above will set the status depending on how well it gets on.
    }

    else if (path.status == multipath_status::buy1uncertain)
    {
      int step = 1;
      multipath_verify_limit_order (&output, &path, step);
      path.status = multipath_status::buy1placed;
    }
    
    else if (path.status == multipath_status::buy1placed)
    {
      // Reset the timer for the balance checker.
      timer = 0;
      // Go to the next status.
      path.status = multipath_status::balance1good;
    }
    
    else if (path.status == multipath_status::balance1good)
    {
      int step = 1;
      multipath_verify_balance (&output, &path, step, update_mutex, timer);
      // The balance verifier will have decided on the new status of this multipath.
    }

    else if (path.status == multipath_status::sell2place)
    {
      int step = 2;
      multipath_place_limit_order (&output, &path, step, update_mutex, minimum_trade_sizes, paused_trading_exchanges_markets_coins);
      // No need to update the status here.
      // The function above will set the status depending on how well it gets on.
    }

    else if (path.status == multipath_status::sell2uncertain)
    {
      int step = 2;
      multipath_verify_limit_order (&output, &path, step);
      path.status = multipath_status::sell2placed;
    }
    
    else if (path.status == multipath_status::sell2placed)
    {
      // Reset the timer for the balance checker.
      timer = 0;
      // Go to the next status.
      path.status = multipath_status::balance2good;
    }
    
    else if (path.status == multipath_status::balance2good)
    {
      int step = 2;
      multipath_verify_balance (&output, &path, step, update_mutex, timer);
      // The balance verifier will have decided on the new status of this multipath.
    }

    else if (path.status == multipath_status::buy3place)
    {
      int step = 3;
      multipath_place_limit_order (&output, &path, step, update_mutex, minimum_trade_sizes, paused_trading_exchanges_markets_coins);
      // No need to update the status here.
      // The function above will set the status depending on how well it gets on.
    }

    else if (path.status == multipath_status::buy3uncertain)
    {
      int step = 3;
      multipath_verify_limit_order (&output, &path, step);
      path.status = multipath_status::buy3placed;
    }
    
    else if (path.status == multipath_status::buy3placed)
    {
      // Reset the timer for the balance checker.
      timer = 0;
      // Go to the next status.
      path.status = multipath_status::balance3good;
    }
    
    else if (path.status == multipath_status::balance3good)
    {
      int step = 3;
      multipath_verify_balance (&output, &path, step, update_mutex, timer);
      // The balance verifier will have decided on the new status of this multipath.
    }

    else if (path.status == multipath_status::sell4place)
    {
      int step = 4;
      multipath_place_limit_order (&output, &path, step, update_mutex, minimum_trade_sizes, paused_trading_exchanges_markets_coins);
      // No need to update the status here.
      // The function above will set the status depending on how well it gets on.
    }

    else if (path.status == multipath_status::sell4uncertain)
    {
      int step = 4;
      multipath_verify_limit_order (&output, &path, step);
      path.status = multipath_status::sell4placed;
    }
    
    else if (path.status == multipath_status::sell4placed)
    {
      // Reset the timer for the balance checker.
      timer = 0;
      // Go to the next status.
      path.status = multipath_status::balance4good;
    }
    
    else if (path.status == multipath_status::balance4good)
    {
      int step = 4;
      multipath_verify_balance (&output, &path, step, update_mutex, timer);
      // The balance verifier will have decided on the new status of this multipath.
    }

    else if (path.status == multipath_status::done)
    {
      // Ready processing this path.
      keep_going = false;
    }

    else if (path.status == multipath_status::error)
    {
      // Ready processing this path.
      keep_going = false;
    }

    else if (path.status == multipath_status::unprofitable)
    {
      // No need to email this.
      output.to_stderr (false);
      // Ready processing this path.
      keep_going = false;
    }
    
    else if (path.status == multipath_status::unrecoverable)
    {
      // No need to email this.
      output.to_stderr (false);
      // Ready processing this path.
      keep_going = false;
    }
    
    else {
      // Unknown status.
      output.add ({"Unknown multipath status", path.status});
      output.to_stderr (true);
      keep_going = false;
    }
    
    // Keep running if there's still enough time left within the current minute.
  } while (running_within_minute () && keep_going);

  // Clear the 'executing' flag.
  // So the next bot instance is free to execute it, if needed, after this is stored in the database.
  path.executing = false;
  // Store the updated multipath in the database.
  // If a multipath has been "done", it remains in the database.
  // This is for statistical or development purposes.
  multipath_update (sql, path);
}


void main_multipath_trader ()
{
  SQL sql (front_bot_ip_address ());
  
  // Run multiple times within the current running minute.
  do {
    
    // Multipath execution jobs in parallel threads.
    vector <thread *> multipath_jobs;

    // Get the multipath opportunities that the back bot has sent to the front bot.
    // It gets them in the order they were stored in the database.
    // This is so as to be sure older paths get executed till completion first, before considering new paths.
    vector <multipath> multipaths = multipath_get (sql);

    // Counter for limiting the number of multipaths being executed simultaneously.
    int multipath_counter = 0;

    // Multipath clash detection container.
    vector <string> multipath_clashes;

    // Iterate over the multipaths.
    for (auto & path : multipaths) {

      // Limit the number of multipaths being executed simultaneously.
      if (multipath_counter > 5) continue;

      // If the status of the path is done or higher, skip it.
      // This includes a status of being unprofitable or in error.
      // If a path was considered to be unprofitable, it will expire after a while.
      if (path.status == multipath_status::done) continue;
      if (path.status == multipath_status::error) continue;
      if (path.status == multipath_status::unprofitable) continue;
      if (path.status == multipath_status::unrecoverable) continue;

      // Check clash of this path with others being executed right now.
      // If one multipath affects one or more order books in other paths, then there's a clash.
      // Because if one open order is fulfilled by one multipath,
      // then this affect the profitability of the other multipaths that trade the same,
      // since fulfilling an open order affects the order book at the exchange.
      bool clash = multipath_clash (&path, multipath_clashes);
      if (clash) continue;

      // Check if the path is being executed right now.
      // This may occur if the previous instance of the bot is delayed,
      // and is still working on this path.
      if (path.executing) continue;
      
      // Check whether any of the market/coins in this multipath is paused.
      bool paused = multipath_paused  (&path, paused_trading_exchanges_markets_coins, true);
      if (paused) continue;

      // Execute the multipath in a thread.
      thread * job = new thread (cryptobot_multipath_execute, path);
      multipath_jobs.push_back (job);

      // Increase the counter for the number of multipaths being executed simultaneously.
      multipath_counter++;
    }
    
    // Wait till the multipath jobs have completed.
    for (auto job : multipath_jobs) {
      job->join ();
    }

    // Wait shortly before running the next iteration.
    this_thread::sleep_for (chrono::seconds (1));
    
    // Keep running if there's still enough time left within the current minute.
  } while (running_within_minute ());

}


void individual_pattern_based_trader (const string & exchange, const string & market, const string & coin,
                                      const string & reason)
{
  (void) exchange;
  (void) market;
  (void) coin;
  (void) reason;
  /*
  // Check whether the coin is paused for trading.
  bool paused = in_array (exchange + market + coin, paused_trading_exchanges_markets_coins);
  if (paused) return;
  
  // Prepare the rates for use.
  unordered_map <int, float> minute_asks, minute_bids, hour_asks, hour_bids;
  pattern_prepare (nullptr, exchange, market, coin,
                   minute_asks, minute_bids, hour_asks, hour_bids);
  
  // Deal with one type of pattern.
  if (reason == pattern_type_increase_day_week ()) {

    // If the rates have not yet been stored, the rate for the current minute can be zero.
    // If that's the case, bail out right here.
    // The rate for the current minute will be available shortly.
    if (hour_bids [0] == 0) return;
    if (minute_bids [0] == 0) return;

    // The following checks on the required pattern.
    bool coin_good = pattern_increase_day_week_detector (nullptr, 0, hour_asks, hour_bids);
    if (coin_good) {
      float buy_price = minute_asks [0];
      // pattern_place_buy_order (exchange, market, coin, buy_price, current_available_balances, update_mutex, minimum_trade_sizes, reason, paused_trading_exchanges_markets_coins);
    }
  }
   */
}


void main_pattern_based_trader ()
{
  SQL sql (front_bot_ip_address ());

  // Coins that are being automatically redistributed at an exchange,
  // do not let the pattern trader sell those.
  // Rather those should be sold manually by the operator.
  // One time a certain amount of digibyte was sold, and a certain amount of zclassic.
  // This disturbed the whole arbitrage process.
  // So get those exchanges and coins here.
  vector <string> redistributing_exchanges_plus_coins;
  {
    vector <int> ids;
    vector <string> exchanges, coins, addresses, paymentids, withdrawals, minwithdraws, greenlanes, comments;
    vector <bool> tradings, balancings;
    wallets_get (sql, ids, exchanges, coins, addresses, paymentids, withdrawals, tradings, balancings, minwithdraws, greenlanes, comments);
    for (unsigned int i = 0; i < ids.size(); i++) {
      if (balancings[i]) {
        string exchange_plus_coin = exchanges[i] + coins[i];
        redistributing_exchanges_plus_coins.push_back (exchange_plus_coin);
      }
    }
  }

  // Get the coins that are good candidates for pattern-based trading.
  //vector <string> good_pattern_exchanges, good_pattern_markets, good_pattern_coins, good_pattern_reasons;
  //patterns_get (sql, good_pattern_exchanges, good_pattern_markets, good_pattern_coins, good_pattern_reasons, true);
  
  // Run multiple times within the current running minute.
  do {

    // Trading jobs in parallel threads.
    vector <thread *> jobs;

    // Start separate threads for all good coins.
    /*
    for (unsigned int i = 0; i < good_pattern_exchanges.size(); i++) {
      string exchange = good_pattern_exchanges [i];
      string market = good_pattern_markets [i];
      string coin = good_pattern_coins [i];
      string reason = good_pattern_reasons [i];
      thread * job = new thread (individual_pattern_based_trader, exchange, market, coin, reason);
      jobs.push_back (job);
    }
     */
    
    // Wait till the jobs have completed.
    for (auto job : jobs) {
      job->join ();
    }
    
    // Run the rates monitor multiple times a minute.
    // Because rates may change quickly.
    pattern_monitor_rates (minimum_trade_sizes, redistributing_exchanges_plus_coins);
  
    // Wait a while before running the next iteration.
    this_thread::sleep_for (chrono::seconds (10));
  
    // Keep running if there's still enough time left within the current minute.
  } while (running_within_minute ());
}


void get_surfing_commands ()
{
  SQL sql (front_bot_ip_address ());
  vector <int> ids, stamps;
  vector <string> coins, trades, statuses, remarks;
  vector <float> rates, amounts;
  surfer_get (sql, ids, stamps, coins, trades, statuses, rates, amounts, remarks);
  for (unsigned int i = 0; i < ids.size(); i++) {
    surfing_ids.push_back (ids[i]);
    surfing_stamps.push_back (stamps[i]);
    surfing_coins.push_back (coins[i]);
    surfing_trades.push_back (trades[i]);
    surfing_statuses.push_back (statuses[i]);
    surfing_rates.push_back (rates[i]);
    surfing_amounts.push_back (amounts[i]);
    surfing_remarks.push_back (remarks[i]);
  }
}


void execute_surfing_commands ()
{
  if (surfing_ids.empty ()) return;
  to_output output ("Doing surfing");
  output.to_stderr (true);
  SQL sql (front_bot_ip_address ());
  for (unsigned int i = 0; i < surfing_ids.size(); i++) {
    
    // Stop executing commands when the time runs out.
    if (!running_within_minute ()) continue;
    
    // Get the details of this trade command.
    string coin = surfing_coins [i];
    string trade = surfing_trades [i];
    string status = surfing_statuses [i];
    int identifier = surfing_ids [i];
    string remark = surfing_remarks [i];

    if (status == surfer_status::entered) {
      // A surfing order has been entered.
      // The bot notices this and does not do any trading of this coin.
      // Take action: Close any open orders related to this coin.
      vector <string> exchanges = exchange_get_names ();
      for (auto exchange : exchanges) {
        vector <string> identifiers;
        vector <string> strings;
        vector <float> floats;
        vector <string> coins;
        string error;
        exchange_get_open_orders (exchange, identifiers, strings, strings, coins, strings, floats, floats, strings, error);
        for (unsigned int i = 0; i < identifiers.size(); i++) {
          if (coin != coins [i]) continue;
          bool cancelled = exchange_cancel_order (exchange, identifiers[i]);
          output.add ({"Cancelling order", identifiers[i], "trading", coin, "success", to_string (cancelled)});
        }
      }
      status = surfer_status::closed;
      surfer_update (sql, identifier, status, remark);
    }

    else if (status == surfer_status::closed) {
      // Variables to quantitize the trade.
      int trade_count = 0;
      float total_traded_amount = 0;
      float average_traded_rate = 0;
      // All orders related to this coin have been closed now.
      // It is one minute later now.
      // The balances of all coins at all exchanges have now been collected.
      // In case of selling the coin, these balances are useful
      // for knowing which exchanges to sell the coin on, and how much to sell.
      // In case of buying the coin, these balances are useful
      // for knowing which exchanges and markets to buy the coin on, and how much to buy.
      if (trade == surfer_sell ()) {
        vector <string> exchanges = exchange_get_names ();
        for (auto exchange : exchanges) {
          // Read the memory cache to get the balance of this coin at this exchange.
          float total, available, reserved, unconfirmed;
          exchange_get_balance_cached (exchange, coin, &total, &available, &reserved, &unconfirmed);
          if (total > 0) {
            output.add ({exchange, "has a total of", float2string (total), coin, "to sell"});
            float quantity = total * exchange_get_sell_balance_factor (exchange);
            vector <float> quantities;
            vector <float> rates;
            string market = bitcoin_id ();
            exchange_get_buyers_via_satellite (exchange, market, coin, quantities, rates, true, nullptr);
            fix_dust_trades (market, quantities, rates);
            fix_rate_for_quantity (quantity, quantities, rates);
            if (order_book_is_good (quantities, rates, false)) {
              float rate = rates [0];
              // Slightly adjust the rates with the goal of getting the order filled immediately.
              rate -= (rate * exchange_get_trade_order_ease_percentage (exchange) / 100);
              if (is_dust_trade (market, quantity, rate)) {
                output.add ({"This would be dust trade"});
              } else {
                string error, json, order_id;
                exchange_limit_sell (exchange, market, coin, quantity, rate, error, json, order_id, &output);
                output.add ({"Order id", order_id});
                if (order_id.empty ()) {
                  output.add ({"The order could not be placed"});
                } else {
                  // Add statistics and info about this trade.
                  trade_count++;
                  total_traded_amount += quantity;
                  average_traded_rate += rate;
                  if (!remark.empty ()) remark.append (" ");
                  remark.append ("A quantity of " + float2string (quantity) + " " + coin + " was sold at rate " + float2string (rate) + " " + market + " at " + exchange + " with JSON " + json);
                }
              }
            } else {
              output.add ({"The order book at the exchange is too small"});
            }
          }
        }
      }
      if (trade == surfer_buy ()) {
        // When buying the coin, there have been cases of crashes in the cryptobot after the buy order was placed.
        // By the time it crashed, the database was not yet updated to indicate the coin was bought.
        // This resulted in repeated buying of the same coin.
        // That puts things completely out of balance, and will result in buying far too many coins.
        // So to make things more robust, remove the surfer command from the database right away.
        surfer_delete (sql, identifier);
        // Start the process to buy the coin.
        pattern_buy_coin_now ({}, coin, update_mutex, minimum_trade_sizes, paused_trading_exchanges_markets_coins);
      }
    }
  }
}


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);

  SQL sql (front_bot_ip_address ());

  
  // Wait shortly till a possible previous instance of the bot has exited.
  bool previous_bot_running;
  for (unsigned int w = 0; w < 4; w++) {
    previous_bot_running = program_running (argv);
    if (previous_bot_running) {
      to_stdout ({"Waiting till the previous instance of the bot has exited..."});
      this_thread::sleep_for (chrono::seconds(5));
    } else {
      break;
    }
  };
  // Possible warning on hanging bot.
  if (previous_bot_running) {
    to_failures (failures_type_api_error (), "", "", 0, {"The previous instance of the bot keeps running longer than usual, does it hang?"});
    // Tests were done on why this delay.
    // The outcome: Arbitrage jobs take longer than normal to cpmplete.
    // This was tested while arbitrage was the only type of trade.
  }

  
  // Get all balances of all coins on all exchanges and cache them.
  // Having accurate balances is essential to many trading and monitoring functions.
  // So betting those is one of the first steps to be taken.
  get_all_balances ();

  
  // Start certain reporting and monitoring threads as background jobs.
  // The purpose is that their authenticated calls share the mutexes that guarantee strictly increasing nonces.
  if ((hours_within_day () == 2) && (minutes_within_hour () == 2)) {
    thread * job = new thread (orders_reporter);
    monitoring_jobs.push_back (job);
  }
  if ((minutes_within_hour () % 15) == 0) {
    thread * job = new thread (monitor_deposits_history);
    monitoring_jobs.push_back (job);
  }
  {
    thread * job = new thread (monitor_deposits_balances);
    monitoring_jobs.push_back (job);
  }
  if ((minutes_within_hour () % 5) == 0)
  {
    {
      thread * job = new thread (monitor_withdrawals);
      monitoring_jobs.push_back (job);
    }
    {
      thread * job = new thread (monitor_poloniex_withdrawals);
      monitoring_jobs.push_back (job);
    }
  }

  
  // Start reading surfing commands in a thread.
  // The thread is so it won't block anything, yet gets read before anything else.
  //thread * surfing_job = new thread (get_surfing_commands);
  
  
  // Twice an hour, do this:
  // * Write all balances to the database.
  // * Equalize the balances on the exchanges.
  // Do this before any trade is done.
  // This results in a more accurate balance record.
  // Previously recording balances and doing trades were done simultaneously,
  // leading to less accurate balance records.
  if (running_within_minute ()) {
    if ((minutes_within_hour () == 0) || (minutes_within_hour () == 30)) {
      record_all_balances ();
      // Store the withdrawals made: 0: exchange | 1: coin_id | 2: withdrawn amount.
      vector <tuple <string, string, float> > withdrawals;
      // Redistribute the coins.
      redistribute_all_coins (withdrawals);
      // If it balanced coins, the balances in memory are no longer accurate.
      // These inaccurate values could lead to a situation that the bot thinks it can trade,
      // whereas due to too low balances, the trade would fail.
      // So update the balances once more if coins were balanced.
      // Iterate over the withdrawn amounts and use that information to update all balances.
      // This saves API calls.
      to_output output ("Corrected balances due to withdrawals");
      for (tuple <string, string, float> withdrawal : withdrawals) {
        // A withdrawal is stored like this: 0: exchange | 1: coin_id | 2: withdrawn amount.
        string exchange = get<0>(withdrawal);
        string coin = get<1>(withdrawal);
        float quantity = get<2>(withdrawal);
        output.add ({"Updating balances in memory based on a withdrawn amount of", float2string (quantity), coin, "from", exchange});
        float total = 0, available = 0, reserved = 0, unconfirmed = 0;
        exchange_get_balance_cached (exchange, coin, &total, &available, &reserved, &unconfirmed);
        available -= quantity;
        reserved += quantity;
        exchange_set_balance_cache (exchange, coin, 0, available, reserved, 0);
      }
    }
  }

  
  // If the previous bot is still running,
  // the balance recording and other essential tasks above still run,
  // but it should not be doing any trades.
  // Because if it were to trade, there could be mixups due to parallel trades.
  /*
  if (previous_bot_running) {
    while (program_running (argv)) {
      // If there's too many parallel bots, the bot will kill all of them.
      to_stdout ({"Waiting till the previous instance of the bot has exited..."});
      this_thread::sleep_for (chrono::seconds(5));
    }
  }
  if (previous_bot_running) return EXIT_SUCCESS;
   */
  // The above used to be the case.
  // But this resulted in a logical error and an infinite loop.
  // As follows:
  // Once one bot took longer than expected,
  // the second bot after that would start.
  // It would keep waiting till the first bot would quit.
  // If that took longer, a third bot would also start, and so on.
  // If now the first bot that was stuck had completed,
  // the subsequent bots were all waiting upon one another.
  // This would lead to more and more bots waiting upon one another,
  // whereas the original bot that delayed, could have quit already.
  // Eventually, if there were ten or so bots waiting one on the other,
  // the system was built to kill all of them at once.
  // So during this period of waiting, there could be ten minutes without any trading being done.
  // This led to missed opportunities to make a gain.
  // In the current configuration, if a bot is delayed,
  // the next bot will wait one minute, then continue.
  
  
  // Get the minimum trade sizes per exchange+market+coin.
  minimum_trade_sizes = mintrade_get (sql);

  
  // Store the pairs of exchange / market / coin that have their trading paused.
  // Storage format: vector of exchange+market+coin, as one string.
  paused_trading_exchanges_markets_coins = pausetrade_get (sql);

  
  // Get the withdrawn amounts that the exchanges have not yet executed.
  transfers_get_non_executed_amounts (sql, non_executed_withdrawals);

  
  // At this stage, be sure that the data has been read from the database,
  // as what follows depends on that data.
  //surfing_job->join ();

  
  // Storage for the arbitrage jobs.
  vector <thread *> arbitrage_jobs;


  // Get the pairs of two arbitraging exchanges and their associated markets and coins.
  vector <string> arbitraging_exchanges1, arbitraging_exchanges2, arbitraging_markets, arbitraging_coins;
  vector <int> arbitraging_days;
  pairs_get (sql, arbitraging_exchanges1, arbitraging_exchanges2, arbitraging_markets, arbitraging_coins, arbitraging_days);

  
  // Iterate over the arbitraging pairs of exchanges and their associated trading markets and trading coins.
  for (unsigned int arb = 0; arb < arbitraging_exchanges1.size (); arb++) {
    string exchange1 = arbitraging_exchanges1[arb];
    string exchange2 = arbitraging_exchanges2[arb];
    string market = arbitraging_markets [arb];
    string coin = arbitraging_coins [arb];
    int days = arbitraging_days [arb]; // Todo
    // Skip empty coins, naturally.
    if (coin.empty ()) continue;
    // If an exchange + market + coin pair is paused, don't do arbitrage there just now.
    bool paused1 = in_array (exchange1 + market + coin, paused_trading_exchanges_markets_coins);
    if (paused1) to_stdout ({"Exchange", exchange1, "paused arbitrage trading", coin, "@", market});
    bool paused2 = in_array (exchange2 + market + coin, paused_trading_exchanges_markets_coins);
    if (paused2) to_stdout ({"Exchange", exchange2, "paused arbitrage trading", coin, "@", market});
    if (paused1 || paused2) continue;
    // Find out if a coin's balance is considered dust trade on the market at both exchanges.
    // If the balances of a coin at both exchanges leads to dust trade,
    // it can't do arbitrage trading, so skip this coin in that case.
    // If it would lead to dust trade on only one exchange, and not on the other exchange, that is fine.
    int dust_trade_count = 0;
    for (auto exchange : {exchange1, exchange2}) {
      float available = 0;
      exchange_get_balance_cached (exchange, coin, NULL, &available, NULL, NULL);
      float rate = rate_get_cached (exchange, market, coin);
      if (is_dust_trade (market, available, rate)) {
        dust_trade_count++;
        // to_stdout ({"Dust trade for", float2string (available), coin, "@", market, "@", exchange, "at rate", float2string (rate)});
      }
    }
    if (dust_trade_count >= 2) {
      to_failures (failures_type_bot_error (), exchange1 + "-" + exchange2, coin, 0, {"Total balance of", coin, "over exchanges", exchange1, "and", exchange2, "is too low for arbitrage trade at market", market});
    }
    // If the coin is now being sold off through the surfer, do not trade it.
    if (in_array (coin, surfing_coins)) continue;
    // Do the arbitrage trading in a new thread.
    this_thread::sleep_for (chrono::milliseconds (1));
    thread * job = new thread (cryptobot_coin_arbitrage_execute, exchange1, exchange2, coin, market, days);
    arbitrage_jobs.push_back (job);
  }

  
  // Multipath trader.
  //thread * multipath_job = new thread (main_multipath_trader);
  // The multipath trader hardly made any gains.
  // So it no longer runs.

  
  // Pattern-based trading jobs.
  //thread * pattern_job = new thread (main_pattern_based_trader);
  // The pattern-based trader made wrong decisions all the time.
  // So it no longer runs now.

  
  // Surfing commands execution.
  //surfing_job = new thread (execute_surfing_commands);

 
  // Wait till the parallel arbitrage jobs have finished.
  for (auto job : arbitrage_jobs) {
    job->join ();
  }

  
  // Wait till the multipath trading job has completed.
  //multipath_job->join ();

  
  // Wait till the pattern-based trading job has completed.
  //pattern_job->join ();

  
  // Wait till the surfing commands have completed.
  //surfing_job->join ();

  
  // Wait till the monitoring jobs have completed, if there are any.
  for (auto job : monitoring_jobs) {
    job->join ();
  }

  
  finalize_program ();
  return EXIT_SUCCESS;
}

