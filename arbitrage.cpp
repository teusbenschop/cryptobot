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
#include "traders.h"


// Storage for the rates v2.
vector <string> distinct_coins;
vector <string> distinct_markets;
vector <string> distinct_exchanges;
unordered_map <string, float> emc_bids;
unordered_map <string, float> emc_asks;


// Storage for the arbitrage items to store in the database.
vector <arbitrage_item> arbitrage_items;


// Store the minimum trade size per exchange/market/coin.
// Storage format: container [exchange+market+coin] = minimum trade size.
map <string, float> minimum_trade_sizes;


void arbitrage_analyze_one (string market, string coin,
                            float buyer_bid, string buyer_exchange,
                            float seller_ask, string seller_exchange)
{
  // Check if there's a buyer who bids more for a coin than a seller asks for it.
  if (buyer_bid > seller_ask) {
    if (buyer_exchange != seller_exchange) {
      float percentage = (100 * buyer_bid / seller_ask) - 100;
      // Percentage difference should be high enough to be able to make a gain with arbitrage.
      // Percentage difference should not be too high, usually there's a problem then, so skip those.
      if ((percentage >= arbitrage_difference_percentage (buyer_exchange, seller_exchange)) && (percentage < 25)) {
        
        // Look for the volumes to trade and remove dust trades.
        // Cache the data for reuse, to reduce the number of API calls.

        float buyer_volume = 0;
        {
          // Get the approximate buyers.
          vector <float> quantities;
          vector <float> rates;
          bool success = get_cached_approximate_buyers (buyer_exchange, market, coin, quantities, rates);
          // If there was a failure, there's no need to proceed: Bail out.
          if (!success) return;
          // Fixing books.
          fix_dust_trades (market, quantities, rates);
          fix_too_low_trades (minimum_trade_sizes, buyer_exchange, market, coin, quantities, rates);
          if (!rates.empty ()) {
            // Take updated quantities and rates.
            buyer_bid = rates [0];
            buyer_volume = quantities [0];
          } else {
            // Failure to find the orderbook: Zero everything.
            buyer_bid = 0;
            buyer_volume = 0;
          }
        }
        
        float seller_volume = 0;
        {
          // Get the approximate sellers.
          vector <float> quantities;
          vector <float> rates;
          bool success = get_cached_approximate_sellers (seller_exchange, market, coin, quantities, rates);
          // If there was a failure, there's no need to proceed: Bail out.
          if (!success) return;
          // Fixing books.
          fix_dust_trades (market, quantities, rates);
          fix_too_low_trades (minimum_trade_sizes, seller_exchange, market, coin, quantities, rates);
          if (!rates.empty ()) {
            // Take updated quantities and rates.
            seller_ask = rates [0];
            seller_volume = quantities [0];
          } else {
            // Failure to find the orderbook: Zero everything.
            seller_ask = 0;
            seller_volume = 0;
          }
        }
        
        percentage = (100 * buyer_bid / seller_ask) - 100;
        float trade_volume = seller_volume;
        if (buyer_volume < trade_volume) trade_volume = buyer_volume;
        float market_volume = trade_volume * (buyer_bid + seller_ask) / 2;
        
        // After dust trade has been removed and the volume has been calculated,
        // check that the percentage difference is still high enough.
        // At this stage it might even be negative.
        if (percentage < arbitrage_difference_percentage (buyer_exchange, seller_exchange)) return;
        // Also eliminate this error:
        // INSERT INTO arbitrage (coin, seller, ask, buyer, bid, percentage, volume) VALUES (  'equitrade'  ,  'cryptopia'  ,  0  ,  ''  ,  1.175494e-38  ,  inf  ,  0  );  Unknown column 'inf' in 'field list'
        // The "inf" means "infinite", so check on that.
        if (isnan (percentage)) return;
        if (isinf (percentage)) return;

        // Store this arbitrage opportunity.
        arbitrage_item item;
        item.market = market;
        item.coin = coin;
        item.seller = seller_exchange;
        item.ask = seller_ask;
        item.buyer = buyer_exchange;
        item.bid = buyer_bid;
        item.percentage = percentage;
        item.volume = market_volume;
        // Store the arbitrage opportunity.
        arbitrage_items.push_back (item);
        
        // Output.
        to_tty ({"market", market, "coin", coin, "seller", seller_exchange, "asks", float2string (seller_ask), "buyer", buyer_exchange, "bids", float2string (buyer_bid), "difference", to_string (percentage), "%"});
      }
    }
  }
}


// Fetch the rates and store them in memory for fast access.
void arbitrage_fetch_rates ()
{
  // Fetch the rates from the front bot.
  SQL sql (front_bot_ip_address ());
  vector <int> seconds;
  vector <string> exchanges, markets, coins;
  vector <float> bids, asks, rates;
  rates_get (sql, seconds, exchanges, markets, coins, bids, asks, rates, true);

  // Convert the rates to objects in memory for fast access
  for (unsigned int i = 0; i < bids.size (); i++) {
    if (!in_array (coins[i], distinct_coins)) distinct_coins.push_back (coins[i]);
    if (!in_array (markets[i], distinct_markets)) distinct_markets.push_back (markets[i]);
    if (!in_array (exchanges[i], distinct_exchanges)) distinct_exchanges.push_back (exchanges[i]);
    string key = exchanges[i] + markets[i] + coins[i];
    emc_bids [key] = bids[i];
    emc_asks [key] = asks[i];
  }
  
  to_stdout ({"Arbitrage analysis on", to_string (distinct_coins.size()), "coins and", to_string (distinct_markets.size()), "markets and", to_string (distinct_exchanges.size()), "exchanges"});

  // Also fetch the minimum trade sizes.
  minimum_trade_sizes = mintrade_get (sql);
}


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);
  to_stdout ({"Arbitrage analyzer started"});
  
  if (program_running (argv)) {
    to_failures (failures_type_api_error (), "", "", 0, {"Delayed arbitrage analysis"});
  } else {
  
    // Fetch the rates.
    arbitrage_fetch_rates ();

    // Iterate over the distinct coins.
    for (auto coin : distinct_coins) {
      
      // Iterate over the distinct markets.
      for (auto market : distinct_markets) {

        // Get the exchanges that support this combination of coin and market.
        vector <string> exchanges;
        for (auto exchange : distinct_exchanges) {
          string key = exchange + market + coin;
          if (emc_bids [key] > 0) {
            exchanges.push_back (exchange);
            //to_tty ({coin, market, exchange});
          }
        }
        
        // Arbitrage requires at least two exchanges.
        if (exchanges.size () < 2) continue;
        
        // Iterate over the exchanges twice and analyze their rates.
        for (size_t i1 = 0; i1 < exchanges.size(); i1++) {
          string buyer_exchange = exchanges[i1];
          string key = exchanges[i1] + market + coin;
          float buyer_bid = emc_bids [key];
          for (size_t i2 = 0; i2 < exchanges.size(); i2++) {
            string seller_exchange = exchanges[i2];
            string key = exchanges[i2] + market + coin;
            float seller_ask = emc_asks [key];
            arbitrage_analyze_one (market, coin, buyer_bid, buyer_exchange, seller_ask, seller_exchange);
          }
        }
      }
    }
    
    // Feedback.
    to_stdout ({"The analyzer found", to_string (arbitrage_items.size()), "arbitrage opportunities"});
    
    // Store all the arbitrage items to the database at the back bot.
    SQL sql (back_bot_ip_address ());
    arbitrage_store (sql, arbitrage_items);
  }

  finalize_program ();
  to_stdout ({"Arbitrage analyzer completed"});
  return EXIT_SUCCESS;
}
