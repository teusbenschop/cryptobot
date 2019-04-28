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
#include "shared.h"
#include "bittrex.h"
#include "cryptopia.h"
#include "bl3p.h"
#include "bitfinex.h"
#include "yobit.h"
#include "kraken.h"
#include "poloniex.h"
#include "tradesatoshi.h"
#include "exchanges.h"
#include "models.h"
#include "controllers.h"
#include "proxy.h"
#include "sqlite.h"
#include "simulators.h"
#include "generators.h"
#include "traders.h"
#include "html.h"


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);

  {
    cout << day_within_week () << endl;
  }
  /*
  {
    generator_load_data ();
    //website_index_generator ();
    arbitrage_performance_page_generator (6);
    //arbitrage_opportunities_page_generator (6);
    //balances_page_generator (6);
    //website_consistency_generator ();
    //website_notes_generator ();
  }
   */
  /*
  {
    SQL sql (front_bot_ip_address ());
    vector <string> exchanges1, exchanges2, markets, coins;
    pairs_get (sql, exchanges1, exchanges2, markets, coins);
    to_tty ({exchanges1});
    to_tty ({exchanges2});
    to_tty ({markets});
    to_tty ({coins});
  }
   */
  /*
  {
    SQL sql (front_bot_ip_address());
    string s = conversions_run (sql, "12xBwKyiujcBQeGNPXKwiDU518rJPuJXjz");
    cout << s << endl;
  }
   */
  /*
  {
    SQL sql (front_bot_ip_address ());
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
    for (unsigned int i = 0; i < minutes.size(); i++) {
      cout << minutes[i] << endl;
      cout << markets[i] << endl;
      cout << sellers[i] << endl;
      cout << coins[i] << endl;
      cout << asks[i] << endl;
      cout << buyers[i] << endl;
      cout << bids[i] << endl;
      cout << percentages[i] << endl;
      cout << volumes[i] << endl;
    }
  }
   */
  /*
  {
    string json, error, orderid;
    orderid = tradesatoshi_withdrawal_order ("dogecoin", 2000, "D6UG6hxSr4q9BAbg1bNydFQ6aeteRtxEmA", "", json, error);
    to_tty ({"json", json, "error", error, "orderId", orderid});
  }
   */
  /*
  {
    vector <string> exchanges = { "bittrex", "cryptopia", "bitfinex" };
    string coin = "bitcoin";

    vector <vector <pair <float, float> > > datasets;
    vector <string> titles;
    for (auto exchange : exchanges) {
      vector <pair <float, float> > plotdata = chart_balances ({exchange}, coin, 30, false, false);
      datasets.push_back (plotdata);
      string title = "balances-" + exchange + "-" + coin + "-month";
      titles.push_back (title);
    }
    string picturepath;
    plot2 (datasets, titles, picturepath);
    string contents = file_get_contents (picturepath);
    unlink (picturepath.c_str());
    string path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
    file_put_contents (path, contents);
  }
   */
  /*
  {
    SQL sql (front_bot_ip_address ());
    vector <int> ids;
    vector <string> exchanges, coins, addresses, paymentids, withdrawals, minwithdraws, greenlanes, comments;
    vector <bool> tradings, balancings;
    wallets_get (sql, ids, exchanges, coins, addresses, paymentids, withdrawals, tradings, balancings, minwithdraws, greenlanes, comments);
    for (unsigned int i = 0; i < ids.size(); i++) {
      cout << ids[i] << endl;
      cout << exchanges[i] << endl;
      cout << coins[i] << endl;
      cout << addresses[i] << endl;
      cout << paymentids[i] << endl;
      cout << withdrawals[i] << endl;
      cout << minwithdraws[i] << endl;
      cout << greenlanes[i] << endl;
      cout << comments[i] << endl;
      cout << (bool)tradings[i] << endl;
      cout << (bool)balancings[i] << endl;
    }
  }
   */
  /*
  {
    string exchange1 = "bittrex", exchange2 = "cryptopia";
    string coin = "blocknet";
    int current_second = seconds_since_epoch ();

    to_tty ({exchange1, exchange2, coin});

    to_tty ({"Reading balances..."});

    SQL sql (front_bot_ip_address ());

    vector <int> seconds;
    vector <string> exchanges;
    vector <string> coins;
    vector <float> totals;
    vector <float> bitcoins;
    vector <float> euros;
    balances_get_month (sql, seconds, exchanges, coins, totals, bitcoins, euros);
    to_tty ({"Balances available"});

    // Storage for all the fractional days thre's data for.
    vector <float> fractionaldays;
    
    // The average daily balances, stored per whole day.
    unordered_map <int, float> average_daily_balance;
    unordered_map <int, int> daily_balance_item_count;
    
    // Conversion map from fractional day to whole day.
    unordered_map <float, int> fractional2wholeday;
    
    // The rates stored as: container [fractional day] = rate.
    unordered_map <float, float> rates;
    
    // Sort all data out for this coin and exchange.
    for (unsigned int i = 0; i < seconds.size(); i++) {
      if ((exchange1 == exchanges[i]) || (exchange2 == exchanges[i])) {
        if (coin == coins[i]) {
          float second_f = seconds[i] - current_second;
          int second_i = seconds[i] - current_second;
          float fractionalday = second_f / 3600 / 24;
          if (!in_array (fractionalday, fractionaldays)) fractionaldays.push_back (fractionalday);
          int wholeday = second_i / 3600 / 24;
          fractional2wholeday [fractionalday] = wholeday;
          average_daily_balance [wholeday] += totals[i];
          daily_balance_item_count [wholeday]++;
          rates [fractionalday] = bitcoins[i] / totals[i];
        }
      }
    }

    // Calculate the average daily coin balance for all whole days.
    for (int day = -30; day <= 0; day++) {
      if (daily_balance_item_count [day]) {
        average_daily_balance [day] /= daily_balance_item_count [day];
      }
    }

    unordered_map <int, vector <float> > daily_valuation_gains;



    float previousrate = 0;
    for (unsigned int i = 0; i < fractionaldays.size(); i++) {
      float fractionday = fractionaldays [i];
      int wholeday = fractional2wholeday [fractionday];
      float rate = rates [fractionday];
      if (i) {
        float balance = average_daily_balance [wholeday];
        float rate_difference = rate - previousrate;
        float percentage_change = abs (get_percentage_change (previousrate, rate));
        if (percentage_change < 5) {
          float gain = balance * (rate_difference);
          daily_valuation_gains [wholeday].push_back (gain);
        }
      }
      previousrate = rate;
    }

    vector <pair <float, float> > plotdata;

    for (int day = -30; day <= 0; day++) {
      vector <float> day_gains = daily_valuation_gains[day];
      float gain = accumulate (day_gains.begin (), day_gains.end (), 0.0);
      plotdata.push_back (make_pair (day, gain));
      //to_tty ({to_string (day), float2visual (gain)});
    }

    to_tty ({"Plotting"});
    
    string title = "valuation-" + exchange1 + "-" + exchange2 + "-" + bitcoin_id () + "-" + coin;
    string picturepath;
    vector <pair <float, float> > flatdata = { make_pair (-30, 0), make_pair (0, 0)};
    plot (plotdata, flatdata, title, "zero", picturepath);
    string contents = file_get_contents (picturepath);
    string path = dirname (dirname (program_path)) + "/cryptodata/website/" + basename (picturepath);
    file_put_contents (path, contents);
  }
   */
  /*
  {
    SQL sql (front_bot_ip_address ());
    vector <string> order_ids, coin_ids, txexchanges, txids, executeds, rxexchanges, addresses, visibles, arriveds;
    vector <int> database_ids, seconds;
    vector <float> txquantities, rxquantities;
    transfers_get_month (sql, database_ids, seconds, order_ids, coin_ids, txexchanges, txquantities, txids, executeds, rxexchanges, addresses, visibles, rxquantities, arriveds);
    for (unsigned int i = 0; i < seconds.size(); i++) {
      cout << database_ids[i] << endl;
      cout << order_ids[i] << endl;
      cout << seconds[i] << endl;
      cout << coin_ids[i] << endl;
      cout << txexchanges[i] << endl;
      cout << txquantities[i] << endl;
      cout << txids[i] << endl;
      cout << executeds[i] << endl;
      cout << rxexchanges[i] << endl;
      cout << addresses[i] << endl;
      cout << visibles[i] << endl;
      cout << rxquantities[i] << endl;
      cout << arriveds[i] << endl;
    }
  }
  {
    SQL sql (front_bot_ip_address ());
    vector <int> seconds;
    vector <string> markets, coins, buyexchanges, sellexchanges;
    vector <float> quantities, marketgains;
    trades_get_month (sql, seconds, markets, coins, buyexchanges, sellexchanges, quantities, marketgains);
    for (unsigned int i = 0; i < seconds.size(); i++) {
      cout << seconds[i] << endl;
      cout << coins[i] << endl;
      cout << buyexchanges[i] << endl;
      cout << sellexchanges[i] << endl;
      cout << quantities[i] << endl;
      cout << marketgains[i] << endl;
    }
  }
   */
  /*
  {
    vector <int> surfing_ids, surfing_stamps;
    vector <string> surfing_coins, surfing_trades, surfing_statuses, surfing_remarks;
    vector <float> surfing_rates, surfing_amounts;
    
    SQL sql (front_bot_ip_address ());
    vector <int> ids, stamps;
    vector <string> coins, trades, statuses, remarks;
    vector <float> rates, amounts;
    surfer_get (sql, ids, stamps, coins, trades, statuses, rates, amounts, remarks);
    for (unsigned int i = 0; i < ids.size(); i++) {
      string status = statuses [i];
      if (status != surfer_status::executed) {
        surfing_ids.push_back (ids[i]);
        surfing_stamps.push_back (stamps[i]);
        surfing_coins.push_back (coins[i]);
        surfing_trades.push_back (trades[i]);
        surfing_statuses.push_back (statuses[i]);
        surfing_rates.push_back (rates[i]);
        surfing_amounts.push_back (amounts[i]);
        surfing_remarks.push_back (remarks[i]);
      }
      cout << (seconds_since_epoch () - stamps[i]) / 3600 / 24 << endl;
    }
  }
   */
  /*
  {
    to_output output ("");
    // Generate sinoidal testing rates over 60+ days.
    unordered_map <int, float> minute_asks, minute_bids, hour_asks, hour_bids;
    {
      vector <int> seconds;
      vector <float> asks, bids;
      int now = seconds_since_epoch ();
      double radians = 0;
      for (int second = now - 61 * 24 * 3600; second <= now; second += 60) {
        seconds.push_back (second);
        radians += 0.0005;
        float rate = sin (radians);
        rate += 50;
        asks.push_back (rate);
        bids.push_back (rate);
      }
      pattern_prepare (&output, seconds, asks, bids, minute_asks, minute_bids, hour_asks, hour_bids);
    }
    
    // Container for the forecast properties for all simulated hours.
    vector <forecast_properties> all_forecast_properties;
    
    // Overall weights for all factors taken in account for making a forecast.
    forecast_weights weights;

    learn_rates_process_observations (&output, "exchange", euro_id (), "coin", minute_asks, minute_bids, hour_asks, hour_bids, all_forecast_properties, weights);
    
    // After gathering all forecast properties, start optimizing the weights,
    // so as to get the most accurate forecast.
    vector <string> msg;
    float sum_absolute_residuals, maximum_residual_percentage, average_residual_percentage;
    optimize_forecast_weights (&output, all_forecast_properties, weights);

    msg = {
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

    // The final residuals calculation, with the optimized weights.
    get_forecast_residuals (&output, all_forecast_properties, weights, sum_absolute_residuals, maximum_residual_percentage, average_residual_percentage);
    
    msg = {
      "Sum residuals", float2string (sum_absolute_residuals),
      "Maximum residual", float2visual (maximum_residual_percentage), "%",
      "Average residual", float2visual (average_residual_percentage), "%",
    };
    output.add (msg);
  }
   */
  /*
  {
    float x_multiplier, x_addend, y_multiplier, y_addend;
    vector <float> xs = { 1, 2, 3, 4, 5, 6 };
    vector <float> ys = { 4, 5.1, 6.3, 7.7, 9.0, 12.0 };
    curve_type curve = curve_fit (xs, ys, x_multiplier, x_addend, y_multiplier, y_addend);
    to_tty ({curve_title (curve), "x_multiplier", float2string (x_multiplier), "x_addend", float2string (x_addend), "y_multiplier", float2string (y_multiplier), "y_addend", float2string (y_addend)});
    {
      vector <pair <float, float> > plotdata;
      for (unsigned int i = 0; i < xs.size(); i++) {
        plotdata.push_back (make_pair (xs[i], ys[i]));
      }
      string title = "observed_data";
      string picturepath;
      plot (plotdata, title, picturepath);
    }
    {
      vector <pair <float, float> > plotdata;
      for (auto x : xs) {
        float y = curve_calculate (curve, x, x_multiplier, x_addend, y_multiplier, y_addend);
        plotdata.push_back (make_pair (x, y));
      }
      string title = "fitted_" + curve_title (curve);
      string picturepath;
      plot (plotdata, title, picturepath);
    }
  }
   */
  /*
  {
    for (int iter = static_cast <int> (curve_type::begin) + 1; iter < static_cast <int> (curve_type::end); iter++)
    {
      curve_type curve = static_cast <curve_type> (iter);
      float x_multiplier, x_addend, y_multiplier, y_addend;
      vector <float> xs = { 1, 2, 3, 4, 5, 6 };
      vector <float> ys = { 4, 5.1, 6.3, 7.7, 9.0, 12.0 };
      float residual = curve_fit (xs, ys, curve, x_multiplier, x_addend, y_multiplier, y_addend);
      to_tty ({curve_title (curve), "x_multiplier", float2string (x_multiplier), "x_addend", float2string (x_addend), "y_multiplier", float2string (y_multiplier), "y_addend", float2string (y_addend), "residue", float2string (residual)});
      {
        vector <pair <float, float> > plotdata;
        for (unsigned int i = 0; i < xs.size(); i++) {
          plotdata.push_back (make_pair (xs[i], ys[i]));
        }
        string title = "observed_data";
        string picturepath;
        plot (plotdata, title, picturepath);
      }
      {
        vector <pair <float, float> > plotdata;
        for (auto x : xs) {
          float y = curve_calculate (curve, x, x_multiplier, x_addend, y_multiplier, y_addend);
          plotdata.push_back (make_pair (x, y));
        }
        string title = "fitted_" + curve_title (curve);
        string picturepath;
        plot (plotdata, title, picturepath);
      }
    }
  }
   */
  /*
  {
    for (int iter = static_cast <int> (curve_type::begin) + 1; iter < static_cast <int> (curve_type::end); iter++)
    {
      float x_multiplier = 1;
      float x_addend = 0;
      float y_multiplier = 1;
      float y_addend = 0;
      curve_type curve = static_cast <curve_type> (iter);
      vector <pair <float, float> > plotdata;
      for (int i = -10; i <= 10; i++) {
        float x = i;
        float y = curve_calculate (curve, x, x_multiplier, x_addend, y_multiplier, y_addend);
        plotdata.push_back (make_pair (x, y));
      }
      string title = curve_title (curve);
      string picturepath;
      plot (plotdata, title, picturepath);
    }
    string folder = dirname (dirname (program_path)) + "/cryptodata/graphs";
    string command = "open " + folder;
    system (command.c_str());
  }
   */
  /*
  {
    cout << bitcoin2euro (0.55) << endl; 
  }
   */
  /*
  {
    string exchange = yobit_get_exchange ();
    string market = bitcoin_id ();
    vector <string> abbreviations, identifiers, dummies;
    exchange_get_coins (exchange, market, abbreviations, identifiers, dummies, dummies);
    for (auto coin : identifiers) {
      to_tty ({coin});
    }
  }
   */

  /*
  {
    vector <string> coins;
    vector <float> bids;
    vector <float> asks;
    string error;
    exchange_get_market_summaries ("yobit", "waves", coins, bids, asks, &error);
    to_tty ({error.substr (0, 100)});
    for (unsigned int i = 0; i < coins.size(); i++) {
      to_tty ({coins[i], float2string (bids[i]), float2string (asks[i])});
    }
  }
   */
  /*
  {
    float rate = rate_get_cached ("bittrex", "bitcoin", "verge");
    cout << rate << endl;
  }
   */
  /*
  {
    vector <string> coins = exchange_get_coins_per_market_cached ("bitfinex", "usdollar");
    to_tty (coins);
  }
   */
  /*
  {
    to_output output ("");
    SQL sql (front_bot_ip_address ());
    map <string, map <string, float> > current_available_balances;
    current_available_balances ["tradesatoshi"] ["dogecoin"] = 354.2340087891;
    current_available_balances ["cryptopia"] ["bitcoin"] = 0.01;
    map <string, float> minimum_trade_sizes = mintrade_get (sql);
    vector <string> paused_trading_exchanges_markets_coins = pausetrade_get (sql);
    mutex update_mutex;
    multipath path;
    path.exchange = "cryptopia";
    path.market1name = "bitcoin";
    path.market1quantity = 0.01;
    path.ask1 = 0.0000194995;
    path.coin1name = "billionairetoken";
    path.coin1quantity = 10;
    int step = 1;
    multipath_place_limit_order (&output, &path, step, current_available_balances, update_mutex, minimum_trade_sizes, paused_trading_exchanges_markets_coins);
  }
   */
  /*
  {
    to_output output ("");
    string error, json, order_id;
    exchange_limit_buy ("cryptopia", "bitcoin", "billionairetoken", 10, 0.0000198999, error, json, order_id, &output);
  }
   */

  /*
  {
    SQL sql (front_bot_ip_address ());
    vector <string> profitable_exchanges_markets_coins_reasons;
    {
      vector <string> exchanges, markets, coins, reasons;
      patterns_get (sql, exchanges, markets, coins, reasons);
      for (unsigned int i = 0; i < exchanges.size (); i++) {
        string signature = exchanges [i] + markets [i] + coins [i] + reasons[i];
        profitable_exchanges_markets_coins_reasons.push_back (signature);
      }
    }
    vector <string> exchanges = exchange_get_names ();
    for (auto exchange : exchanges) {
      vector <string> markets = exchange_get_supported_markets (exchange);
      for (auto market : markets) {
        vector <string> coins = exchange_get_coin_ids (exchange);
        for (auto coin : coins) {
          if (exchange != "bitfinex") continue;
          if (market != "bitcoin") continue;
          //if (coin != "bitbay") continue;
          //continue;
          to_output output ("Patterns for " + coin + " @ " + market + " @ " + exchange);
          unordered_map <int, float> minute_asks, minute_bids, hour_asks, hour_bids;
          pattern_prepare (&output, exchange, market, coin, minute_asks, minute_bids, hour_asks, hour_bids);
          if (hour_asks.size () < 10) continue;
          string reason = pattern_type_increase_day_week ();
          bool coin_good = pattern_increase_day_week_simulator (&output, hour_asks, hour_bids, reason);
          {
            string signature = exchange + market + coin + reason;
            bool good_coin_stored = in_array (signature, profitable_exchanges_markets_coins_reasons);
            if (coin_good) {
              if (!good_coin_stored) patterns_store (sql, exchange, market, coin, reason);
            } else {
              if (good_coin_stored) patterns_delete (sql, exchange, market, coin, reason);
            }
          }
          //coin_good = true;
          coin_good = false;
          if (coin_good) {
            vector <pair <float, float> > plotdata;
            string picturepath;
            string basename = exchange + "-" + market + "-" + coin;
            for (int hour = -24 * 14; hour <= 0; hour++) {
              float rate = (hour_asks [hour] + hour_bids [hour]) / 2;
              if (rate > 0) {
                plotdata.push_back (make_pair (hour, rate));
              }
            }
            plot (plotdata, basename, picturepath);
          }
        }
      }
    }
  }
   */

  /*
  {
    bool cancelled = exchange_cancel_order (bittrex_get_exchange (), "59c9bfba-9c4b-4090-8aee-3e0aa374f3a1");
    cout << cancelled << endl;
  }
   */
  /*
  {
    SQL sql (front_bot_ip_address ());
    vector <int> seconds;
    vector <string> exchanges, markets, coins;
    vector <float> bids, asks, rates;
    bool only_recent = false;
    rates_get (sql, seconds, exchanges, markets, coins, bids, asks, rates, only_recent);
    for (unsigned int i = 0; i < rates.size(); i++) {
      cout << rates[i] << endl;
    }
    cout << rates.size() << endl;
  }
   */
  /*
  {
    set <tuple <string, int, float, int, string> > distinct_values;
    distinct_values.insert (make_tuple ("a", 1, 1.1, 1, "a"));
    distinct_values.insert (make_tuple ("b", 2, 2.2, 2, "b"));
    distinct_values.insert (make_tuple ("a", 1, 1.1, 1, "a"));
    for (auto & element : distinct_values) {
      cout << get<0>(element) << endl;
    }
  }
   */
  
  /*
  {
    SQL sql (front_bot_ip_address ());
    string exchange = "tradesatoshi";
    string market = "bitcoin";
    string coin = "litecoincash";
    to_output output ("");
    unordered_map <string, vector <int> > allseconds;
    unordered_map <string, vector <float> > allbids, allasks;
    patterns_rates_get (sql, allseconds, allbids, allasks);
    unordered_map <int, float> minute_asks, minute_bids, hour_asks, hour_bids;
    pattern_prepare (&output, exchange, market, coin,
                     allseconds, allbids, allasks,
                     minute_asks, minute_bids, hour_asks, hour_bids);
    for (auto element : minute_asks) {
      output.add ({"minute", to_string (element.first), "ask", float2string (element.second)});
    }
  }
   */
  /*
  {
    string exchange = "tradesatoshi";
    string market = "bitcoin";
    string coin = "bitblocks";
    to_output output ("");
    unordered_map <int, float> minute_asks, minute_bids, hour_asks, hour_bids;
    pattern_prepare (&output, exchange, market, coin, minute_asks, minute_bids, hour_asks, hour_bids);
    output.add ({"size", to_string (minute_asks.size())});
  }
   */
  /*
  {
    vector <string> identifier;
    vector <string> market;
    vector <string> coin_abbrev;
    vector <string> coin_ids;
    vector <string> buy_sell;
    vector <float> quantity;
    vector <float> rate;
    vector <string> date;
    exchange_get_open_orders ("bl3p", identifier, market, coin_abbrev, coin_ids, buy_sell, quantity, rate, date);
    for (unsigned int i = 0; i < identifier.size(); i++) {
      string coin_identifier = coin_ids [i];
      string market_id = market[i];
      cout << buy_sell[i] << " " << float2string (quantity[i]) << " " << coin_identifier << " at rate " << float2string (rate[i]) << " total " << float2string (quantity[i] * rate[i]) << " " << market_id << " date " << date[i] << endl;
    }
  }
   */
  /*
  {
    string market = "euro";
    string coin = "litecoin";
    vector <float> quantity;
    vector <float> rate;
    string error;
    bl3p_get_sellers (market, coin, quantity, rate, error, false);
    for (unsigned int i = 0; i < quantity.size(); i++) {
      cout << "ask " << float2string (quantity[i]) << " " << coin << " at " << float2string (rate[i]) << endl;
    }
    cout << error << endl;
  }
   */
  /*
  {
    vector <string> coins;
    vector <float> bids;
    vector <float> asks;
    string error;
    kraken_get_market_summaries (euro_id (), coins, bids, asks, error);
    to_output output ("");
    for (unsigned int i = 0; i < coins.size(); i++) {
      output.add ({coins[i], float2string (bids[i]), float2string (asks[i])});
    }
    output.add ({error});
  }
   */
  /*
  {
    vector <float> quantities;
    vector <float> rates;
    string error;
    tradesatoshi_get_sellers (bitcoin_id (), "litecoin", quantities, rates, error, true);
    for (unsigned int i = 0; i < quantities.size(); i++) {
      to_tty ({float2string (quantities[i]), float2string (rates[i])});
    }
    to_tty ({error});
  }
   */

  /*
  {
    vector <string> exchanges = exchange_get_names ();;
    for (unsigned int i = 0; i < 1; i++) {
      for (auto exchange : exchanges) {
        {
          to_output output ("");
          string coin = "verge";
          vector <float> quantities;
          vector <float> rates;
          string error;
          exchange_get_buyers_via_satellite (exchange, bitcoin_id (), coin, quantities, rates, true, &output);
          output.add ({exchange, "get buyers via satellite, got", to_string (quantities.size()), "results"});
          for (unsigned int i = 0; i < quantities.size(); i++) {
            if (exchange == "tradesatoshi") {
              output.add ({float2string (quantities[i]), float2string (rates[i])});
            }
          }
          output.add ({error});
        }
        {
          to_output output ("");
          string coin = "verge";
          vector <float> quantities;
          vector <float> rates;
          string error;
          exchange_get_sellers_via_satellite (exchange, bitcoin_id (), coin, quantities, rates, true, &output);
          output.add ({exchange, "get sellers via satellite, got", to_string (quantities.size()), "results"});
          for (unsigned int i = 0; i < quantities.size(); i++) {
            if (exchange == "tradesatoshi") {
              output.add ({float2string (quantities[i]), float2string (rates[i])});
            }
          }
          output.add ({error});
        }
      }
    }
  }
  */
  
  finalize_program ();
  return EXIT_SUCCESS;
}
