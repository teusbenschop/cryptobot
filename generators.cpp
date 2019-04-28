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


#include "generators.h"
#include "sql.h"
#include "shared.h"
#include "models.h"
#include "exchanges.h"
#include "html.h"
#include "controllers.h"
#include "bittrex.h"
#include "cryptopia.h"
#include "proxy.h"


mutex generator_mutex;


class generator_output
{
public:
  string title;
  vector <string> lines;
  vector <string> images;
};


map <float, generator_output> valuation_map;
vector <generator_output> all_balances;
// The rates and the hours ago the coins were bought for.
// Storage format: container [exchange+market+coin] = value;
unordered_map <string, float> bought_rates;
unordered_map <string, float> bought_hours;


float generator_unique_key (float key)
{
  int iterations = 0;
  while (valuation_map.find (key) != valuation_map.end ()) {
    float difference = key * 0.000001;
    key -= abs (difference);
    iterations++;
    if (iterations > 1000) {
      return static_cast <float> (rand());
    }
  }
  return key;
}


// Storage for this month's transfer statistics.
vector <string> transfers_coin_abbrevs, transfers_txexchanges, transfers_rxexchanges;
vector <int> transfers_seconds;
vector <float> transfers_txquantities, transfers_rxquantities;

// Storage for arbitrage trade statistics for a certain period.
vector <int> trade_seconds;
vector <string> trade_markets, trade_coins, trade_buyexchanges, trade_sellexchanges;
vector <float> trade_quantities, trade_marketgains;

// Storage for the pairs of trading exchanges and their markets and coins.
vector <string> trading_exchange1s, trading_exchange2s, trading_markets, trading_coins;
vector <int> trading_days;

// The coin balanes and associated values.
vector <int> balances_seconds;
vector <string> balances_exchanges;
vector <string> balances_coins;
vector <float> balances_totals;
vector <float> balances_bitcoins;
vector <float> balances_euros;

// The available arbitage amounts.
vector <int> availables_seconds;
vector <string> availables_sellers;
vector <string> availables_buyers;
vector <string> availables_markets;
vector <string> availables_coins;
vector <float> availables_bidsizes;
vector <float> availables_asksizes;
vector <float> availables_realizeds;


void generator_load_data ()
{
  to_tty ({"Reading data"});
  
  SQL sql_front (front_bot_ip_address ());
  SQL sql_back (back_bot_ip_address ());

  // Retrieve the coins' transfer statistics.
  {
    vector <string> order_ids, txids, executeds, addresses, visibles, arriveds;
    vector <int> database_ids;
    transfers_get_month (sql_front, database_ids, transfers_seconds, order_ids, transfers_coin_abbrevs, transfers_txexchanges, transfers_txquantities, txids, executeds, transfers_rxexchanges, addresses, visibles, transfers_rxquantities, arriveds);
  }
  
  // Get the arbitrage trade statistics for the whole past month.
  trades_get_month (sql_front, trade_seconds, trade_markets, trade_coins, trade_buyexchanges, trade_sellexchanges, trade_quantities, trade_marketgains);
  
  // The pairs of trading exchanges and their markets.
  pairs_get (sql_front, trading_exchange1s, trading_exchange2s, trading_markets, trading_coins, trading_days);
  
  // The recent coin balances and associated values.
  balances_get_month (sql_back, balances_seconds, balances_exchanges, balances_coins, balances_totals, balances_bitcoins, balances_euros);
  
  // The available arbitrage amounts.
  {
    vector <int> ids;
    vector <string> stamps;
    availables_get (sql_back, ids, stamps, availables_seconds, availables_sellers, availables_buyers, availables_markets, availables_coins, availables_bidsizes, availables_asksizes, availables_realizeds);
  }
  
  to_tty ({"Data is available"});
}


void website_index_generator ()
{
  Html index ("Arbitrage", "");
  index.h (1, "Arbitrage");
  /*
  Html index ("Trader", "");
  index.h (1, "Trader");
  index.p ();
  index.a ("prospectives.html", "Prospective coins");
  index.p ();
  index.a ("assets.html", "Asset performance");
   */
  index.p ();
  index.a ("balances.html", "Balances");
  index.p ();
  index.a ("performance.html", "Performance");
  index.p ();
  index.a ("opportunities.html", "Opportunities");
  index.p ();
  index.a ("consistency.html", "Consistency");
  index.p ();
  index.a ("notes.html", "Notes");
  index.p ();
  index.txt (generator_page_timestamp ());
  index.save ("index.html");
}


void prospective_coin_item_generator (string coin, int & thread_count)
{
  generator_mutex.lock ();
  thread_count++;
  generator_mutex.unlock ();
  
  to_tty ({coin});

  SQL sql_back (back_bot_ip_address ());

  // Storage for the output specific to the current coin.
  generator_output appreciation;
  
  // The coin's names.
  appreciation.title = coin;
  string coin_abbrev = exchange_get_coin_abbrev ("", coin);
  string coin_name = exchange_get_coin_name ("", coin);
  appreciation.lines.push_back ("Coin: " + coin + " " + coin_name + " " + coin_abbrev);
  
  // If the coin has a balance on any of the exchanges,
  // and if the balance is larger than the dust trade limit,
  // do not include this coin in the prospective coins.
  // The bot should not buy coins that still have a balance.
  // Rather it waits till all of the balance has been sold.
  // So to make the prospective assets more concise,
  // and easier to use by the operator,
  // exclude the coins that have a balance.
  bool include_coin = true;
  for (auto exchange : exchange_get_names ()) {
    float btc_equivalent = balances_get_average_hourly_btc_equivalent (sql_back, exchange, coin, 2, 0);
    if (!is_dust_trade (bitcoin_id (), btc_equivalent, 1)) {
      include_coin = false;
    }
  }
  
  // Only continue when the coin is to be included on this page.
  if (include_coin) {
    
    to_tty ({"Include", coin});

    // State the exchanges that support this coin.
    string exchange_for_graphs;
    vector <string> exchanges;
    for (auto exchange : exchange_get_names ()) {
      vector <string> coins = exchange_get_coins_per_market_cached (exchange, bitcoin_id ());
      if (in_array (coin, coins)) {
        exchanges.push_back (exchange);
        if (exchange_for_graphs.empty ()) exchange_for_graphs = exchange;
      }
    }
    appreciation.lines.push_back ("Exchanges: " + implode (exchanges, " "));
    
    // Prepare the rates for processing them.
    unordered_map <int, float> minute_asks, minute_bids, hour_asks, hour_bids;
    pattern_prepare (NULL, exchange_for_graphs, bitcoin_id (), coin, minute_asks, minute_bids, hour_asks, hour_bids);
    
    // The daily / weekly / monthly change in value of the coin.
    float change_percentage_day = pattern_change (hour_asks, hour_bids, 0, 24);
    float change_percentage_week = pattern_change (hour_asks, hour_bids, 0, 168);
    float change_percentage_month = pattern_change (hour_asks, hour_bids, 0, 720);
    appreciation.lines.push_back ("Change: daily " + float2visual (change_percentage_day) + "% weekly " + float2visual (change_percentage_week) + "% monthly " + float2visual (change_percentage_month) + "%");

    // Create the rates graph over plenty of days.
    // These many days help to view the coin's long-term development.
    // This aids in making better decisions as to when to buy this coin.
    if (!exchanges.empty ()) {
      vector <pair <float, float> > plotdata;
      int days = 180;
      plotdata = chart_rates (exchange_for_graphs, bitcoin_id (), coin, days);
      string title = "rates-" + exchange_for_graphs + "-" + bitcoin_id () + "-" + coin + "-" + to_string (days);
      string picturepath;
      plot (plotdata, {}, title, "", picturepath);
      string contents = file_get_contents (picturepath);
      string path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
      file_put_contents (path, contents);
      appreciation.images.push_back (basename (picturepath));
    }
    
    // Add any comments.
    generator_list_comments (coin, appreciation.lines);

    
    // Add the bundle to the sorted output bundle.
    // Enesure unique keys so there's no loss of information in case coins have identical keys.
    float key = change_percentage_week + change_percentage_month;
    if ((isnan (key)) || (isinf (key))) key = numeric_limits<float>::max();
    key = generator_unique_key (key);
    generator_mutex.lock ();
    valuation_map [key] = appreciation;
    generator_mutex.unlock ();
  }
  
  // One producer has completed.
  generator_mutex.lock ();
  thread_count--;
  generator_mutex.unlock ();
}


void prospective_coin_page_generator (int max_threads)
{
  valuation_map.clear ();
  
  Html html ("Prospective coins", "prospectives");
  html.h (1, "Prospective coins");
  html.p ();
  html.txt (generator_page_timestamp ());

  // Get a container of all supported coins on all exchanges combined.
  vector <string> all_coins;
  for (auto exchange : exchange_get_names ()) {
    vector <string> coins = exchange_get_coin_ids (exchange);
    for (auto coin : coins) {
      if (!in_array (coin, all_coins)) {
        all_coins.push_back (coin);
      }
    }
  }
  
  int thread_count = 0;

  // Iterate over all supported coins.
  for (auto coin : all_coins) {
  
    // static int counter = 0; counter++; if (counter > 30) continue;
    
    // Wait till there's an available slot for this new thread.
    while (thread_count >= max_threads) {
      this_thread::sleep_for (chrono::milliseconds (10));
    }
    
    // Start the thread.
    new thread (prospective_coin_item_generator, coin, ref (thread_count));
    
    // Wait shortly to give the previously started thread time to update the thread count.
    this_thread::sleep_for (chrono::milliseconds (1));
  }
  
  // Wait till all threads have completed.
  while (thread_count > 0) {
    this_thread::sleep_for (chrono::milliseconds (10));
  }
  
  // Write the appreciation items to the html page.
  // Sorted on best appreciation at the top.
  map <float, generator_output> ::reverse_iterator rit;
  for (rit = valuation_map.rbegin(); rit != valuation_map.rend(); rit++) {
    generator_output appreciation = rit->second;
    html.h (2, appreciation.title);
    for (auto s : appreciation.lines) {
      html.p ();
      html.txt (s);
    }
    // Add a buy button for this coin.
    html.p ();
    html.buy (appreciation.title);
    for (auto img : appreciation.images) {
      html.p ();
      html.img (img);
    }
  }
  
  // Add the <div> right at the bottom of the page.
  // Here's where the buy commands go.
  html.div ("buy");
  html.txt (" ");
  // Include new line for easier pasting of the surfer commands.
  html.p ();
  html.txt (" "); // Non-breaking space.
  
  // Output.
  html.save ("prospectives.html");
}


void asset_performance_coin_generator (string coin, int & thread_count)
{
  // A counter for development purposes.
  static int coins_done_count = 0;
  //if (coins_done_count >= 2) return;
  //if (coin != "vsync") return;

  generator_mutex.lock ();
  thread_count++;
  generator_mutex.unlock ();

  to_tty ({coin});

  SQL sql_back (back_bot_ip_address ());

  // Storage for the output specific to the current coin.
  generator_output webpage;
  
  // Containers for the balances of this coin stored per exchange:
  // container [exchange] = value.
  map <string, float> coin_balances;
  map <string, float> btc_equivalents;
  
  vector <string> exchanges = exchange_get_names ();
  
  string exchange_with_highest_balance;
  float exchange_highest_balance = 0;
  
  float total_coin_balance = 0;
  float total_btc_equivalent = 0;
  
  // Iterate over the supported exchanges.
  // Get the balance of this coin at each exchange.
  // If the balance is so low that it would be dust trade, do not include it.
  // It cannot be sold on that exchange, so there's no reason for including it.
  for (auto exchange : exchanges) {
    float coin_balance = balances_get_average_hourly_balance (sql_back, exchange, coin, 2, 0);
    if (coin_balance > 0) {
      float btc_equivalent = balances_get_average_hourly_btc_equivalent (sql_back, exchange, coin, 2, 0);
      if (!is_dust_trade (bitcoin_id (), btc_equivalent, 1)) {
        coin_balances [exchange] = coin_balance;
        btc_equivalents [exchange] = btc_equivalent;
        total_coin_balance += coin_balance;
        total_btc_equivalent += btc_equivalent;
        if (coin_balance > exchange_highest_balance) {
          exchange_highest_balance = coin_balance;
          exchange_with_highest_balance = exchange;
        }
      }
    }
  }
  
  // Proceed if there's any balance of this coin on any of the exchanges.
  if (!coin_balances.empty ()) {
    
    // A counter for development purposes.
    coins_done_count++;
    
    // The coin's names.
    webpage.title = coin;
    string coin_abbrev = exchange_get_coin_abbrev ("", coin);
    string coin_name = exchange_get_coin_name ("", coin);
    webpage.lines.push_back ("Coin: " + coin + " " + coin_name + " " + coin_abbrev);
    
    // Output the average balances for the last 24 hours.
    string balance_line = "Total balance: " + float2visual (total_coin_balance) + " " + coin + " " + float2string (total_btc_equivalent) + " " + bitcoin_id ();
    for (auto & element : coin_balances) {
      string exchange = element.first;
      float coin_balance = element.second;
      balance_line.append (" (" + exchange + ": " + float2visual (coin_balance) + ")");
    }
    webpage.lines.push_back (balance_line);
    
    // Prepare the rates for processing.
    unordered_map <int, float> minute_asks, minute_bids, hour_asks, hour_bids;
    pattern_prepare (NULL, exchange_with_highest_balance, bitcoin_id (), coin, minute_asks, minute_bids, hour_asks, hour_bids);
    
    // Output how long ago this coin was bought and for how much.
    float bought_days_ago = 0;
    string bought_line;
    for (auto & element : coin_balances) {
      string exchange = element.first;
      string key = exchange + bitcoin_id () + coin;
      int hours_ago = bought_hours [key];
      if (!bought_line.empty ()) bought_line.append (" ");
      if (hours_ago < 48) {
        bought_line.append (to_string (hours_ago) + " hours ago ");
      } else {
        bought_line.append (to_string (hours_ago / 24) + " days ago ");
      }
      float current_bid = hour_bids [0];
      float bought_rate = bought_rates[key];
      float percentage = get_percentage_change (bought_rate, current_bid);
      float coin_balance = coin_balances [exchange];
      bought_line.append (float2visual (coin_balance) + " @ " + float2string (bought_rate) + " gain " + float2visual (percentage) + " %");
      bought_days_ago = hours_ago;
      bought_days_ago /= 24;
    }
    if (!bought_line.empty ()) {
      webpage.lines.push_back ("Bought: " + bought_line);
    }
    
    // The daily / weekly / monthly change in value of the coin.
    float change_percentage_day = pattern_change (hour_asks, hour_bids, 0, 24);
    float change_percentage_week = pattern_change (hour_asks, hour_bids, 0, 168);
    float change_percentage_month = pattern_change (hour_asks, hour_bids, 0, 720);
    webpage.lines.push_back ("Change: daily " + float2visual (change_percentage_day) + "%, weekly " + float2visual (change_percentage_week) + "%, monthly " + float2visual (change_percentage_month) + "%");

    // Number of days to graph.
    int days = 30;

    // Create 30-days rates graph.
    vector <pair <float, float> > ratesplotdata;
    ratesplotdata = chart_rates (exchange_with_highest_balance, bitcoin_id (), coin, days);

    // Create plotting data for having a vertical line which indicates when the coin was bought.
    vector <pair <float, float> > buyplotdata;
    float highest_ask = 0;
    float lowest_ask = numeric_limits<float>::max();
    for (auto element : ratesplotdata) {
      float rate = element.second;
      if (rate < lowest_ask) lowest_ask = rate;
      if (rate > highest_ask) highest_ask = rate;
    }
    buyplotdata.push_back (make_pair (0 - bought_days_ago, highest_ask));
    buyplotdata.push_back (make_pair (0 - bought_days_ago, lowest_ask));

    // Run the plotter.
    string title = "rates-" + exchange_with_highest_balance + "-" + bitcoin_id () + "-" + coin;
    string picturepath;
    plot (ratesplotdata, buyplotdata, title, "bought", picturepath);
    string contents = file_get_contents (picturepath);
    string path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
    file_put_contents (path, contents);
    webpage.images.push_back (basename (picturepath));

    // Add any comments.
    generator_list_comments (coin, webpage.lines);

    // Add the bundle to the sorted output bundle.
    float key = change_percentage_week + change_percentage_month;
    if ((isnan (key)) || (isinf (key))) key = numeric_limits<float>::max();
    key = generator_unique_key (key);
    generator_mutex.lock ();
    valuation_map [key] = webpage;
    generator_mutex.unlock ();
  }
  
  // One producer has completed.
  generator_mutex.lock ();
  thread_count--;
  generator_mutex.unlock ();
}


void asset_performance_page_generator (int max_threads)
{
  valuation_map.clear ();
  
  Html html ("Asset performance", "");
  html.h (1, "Asset performance");
  html.p ();
  html.txt (generator_page_timestamp ());

  SQL sql_back (back_bot_ip_address ());
  
  int thread_count = 0;
  
  vector <string> coins_with_balance = balances_get_distinct_daily_coins (sql_back);
  for (auto coin : coins_with_balance) {
    
    // Wait till there's an available slot for this new thread.
    while (thread_count >= max_threads) {
      this_thread::sleep_for (chrono::milliseconds (10));
    }
    
    // Start the thread.
    new thread (asset_performance_coin_generator, coin, ref (thread_count));
    
    // Wait shortly to give the previously started thread time to update the thread count.
    this_thread::sleep_for (chrono::milliseconds (1));
  }
  
  // Wait till all threads have completed.
  while (thread_count > 0) {
    this_thread::sleep_for (chrono::milliseconds (10));
  }
  
  // Write the depreciation items to the html page.
  // Sorted on worst depreciation at the top.
  for (auto & element : valuation_map) {
    generator_output depreciation = element.second;
    html.h (2, depreciation.title);
    for (auto s : depreciation.lines) {
      html.p ();
      html.txt (s);
    }
    for (auto img : depreciation.images) {
      html.p ();
      html.img (img);
    }
  }
  
  // Output.
  html.save ("assets.html");
}


void balances_total_bitcoin_generator ()
{
  generator_output local_balances;
  
  local_balances.title = "total-bitcoin";
  
  vector <string> exchanges = exchange_get_names ();
  
  vector <pair <float, float> > plotdata;
  string title, picturepath, contents, path;
  
  // Total coin balances over all exchanges converted to Bitcoins.
  plotdata = chart_balances (exchanges, bitcoin_id (), 1, false, false);
  title = "balances-total-bitcoin-day";
  plot (plotdata, {}, title, "", picturepath);
  contents = file_get_contents (picturepath);
  unlink (picturepath.c_str());
  path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
  file_put_contents (path, contents);
  local_balances.images.push_back (basename (picturepath));

  plotdata = chart_balances (exchanges, bitcoin_id (), 7, false, false);
  title = "balances-total-bitcoin-week";
  plot (plotdata, {}, title, "", picturepath);
  contents = file_get_contents (picturepath);
  unlink (picturepath.c_str());
  path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
  file_put_contents (path, contents);
  local_balances.images.push_back (basename (picturepath));

  plotdata = chart_balances (exchanges, bitcoin_id (), 30, false, false);
  title = "balances-total-bitcoin-month";
  plot (plotdata, {}, title, "", picturepath);
  contents = file_get_contents (picturepath);
  unlink (picturepath.c_str());
  path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
  file_put_contents (path, contents);
  local_balances.images.push_back (basename (picturepath));
  
  all_balances.push_back (local_balances);
}


void balances_total_converted_to_bitcoin_generator ()
{
  SQL sql (front_bot_ip_address ());
  
  generator_output local_balances;
  
  local_balances.title = "total-converted-to-bitcoin";
  
  vector <string> exchanges = exchange_get_names ();
  
  vector <pair <float, float> > plotdata;
  string title, picturepath, contents, path;
  
  // Total coin balances over all exchanges converted to Bitcoins.
  plotdata = chart_balances (exchanges, "", 1, true, false);
  title = "balances-converted-to-bitcoin-day";
  plot (plotdata, {}, title, "", picturepath);
  contents = file_get_contents (picturepath);
  unlink (picturepath.c_str());
  path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
  file_put_contents (path, contents);
  local_balances.images.push_back (basename (picturepath));
  
  plotdata = chart_balances (exchanges, "", 7, true, false);
  title = "balances-converted-to-bitcoin-week";
  plot (plotdata, {}, title, "", picturepath);
  contents = file_get_contents (picturepath);
  unlink (picturepath.c_str());
  path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
  file_put_contents (path, contents);
  local_balances.images.push_back (basename (picturepath));
  
  plotdata = chart_balances (exchanges, "", 30, true, false);
  title = "balances-converted-to-bitcoin-month";
  plot (plotdata, {}, title, "", picturepath);
  contents = file_get_contents (picturepath);
  unlink (picturepath.c_str());
  path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
  file_put_contents (path, contents);
  local_balances.images.push_back (basename (picturepath));
  
  all_balances.push_back (local_balances);
}


void balances_arbitrage_generator ()
{
  SQL sql (front_bot_ip_address ());
  
  // The pairs of trading exchanges and their base markets and trading coins.
  vector <string> exchanges_market_done;
  vector <string> exchange1s, exchange2s, markets, coins;
  vector <int> days;
  pairs_get (sql, exchange1s, exchange2s, markets, coins, days);
  for (unsigned int axm = 0; axm < exchange1s.size(); axm++) {

    string exchange1 = exchange1s[axm];
    string exchange2 = exchange2s[axm];
    string market = markets[axm];

    // Since there's multiple of the same, check on and skip duplicates.
    string key = exchange1 + exchange2 + market;
    if (in_array (key, exchanges_market_done)) continue;
    exchanges_market_done.push_back (key);
    
    vector <string> exchanges = { exchange1, exchange2 };

    generator_output local_balances;
    
    local_balances.title = exchange1 + "-" + exchange2 + "-" + market;
    
    vector <pair <float, float> > plotdata;
    string title, picturepath, contents, path;
    
    // Total market balances over the arbitraging exchange.
    plotdata = chart_balances (exchanges, market, 30, false, false);
    title = "balances-" + local_balances.title + "-month";
    plot (plotdata, {}, title, "", picturepath);
    contents = file_get_contents (picturepath);
    unlink (picturepath.c_str());
    path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
    file_put_contents (path, contents);
    local_balances.images.push_back (basename (picturepath));
    
    plotdata = chart_balances (exchanges, market, 7, false, false);
    title = "balances-" + local_balances.title + "-week";
    plot (plotdata, {}, title, "", picturepath);
    contents = file_get_contents (picturepath);
    unlink (picturepath.c_str());
    path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
    file_put_contents (path, contents);
    local_balances.images.push_back (basename (picturepath));
    
    plotdata = chart_balances (exchanges, market, 1, false, false);
    title = "balances-" + local_balances.title + "-day";
    plot (plotdata, {}, title, "", picturepath);
    contents = file_get_contents (picturepath);
    unlink (picturepath.c_str());
    path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
    file_put_contents (path, contents);
    local_balances.images.push_back (basename (picturepath));
    
    all_balances.push_back (local_balances);
  }
}


void balances_coin_generator (string coin, int & thread_count)
{
  generator_mutex.lock ();
  thread_count++;
  generator_mutex.unlock ();

  to_tty ({coin});

  SQL sql_back (back_bot_ip_address ());

  // Storage for the output specific to the current coin.
  generator_output local_balances;
  
  // Containers for the balances of this coin stored per exchange.
  map <string, float> coin_balances;
  map <string, float> btc_equivalents;
  
  vector <string> exchanges = exchange_get_names ();
  
  // Iterate over the supported exchanges.
  // Get the balance of this coin at each exchange.
  // If the balance is so low that it would be dust trade, do not include it.
  // It cannot be sold on that exchange, so there's no reason for including it.
  // Update: It will be included even if it's dust trade now.
  // Cause of the update:
  // There was a balance of a coin trading at the Dogecoin market.
  // This balances was so low that it was dust trade at the Bitcoin market.
  // So it's easier to just be realistic, and write whatever balances there are.
  // To not consider whether it's dust trade.
  for (auto exchange : exchanges) {
    float coin_balance = balances_get_average_hourly_balance (sql_back, exchange, coin, 2, 0);
    if (coin_balance > 0) {
      float btc_equivalent = balances_get_average_hourly_btc_equivalent (sql_back, exchange, coin, 2, 0);
      coin_balances [exchange] = coin_balance;
      btc_equivalents [exchange] = btc_equivalent;
    }
  }
  
  // Proceed if there's any balance of this coin on any of the exchanges.
  if (!coin_balances.empty ()) {
    
    // The coin's names.
    local_balances.title = coin;
    string coin_abbrev = exchange_get_coin_abbrev ("", coin);
    string coin_name = exchange_get_coin_name ("", coin);
    local_balances.lines.push_back ("Coin: " + coin + " " + coin_name + " " + coin_abbrev);
    
    // Output the average total balances for the last few hours.
    float total_coin_balance = 0;
    float total_btc_equivalent = 0;
    for (auto & element : coin_balances) total_coin_balance += element.second;
    for (auto & element : btc_equivalents) total_btc_equivalent += element.second;
    local_balances.lines.push_back ("Balance: " + float2string (total_coin_balance) + " " + coin + " " + float2string (total_btc_equivalent) + " " + bitcoin_id ());
    
    // Gather the list of all the exchanges that have a balance of this coin.
    vector <string> all_exchanges;
    for (auto & element : coin_balances) all_exchanges.push_back (element.first);

    // Output the balance at each exchange separately.
    for (auto exchange : all_exchanges) {
      float coin_balance = balances_get_average_hourly_balance (sql_back, exchange, coin, 2, 0);
      float btc_equivalent = balances_get_average_hourly_btc_equivalent (sql_back, exchange, coin, 2, 0);
      local_balances.lines.push_back (exchange + ": " + float2string (coin_balance) + " " + coin + " " + float2string (btc_equivalent) + " " + bitcoin_id ());
    }

    // Plot the bundle of balances over a few days for higher resolution and great detail.
    // Also plot it over the full month to get the overview.
    for (int days : {4, 30}) {

      // The bundles of data for the plotter.
      vector <vector <pair <float, float> > > datasets;
      vector <string> titles;
      
      // Iterate over each exchange, so the balance will be plotted separately for each exchange.
      for (auto exchange : all_exchanges) {
        vector <pair <float, float> > plotdata = chart_balances ({exchange}, coin, days, false, false);
        datasets.push_back (plotdata);
        string title = "balances-" + exchange + "-" + coin + "-" + to_string (days) + "-days";
        titles.push_back (title);
      }
      
      // Also plot the total balances over all exchanges.
      // This enabled the operator to see the slow drop in balances due to transferring the coin.
      {
        vector <pair <float, float> > plotdata = chart_balances (all_exchanges, coin, days, false, false);
        datasets.push_back (plotdata);
        string title = "balances-all-exchanges-" + coin + "-" + to_string (days) + "-days";
        titles.push_back (title);
      }
      
      // Run the plotter.
      string picturepath;
      plot2 (datasets, titles, picturepath);
      
      // Store the file into place in the website.
      string contents = file_get_contents (picturepath);
      unlink (picturepath.c_str());
      string path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
      file_put_contents (path, contents);
      local_balances.images.push_back (basename (picturepath));
    }
    
    // Add the bundle to the total output.
    generator_mutex.lock ();
    all_balances.push_back (local_balances);
    generator_mutex.unlock ();
  }
  
  // One producer has completed.
  generator_mutex.lock ();
  thread_count--;
  generator_mutex.unlock ();
}


void balances_page_generator (int max_threads)
{
  Html html ("Balances", "");
  html.h (1, "Balances");
  html.p ();
  html.txt (generator_page_timestamp ());

  // Chart the total Bitcoins over all exchanges.
  // Update: This is not relevant when trading on the Litecoin and Dogecoin markets.
  //balances_total_bitcoin_generator ();
  
  // Chart the combined balances of the arbitraging market on the pair of arbitraging exchanges.
  balances_arbitrage_generator ();
  
  // Chart the combined balance of all coins on all exchanges converted to a value in Bitcoins.
  // Update: This information is not often used, can be left out.
  //balances_total_converted_to_bitcoin_generator ();
  
  SQL sql_back (back_bot_ip_address ());

  int thread_count = 0;

  // Iterate over the coins that are doing arbitrage trade.
  set <string> coins_with_balance;
  for (auto coin : trading_coins) coins_with_balance.insert (coin);
  for (auto coin : coins_with_balance) {
    
    // Wait till there's an available slot for this new thread.
    while (thread_count >= max_threads) {
      this_thread::sleep_for (chrono::milliseconds (10));
    }
    
    // Start the thread.
    new thread (balances_coin_generator, coin, ref (thread_count));
    
    // Wait shortly to give the previously started thread time to update the thread count.
    this_thread::sleep_for (chrono::milliseconds (1));
  }
  
  // Wait till all threads have completed.
  while (thread_count > 0) {
    this_thread::sleep_for (chrono::milliseconds (10));
  }
  
  // Write the items to the html page.
  // Sorted on lowest balances at the top.
  for (auto & balances : all_balances) {
    html.h (2, balances.title);
    for (auto s : balances.lines) {
      html.p ();
      html.txt (s);
    }
    for (auto img : balances.images) {
      html.p ();
      html.img (img);
    }
  }
  
  // Output.
  html.save ("balances.html");
}


void website_javascript_copier ()
{
  string source_path = dirname (program_path);
  string website_path = dirname (dirname (program_path)) + "/cryptodata/website";
  string contents;
  
  string jquery = "jquery.js";
  contents = file_get_contents (source_path + "/" + jquery);
  file_put_contents (website_path + "/" + jquery, contents);

  string prospectives = "prospectives.js";
  contents = file_get_contents (source_path + "/" + prospectives);
  file_put_contents (website_path + "/" + prospectives, contents);
}



// The performance statistics.
void performance_reporter (bool mail)
{
  string title = "Performance";
  
  SQL sql_front (front_bot_ip_address ());
  SQL sql_back (back_bot_ip_address ());

  // Storage for the output to be mailed or to be sent to the terminal.
  vector <vector <string> > output;
  
  // Load prices coins were bought for.
  load_bought_coins_details ();

  
  {
    output.push_back ({" "});
    output.push_back ({"Pattern-based trade statistics"});
    output.push_back ({"=============================="});
    output.push_back ({" "});
    
    // Get all values of the database with sold assets.
    vector <int> ids, hours;
    vector <string> exchanges, markets, coins;
    vector <float> quantities, buyrates, sellrates;
    sold_get (sql_front, ids, hours, exchanges, markets, coins, quantities, buyrates, sellrates);

    // Storage for the gains per day.
    map <int, float> days_gains;
    
    for (unsigned int i = 0; i < ids.size(); i++) {
      
      // Get the details for this sale.
      int hour = hours [i];
      string exchange = exchanges[i];
      string market = markets[i];
      string coin = coins [i];
      float quantity = quantities [i];
      float buyrate = buyrates [i];
      float sellrate = sellrates [i];
      
      // Do the Bitcoin market only, for just now.
      if (market != bitcoin_id ()) continue;
      
      // Calculate the gain for this sale.
      float market_gain = (sellrate - buyrate) * quantity;
      int day = 0 - (hour / 24);
      days_gains [day] += market_gain;
    }
    
    // Create output for the report.
    for (int day = 0; day >= -7; day--) {
      float gain = days_gains [day];
      output.push_back ({"Day", (day == 0 ? " " : "") + to_string (day), "gain", to_string (gain), bitcoin_id ()});
    }
    
    // Storage for the potential negative gain.
    float total_potential = 0;
    
    // Get all bid rates and store them for fast access.
    // Store them as container [exchange+market+coin] = bid;
    map <string, float> exchange_market_coin_bids;
    {
      vector <int> seconds;
      vector <string> exchanges, markets, coins;
      vector <float> bids, asks, rates;
      rates_get (sql_front, seconds, exchanges, markets, coins, bids, asks, rates, true);
      for (unsigned int i = 0; i < bids.size (); i++) {
        string exchange = exchanges[i];
        string market = markets[i];
        string coin = coins [i];
        float bid = bids[i];
        exchange_market_coin_bids [exchange + market + coin] = bid;
      }
    }
    
    {
      // Read the current balances from the database.
      vector <string> exchanges, coins;
      vector <float> totals;
      balances_get_current (sql_back, exchanges, coins, totals);
      for (unsigned int i = 0; i < exchanges.size(); i++) {
        string exchange = exchanges [i];
        string coin = coins [i];
        float total = totals [i];
        // The key for accessing the maps.
        string key = exchange + bitcoin_id () + coin;
        // Proceed if the price is known this coin was bought for.
        float bought_rate = bought_rates [key];
        // The potential negative gain is the difference
        // between the price the coin was bought for,
        // and the price it could now be sold for again.
        if (bought_rate == 0) continue;
        float current_bid = exchange_market_coin_bids [key];
        float potential_gain = total * (current_bid - bought_rate);
        // Add this gain to the total potential gain.
        total_potential += potential_gain;
      }
    }
    
    // Create the output for the potential negative gain.
    output.push_back ({"Potential gain", to_string (total_potential), bitcoin_id ()});
  }
  
  
  if (mail) {
    // Output to email.
    to_email mail (title);
    for (auto line : output) {
      mail.add ({line});
    }
  } else {
    // Output to the terminal.
    to_tty ({title});
    for (auto line : output) {
      to_tty ({line});
    }
  }
}


void load_bought_coins_details ()
{
  SQL sql (front_bot_ip_address ());
  vector <int> ids, hours;
  vector <string> exchanges, markets, coins;
  vector <float> rates;
  bought_get (sql, ids, hours, exchanges, markets, coins, rates);
  // The database contains older rates too.
  // The data is fetched in the order it was entered.
  // So newer rates for the same coin will overwrite older rates.
  // This way the bot always has the newest rate a coin was bought for.
  for (unsigned int i = 0; i < ids.size(); i++) {
    // Store them in this format: container [exchange+market+coin] = value;
    string key = exchanges[i] + markets[i] + coins[i];
    bought_rates [key] = rates [i];
    bought_hours [key] = hours [i];
  }
}


void delayed_transfers_reporter (bool mail)
{
  // The report's title.
  string title = "Delayed transfers";

  // The lines to appear in the report.
  vector <string> lines;

  // Database access.
  SQL sql (front_bot_ip_address ());
  
  {
    // Get the orders that were placed but have not yet been executed by the exchange.
    vector <string> pending_order_ids;
    {
      vector <string> v_dummy;
      vector <float> f_dummy;
      transfers_get_non_executed_orders (sql, pending_order_ids, v_dummy, v_dummy, v_dummy, f_dummy);
    }
    
    // Iterate over the pending withdrawal orders.
    for (auto & order_id : pending_order_ids) {
      // Get the order details.
      string stamp, withdraw, coin, txid, deposit, rxcoin, address, rxid;
      int seconds = 0;
      float quantity = 0;
      transfers_get_order_details (sql, order_id, stamp, seconds, withdraw, coin, quantity, txid, deposit, rxcoin, address, rxid);
      int elapsed_hours = (seconds_since_epoch () - seconds) / 3600;
      string message = "Withdrawal of " + float2string (quantity) + " " + coin + " from " + withdraw + " has been delayed for " + to_string (elapsed_hours) + " hours - order ID " + order_id + " - address " + address + " - txid " + txid;
      
      // Email report: Only warn after an hour or so.
      // There's coins like Ripple that should transfer within seconds, so it's good to notice delays on that soon,
      // rather than only report them after 24+ hours.
      // Terminal report: Output all pending withdrawals, no matter their delays.
      if ((elapsed_hours > 1) || !mail) {
        lines.push_back ("");
        lines.push_back (message);
      }
    }
  }

  {
    // Get all transfer orders of the past month.
    vector <string> order_ids, coin_abbrevs, txexchanges, txids, executeds, rxexchanges, addresses, visibles, arriveds;
    vector <int> db_ids, seconds;
    vector <float> txquantities, rxquantities;
    transfers_get_month (sql, db_ids, seconds, order_ids, coin_abbrevs, txexchanges, txquantities, txids, executeds, rxexchanges, addresses, visibles, rxquantities, arriveds);
    // Iterate over the most recent transfers.
    for (unsigned int db = 0; db < db_ids.size(); db++) {
      // Skip orders that have not executed yet, as they cannot have arrived yet.
      if (executeds [db].empty ()) continue;
      // Skip orders that have arrived.
      if (!arriveds [db].empty ()) continue;
      // Give a warning if a certain deposit has not yet arrived after a while.
      int second = seconds[db];
      int elapsed_hours = (seconds_since_epoch () - second) / 3600;
      // Email report: Only warn after an hour or so.
      // Terminal report: Output all pending withdrawals, no matter their delays.
      if ((elapsed_hours > 1) || !mail) {
        // Get the coin identifier.
        // Currently the database contains two coin abbreviations. Usually these two are the same.
        // But there's cases that the coin abbreviations are different.
        // An example of this is BCC or BCH. Both are used for Bitcoin Cash.
        string coin = exchange_get_coin_id (txexchanges[db], coin_abbrevs[db]);
        // Feedback.
        lines.push_back ("");
        lines.push_back ("A deposit of " + float2string (txquantities[db]) + " " + coin + " has not yet arrived at " + rxexchanges[db] + " for " + to_string (elapsed_hours) + " hours");
        lines.push_back ("Order id: " + order_ids[db]);
        lines.push_back ("Withdrawn from " + txexchanges[db]);
        lines.push_back ("Coin abbreviation withdrawal: " + coin_abbrevs[db]);
        lines.push_back ("Quantity: " + float2string (txquantities[db]));
        lines.push_back ("Deposited to " + rxexchanges[db]);
        string visible = visibles[db];
        if (!visible.empty ()) lines.push_back ("Deposit became visible " + visible + " minutes after withdrawal");
        else lines.push_back ("Deposit not yet visible");
        lines.push_back ("Address: " + addresses[db]);
        lines.push_back ("Transfer id withdrawal: " + txids[db]);
      }
    }
  }
  
  if (lines.empty ()) return;
  
  if (mail) {
    // Output to email.
    to_email mail (title);
    for (auto line : lines) {
      mail.add ({line});
    }
  } else {
    // Output to the terminal.
    to_tty ({title});
    for (auto line : lines) {
      to_tty ({line});
    }
  }
}


void arbitrage_performance_page_generator (int max_threads)
{
  SQL sql (front_bot_ip_address ());

  Html html ("Performance", "");
  html.h (1, "Performance");

  int thread_count = 0;
  
  valuation_map.clear ();
  
  html.h (2, "Coins");

  // Go over the arbitraging exchanges/markets/coins as defined in the database.
  // This does not consider the amount of trades done.
  // It also does not consider the amount of transfers done.
  vector <string> markets_done;
  for (size_t i = 0; i < trading_exchange1s.size(); i++) {

    // Wait till there's an available slot for this new thread.
    while (thread_count >= max_threads) this_thread::sleep_for (chrono::milliseconds (10));
    
    // Start the thread.
    string exchange1 = trading_exchange1s[i];
    string exchange2 = trading_exchange2s[i];
    string market = trading_markets[i];
    string coin = trading_coins[i];
    new thread (arbitrage_coin_performance_generator, exchange1, exchange2, market, coin, ref (thread_count));
    
    // Wait shortly to give the previously started thread time to update the thread count.
    this_thread::sleep_for (chrono::milliseconds (1));
    
    // The base market may have transfer losses, so list it too, even though it has no trades.
    string key = exchange1 + exchange2 + market;
    if (!in_array (key, markets_done)) {
      new thread (arbitrage_coin_performance_generator, exchange1, exchange2, market, market, ref (thread_count));
      markets_done.push_back (key);
    }
  }

  // Wait till all threads have completed.
  while (thread_count > 0) this_thread::sleep_for (chrono::milliseconds (10));
  
  // Write the reported items to the html page.
  // Sorted on worst performing coins at the top.
  // They are at the top because the need most attention.
  // The better performing coins need less attention.
  for (auto element : valuation_map) {
    generator_output fragment = element.second;
    html.h (3, fragment.title);
    for (auto s : fragment.lines) {
      html.p ();
      html.txt (s);
    }
    for (auto img : fragment.images) {
      html.p ();
      html.img (img);
    }
  }
  
  valuation_map.clear ();
  
  html.h (2, "Exchange pairs");

  {
    vector <string> keys;
    for (unsigned int i = 0; i < trading_exchange1s.size (); i++) {
      string exchange1 = trading_exchange1s[i];
      string exchange2 = trading_exchange2s[i];
      string market = trading_markets[i];
      string key = exchange1 + exchange2 + market;
      if (in_array (key, keys)) continue;
      keys.push_back (key);
      html.p ();
      int count;
      float gain;
      trades_get_statistics_last_24_hours (sql, exchange1, market, count, gain);
      string line = exchange1 + " and " + exchange2 + " traded " + to_string (count) + " times and gained " + float2string (gain) + " " + market;
      html.txt (line);
    }
  }
  
  html.p ();
  html.txt (generator_page_timestamp ());

  // Output.
  html.save ("performance.html");
}


void arbitrage_coin_performance_generator (string exchange1, string exchange2, string market, string coin, int & thread_count)
{
  generator_mutex.lock ();
  thread_count++;
  generator_mutex.unlock ();

  to_tty ({exchange1, exchange2, market, coin});

  SQL sql_front (front_bot_ip_address ());
  SQL sql_back (back_bot_ip_address ());

  // Storage for the output specific to the current coin.
  generator_output textfragment;

  int current_second = seconds_since_epoch ();

  // The title.
  textfragment.title = coin;

  // The coin.
  string coin_abbrev = exchange_get_coin_abbrev ("", coin);
  string coin_name = exchange_get_coin_name ("", coin);
  textfragment.lines.push_back ("Coin: " + coin + " " + coin_name + " " + coin_abbrev);
  
  // The exchanges and the market.
  textfragment.lines.push_back ("Exchanges: " + exchange1 + " " + exchange2);
  textfragment.lines.push_back ("Market: " + market);

  // How often this coin was transferred per day.
  unordered_map <int, int> transfer_counts;
  // The volume of the coin transferred per day.
  unordered_map <int, float> transfer_volumes;
  // The number of coins lost due to the transfer, per day.
  unordered_map <int, float> transfer_losses;
  // Obtain the values.
  for (unsigned int i = 0; i < transfers_seconds.size(); i++) {
    // It used to check on the sending and the receiving exchange.
    // But this does not work well in cases of balancing coins over multiple exchanges.
    // It now checks only balancing on both arbitraging exchanges.
    // Rather it should now record all balancing here, no matter what.
    // So the following update was made:
    // To no longer check on the sending and receiving exchange,
    // but rather to record transfers between any exchange.
    // string txexchange = transfers_txexchanges[i];
    // if ((txexchange == exchange1) || (txexchange == exchange2)) {
      // string rxexchange = transfers_rxexchanges[i];
      // if ((rxexchange == exchange1) || (rxexchange == exchange2)) {
        if (coin_abbrev == transfers_coin_abbrevs[i]) {
          int seconds = transfers_seconds[i] - current_second;
          int wholedays = seconds / 3600 / 24;
          transfer_counts[wholedays]++;
          float txquantity = transfers_txquantities[i];
          transfer_volumes[wholedays] += txquantity;
          float rxquantity = transfers_rxquantities[i];
          if (rxquantity > 0) {
            float loss = txquantity - rxquantity;
            transfer_losses[wholedays] += loss;
          }
        }
      //}
    //}
  }

  // How often this coin was transferred.
  if (!transfer_counts.empty ()) {
    string line = "Transfer count 30 days ago to now";
    for (int day = -30; day <= 0; day++) {
      line.append (" ");
      line.append (to_string (transfer_counts[day]));
    }
    textfragment.lines.push_back (line);
  }

  // The volume transferred.
  if (!transfer_volumes.empty ()) {
    string line = "Transfer volume";
    for (int day = -30; day <= 0; day++) {
      line.append (" ");
      float volume = transfer_volumes[day];
      if (volume > 0) line.append (float2visual (volume));
      else line.append ("0");
    }
    textfragment.lines.push_back (line);
  }

  // The transfer loss.
  if (!transfer_losses.empty ()) {
    string line = "Transfer loss";
    for (int day = -30; day <= 0; day++) {
      line.append (" ");
      float loss = transfer_losses[day];
      if (loss > 0) {
        float percentage = loss / transfer_volumes[day] * 100;
        line.append (float2visual (percentage) + "%");
      } else line.append ("0%");
    }
    textfragment.lines.push_back (line);
  }

  // How often this coin was traded.
  unordered_map <int, int> trade_counts;
  // The market gains made.
  unordered_map <int, float> market_gains;

  // Extract the values from the trade statistics.
  for (unsigned int i = 0; i < trade_seconds.size(); i++) {
    if (coin == trade_coins[i]) {
      if (trade_markets[i] == market) {
        int negative_seconds = trade_seconds[i] - current_second;
        int wholedays = negative_seconds / 3600 / 24;
        trade_counts [wholedays]++;
        float market_gain = trade_marketgains[i];
        market_gains[wholedays] += market_gain;
      }
    }
  }

  // The daily rates of the coin over 30 days.
  unordered_map <int, float> average_daily_rates;
  // The average daily balances of the coin over the 30 days.
  unordered_map <int, float> average_daily_balances;
  {
    unordered_map <int, int> item_count;
    for (unsigned int i = 0; i < balances_seconds.size(); i++) {
      if ((exchange1 == balances_exchanges[i]) || (exchange2 == balances_exchanges[i])) {
        if (coin == balances_coins[i]) {
          int second = balances_seconds[i] - current_second;
          int wholeday = second / 3600 / 24;
          float total = balances_totals[i];
          float rate = balances_bitcoins[i] / total;
          if (rate > 0) {
            if (!isinf (rate)) {
              if (!isnan (rate)) {
                item_count [wholeday]++;
                average_daily_rates [wholeday] += rate;
                average_daily_balances [wholeday] += total;
              }
            }
          }
        }
      }
    }
    for (int day = 0; day >= -30; day--) {
      average_daily_rates [day] /= item_count [day];
      average_daily_balances [day] /= item_count [day];
    }
  }

  // Various calculations.
  unordered_map <int, float> market_gain_minus_transfer_loss;
  for (int day = 0; day >= -30; day--) {
    float total_gain = 0;
    float gain = market_gains[day];
    if (!isnan (gain)) total_gain += gain;
    gain = transfer_losses[day] * average_daily_rates [day];
    if (!isnan (gain)) total_gain -= gain;
    market_gain_minus_transfer_loss [day] = total_gain;
  }
  
  {
    string line = "Trade count";
    for (int day = -30; day <= 0; day++) {
      line.append (" ");
      int count = trade_counts[day];
      line.append (to_string (count));
    }
    textfragment.lines.push_back (line);
  }
  {
    vector <float> gains;
    string line = "Market gains";
    for (int day = -30; day <= 0; day++) {
      line.append (" ");
      float gain = market_gains[day];
      if (gain > 0) {
        line.append (float2visual (gain));
        gains.push_back (gain);
      }
      else line.append ("0");
    }
    float average = accumulate (gains.begin(), gains.end(), 0.0) / gains.size();
    if (isnan (average)) average = 0;
    line.append (" average " + float2visual (average));
    textfragment.lines.push_back (line);
  }
  {
    vector <float> gains;
    string line = "Market gains minus transfer loss";
    for (int day = -30; day <= 0; day++) {
      line.append (" ");
      float gain = market_gain_minus_transfer_loss[day];
      if (gain != 0) {
        line.append (float2visual (gain));
        gains.push_back (gain);
      }
      else line.append ("0");
    }
    float average_market_gain = accumulate (gains.begin(), gains.end(), 0.0) / gains.size();
    if (isnan (average_market_gain)) average_market_gain = 0;
    line.append (" average " + float2visual (average_market_gain) + " " + market);
    textfragment.lines.push_back (line);
  }
  // It used also to output the effectve gains with the valuation gain subtracted.
  // But since the valuation gain or loss varies wildly, this information is confusing.
  // And if a coin is kept long enough, any loss due to coin devaluation
  // will be gained again once the coin increases in value again.

  // Is the coin being redistributed?
  for (auto exchange : {exchange1, exchange2}) {
    bool balancing = wallets_get_balancing (sql_front, exchange, coin);
    if (balancing) {
      textfragment.lines.push_back ("This coin is being automatically redistributed on " + exchange);
    } else {
      textfragment.lines.push_back ("This coin is not being redistributed on " + exchange);
    }
  }
  
  // Get the average balance for the last few hours.
  float average_coin_balance1 = balances_get_average_hourly_balance (sql_back, exchange1, coin, 5, 0);
  float average_btc_equivalent1 = balances_get_average_hourly_btc_equivalent (sql_back, exchange1, coin, 5, 0);
  float average_rate1 = rate_get_cached (exchange1, market, coin);
  float average_market_equivalent1 = average_coin_balance1 * average_rate1;
  float average_coin_balance2 = balances_get_average_hourly_balance (sql_back, exchange2, coin, 5, 0);
  float average_btc_equivalent2 = balances_get_average_hourly_btc_equivalent (sql_back, exchange2, coin, 5, 0);
  float average_rate2 = rate_get_cached (exchange2, market, coin);
  float average_market_equivalent2 = average_coin_balance2 * average_rate2;
  textfragment.lines.push_back ("Balance: " + exchange1 + " / " + exchange2 + ": " + float2string (average_coin_balance1) + " / " + float2string (average_coin_balance2) + " " + coin + " " + float2string (average_btc_equivalent1) + " / " + float2string (average_btc_equivalent2) + " " + bitcoin_id ()+ " " + float2string (average_market_equivalent1) + " / " + float2string (average_market_equivalent2) + " " + market);
  
  // Create the effective gains graph over one month.
  {
    vector <pair <float, float> > plotdata;
    for (int day = -30; day <= 0; day++) {
      float gain = market_gain_minus_transfer_loss[day];
      plotdata.push_back (make_pair (day, gain));
    }
    string title = "gains-" + exchange1 + "-" + exchange2 + "-" + market + "-" + coin;
    string picturepath;
    vector <pair <float, float> > flatdata = { make_pair (-30, 0), make_pair (0, 0)};
    plot (plotdata, flatdata, title, "zero", picturepath);
    string contents = file_get_contents (picturepath);
    string path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
    file_put_contents (path, contents);
    textfragment.images.push_back (basename (picturepath));
  }

  // Create the rates graph over one month.
  // These many days help to view the coin's long-term development.
  // This aids in making better decisions as to when to buy or sell this coin.
  textfragment.images.push_back (generator_chart_rates (exchange1, exchange2, market, coin, 30));

  // Add any comments.
  generator_list_comments (coin, textfragment.lines);

  // Add a graph describing the available and realized arbitrage amounts.
  {
    // The bundles of data for the plotter.
    vector <vector <pair <float, float> > > datasets;
    vector <string> titles;
    vector <pair <float, float> > availableplotdata, realizedplotdata;

    // Pick out the relevant amounts and store them for the plotters.
    for (size_t i = 0; i < availables_seconds.size(); i++) {
      if (coin != availables_coins[i]) continue;
      if (market != availables_markets[i]) continue;
      bool positive = ((exchange1 == availables_sellers[i]) && (exchange2 == availables_buyers[i]));
      bool negative = ((exchange1 == availables_buyers[i]) && (exchange2 == availables_sellers[i]));
      if (!positive && !negative) continue;
      int second = availables_seconds[i];
      float day = float (second - current_second) / 24 / 3600;
      float bidsize = availables_bidsizes[i];
      float asksize = availables_asksizes[i];
      float available = min (bidsize, asksize);
      float realized = availables_realizeds[i];
      if (negative) {
        available = 0 - available;
        realized = 0 - realized;
      }
      availableplotdata.push_back (make_pair (day, available));
      realizedplotdata.push_back (make_pair (day, realized));
    }
    string title = "-" + exchange1 + "-" + exchange2 + "-" + market + "-" + coin;
    datasets.push_back (availableplotdata);
    titles.push_back ("available" + title);
    datasets.push_back (realizedplotdata);
    titles.push_back ("realized" + title);

    // Run the plotter.
    string picturepath;
    plot2 (datasets, titles, picturepath);

    // Store the file into place in the website.
    string contents = file_get_contents (picturepath);
    unlink (picturepath.c_str());
    string path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
    file_put_contents (path, contents);
    textfragment.images.push_back (basename (picturepath));
  }
  
  // Add the bundle to the sorted output bundle.
  // Enesure unique keys so there's no loss of information in case coins have identical keys.
  // The system will sort on the gains as expressed in Bitcoins.
  // The advantage of this system is that it is independent of the market coin.
  // Because the number of Dogecoins gained will be far more than the number of e.g. Litecoins gained.
  // So expressing the gain in Bitcoins is needed for proper sorting.
  float total_market_gain_five_days = 0;
  for (int day = -5; day <= 0; day++) {
    total_market_gain_five_days += market_gain_minus_transfer_loss[day];
  }
  float btc_rate = 1;
  {
    if (market != bitcoin_id ()) {
      float btc_rate1 = rate_get_cached (exchange1, bitcoin_id (), market);
      float btc_rate2 = rate_get_cached (exchange2, bitcoin_id (), market);
      btc_rate = (btc_rate1 + btc_rate2) / 2;
    }
  }
  float key = total_market_gain_five_days * btc_rate;
  if ((isnan (key)) || (isinf (key)) || (key == 0)) key = -0.100001;
  key = generator_unique_key (key);
  generator_mutex.lock ();
  valuation_map [key] = textfragment;
  generator_mutex.unlock ();

  // One producer has completed.
  generator_mutex.lock ();
  thread_count--;
  generator_mutex.unlock ();
}


void arbitrage_opportunities_page_generator (int max_threads)
{
  SQL sql (back_bot_ip_address ());
  
  Html html ("Arbitrage opportunities", "");
  html.h (1, "Arbitrage opportunities");
  
  // Read all available arbitrage data from the database.
  vector <int> minutes;
  vector <string> markets;
  vector <string> sellers;
  vector <string> coins;
  vector <float> asks;
  vector <string> buyers;
  vector <float> bids;
  vector <int> percentages;
  vector <float> volumes;
  arbitrage_get (sql, minutes, markets, sellers, coins, asks, buyers, bids, percentages, volumes);

  // Write links to the following sections per market.
  set <string> distinct_markets;
  for (auto market : markets) distinct_markets.insert (market);
  for (auto market : distinct_markets) {
    html.p ();
    html.a ("#" + market, market);
  }

  // Iterate over the distinct markets to process them.
  for (auto current_market : distinct_markets) {

    html.h (2, "Opportunities market " + current_market, current_market);

    valuation_map.clear ();
    
    // Storage for the market trade volume for the exchanges + market + coin.
    unordered_map <string, float> exchanges_market_coin_volume;
    for (unsigned int i = 0; i < minutes.size(); i++) {
      string market = markets[i];
      if (market != current_market) continue;
      // The two exchanges.
      string seller = sellers[i];
      string buyer = buyers[i];
      string exchange1, exchange2;
      if (seller.compare (buyer) > 0) {
        exchange1 = buyer;
        exchange2 = seller;
      } else {
        exchange1 = seller;
        exchange2 = buyer;
      }
      // The coin.
      string coin = coins[i];
      // Add the volume.
      float volume = volumes[i];
      string key = exchange1 + "_" + exchange2 + "_" + market + "_" + coin;
      exchanges_market_coin_volume [key] += volume;
    }
    
    // Storage for sorting out the best opportunities.
    map <float, string> volume_exchanges_market_coin;
    for (auto element : exchanges_market_coin_volume) {
      // Use negative volumes to get the desired sorting in the map.
      float volume =  0 - element.second;
      // The exchanges/market/coin.
      string exchanges_market_coin = element.first;
      volume_exchanges_market_coin [volume] = exchanges_market_coin;
    }
    
    // List coins that can be traded with profit on exchange pairs and their markets.
    {
      int thread_count = 0;
      int opportunities_count = 0;
      
      // Iterate over the opportunities, best volumes first.
      for (auto element : volume_exchanges_market_coin) {
        
        vector <string> items = explode (element.second, '_');
        if (items.size () != 4) continue;
        
        // The two paired exchanges.
        string exchange1 = items [0];
        string exchange2 = items [1];
        
        // The market.
        string market12 = items [2];
        
        // The coin.
        string coin = items [3];
        
        // The maximum obtainable volume under ideal circumstances.
        float volume = 0 - element.first;
        
        // Wait till there's an available slot for this new thread.
        while (thread_count >= max_threads) this_thread::sleep_for (chrono::milliseconds (10));
        
        // Start the thread.
        new thread (arbitrage_coin_opportunity_generator, exchange1, exchange2, market12, coin, volume, ref (thread_count), ref (opportunities_count));
        
        // Wait shortly to give the previously started thread time to update the thread count.
        this_thread::sleep_for (chrono::milliseconds (1));
      }
      
      // Wait till all threads have completed.
      while (thread_count > 0) this_thread::sleep_for (chrono::milliseconds (10));
      
      // Write the reported items to the html page.
      // Sorted on best coins at the top.
      map <float, generator_output> ::reverse_iterator rit;
      for (rit = valuation_map.rbegin(); rit != valuation_map.rend(); rit++) {
        generator_output fragment = rit->second;
        html.h (3, fragment.title);
        for (auto s : fragment.lines) {
          html.p ();
          html.txt (s);
        }
        for (auto img : fragment.images) {
          html.p ();
          html.img (img);
        }
      }
      
    }

  }

  html.p ();
  html.txt (generator_page_timestamp ());

  // Output.
  html.save ("opportunities.html");
}


void arbitrage_coin_opportunity_generator (string exchange1, string exchange2,
                                           string market, string coin,
                                           float volume,
                                           int & thread_count,
                                           int & opportunities_count)
{
  generator_mutex.lock ();
  thread_count++;
  generator_mutex.unlock ();

  // Limit the number of opportunities to list.
  if (opportunities_count <= 100) {

    // Is this already being traded?
    bool being_traded = false;
    for (size_t i = 0; i < trading_exchange1s.size(); i++) {
      if ((exchange1 != trading_exchange1s[i]) && (exchange2 != trading_exchange1s[i])) continue;
      if ((exchange1 != trading_exchange2s[i]) && (exchange2 != trading_exchange2s[i])) continue;
      if (market != trading_markets[i]) continue;
      if (coin != trading_coins[i]) continue;
      being_traded = true;
    }
    if (!being_traded) {
      // New opportunity is here.

      // Storage for the output specific to the current coin.
      generator_output textfragment;
      
      textfragment.title = coin;
      to_tty ({"opportunity", coin, "@", market, "@", exchange1, "<>", exchange2});
      
      // Output.
      textfragment.lines.push_back (coin + " @ " + market + " @ " + exchange1 + " <> " + exchange2 + " could gain " + float2visual (volume) + " " + market + " over time");

      // Add the graph.
      textfragment.images.push_back (generator_chart_rates (exchange1, exchange2, market, coin, 30));

      // Output several of the best new opportunities.
      opportunities_count++;
      
      // Add any comments.
      generator_list_comments (coin, textfragment.lines);
      
      // Add the bundle to the sorted output bundle.
      // Enesure unique keys so there's no loss of information in case coins have identical keys.
      float key = volume;
      key = generator_unique_key (key);
      generator_mutex.lock ();
      valuation_map [key] = textfragment;
      generator_mutex.unlock ();
    }
  }
  
  // One producer has completed.
  generator_mutex.lock ();
  thread_count--;
  generator_mutex.unlock ();
}


string generator_chart_rates (string exchange1, string exchange2, string market, string coin, int days)
{
  vector <pair <float, float> > plotdata1;
  plotdata1 = chart_rates (exchange1, market, coin, days);
  vector <pair <float, float> > plotdata2;
  plotdata2 = chart_rates (exchange2, market, coin, days);
  string title1 = "rates-" + exchange1 + "-" + market + "-" + coin + "-" + to_string (days);
  string title2 = "rates-" + exchange2 + "-" + market + "-" + coin + "-" + to_string (days);
  string picturepath;
  plot (plotdata1, plotdata2, title1, title2, picturepath);
  string contents = file_get_contents (picturepath);
  string path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
  file_put_contents (path, contents);
  return basename (picturepath);
}


void generator_list_comments (string coin, vector <string> & lines)
{
  SQL sql (front_bot_ip_address ());
  vector <int> ids;
  vector <string> stamps, coins, comments;
  comments_get (sql, coin, ids, stamps, coins, comments);
  for (size_t i = 0; i < ids.size (); i++) {
    string line = stamps[i] + " " + comments[i];
    lines.push_back (line);
  }
}


string generator_page_timestamp ()
{
  return "Page generated at " + timestamp ();
}


void website_consistency_generator ()
{
  Html html ("Consistency", "");
  html.h (1, "Consistency");
  
  // Report balances at exchanges not doing arbitrage trade.
  int current_second = seconds_since_epoch ();
  vector <string> exchange_balances_reported;
  for (size_t i = 0; i < balances_exchanges.size(); i++) {

    // The age of the balances.
    // Should be young.
    int second = current_second - balances_seconds[i];
    if (second > 3600) continue;
    
    // Details.
    string exchange = balances_exchanges [i];
    string coin = balances_coins [i];
    float total = balances_totals [i];
    float bitcoin_equivalent = balances_bitcoins [i];
    
    // Skip balances that would be dust trade.
    bool dust = is_dust_trade (bitcoin_id (), bitcoin_equivalent, 1);
    if (dust) continue;

    // Check whether the coin trades or is a base market coin, at the exchange.
    bool coin_is_trading = false;
    for (size_t i2 = 0; i2 < trading_exchange1s.size(); i2++) {
      if ((exchange == trading_exchange1s [i2]) || (exchange == trading_exchange2s [i2])) {
        if (coin == trading_markets[i2]) coin_is_trading = true;
        if (coin == trading_coins[i2]) coin_is_trading = true;
      }
    }
    if (coin_is_trading) continue;
    
    // Check if it has been reported already.
    string key = exchange + coin;
    if (in_array (key, exchange_balances_reported)) continue;
    exchange_balances_reported.push_back (key);

    // Feedback.
    html.p ();
    html.txt ("The balance of " + float2visual (total) + " " + coin + " at " + exchange + " is not used for arbitrage trade");
  }

  html.p ();
  html.txt (generator_page_timestamp ());
  html.save ("consistency.html");
}


void website_notes_generator ()
{
  Html html ("Notes", "");
  html.h (1, "Notes");
  
  html.h (2, "Adding coin");
  html.p ();
  html.txt ("A coin rising in value is a good candidate to add.");
  html.p ();
  html.txt ("If the coin rates graph is ragged, it means there's more arbitrage possible there.");
  html.p ();
  html.txt ("It helps to first add the details to the ./pair without buying balance. This enables the operator to see if any trade would be possible on this arbitrage pair. It it works out well, the operator may decide to continue with this.");
  html.p ();
  html.txt ("The withdrawal fees should not be too high for proper arbitrage gain.");
  html.p ();
  html.txt ("What info does ./wallet give?");
  html.p ();
  html.txt ("What information does the withdrawal page at the exchange give?");
  html.p ();
  html.txt ("What information does the deposit page at the exchange give?");
  html.p ();
  html.txt ("Do block updates occur frequently?");
  html.p ();
  html.txt ("Are coin explorers operational?");
  html.p ();
  html.txt ("Does the difference in rate between the exchanges vary regularly? This indicates trade and arbitrage is being done, and so it is working");
  html.p ();
  html.txt ("Are the bid and ask prices close to each other? If not, arbitrage trading becomes harder.");
  html.p ();
  html.txt ("Do arbitrage on any market except the Bitcoin market. The Bitcoin market is overcrowded. The competition is too stiff so making a gain is very hard now. Usually the bot runs at a loss at this market. Later it was discovered that doing arbitrage on the Litecoin and Dogecoin markets gets hard too. Just the same as on the Bitcoin market.");

  html.h (2, "Running coin");
  html.p ();
  html.txt ("Have a sufficient balance of the coin for proper trading.");

  html.h (2, "Removing coin");
  html.p ();
  html.txt ("Don't sell a coin if it drops in value. Just keep it, it will rise again. So nothing is lost.");

  html.h (2, "Affecting gain");
  html.p ();
  html.txt ("If there's too many open orders at an exchange, this will negatively affect the gains. Update the trade easing factor so as to have fewer open orders. This is in function exchange_get_trade_order_ease_percentage.");
  html.p ();
  html.txt ("The minimum arbitrage_difference is a factor that affects the number of possible arbitrage trades, and the gain. If the factor is too low, the gain may become negative. If it is too high, there's fewer possible trades, so no gain is made too.");

  html.p ();
  html.txt (generator_page_timestamp ());
  html.save ("notes.html");
}
