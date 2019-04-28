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


#include "traders.h"
#include "sql.h"
#include "shared.h"
#include "models.h"
#include "exchanges.h"
#include "cryptopia.h"
#include "io.h"
#include "proxy.h"
#include "bitfinex.h"
#include "kraken.h"
#include "bl3p.h"
#include "controllers.h"


mutex limit_trade_follow_up_mutex;


// Follows up on a limit trade.
// Tries to optimize the bot parameters based on what it finds.
void limit_trade_follow_up (void * output, string exchange, string market, string coin, const string & error, const string & json, string order_id, float quantity, float rate, string direction, vector <string> & exchanges)
{
  // Block output.
  to_output * output_block = (to_output *) output;
  
  // Feedback about this order identifier.
  output_block->add ({"Follow-up on", direction, to_string (quantity), coin, "@", market, "at rate", float2string (rate), "on", exchange, "error", error, "JSON", json, "order id", order_id});
  
  // The number of minutes to pause trading this coin at this exchange.
  int pause_trading_minutes = 0;
  
  // If this order was fulfilled, there's no more follow-up to do.
  if (order_id == trade_order_fulfilled ()) {
    output_block->add ({"The order has been fulfilled"});
  }
  
  // Deal with a timed out API call.
  // Timeouts occur fairly often at Cryptopia at the time of writing this, January 2018.
  // So there's no point in sending it to be mailed to the user every so often.
  // It got recorded in the failures database, so that's enough.
  else if (error.find ("Timeout was reached") != string::npos) {
    // Failure message.
    output_block->add ({"Response timeout after placing the order although the order still may have been placed"});
    // Since the order placement timed outy, there's no way to follow-up on this order.
    // Temporarily disable the wallet for some minutes so there's no further trading for a while.
    pause_trading_minutes = 5;
    // Send the whole output block also to the standard error file descriptor.
    output_block->to_stderr (true);
  }
  
  // Deal with a market offline.
  else if (json.find ("MARKET_OFFLINE") != string::npos) {
    // Temporarily disable the trade pair for an hour so there's no further trading for a while.
    // Update: Disabling the trade pair for one hour is too short,
    // because it has been noticed that in multipath trading,
    // the bot kept buying blockmasoncreditprotocol on the bitcoin market,
    // while failing to sell them on the ethereum market.
    // So that blockmasoncreditprotocol kept being piled up and losing value.
    // And this cost money.
    // So now it disables the trade pair for two days.
    output_block->add ({"Pausing trading this coin at this exchange for two days"});
    pause_trading_minutes = 2880;
    // Send the whole output block also to the standard error file descriptor.
    output_block->to_stderr (true);
  }
  
  // Deal with a service unavailable.
  else if ((json.find ("Unavailable") != string::npos) || (json.find ("unavailable") != string::npos)) {
    // Temporarily disable the wallet for an hour so there's no further trading for a while.
    output_block->add ({"Pausing trading this coin at this exchange for an hour"});
    pause_trading_minutes = 60;
    // Send the whole output block also to the standard error file descriptor.
    output_block->to_stderr (true);
  }
  
  // Deal with insufficient funds.
  else if (json.find ("Insufficient") != string::npos) {
    // Temporarily disable the wallet for an hour so there's no further trading for a while.
    output_block->add ({"Pausing trading this coin at this exchange for an hour"});
    pause_trading_minutes = 60;
    // Send the whole output block also to the standard error file descriptor.
    output_block->to_stderr (true);
  }
  
  // Deal with too low a trade amount:
  // API call error exchange_limit_buy kraken bitcoin XMR {"error":["EGeneral:Invalid arguments:volume"]}
  else if (json.find ("Invalid arguments:volume") != string::npos) {
    // Temporarily disable the wallet for an hour so there's no further trading for a while.
    output_block->add ({"Pausing trading this coin at this exchange for an hour"});
    pause_trading_minutes = 60;
    // Send the whole output block also to the standard error file descriptor.
    output_block->to_stderr (true);
  }
  
  // Deal with too low a trade amount:
  // API call error exchange_limit_sell bitfinex bitcoin XMR bitfinex_limit_trade {"message":"Invalid order: minimum size for XMR/BTC is 0.06"}
  else if (json.find ("minimum size for") != string::npos) {
    // Temporarily disable the wallet for an hour so there's no further trading for a while.
    output_block->add ({"Pausing trading this coin at this exchange for an hour"});
    pause_trading_minutes = 60;
    // Send the whole output block also to the standard error file descriptor.
    output_block->to_stderr (true);
  }
  
  // Deal with non-existing trade pair.
  // cryptopia error  JSON {"Success":false,"Error":"TradePair does not exist or is disabled","Data":null} order id 123456789
  else if (json.find ("does not exist") != string::npos) {
    // Temporarily disable the wallet for an hour so there's no further trading for a while.
    output_block->add ({"Pausing trading this coin at this exchange for a day"});
    pause_trading_minutes = 1440;
    // Send the whole output block also to the standard error file descriptor.
    output_block->to_stderr (true);
  }
  
  // Deal with disabled trade pair.
  // cryptopia error  JSON {"Success":false,"Error":"TradePair does not exist or is disabled","Data":null} order id 123456789
  else if (json.find ("is disabled") != string::npos) {
    // Temporarily disable the wallet for an hour so there's no further trading for a while.
    output_block->add ({"Pausing trading this coin at this exchange for a day"});
    pause_trading_minutes = 1440;
    // Send the whole output block also to the standard error file descriptor.
    output_block->to_stderr (true);
  }
  
  // Deal with an unknown error at Tradesatoshi.
  // API call error exchange_limit_buy tradesatoshi dogecoin unobtanium  {"success":false,"message":"An unknown error occured","result":null}
  else if (json.find ("An unknown error occured") != string::npos) {
    // Temporarily disable the wallet for an hour so there's no further trading for a while.
    output_block->add ({"Pausing trading this coin at this exchange for an hour"});
    pause_trading_minutes = 60;
    // Send the whole output block also to the standard error file descriptor.
    output_block->to_stderr (true);
  }
  
  // Deal with an empty order ID.
  else if (order_id.empty ()) {
    // Failure message.
    output_block->add ({"Failed to", direction, float2string (quantity), coin, "@", market, "on", exchange, "at rate", float2string (rate)});
    // Since the order identifier is empty, there's no way to follow-up on this order.
    // Temporarily disable the wallet for some minutes so there's no further trading for a while.
    pause_trading_minutes = 5;
    // Send the whole output block also to the standard error file descriptor.
    output_block->to_stderr (true);
  }
  
  // Deal with a normal order ID.
  else if (!order_id.empty ()) {
    
  }
  
  // Unknown situation yet.
  else {
    // Send the whole output block also to the standard error file descriptor.
    output_block->to_stderr (true);
    // Temporarily disable the wallet for some minutes so there's no further trading for a while.
    pause_trading_minutes = 5;
  }
  
  
  // The limit order will be or might have been placed.
  // However the bot is not yet sure at this point whether the order was really placed.
  // And whether the order, if placed, has been fulfilled.
  // If an order was not placed due to an error or what,
  // then the bot will see the same trade opportunity again during this same mninute.
  // And it might place the same order, and do the same failure, and so on, and so run into a loss.
  // To counteract this, an exchange where an order was placed will be removed for this minute,
  // unless the bot is sure that the order has been fulflled, or partially fulfilled.
  // This is all about arbitrage, where the margins are very small, and the amounts traded are relatively high.
  // So the bot should be careful to be sure the expected gain is made.
  // If the bot continues placing orders blindly, it runs into a loss instead of making a gain.
  // And this situation has been observed in real life: The bot made a loss rather than a gain.
  // So the bot should be very sure about what it does, rather than just carry on happily and be sorry at the end.
  // The default action of the bot is to remove the exchange, where the limit order was placed,
  // for the remainder of this trading minute.
  // And that is what it does here:
  // Output current trading exchanges before removing it.
  output_block->add ({"Currently trading exchanges"});
  output_block->add (exchanges);
  // Remove the exchange in a thread-safe manner.
  limit_trade_follow_up_mutex.lock ();
  array_remove (exchange, exchanges);
  limit_trade_follow_up_mutex.unlock ();
  // Feedback after removing the exchange.
  output_block->add ({"Trading exchanges after removing one"});
  output_block->add (exchanges);
  
  
  // Whether to disable trading the coin at the exchange for some minutes.
  if (pause_trading_minutes > 0) {
    temporarily_pause_trading (exchange, market, coin, pause_trading_minutes, error + " " + json);
    output_block->add ({"Trading", coin, "@", market, "@", exchange, "was disabled for", to_string (pause_trading_minutes), "minutes"});
  }
}


// This monitors the rates at the exchanges for the live coins.
// If the rate is high enough, it sells the coin for that higher rate.
// While the rate is good, there's no order placed related to this coin.
// This keeps the coins available for good trading opportunities that may arise.
// So the coin is not stuck in orders for multiple days letting all good opportunities go.
// https://www.investopedia.com/articles/stocks/09/use-stop-loss.asp
// There was a time that the operator did monitor the rates,
// and then decided when to sell a coin.
// But the operator missed opportunities this way.
// Some of the good sell opportunities were at night.
// Some of these opportunities lasted for a short time only.
// To automatc this is the sensible thing to do.
// The bot is better at it than a human could ever be.
void pattern_monitor_rates (map <string, float> minimum_trade_sizes,
                            vector <string> redistributing_exchanges_plus_coins)
{
  SQL sql (front_bot_ip_address ());
  
  string tag = "Pattern:";

  // Get the rates that the coins were bought for.
  // Store them in this format:
  // container [exchange+market+coin] = value;
  unordered_map <string, float> bought_rates;
  {
    vector <int> ids, hours;
    vector <string> exchanges, markets, coins;
    vector <float> rates;
    bought_get (sql, ids, hours, exchanges, markets, coins, rates);
    // The database contains older rates too.
    // The data is fetched in the order it was entered.
    // So newer rates for the same coin will overwrite older rates.
    // This way the bot always has the newest rate a coin was bought for.
    for (unsigned int i = 0; i < ids.size(); i++) {
      string key = exchanges[i] + markets[i] + coins[i];
      bought_rates [key] = rates [i];
    }
  }
  
  // Iterate over all exchanges.
  vector <string> exchanges = exchange_get_names ();
  for (auto exchange : exchanges) {
    
    // Iterate over all coins on this exchange.
    vector <string> coins = exchange_get_coin_ids (exchange);
    for (auto coin : coins) {
      
      // Proceed with this if there is an available balance of this coin.
      float available_balance;
      exchange_get_balance_cached (exchange, coin, nullptr, &available_balance, nullptr, nullptr);
      if (available_balance > 0) {

        // The output object.
        // This object used to be a level higher, in the main monitor.
        // But that led to a lot of output in case of only one possible trade.
        // Moving the output object here will lead to far less output.
        // And that is better, at this stage.
        to_output output ("");

        // Proceed with this if the exchange plus coin is not being automatically redistributed.
        // It should not sell coins automatically being redistributed for arbitrage.
        // If the pattern-based trader would sell those arbitraging coins,
        // then this can disturb the arbitrage process a lot.
        // Rather the operator is expected to manually buy or sell those coins.
        if (in_array (exchange + coin, redistributing_exchanges_plus_coins)) {
          //output.add ({tag, exchange, coin, "is being automatically redistributed: cancel"});
          continue;
        }

        // Work on the Bitcoin market, just now, for a start.
        string market = bitcoin_id ();

        // Use the available balance at this exchange,
        // and use the recorded rate the coin was bought for,
        // to establish whether this would be dust trade.
        // If it is found to be dust trade right here at this point,
        // skip it right here.
        // This saves API calls to the exchanges.
        // It reduces the load on the bot and on the exchange.
        float bought_rate = bought_rates [exchange + market + coin];
        if (bought_rate == 0) {
          // If the rate is unknown, record the rate in the database,
          // for a future sale with gain.
          set_likely_coin_bought_price (exchange, market, coin);
          continue;
        }
        // Skip dust trade.
        if (is_dust_trade (market, available_balance, bought_rate)) {
          continue;
        }

        // Get the buyers at the exchange.
        vector <float> buyer_quantities;
        vector <float> buyer_rates;
        exchange_get_buyers_via_satellite (exchange, market, coin, buyer_quantities, buyer_rates, true, &output);
        
        // Filter dust trades out.
        fix_dust_trades (market, buyer_quantities, buyer_rates);
        
        // Filter too low trades out.
        fix_too_low_trades (minimum_trade_sizes, exchange, market, coin, buyer_quantities, buyer_rates);
        
        // Check that the order books are good.
        // In case of API errors, the books won't be good.
        // And if the order book is small, it won't be good either.
        // Only proceed if the order books are good.
        if (!order_book_is_good (buyer_quantities, buyer_rates, false)) {
          output.add ({tag, "Monitoring rate:", exchange, market, coin, "buyers order book too small"});
          continue;
        }
        
        // Calculate the actual change percentages from the actual rates.
        float buyer_rate = buyer_rates [0];
        float gain_percentage = get_percentage_change (bought_rate, buyer_rate);
        
        // It used to do stop-loss ordering,
        // but this was stopped because it led to loss (no pun intended :).
        // The reason was that a new strategy was adopted, which is this:
        // Never sell (automatically) at a loss.
        // It is expected that this leads to better gains, albeit at a slower pace.
        
        if (gain_percentage > 0) {
          output.add ({tag, "The", float2string (available_balance), coin, "@", market, "@", exchange, "was bought for", float2string (bought_rate), "can be sold with gain", float2visual (gain_percentage), "%"});
        }
        
        bool sell_at_market_rate = false;
        
        // Check if the coin can be sold for a few percents more it was bought for.
        if (gain_percentage >= 2) {
          // Sell the coin now at the market rate.
          sell_at_market_rate = true;
        }
        
        if (sell_at_market_rate) {
          
          // Send email initially, for monitoring this.
          output.to_stderr (true);
          
          // The bot is now going to check the size of the best buyer entry in the orderbook.
          // Then it checks the balance available at this exchange, that it could sell.
          // It then determines the actual balance of the coins to be able to sell at the best rate.
          float sell_quantity = available_balance;
          float buyer_quantity = buyer_quantities [0];
          if (buyer_quantity < sell_quantity) {
            sell_quantity = buyer_quantity;
            output.add ({tag, "Due to a small buyer order, the quantity to sell was reduced to", float2string (sell_quantity)});
          }
          
          // As for the quantity to sell, apply the sell/balance factor per exchange,
          // to be sure there is not the "Insufficient funds" error at the exchange.
          sell_quantity *= exchange_get_sell_balance_factor (exchange);
          
          // Decrease the price slightly.
          // The reason for this is as follows:
          // When selling coins for the bid price, unfulfilled buy orders slowly accumulate on the various exchanges.
          // This is what has been observed in real life.
          // Decreasing the price to sell the coins for results in fewer open sell orders.
          float rate2 = buyer_rate;
          rate2 -= (rate2 * exchange_get_trade_order_ease_percentage (exchange) / 100);
          output.add ({tag, "Reduce the price the bot asks for the coin to", float2string (rate2)});
          
          output.add ({tag, "The", float2string (sell_quantity), coin, "@", market, "@", exchange, "was bought for", float2string (bought_rate), "selling it for", float2string (rate2)});
          
          // Check on dust trade at this stage.
          if (is_dust_trade (market, sell_quantity, rate2)) {
            output.add ({tag, "This would be dust trade: Cancel"});
            sell_at_market_rate = false;
          }
          
          if (sell_at_market_rate) {
            
            // Place the sell order at the exchange.
            string order_id, error, json;
            exchange_limit_sell (exchange, market, coin, sell_quantity, rate2, error, json, order_id, &output);
            output.add ({tag, "Sell order placed with ID", order_id});
            
            // Clearer error recording.
            if (order_id.empty ()) order_id = json;
            
            // Record this sale in the database.
            // For statistical purposes.
            sold_store (sql, exchange, market, coin, sell_quantity, bought_rate, rate2);

            // Feedback about the remaining unsold quantity.
            if (sell_quantity < available_balance) {
              output.add ({tag, "Remaining quantity is", float2string (available_balance - sell_quantity), coin});
            }

            // Subtract the amount sold from the balances memory cache.
            // So it won't try to use more balance than it actually has
            // in a subsequent trade during this same minute.
            {
              float total, available, reserved, unconfirmed;
              exchange_get_balance_cached (exchange, coin, &total, &available, &reserved, &unconfirmed);
              total -= sell_quantity;
              available -= sell_quantity;
              exchange_set_balance_cache (exchange, coin, total, available, reserved, unconfirmed);
            }
          }
        }
      }
    }
  }
}


// This function will place appropriate buy orders for the coin.
// exchanges: Which exchanges to buy on. If empty: Consider all supported exchanges.
void pattern_buy_coin_now (vector <string> exchanges,
                           string coin,
                           mutex & update_mutex,
                           map <string, float> & minimum_trade_sizes,
                           vector <string> & paused_trades)
{
  // It used to do manual buy and manual sell of the surfing commands.
  // Manual buy worked fine.
  // But manual sell had a problem, which is this:
  // The operator does not and cannot monitor the rates 24/7.
  // So when there was a good sell opportunity, the operator would have missed this.
  // Since the rates fluctuate so wildly, a good sell opportunity can easily be missed by the operator.
  // The bot looks at the rates 24/7.
  // Whenever it notices a good sell opportunity, it will straightaway sell the coin.
  // This uses the principle of automate what can be automated,
  // and do manually what only humans can do.
  // So to make this work, the manually entered buy command is now executed by the pattern-based trader.
  // Thus this becomes a pattern, and the bot will now look for a good sell opportunity, every minute.

  SQL sql_front (front_bot_ip_address ());
  SQL sql_back (back_bot_ip_address ());

  to_output output ("");
  
  // Send the output to the operator, just now.
  output.to_stderr (true);
  
  // Which exchanges to buy the coin on.
  if (exchanges.empty ()) exchanges = exchange_get_names ();
  
  // Check that the balances of this coin at all the exchanges have been sold,
  // before buying a new lot of those coins.
  // One might think that it is enough to check per exchange, rather than over all exchanges,
  // so that if a coin has been sold on one exchange, new coins can be bought there,
  // despite the coin being unsold on other exchanges.
  // But this reasoning is not good, due to other trade going on simultaneously.
  // The arbitrage trade can create a coin balance at an exchange,
  // bought for a certain price.
  // So if this new price is lower than the initial lot of coins was bought for,
  // it will also be sold for a lower price, and so a loss is incurred.
  // Therefore all balances of this coin on all exchanges should have been sold.
  bool unsold_balance = false;
  for (auto exchange : exchanges) {
    float balance = balances_get_average_hourly_balance (sql_back, exchange, coin, 1, 0);
    if (balance > 0) {
      float rate = rate_get_cached (exchange, bitcoin_id (), coin);
      if (!is_dust_trade (bitcoin_id (), balance, rate)) {
        unsold_balance = true;
        output.add ({exchange, "still has an unsold balance of", float2string (balance), coin});
      }
    }
  }
  if (unsold_balance) {
    output.add ({"Only if the balance of", coin, "has been sold, can the trader buy more of it: Cancel"});
    return;
  }
  
  // Iterate over the exchanges.
  for (auto exchange : exchanges) {
    
    // Check that the coin is available at this market.
    string market = bitcoin_id ();
    vector <string> coins = exchange_get_coins_per_market_cached (exchange, market);
    if (in_array (coin, coins)) {

      // Flag for whether all checks are still good to go.
      bool checks_good = true;
      
      // Read the memory cache to get the balance of the market at this exchange.
      float total, available, reserved, unconfirmed;
      exchange_get_balance_cached (exchange, market, &total, &available, &reserved, &unconfirmed);
      output.add ({exchange, "has", float2string (available), market, "available to buy", coin, "with"});

      // The bot will use no more than 0.01 Bitcoin to buy coins with.
      // So check that this balance is available.
      if (available < 0.01) {
        output.add ({"The balance of", float2string (available), market, "is too low: Cancel"});
        checks_good = false;
      }

      // Get the buyers order book at the exchange.
      // This order book is not really needed right now.
      // But its purpose is to establish whether there's only a small gap between buyer and seller rates.
      // And to establish whether there enough buyers,
      // to enable the bot to sell the coin to them when the rates are good.
      vector <float> buyer_quantities;
      vector <float> buyer_rates;
      if (checks_good) {
        exchange_get_buyers_via_satellite (exchange, market, coin, buyer_quantities, buyer_rates, true, nullptr);
        fix_dust_trades (market, buyer_quantities, buyer_rates);
        fix_too_low_trades (minimum_trade_sizes, exchange, market, coin, buyer_quantities, buyer_rates);
      }
      
      // Get the sellers order book at the exchange.
      vector <float> seller_quantities;
      vector <float> seller_rates;
      if (checks_good) {
        exchange_get_sellers_via_satellite (exchange, market, coin, seller_quantities, seller_rates, true, nullptr);
        if (seller_quantities.empty ()) {
          output.add ({"Cannot get the sellers of", coin, ": Cancel"});
          checks_good = false;
        }
        if (checks_good) {
          output.add ({"Successfully got the sellers"});
        }
      }

      // Remove trades too low.
      if (checks_good) {
        fix_dust_trades (market, seller_quantities, seller_rates);
        fix_too_low_trades (minimum_trade_sizes, exchange, market, coin, seller_quantities, seller_rates);
      }

      // The order book should be large enough for this trade.
      if (checks_good) {
        if (order_book_is_good (seller_quantities, seller_rates, false)) {
          output.add ({"The order book is good"});
        } else {
          output.add ({"The order book is too small: Cancel"});
          checks_good = false;
        }
      }

      // To ge the optimum price, take the price and quantity of the best seller.
      float quantity = 0, rate = 0;
      if (checks_good) {
        quantity = seller_quantities [0];
        rate = seller_rates [0];
        output.add ({"The sellers offer", float2string (quantity), coin, "at rate", float2string (rate), market, "for sale as the best deal"});
      }
      
      // Check if the bid and the ask price are near each other.
      if (checks_good) {
        if (order_book_is_good (buyer_quantities, buyer_rates, false)) {
          output.add ({"The future buyer order book is good"});
        } else {
          output.add ({"The future buyer order book is too small: Cancel"});
          checks_good = false;
        }
      }
      if (checks_good) {
        float buyer_rate = buyer_rates [0];
        float seller_rate = seller_rates [0];
        float percentage = get_percentage_change (buyer_rate, seller_rate);
        output.add ({"The difference between buyer rate and seller rate is", float2visual(percentage), "%"});
        if (percentage > 1) {
          output.add ({"This difference is too large: Cancel"});
          checks_good = false;
        }
      }
      
      // Check the average rates over the past hour.
      if (checks_good) {
        unordered_map <int, float> minute_asks, minute_bids, hour_asks, hour_bids;
        pattern_prepare (&output, exchange, market, coin, minute_asks, minute_bids, hour_asks, hour_bids);
        float average_hourly_rate = hour_asks [0];
        if (average_hourly_rate > 0) {
          output.add ({"The average hourly rate just now is", float2string (average_hourly_rate), market});
          if (average_hourly_rate < rate) {
            float difference = get_percentage_change (rate, average_hourly_rate);
            output.add ({"The difference is", float2visual (difference), "%, attempting to buy the coin at this rate"});
            rate = average_hourly_rate;
          }
        }
      }

      if (checks_good) {
        output.add ({"Intention to buy", float2string (quantity), coin, "@", market, "@", exchange, "at rate", float2string (rate)});
      }

      // Check on a rate of zero.
      // This has been noticed to happen when there's an error communicating with the exchange.
      // If this is the case, just disregard this buy order.
      if (checks_good) {
        if (rate == 0) {
          output.add ({"A rate of zero is not realistic: Cancel"});
          checks_good = false;
        }
      }

      // Intent to buy a calculated amount of coins equivalent to no more than 0.01 Bitcoin
      // If the optimum amount to trade is lower than the equivalent of 0.01 Bitcoin,
      // reduce the quantity to buy.
      if (checks_good) {
        float max_coin_quantity = 0.01 / rate;
        if (quantity > max_coin_quantity) {
          quantity = max_coin_quantity;
          output.add ({"Reducing the quantity to buy to", float2string (quantity), coin, "so the total spent is no more than 0.01 Bitcoin"});
        }
      }

      // Increase the price offered slightly.
      // The reason for this is as follows:
      // When buying coins for the ask price, unfulfilled buy orders slowly accumulate on the various exchanges.
      // This is what happens in practise.
      // Increasing the price to buy the coins for is expected to result in fewer open buy orders.
      if (checks_good) {
        rate += (rate * exchange_get_trade_order_ease_percentage (exchange) / 100);
        output.add ({"Raise the rate of the coin to", float2string (rate)});
      }

      // Lock the container with balances.
      // This is because there's multiple parallel traders all accessing this container.
      // Do this regardless of whether the checks are good, so locking and unlocking are always in balance.
      update_mutex.lock ();

      // The amount of market balance this requires.
      float market_amount = 0;
      if (checks_good) {
        market_amount = quantity * rate;
        output.add ({"This would need", float2string (market_amount), market});
      }

      // Check that the trade is not lower than the possible minimum trade size at the exchange.
      if (checks_good) {
        float minimum = minimum_trade_sizes [exchange + market + coin];
        // A value of 0 means: No limits set by the exchange.
        if (minimum > 0) {
          // Use a 2 percent margin for safety.
          if (quantity < (minimum * 1.02)) {
            output.add ({"Exchange", exchange, "has a minimum order size of", float2string (minimum), coin, "@", market, ": Cancel"});
            checks_good = false;
          } else {
            output.add ({"Passed the minimum order size check"});
          }
        }
      }

      // At this point the limit buy order is going to be placed.
      // Subtract the required market quantity from the available balances.
      if (checks_good) {
        available -= market_amount;
        exchange_set_balance_cache (exchange, market, total, available, reserved, unconfirmed);
      }

      // Make the balances available again to other threads.
      // Do this regardless of whether the checks are good, so locking and unlocking are always in balance.
      update_mutex.unlock ();

      // Place the order at the exchange.
      string order_id, error, json;
      if (checks_good) {
        exchange_limit_buy (exchange, market, coin, quantity, rate, error, json, order_id, &output);
        output.add ({"Order placed with ID", order_id});
      }

      // Record the price the coin was bought for in the database.
      if (checks_good) {
        bought_add (sql_front, exchange, market, coin, rate);
      }
    }
  }
}


// Sets the price the coin most likely was bought for.
void set_likely_coin_bought_price (string exchange, string market, string coin)
{
  // When encountering a balance,
  // execute a few steps to get the rate this coin would have been bought for.
  float buy_rate = 0;
  string rate_method;
  
  SQL sql (front_bot_ip_address ());

  // Get all prices coins were bought for from the database, for consulting them.
  // The database contains older rates too.
  // The data is fetched in the order it was entered.
  // So newer rates for the same coin will overwrite older rates.
  // This way the bot always has the newest rate a coin was bought for.
  vector <int> ids, hours;
  vector <string> exchanges, markets, coins;
  vector <float> rates;
  bought_get (sql, ids, hours, exchanges, markets, coins, rates);
  
  // It used to look for a former record in the database,
  // where this coin was bought on this same exchange,
  // or where this coin was bought on any exchange.
  // It would take that rate, if such a rate was found.
  // But this is a method that can lead to losses.
  // An example of a possible loss would be this:
  // 1. The coin was bought some time ago for a certain low price.
  // 2. That coin was sold also a good while ago.
  // 3. The price has risen considerably.
  // 4. A coin is encountered on an exchange (due to e.g. arbitrage or multipath trading or redistribution).
  // 5. The bot thinks that the coin was bought for a lower price.
  // 6. The bot notices a possible gain of multiple percents.
  // 7. The bot immediately sells the coin balance it encountered.
  // For these reasons the above methods for determing what price the coin was bought for,
  // is no longer used.
  
  // Step one:
  // Take the average rate of this coin at this market at this exchange over the last hour.
  // The reason this is a good way of doing things is that this coin is just now encountered,
  // so taking the current price is a very realistic thing to do.
  if (buy_rate == 0) {
    unordered_map <int, float> minute_asks, minute_bids, hour_asks, hour_bids;
    pattern_prepare (NULL, exchange, market, coin, minute_asks, minute_bids, hour_asks, hour_bids);
    buy_rate = hour_asks[0];
    rate_method = "current hourly average";
  }
  
  // Step two:
  // If no rate is found yet,
  // take the current rate of the coin at this market at this exchange.
  if (buy_rate == 0) {
    buy_rate = rate_get_cached (exchange, market, coin);
    rate_method = "current live rate";
  }
  
  // There was a case like this:
  // exchange market coin was bought for 0.0000000000 can be sold with gain inf %
  // Here the issue is that there's an invalid buy_rate.
  // So if the rate is 0, there's something wrong, so don't enter this coin right now.
  // Perhaps it will get a correct rate next time.
  if (buy_rate > 0) {
    bought_add (sql, exchange, market, coin, buy_rate);
    to_stderr ({"A certain amount of", coin, "was encountered at", exchange, "bought at rate", float2string (buy_rate), market, "obtained through", rate_method, "and will be monitored"});
  } else {
    to_stdout ({"A certain amount of", coin, "was encountered at", exchange, "but will not be monitored yet because the rate it was bought for cannot be established"});
  }
}


void redistribute_coin_bought_price (string original_exchange, string target_exchange, string market, string coin, float price)
{
  SQL sql (front_bot_ip_address ());

  // Get all the prices the coins were bought for.
  vector <int> ids, hours;
  vector <string> exchanges, markets, coins;
  vector <float> rates;
  bought_get (sql, ids, hours, exchanges, markets, coins, rates);

  // Determine the new price the coin is supposed to be bought for.
  // If a price is passed to this function, take that.
  // If no price is passed, that is, the price passed is zero,
  // then take the price the coin was bought for at the original_exchange.
  if (price == 0) {
    for (unsigned int i = 0; i < ids.size(); i++) {
      if (original_exchange != exchanges[i]) continue;
      if (market != markets[i]) continue;
      if (coin != coins[i]) continue;
      // There can be multiple prices: The most recent one is taken here.
      price = rates[i];
    }
  }
  
  // Check if any price is given or found at all.
  // If not: Ready: Nothing to do right now.
  if (price == 0) return;
  
  // Get the two most recent prices the coin was bought for at the two exchanges.
  float original_price = 0;
  float target_price = 0;
  for (unsigned int i = 0; i < ids.size(); i++) {
    if (market != markets[i]) continue;
    if (coin != coins[i]) continue;
    float price = rates[i];
    if (original_exchange == exchanges[i]) original_price = price;
    if (target_exchange == exchanges[i]) target_price = price;
  }
  
  // If the new price is higher than the existing ones, store the new price.
  // The reasons for this are as follows:
  // * A coin is bought due to arbitrage.
  // * The arbitrage buy price is higher than the price "bought" for as recorded in the database.
  // * The bot thinks that gains can be made, and immediately sells this coin balance again.
  // In reality is runs a loss.
  // The same problem may occur due to coin redistribution.
  if (price > original_price) {
    bought_add (sql, original_exchange, market, coin, price);
  }
  if (price > target_price) {
    bought_add (sql, target_exchange, market, coin, price);
  }
}


// The minimum difference in percents to make arbitrage profitable.
float arbitrage_difference_percentage (string exchange1, string exchange2)
{
  // Take the trade commission into account.
  // Bittrex takes 0.25 %.
  // Cryptopia takes 0.20 %.
  // Other exchange take similar amounts.
  // So the difference should be high enough to cover the trade commissions on both exchanges.
  // There is also the losses incurred in regular transferring coins from one exchange to the other.
  // The difference should also be high enough to conver this.
  
  // It has been set to 1.25 % in the hope that more gains would be made, but this was not the case.
  // So now it's back to 0.75 %.
  
  // Here's a change again:
  // Since there's no balancing being done now, the losses due to balancing are no longer there.
  // So the minimum difference was lowered to 0.60%
  
  // Update: The 0.60% led to incidental small losses as calculated, so it was increased to 0.65%
  
  // Update: The 0.65% led to incidental zero gains as calculated, so it was increases to 0.70% again.
  
  // Update August 2018:
  // Balancing is done again.
  // So to offset the costs, the factor should increase to 0.8%
  
  // Update August 2018:
  // The difference will now be calculated in a different way.
  // It will now be made dependent upon the exchange easing percentage.
  // The easing percentage on both exchanges will be taken in account.
  float percentage = 0;
  percentage += exchange_get_trade_order_ease_percentage (exchange1);
  percentage += exchange_get_trade_order_ease_percentage (exchange2);
  percentage += 0.55f;
  return percentage;
}


// This is some core logic that processes and decides whether to do a trade to equalize coin rates between exchanges.
// The core logic is separated to enable regression testing without doing real trades.
void arbitrage_processor (void * output, string market, string coin, float & quantity,
                          float ask_quantity, float bid_quantity,
                          float & minimum_ask, float & maximum_bid,
                          float asking_exchange_balance,
                          float bidding_exchange_balance,
                          string asking_exchange, string bidding_exchange,
                          bool & market_balance_too_low, bool & coin_balance_too_low,
                          map <string, float> minimum_trade_sizes,
                          vector <float> & buyers_quantities, vector <float> & buyers_rates,
                          vector <float> & sellers_quantities, vector <float> & sellers_rates)
{
  // The possible object to write the output to in bundled format.
  to_output * output_block = NULL;
  if (output) output_block = (to_output *) output;
  
  // Find the minimum amount of coins available with both the bidder and asker.
  quantity = bid_quantity;
  if (ask_quantity < quantity) quantity = ask_quantity;
  if (output) output_block->add ({"Available for trade is", float2string (quantity), coin, "@", market});
  
  // Fix the rates based on the quantities and the order books.
  // The issue here is that some quantities cannot be traded for the rate given.
  // So to get higher quantities, the rates might become less profitable.
  fix_rate_for_quantity (quantity, buyers_quantities, buyers_rates);
  if (order_book_is_good (buyers_quantities, buyers_rates, false)) {
    if (maximum_bid != buyers_rates [0]) {
      maximum_bid = buyers_rates [0];
      if (output) output_block->add ({"Due to a small order book, the bid price was reduced to", float2string (maximum_bid)});
    }
  } else {
    if (output) output_block->add ({"The order book at exchange", bidding_exchange, "is too small, cancelling the trade"});
    quantity = 0;
  }
  fix_rate_for_quantity (quantity, sellers_quantities, sellers_rates);
  if (order_book_is_good (sellers_quantities, sellers_rates, false)) {
    if (minimum_ask != sellers_rates [0]) {
      minimum_ask = sellers_rates [0];
      if (output) output_block->add ({"Due to a small order book, the ask price was increased to", float2string (minimum_ask)});
    }
  } else {
    if (output) output_block->add ({"The order book at exchange", asking_exchange, "is too small, cancelling the trade"});
    quantity = 0;
  }
  
  // Increase the minimum ask price slightly.
  // The reason for this is as follows:
  // When buying coins for the ask price, unfulfilled buy orders slowly accumulate on the various exchanges.
  // This is what happens in practise.
  // Increasing the minimum ask price here results in a slightly higher bid price
  // for the limit buy order the bot places.
  // This is expected to result in fewer open buy orders.
  minimum_ask += (minimum_ask * exchange_get_trade_order_ease_percentage (asking_exchange) / 100);
  if (output) output_block->add ({"Correction to reduce the amount of open buy orders", asking_exchange, "asks", float2string (minimum_ask), "for", float2string (ask_quantity), coin, "@", market});
  
  // In the same manner, apply a decrease to the maximum bid price.
  // This is supposed to decrease the number of open sell orders.
  maximum_bid -= (maximum_bid * exchange_get_trade_order_ease_percentage (bidding_exchange) / 100);
  if (output) output_block->add ({"Correction to reduce the amount of open sell orders", bidding_exchange, "bids", float2string (maximum_bid), "for", float2string (bid_quantity), coin, "@", market});
  
  // Find out if the arbitrage trade will still be profitable,
  // since the rates may have been updated at this stage.
  if (quantity > 0) {
    if (maximum_bid > minimum_ask) {
      float percentage = (100 * maximum_bid / minimum_ask) - 100;
      if (output) output_block->add ({"Arbitrage difference is", float2string (percentage), "%" });
      if (percentage < arbitrage_difference_percentage (bidding_exchange, asking_exchange)) {
        if (output) {
          output_block->add ({"The rates were updated because of a small order in the order book at the exchange"});
          output_block->add ({"The updated arbitrage difference is", to_string (percentage), "% cancelling the trade"});
        }
        quantity = 0;
      }
    }
  }
  
  // Check that the Bitcoin or base market coin balance on the asking exchange,
  // where the bot will buy coins,
  // is sufficient to buy the intended amount of coins.
  float available_base_market_coin = asking_exchange_balance;
  if (quantity > 0) {
    if (output) output_block->add ({"Funds available to buy with is", to_string (available_base_market_coin), market});
  }
  if (quantity > 0) {
    // Calculate the amount of coins that the available Bitcoins or base market coins can buy.
    // Use the worst-case scenario.
    float amount = available_base_market_coin / maximum_bid;
    // Have a few percents of margin.
    // At first the margin was 98% but this still was not good enough.
    // It led to "Error":"Insufficient Funds.".
    // So now it is 95%.
    amount *= 0.95;
    if (quantity > amount) {
      // Reduce the quantity to the amount the available Bitcoins can buy.
      quantity = amount;
      if (output) {
        output_block->add ({"The wallet on the exchange where to buy has only", float2string (available_base_market_coin), "available", market});
        output_block->add ({"The available", market, "can buy only", float2string (amount), coin});
        output_block->add ({"The amount to buy was reduced to", float2string (quantity), coin});
      }
    }
  }
  
  // Check whether the Bitcoin or base market coin balance is too low to trade.
  if (quantity > 0) {
    if (is_dust_trade (market, available_base_market_coin * 0.95, 1)) {
      quantity = 0;
      if (output) output_block->add ({"Insufficient available", market, "so trade of", coin, "was cancelled"});
      market_balance_too_low = true;
    }
  }
  
  // Check that the balance on the bidding exchange,
  // where the bot will sell the coin,
  // is enough to cover the purchase of the intended amount of coins.
  if (quantity > 0) {
    float available = bidding_exchange_balance;
    if (output) output_block->add ({"Funds available to sell with is", to_string (available), coin});
    if (available < quantity) {
      // Some coins need more margin than others: Play on safe.
      // At first the margin was 97% but this still was not good enough.
      // It led to "Error":"Insufficient Funds.".
      // So now it is 95%.
      quantity = available * 0.95;
      if (output) {
        output_block->add ({"The exchange where to sell has only", float2string (available), "available", coin});
        output_block->add ({"The amount to sell was reduced to", float2string (quantity), coin});
      }
    }
  }
  
  // Prevent dust trade: Minimum trade amount will be a certain value.
  // Check on too low coin balance.
  if (quantity > 0) {
    if (is_dust_trade (market, quantity, minimum_ask)) {
      quantity = 0;
      if (output) output_block->add ({"Not enough available balance: Trade of", coin, "was cancelled"});
      coin_balance_too_low = true;
    }
  }
  
  // Check that the trade is not lower than possible minimum trade amounts set by any of the two exchanges.
  vector <string> both_exchanges = { asking_exchange, bidding_exchange };
  for (auto exchange : both_exchanges) {
    if (quantity > 0) {
      float minimum = minimum_trade_sizes [exchange + market + coin];
      if (output) output_block->add ({"Exchange", exchange, "has a minimum order size of", float2string (minimum), coin, "@", market});
      // A value of 0 means: No limits set by the exchange.
      if (minimum > 0) {
        // Use a 2 percent margin for safety.
        if (quantity < (minimum * 1.02)) {
          quantity = 0;
          if (output) output_block->add ({"Exchange", exchange, "has a minimum order size of", float2string (minimum), coin, "@", market, "cancelling the trade"});
        }
      }
    }
  }
}


