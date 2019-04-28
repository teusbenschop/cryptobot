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


#include "exchanges.h"
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
#include "models.h"
#include "proxy.h"


// Hurried call counters.
mutex exchange_hurried_mutex;
unordered_map <string, int> exchange_hurried_call_pass;
unordered_map <string, int> exchange_hurried_call_fail;


vector <string> exchange_get_names ()
{
  return {
    bittrex_get_exchange (),
    cryptopia_get_exchange (),
    bl3p_get_exchange (),
    bitfinex_get_exchange (),
    // There's many reviews of Yobit that complain about this exchange.
    // Many API calls return this:
    // Access denied | yobit.net used Cloudflare to restrict access
    // So often good trades fail due to restricted access.
    // So the bot is not using Yobit anymore.
    //yobit_get_exchange (),
    poloniex_get_exchange (),
    kraken_get_exchange (),
    tradesatoshi_get_exchange (),
  };
}


vector <string> exchange_get_coin_ids (string exchange)
{
  // Select the coin identifiers on the requested exchange.
  if (exchange == bittrex_get_exchange ()) {
    return bittrex_get_coin_ids ();
  };
  if (exchange == cryptopia_get_exchange ()) {
    return cryptopia_get_coin_ids ();
  };
  if (exchange == bl3p_get_exchange ()) {
    return bl3p_get_coin_ids ();
  };
  if (exchange == bitfinex_get_exchange ()) {
    return bitfinex_get_coin_ids ();
  };
  if (exchange == yobit_get_exchange ()) {
    return yobit_get_coin_ids ();
  };
  if (exchange == kraken_get_exchange ()) {
    return kraken_get_coin_ids ();
  };
  if (exchange == poloniex_get_exchange ()) {
    return poloniex_get_coin_ids ();
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    return tradesatoshi_get_coin_ids ();
  };
  // No exchange given: Provide all coin identifiers on all exchanges.
  if (exchange.empty()) {
    vector <string> all_ids;
    vector <string> exchanges = exchange_get_names ();
    for (auto exchange : exchanges) {
      vector <string> ids = exchange_get_coin_ids (exchange);
      all_ids.insert (all_ids .end(), ids.cbegin(), ids.cend());
    }
    // Select distinct coin ids.
    sort (all_ids.begin(), all_ids.end());
    all_ids.erase (unique (all_ids.begin(), all_ids.end ()), all_ids.end ());
    // Done.
    return all_ids;
  }
  // No coins here.
  return {};
}


string exchange_get_coin_id (const string & exchange, const string & coin)
{
  // If an exchange is given, return the translation given by that exchange.
  if (exchange == bittrex_get_exchange ()) {
    return bittrex_get_coin_id (coin);
  };
  if (exchange == cryptopia_get_exchange ()) {
    return cryptopia_get_coin_id (coin);
  };
  if (exchange == bl3p_get_exchange ()) {
    return bl3p_get_coin_id (coin);
  };
  if (exchange == bitfinex_get_exchange ()) {
    return bitfinex_get_coin_id (coin);
  };
  if (exchange == yobit_get_exchange ()) {
    return yobit_get_coin_id (coin);
  };
  if (exchange == kraken_get_exchange ()) {
    return kraken_get_coin_id (coin);
  };
  if (exchange == poloniex_get_exchange ()) {
    return poloniex_get_coin_id (coin);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    return tradesatoshi_get_coin_id (coin);
  };
  // If no exchange is given, iterate over all known exchange to find the coin identifier.
  if (exchange.empty ()) {
    vector <string> exchanges = exchange_get_names ();
    for (auto & exchange : exchanges) {
      string coin_id = exchange_get_coin_id (exchange, coin);
      if (!coin_id.empty ()) return coin_id;
    }
  }
  // Failed to find the coin identifier.
  return "";
}


string exchange_get_coin_abbrev (string exchange, string coin)
{
  // If an exchange is given, return the translation given by that exchange.
  if (exchange == bittrex_get_exchange ()) {
    return bittrex_get_coin_abbrev (coin);
  };
  if (exchange == cryptopia_get_exchange ()) {
    return cryptopia_get_coin_abbrev (coin);
  };
  if (exchange == bl3p_get_exchange ()) {
    return bl3p_get_coin_abbrev (coin);
  };
  if (exchange == bitfinex_get_exchange ()) {
    return bitfinex_get_coin_abbrev (coin);
  };
  if (exchange == yobit_get_exchange ()) {
    return yobit_get_coin_abbrev (coin);
  };
  if (exchange == kraken_get_exchange ()) {
    return kraken_get_coin_abbrev (coin);
  };
  if (exchange == poloniex_get_exchange ()) {
    return poloniex_get_coin_abbrev (coin);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    return tradesatoshi_get_coin_abbrev (coin);
  };
  // If no exchange is given, iterate over all known exchange to find the coin abbreviation.
  if (exchange.empty ()) {
    vector <string> exchanges = exchange_get_names ();
    for (auto & exchange : exchanges) {
      string coin_abbrev = exchange_get_coin_abbrev (exchange, coin);
      if (!coin_abbrev.empty ()) return coin_abbrev;
    }
  }
  // Failed to find the coin abbreviation.
  return "";
}


string exchange_get_coin_name (string exchange, string coin)
{
  // If an exchange is given, return the translation given by that exchange.
  if (exchange == bittrex_get_exchange ()) {
    return bittrex_get_coin_name (coin);
  };
  if (exchange == cryptopia_get_exchange ()) {
    return cryptopia_get_coin_name (coin);
  };
  if (exchange == bl3p_get_exchange ()) {
    return bl3p_get_coin_name (coin);
  };
  if (exchange == bitfinex_get_exchange ()) {
    return bitfinex_get_coin_name (coin);
  };
  if (exchange == yobit_get_exchange ()) {
    return yobit_get_coin_name (coin);
  };
  if (exchange == kraken_get_exchange ()) {
    return kraken_get_coin_name (coin);
  };
  if (exchange == poloniex_get_exchange ()) {
    return poloniex_get_coin_name (coin);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    return tradesatoshi_get_coin_name (coin);
  };
  // If no exchange is given, iterate over all known exchange to find the coin abbreviation.
  if (exchange.empty ()) {
    vector <string> exchanges = exchange_get_names ();
    for (auto & exchange : exchanges) {
      string coin_name = exchange_get_coin_name (exchange, coin);
      if (!coin_name.empty ()) return coin_name;
    }
  }
  // Failed to find the coin name.
  return "";
}


void exchange_get_coins (const string & exchange,
                         const string & market,
                         vector <string> & coin_abbrevs,
                         vector <string> & coin_ids,
                         vector <string> & coin_names,
                         vector <string> & exchange_coin_abbreviations,
                         string & error)
{
  if (exchange == bittrex_get_exchange ()) {
    // Get them from the API.
    bittrex_get_coins (market, coin_abbrevs, coin_ids, coin_names, error);
  };
  if (exchange == cryptopia_get_exchange ()) {
    // Get them from the API.
    cryptopia_get_coins (market, coin_abbrevs, coin_ids, coin_names, error);
  };
  if (exchange == bl3p_get_exchange ()) {
    // Get them from the API.
    bl3p_get_coins (coin_abbrevs, coin_ids, coin_names, error);
  };
  if (exchange == bitfinex_get_exchange ()) {
    // Get them from the API.
    bitfinex_get_coins (market, coin_abbrevs, coin_ids, coin_names, error);
  };
  if (exchange == yobit_get_exchange ()) {
    // The coin names are not available through the Yobit API.
    // It provides the coin abbreviations only.
    vector <string> yobit_abbreviations;
    vector <string> no_names;
    yobit_get_coins (market, yobit_abbreviations, error);
    // Try to obtain the full coin names and coin ids from the other exchanges.
    for (auto yobit_abbrev : yobit_abbreviations) {
      string coin_name = bittrex_get_coin_name (yobit_abbrev);
      string coin_id = bittrex_get_coin_id (yobit_abbrev);;
      if (coin_name.empty ()) {
        coin_name = cryptopia_get_coin_name (yobit_abbrev);
        coin_id = cryptopia_get_coin_id (yobit_abbrev);;
      }
      if (!coin_name.empty ()) {
        coin_abbrevs.push_back (yobit_abbrev);
        coin_ids.push_back (coin_id);
        coin_names.push_back (coin_name);
      }
    }
  };
  if (exchange == kraken_get_exchange ()) {
    // The asset names available from the Kraken API are different from what is used commonly.
    // The altername names from the API are more like the commonly known coin abbreviations.
    // So make the best of it by quering known coins to interprete the Kraken assets passed through the API.
    vector <string> kraken_assets;
    vector <string> kraken_altnames;
    kraken_get_coins (market, kraken_assets, kraken_altnames, error);
    for (unsigned int i = 0; i < kraken_assets.size(); i++) {
      string kraken_asset = kraken_assets[i];
      string kraken_altname = kraken_altnames[i];
      string coin_name = bittrex_get_coin_name (kraken_altname);
      string coin_id = bittrex_get_coin_id (kraken_altname);
      if (coin_name.empty ()) {
        coin_name = cryptopia_get_coin_name (kraken_altname);
        coin_id = cryptopia_get_coin_id (kraken_altname);
      }
      if (coin_name.empty ()) {
        // The Bitcoin is called "XBT" at Kraken.
        if (kraken_altname == "XBT") {
          coin_name = "Bitcoin";
          coin_id = bitcoin_id ();
        }
      }
      if (!coin_name.empty ()) {
        coin_abbrevs.push_back (kraken_altname);
        coin_ids.push_back (coin_id);
        coin_names.push_back (coin_name);
        // Also store the standard abbreviation Kraken uses for its API calls.
        exchange_coin_abbreviations.push_back (kraken_asset);
      }
    }
  }
  if (exchange == poloniex_get_exchange ()) {
    // Get the coin abbreviations from the API.
    vector <string> poloniex_coin_abbreviations;
    poloniex_get_coins (market, poloniex_coin_abbreviations, error);
    // It does not provide the coin names, so get these from the known coins at the other exchanges.
    for (auto poloniex_coin_abbrev : poloniex_coin_abbreviations) {
      string coin_name = cryptopia_get_coin_name (poloniex_coin_abbrev);
      string coin_id = cryptopia_get_coin_id (poloniex_coin_abbrev);
      if (coin_name.empty ()) {
        coin_name = bittrex_get_coin_name (poloniex_coin_abbrev);
        coin_id = bittrex_get_coin_id (poloniex_coin_abbrev);
      }
      if (!coin_name.empty ()) {
        coin_abbrevs.push_back (poloniex_coin_abbrev);
        coin_ids.push_back (coin_id);
        coin_names.push_back (coin_name);
      }
    }
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    // Get the coin abbreviations from the API.
    vector <string> statuses;
    tradesatoshi_get_coins (market, coin_abbrevs, coin_ids, coin_names, statuses, error);
  }
  // Error handler.
  to_stdout (exchange_format_error (error, __FUNCTION__, exchange, "", "", ""));
}


mutex coins_market_mutex;
unordered_map <string, unordered_map <string, vector <string> > > exchanges_markets_coins_cache;


vector <string> exchange_get_coins_per_market_cached (const string & exchange, const string & market)
{
  // The container that will hold the coins at this market at this excahnge.
  vector <string> coins;
  
  // Thread safety.
  // Also preventing multiple database calls,
  // in case multiple simultaneous requests take place,
  // while the data has not yet been read from the database.
  coins_market_mutex.lock ();
  
  // If the memory cache is empty,
  // read the data from the database,
  // and store it in that cache.
  if (exchanges_markets_coins_cache.empty ()) {
    SQL sql (front_bot_ip_address ());
    vector <string> exchanges, markets, coins;
    coins_market_get (sql, exchanges, markets, coins);
    for (unsigned int i = 0; i < exchanges.size(); i++) {
      string exchange = exchanges[i];
      string market = markets[i];
      exchanges_markets_coins_cache [exchange][market] = explode (coins [i], ' ');
    }
  }

  // Read the coins from the memory cache.
  coins = exchanges_markets_coins_cache [exchange][market];
  
  // Unlock again.
  coins_market_mutex.unlock ();
  
  // Pass the coins to the caller.
  return coins;
}


void exchange_get_wallet_details (string exchange, string coin,
                                  string & address, string & payment_id)
{
  address.clear ();
  payment_id.clear ();
  string error;
  string json;
  if (exchange == bittrex_get_exchange ()) {
    bittrex_get_wallet_details (coin, json, error, address, payment_id);
  };
  if (exchange == cryptopia_get_exchange ()) {
    cryptopia_get_wallet_details (coin, json, error, address, payment_id);
  };
  if (exchange == bl3p_get_exchange ()) {
    bl3p_get_wallet_details (coin, json, error, address, payment_id);
  };
  if (exchange == bitfinex_get_exchange ()) {
    bitfinex_get_wallet_details (coin, json, error, address, payment_id);
  };
  if (exchange == yobit_get_exchange ()) {
    yobit_get_wallet_details (coin, json, error, address, payment_id);
  };
  if (exchange == kraken_get_exchange ()) {
    kraken_get_wallet_details (coin, json, error, address, payment_id);
  };
  if (exchange == poloniex_get_exchange ()) {
    poloniex_get_wallet_details (coin, json, error, address, payment_id);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    // Getting the wallet details from TradeSatoshi works in a different way.
    // TradeSatoshi is not able to pass details of an existing wallet.
    // It only is able to generate a new address.
    // This is the case while writing this, August 2018.
    // So another method must be employed to simulate getting the wallet details.
    // Here's the method:
    // If the database does not have the address stored, call the API at TradeSatoshi, to generate a new adddress.
    // If the database does have the address, pass those details to the caller.
    // But also verify the status of the wallet, if the status is given as "OK".
    SQL sql (front_bot_ip_address ());
    string database_address = wallets_get_address (sql, exchange, coin);
    string database_payment_id = wallets_get_payment_id (sql, exchange, coin);
    if (database_address.empty ()) {
      tradesatoshi_get_wallet_details (coin, json, error, address, payment_id);
    } else {
      string status;
      vector <string> coin_abbrevs, coin_ids, names, statuses;
      tradesatoshi_get_coins (bitcoin_id(), coin_abbrevs, coin_ids, names, statuses, error);
      for (unsigned int i = 0; i < names.size (); i++) {
        if (coin_ids[i] == coin) status = statuses[i];
      }
      if (status == "OK") {
        address = database_address;
        payment_id = database_payment_id;
      }
    }
  };
  // Output the JSON for debugging purposes.
  to_stdout ({"Get wallet details JSON:", json});
  // Output possible error messages.
  to_failures (failures_type_api_error (), exchange, coin, 0, exchange_format_error (error, __FUNCTION__, exchange, "", coin, ""));
}


void exchange_get_balances (string exchange,
                            vector <string> & coin_abbrev,
                            vector <string> & coin_ids,
                            vector <float> & total,
                            vector <float> & reserved,
                            vector <float> & unconfirmed)
{
  // Clear containers just to be sure.
  coin_abbrev.clear ();
  coin_ids.clear ();
  total.clear ();
  reserved.clear ();
  unconfirmed.clear ();
  // Get balances from the correct exchange.
  string error;
  if (exchange == bittrex_get_exchange ()) {
    bittrex_get_balances (coin_abbrev, coin_ids, total, reserved, unconfirmed, error);
  };
  if (exchange == cryptopia_get_exchange ()) {
    cryptopia_get_balances (coin_abbrev, coin_ids, total, reserved, unconfirmed, error);
  };
  if (exchange == bl3p_get_exchange ()) {
    bl3p_get_balances (coin_abbrev, coin_ids, total, reserved, unconfirmed, error);
  };
  if (exchange == bitfinex_get_exchange ()) {
    bitfinex_get_balances (coin_abbrev, coin_ids, total, reserved, unconfirmed, error);
  };
  if (exchange == yobit_get_exchange ()) {
    yobit_get_balances (coin_abbrev, coin_ids, total, reserved, unconfirmed, error);
  };
  if (exchange == kraken_get_exchange ()) {
    kraken_get_balances (coin_abbrev, coin_ids, total, reserved, unconfirmed, error);
  };
  if (exchange == poloniex_get_exchange ()) {
    poloniex_get_balances (coin_abbrev, coin_ids, total, reserved, unconfirmed, error);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    tradesatoshi_get_balances (coin_abbrev, coin_ids, total, reserved, unconfirmed, error);
  };
  // Error handling and feedback.
  to_stdout (exchange_format_error (error, __FUNCTION__, exchange, "", "", ""));
  if (!error.empty()) {
    // In case of an error, be sure it returns nothing rather than posssible inaccurate data.
    coin_abbrev.clear ();
    coin_ids.clear ();
    total.clear ();
    reserved.clear ();
    unconfirmed.clear ();
  }
}


void exchange_get_balance_cached (const string & exchange, const string & coin,
                                  float * total, float * available, float * reserved, float * unconfirmed)
{
  if (total) * total = 0;
  if (available) * available = 0;
  if (reserved) * reserved = 0;
  if (unconfirmed) * unconfirmed = 0;
  if (exchange == bittrex_get_exchange ()) {
    bittrex_get_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == cryptopia_get_exchange ()) {
    cryptopia_get_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == bl3p_get_exchange ()) {
    bl3p_get_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == bitfinex_get_exchange ()) {
    bitfinex_get_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == yobit_get_exchange ()) {
    yobit_get_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == kraken_get_exchange ()) {
    kraken_get_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == poloniex_get_exchange ()) {
    poloniex_get_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    tradesatoshi_get_balance (coin, total, available, reserved, unconfirmed);
  };
}


void exchange_set_balance_cache (const string & exchange, const string & coin,
                                 float total, float available, float reserved, float unconfirmed)
{
  if (exchange == bittrex_get_exchange ()) {
    bittrex_set_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == cryptopia_get_exchange ()) {
    cryptopia_set_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == bl3p_get_exchange ()) {
    bl3p_set_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == bitfinex_get_exchange ()) {
    bitfinex_set_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == yobit_get_exchange ()) {
    yobit_set_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == kraken_get_exchange ()) {
    kraken_set_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == poloniex_get_exchange ()) {
    poloniex_set_balance (coin, total, available, reserved, unconfirmed);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    tradesatoshi_set_balance (coin, total, available, reserved, unconfirmed);
  };
}


vector <string> exchange_get_coins_with_balances (const string & exchange)
{
  vector <string> coins;
  if (exchange == bittrex_get_exchange ()) {
    coins = bittrex_get_coins_with_balances ();
  };
  if (exchange == cryptopia_get_exchange ()) {
    coins = cryptopia_get_coins_with_balances ();
  };
  if (exchange == bl3p_get_exchange ()) {
    coins = bl3p_get_coins_with_balances ();
  };
  if (exchange == bitfinex_get_exchange ()) {
    coins = bitfinex_get_coins_with_balances ();
  };
  if (exchange == yobit_get_exchange ()) {
    coins = yobit_get_coins_with_balances ();
  };
  if (exchange == kraken_get_exchange ()) {
    coins = kraken_get_coins_with_balances ();
  };
  if (exchange == poloniex_get_exchange ()) {
    coins = poloniex_get_coins_with_balances ();
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    coins = tradesatoshi_get_coins_with_balances ();
  };
  return coins;
}


void exchange_get_market_summaries (string exchange, string market,
                                    vector <string> & coin_ids,
                                    vector <float> & bid,
                                    vector <float> & ask,
                                    string * error)
{
  coin_ids.clear ();
  bid.clear ();
  ask.clear ();
  string localerror;
  if (exchange == bittrex_get_exchange ()) {
    bittrex_get_market_summaries (market, coin_ids, bid, ask, localerror);
  };
  if (exchange == cryptopia_get_exchange ()) {
    cryptopia_get_market_summaries (market, coin_ids, bid, ask, localerror);
  };
  if (exchange == bl3p_get_exchange ()) {
    bl3p_get_market_summaries (market, coin_ids, bid, ask, localerror);
  };
  if (exchange == bitfinex_get_exchange ()) {
    bitfinex_get_market_summaries (market, coin_ids, bid, ask, localerror);
  };
  if (exchange == yobit_get_exchange ()) {
    yobit_get_market_summaries (market, coin_ids, bid, ask, localerror);
  };
  if (exchange == kraken_get_exchange ()) {
    kraken_get_market_summaries (market, coin_ids, bid, ask, localerror);
  };
  if (exchange == poloniex_get_exchange ()) {
    poloniex_get_market_summaries (market, coin_ids, bid, ask, localerror);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    tradesatoshi_get_market_summaries (market, coin_ids, bid, ask, localerror);
  };
  // Add the market coin with rate 1.
  // E.g. if requesting the rates of the coins at the Bitcoin market,
  // add the Bitcoin itself with rate 1:1.
  coin_ids.push_back (market);
  bid.push_back (1);
  ask.push_back (1);
  // Error handler.
  if (error) {
    error->assign (localerror);
  } else {
    to_stdout (exchange_format_error (localerror, __FUNCTION__, exchange, market, "", ""));
  }
}


void exchange_get_market_summaries_via_satellite (const string & exchange, const string & market,
                                                  vector <string> & coins,
                                                  vector <float> & bids,
                                                  vector <float> & asks)
{
  // Assemble the call to the satellite.
  vector <string> call = { "summaries", exchange, market };
  
  // Call the satellite.
  string error;
  string summaries = satellite_request (satellite_get (exchange), implode (call, " "), false, error);

  // Sample contents:
  // lumen 0.0000470000 0.0000471300
  // magi 0.0000551100 0.0000555000
  // monero 0.0268538408 0.0269063301

  // Interpret the response.
  vector <string> lines = explode (summaries, '\n');

  // First error handler:
  // If the satellite returns one line only, this will be the error.
  if (lines.size () == 1) {
    error.append (summaries);
  }
  
  // No error: Parse the response.
  else {
    for (auto & line : lines) {
      vector <string> words = explode (line, ' ');
      if (words.size () == 3) {
        string coin = words[0];
        if (coin.empty ()) continue;
        float bid = str2float (words[1]);
        float ask = str2float (words[2]);
        coins.push_back (coin);
        bids.push_back (bid);
        asks.push_back (ask);
      } else {
        if (!error.empty ()) error.append (" ");
        error.append (line);
      }
    }
  }
  
  // Error handler.
  to_stdout (exchange_format_error (error, __FUNCTION__, exchange, market, "", ""));
}


void exchange_get_buyers (string exchange,
                          string market,
                          string coin,
                          vector <float> & quantity,
                          vector <float> & rate,
                          bool hurry,
                          string & error)
{
  quantity.clear ();
  rate.clear ();
  if (exchange == bittrex_get_exchange ()) {
    bittrex_get_buyers (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == cryptopia_get_exchange ()) {
    cryptopia_get_buyers (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == bl3p_get_exchange ()) {
    bl3p_get_buyers (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == bitfinex_get_exchange ()) {
    bitfinex_get_buyers_rest (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == yobit_get_exchange ()) {
    yobit_get_buyers (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == kraken_get_exchange ()) {
    kraken_get_buyers (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == poloniex_get_exchange ()) {
    poloniex_get_buyers (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    tradesatoshi_get_buyers (market, coin, quantity, rate, error, hurry);
  };
}


void exchange_get_buyers_via_satellite (string exchange,
                                        string market,
                                        string coin,
                                        vector <float> & quantities,
                                        vector <float> & rates,
                                        bool hurry,
                                        void * output)
{
  // Clear the containers, to start off.
  quantities.clear ();
  rates.clear ();
  
  // Assemble the call to the satellite.
  vector <string> call = { "buyers", exchange, market, coin };

  // Call the satellite.
  string satellite = satellite_get (exchange);
  string error;
  string buyers = satellite_request (satellite, implode (call, " "), hurry, error);

  // Sample contents:
  // 1260.0000000000 0.0000003500
  // 164.7058868408 0.0000003400
  // 189.1515197754 0.0000003300

  // Interpret the response.
  vector <string> lines = explode (buyers, '\n');
  
  // First error handler:
  // If the satellite returns one line only, this will be the error.
  // But if that single line contains floats only,
  // it means that the order book consists of one entry only,
  // and that is no error.
  if ((lines.size () == 1) && (!has_floats_only (lines[0]))) {
    error.append (buyers);
  }
  
  // No error: Parse the response.
  else {
    for (auto & line : lines) {
      vector <string> words = explode (line, ' ');
      if (words.size () == 2) {
        float quantity = str2float (words[0]);
        float rate = str2float (words[1]);
        quantities.push_back (quantity);
        rates.push_back (rate);
      } else {
        if (!error.empty ()) error.append (" ");
        error.append (line);
      }
    }
  }

  // Error handler.
  // Send it to either the stand-alone output, or to the bundled output.
  if (output) {
    to_output * out = (to_output *) output;
    out->add (exchange_format_error (error, __FUNCTION__, exchange, market, coin, satellite));
  } else {
    to_stdout (exchange_format_error (error, __FUNCTION__, exchange, market, coin, satellite));
  }
  
  // Hurried call statistics.
  if (hurry) {
    exchange_hurried_mutex.lock ();
    if (error.empty ()) {
      exchange_hurried_call_pass [exchange]++;
    } else {
      exchange_hurried_call_fail [exchange]++;
    }
    exchange_hurried_mutex.unlock ();
  }
}


void exchange_get_sellers (string exchange,
                           string market,
                           string coin,
                           vector <float> & quantity,
                           vector <float> & rate,
                           bool hurry,
                           string & error)
{
  quantity.clear ();
  rate.clear ();
  if (exchange == bittrex_get_exchange ()) {
    bittrex_get_sellers (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == cryptopia_get_exchange ()) {
    cryptopia_get_sellers (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == bl3p_get_exchange ()) {
    bl3p_get_sellers (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == bitfinex_get_exchange ()) {
    bitfinex_get_sellers_rest (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == yobit_get_exchange ()) {
    yobit_get_sellers (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == kraken_get_exchange ()) {
    kraken_get_sellers (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == poloniex_get_exchange ()) {
    poloniex_get_sellers (market, coin, quantity, rate, error, hurry);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    tradesatoshi_get_sellers (market, coin, quantity, rate, error, hurry);
  };
}


void exchange_get_sellers_via_satellite (string exchange,
                                         string market,
                                         string coin,
                                         vector <float> & quantities,
                                         vector <float> & rates,
                                         bool hurry,
                                         void * output)
{
  // Clear the containers, to start off.
  quantities.clear ();
  rates.clear ();

  // Assemble the call to the satellite.
  vector <string> call = { "sellers", exchange, market, coin };
  
  // Call the satellite.
  string satellite = satellite_get (exchange);
  string error;
  string buyers = satellite_request (satellite, implode (call, " "), hurry, error);
  
  // Sample contents:
  // 1260.0000000000 0.0000003500
  // 164.7058868408 0.0000003400
  // 189.1515197754 0.0000003300
  
  // Interpret the response.
  vector <string> lines = explode (buyers, '\n');
  
  // First error handler:
  // If the satellite returns one line only, this will be the error.
  // But if that single line contains floats only,
  // it means that the order book consists of one entry only,
  // and that is no error.
  if ((lines.size () == 1) && (!has_floats_only (lines[0]))) {
    error.append (buyers);
  }
  
  // No error: Parse the response.
  else {
    for (auto & line : lines) {
      vector <string> words = explode (line, ' ');
      if (words.size () == 2) {
        float quantity = str2float (words[0]);
        float rate = str2float (words[1]);
        quantities.push_back (quantity);
        rates.push_back (rate);
      } else {
        if (!error.empty ()) error.append (" ");
        error.append (line);
      }
    }
  }
  
  // Error handler.
  // Send it to either the stand-alone output, or to the bundled output.
  if (output) {
    to_output * out = (to_output *) output;
    out->add (exchange_format_error (error, __FUNCTION__, exchange, market, coin, satellite));
  } else {
    to_stdout (exchange_format_error (error, __FUNCTION__, exchange, market, coin, satellite));
  }
  
  // Hurried call statistics.
  if (hurry) {
    exchange_hurried_mutex.lock ();
    if (error.empty ()) {
      exchange_hurried_call_pass [exchange]++;
    } else {
      exchange_hurried_call_fail [exchange]++;
    }
    exchange_hurried_mutex.unlock ();
  }
}


void exchange_get_open_orders (string exchange,
                               vector <string> & identifier,
                               vector <string> & market,
                               vector <string> & coin_abbreviation,
                               vector <string> & coin_ids,
                               vector <string> & buy_sell,
                               vector <float> & quantity,
                               vector <float> & rate,
                               vector <string> & date,
                               string & error)
{
  identifier.clear ();
  market.clear ();
  coin_abbreviation.clear ();
  coin_ids.clear ();
  buy_sell.clear ();
  quantity.clear ();
  rate.clear ();
  if (exchange == bittrex_get_exchange ()) {
    bittrex_get_open_orders (identifier, market, coin_abbreviation, coin_ids, buy_sell, quantity, rate, date, error);
  };
  if (exchange == cryptopia_get_exchange ()) {
    cryptopia_get_open_orders (identifier, market, coin_abbreviation, coin_ids, buy_sell, quantity, rate, date, error);
  };
  if (exchange == bl3p_get_exchange ()) {
    bl3p_get_open_orders (identifier, market, coin_abbreviation, coin_ids, buy_sell, quantity, rate, date, error);
  };
  if (exchange == bitfinex_get_exchange ()) {
    bitfinex_get_open_orders (identifier, market, coin_abbreviation, coin_ids, buy_sell, quantity, rate, date, error);
  };
  if (exchange == yobit_get_exchange ()) {
    yobit_get_open_orders (identifier, market, coin_abbreviation, coin_ids, buy_sell, quantity, rate, date, error);
  };
  if (exchange == kraken_get_exchange ()) {
    kraken_get_open_orders (identifier, market, coin_abbreviation, coin_ids, buy_sell, quantity, rate, date, error);
  };
  if (exchange == poloniex_get_exchange ()) {
    poloniex_get_open_orders (identifier, market, coin_abbreviation, coin_ids, buy_sell, quantity, rate, date, error);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    tradesatoshi_get_open_orders (identifier, market, coin_abbreviation, coin_ids, buy_sell, quantity, rate, date, error);
  };
  to_stdout (exchange_format_error (error, __FUNCTION__, exchange, "", "", ""));
}


void exchange_limit_buy (const string & exchange,
                         const string & market,
                         const string & coin, float quantity, float rate,
                         string & error, string & json, string & order_id,
                         void * output)
{
  if (exchange == bittrex_get_exchange ()) {
    order_id = bittrex_limit_buy (market, coin, quantity, rate, json, error);
  };
  if (exchange == cryptopia_get_exchange ()) {
    order_id = cryptopia_limit_buy (market, coin, quantity, rate, json, error);
  };
  if (exchange == bl3p_get_exchange ()) {
    order_id = bl3p_limit_buy (market, coin, quantity, rate, json, error);
  };
  if (exchange == bitfinex_get_exchange ()) {
    order_id = bitfinex_limit_buy (market, coin, quantity, rate, json, error);
  };
  if (exchange == yobit_get_exchange ()) {
    order_id = yobit_limit_buy (market, coin, quantity, rate, json, error);
  };
  if (exchange == kraken_get_exchange ()) {
    order_id = kraken_limit_buy (market, coin, quantity, rate, json, error);
  }
  if (exchange == poloniex_get_exchange ()) {
    order_id = poloniex_limit_buy (market, coin, quantity, rate, json, error);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    order_id = tradesatoshi_limit_buy (market, coin, quantity, rate, json, error);
  }

  // The logger.
  to_output * output_block = (to_output *) output;
  
  // Output the (condensed) JSON for debugging purposes.
  json = exchange_condense_message (json);
  output_block->add ({"Limit buy JSON:", json});
  
  // This call should never repeat on error because of the financial impact it could have.
  // Despite the error, the exchange could have placed the limit trade order.
  // This has been seen with Cryptopia, and with Bitfinex. There might be more exchanges.
  // If it were to repeat, it could even place multiple trade orders, and that would disturb the trading.
  
  // It outputs any error for feedback.
  output_block->failure (failures_type_api_error (), exchange_format_error (error, __FUNCTION__, exchange, market, coin, ""));
  
  // Remove any padding spaces.
  order_id = trim (order_id);
  
  // Extra info.
  if (order_id.empty ()) {
    output_block->add ({error});
    output_block->to_stderr (true);
  }
}


void exchange_limit_sell (const string & exchange,
                          const string & market,
                          const string & coin, float quantity, float rate,
                          string & error, string & json, string & order_id,
                          void * output)
{
  if (exchange == bittrex_get_exchange ()) {
    order_id = bittrex_limit_sell (market, coin, quantity, rate, json, error);
  };
  if (exchange == cryptopia_get_exchange ()) {
    order_id = cryptopia_limit_sell (market, coin, quantity, rate, json, error);
  };
  if (exchange == bl3p_get_exchange ()) {
    order_id = bl3p_limit_sell (market, coin, quantity, rate, json, error);
  };
  if (exchange == bitfinex_get_exchange ()) {
    order_id = bitfinex_limit_sell (market, coin, quantity, rate, json, error);
  };
  if (exchange == yobit_get_exchange ()) {
    order_id = yobit_limit_sell (market, coin, quantity, rate, json, error);
  };
  if (exchange == kraken_get_exchange ()) {
    order_id = kraken_limit_sell (market, coin, quantity, rate, json, error);
  }
  if (exchange == poloniex_get_exchange ()) {
    order_id = poloniex_limit_sell (market, coin, quantity, rate, json, error);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    order_id = tradesatoshi_limit_sell (market, coin, quantity, rate, json, error);
  }

  // The logger.
  to_output * output_block = (to_output *) output;
  
  // Output the (condensed) JSON for debugging purposes.
  json = exchange_condense_message (json);
  output_block->add ({"Limit sell JSON:", json});
  
  // This should never repeat on error because of the financial impact it could have.
  // Despite the error, the exchange could have placed the limit trade order.
  // This has been seen with Cryptopia, and with Bitfinex. There might be more.
  // If it were to repeat, it could even place multiple trade orders, and that would disturb the trading.
  
  // It outputs any error for feedback.
  output_block->failure (failures_type_api_error (), exchange_format_error (error, __FUNCTION__, exchange, market, coin, ""));
  
  // Remove any padding spaces.
  order_id = trim (order_id);
  
  // Extra info.
  if (order_id.empty ()) {
    output_block->add ({error});
    output_block->to_stderr (true);
  }
}


bool exchange_cancel_order (string exchange, string order_id)
{
  string error;
  bool cancelled = false;
  if (exchange == bittrex_get_exchange ()) {
    cancelled = bittrex_cancel_order (order_id, error);
  };
  if (exchange == cryptopia_get_exchange ()) {
    cancelled = cryptopia_cancel_order (order_id, error);
  };
  if (exchange == bl3p_get_exchange ()) {
    cancelled = bl3p_cancel_order (order_id, error);
  };
  if (exchange == bitfinex_get_exchange ()) {
    cancelled = bitfinex_cancel_order (order_id, error);
  };
  if (exchange == yobit_get_exchange ()) {
    cancelled = yobit_cancel_order (order_id, error);
  };
  if (exchange == kraken_get_exchange ()) {
    cancelled = kraken_cancel_order (order_id, error);
  };
  if (exchange == poloniex_get_exchange ()) {
    cancelled = poloniex_cancel_order (order_id, error);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    cancelled = tradesatoshi_cancel_order (order_id, error);
  };
  to_stdout (exchange_format_error (error, __FUNCTION__, exchange, "", "", ""));
  return cancelled;
}


string exchange_withdrawal_order (const string & exchange,
                                  const string & coin,
                                  float quantity,
                                  const string & address,
                                  const string & paymentid,
                                  string & json,
                                  void * output)
{
  string result;
  string error;
  if (exchange == bittrex_get_exchange ()) {
    result = bittrex_withdrawal_order (coin, quantity, address, paymentid, json, error);
  };
  if (exchange == cryptopia_get_exchange ()) {
    result = cryptopia_withdrawal_order (coin, quantity, address, paymentid, json, error);
  };
  if (exchange == bl3p_get_exchange ()) {
    result = "";
  };
  if (exchange == bitfinex_get_exchange ()) {
    result = bitfinex_withdrawal_order (coin, quantity, address, paymentid, json, error);
  };
  if (exchange == yobit_get_exchange ()) {
    result = yobit_withdrawal_order (coin, quantity, address, paymentid, json, error);
  };
  if (exchange == poloniex_get_exchange ()) {
    result = poloniex_withdrawal_order (coin, quantity, address, paymentid, json, error);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    result = tradesatoshi_withdrawal_order (coin, quantity, address, paymentid, json, error);
  };
  // This should never repeat on error because of the financial impact it could have.
  // Despite the error, the exchange could have placed the withdrawal order.
  // If it were to repeat, it could even place multiple withdrawal orders, and that would disturb the balances.
  // It outputs any error for feedback.
  to_output * output_block = (to_output *) output;
  output_block->failure (failures_type_api_error (), exchange_format_error (error, __FUNCTION__, exchange, "", coin, ""));
  // In case of failure, output the JSON.
  if (result.empty ()) {
    output_block->add ({json});
    output_block->to_stderr (true);
  }
  // Done.
  return result;
}


void exchange_withdrawalhistory (string exchange,
                                 string coin,
                                 vector <string> & order_ids,
                                 vector <string> & coin_abbreviations,
                                 vector <string> & coin_ids,
                                 vector <float> & quantities,
                                 vector <string> & addresses,
                                 vector <string> & transaction_ids)
{
  order_ids.clear ();
  coin_abbreviations.clear ();
  coin_ids.clear ();
  quantities.clear ();
  addresses.clear ();
  string error;
  if (exchange == bittrex_get_exchange ()) {
    bittrex_withdrawalhistory (order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
  };
  if (exchange == cryptopia_get_exchange ()) {
    cryptopia_withdrawalhistory (order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
  };
  if (exchange == bl3p_get_exchange ()) {
  };
  if (exchange == bitfinex_get_exchange ()) {
    bitfinex_withdrawalhistory (coin, order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
  };
  if (exchange == yobit_get_exchange ()) {
    yobit_withdrawalhistory (order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
  };
  if (exchange == poloniex_get_exchange ()) {
    poloniex_withdrawalhistory (coin, order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    tradesatoshi_withdrawalhistory (coin, order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
  };
  to_stdout (exchange_format_error (error, __FUNCTION__, exchange, "", coin, ""));
}


void exchange_deposithistory (string exchange,
                              string coin,
                              vector <string> & order_ids,
                              vector <string> & coin_abbreviations,
                              vector <string> & coin_ids,
                              vector <float> & quantities,
                              vector <string> & addresses,
                              vector <string> & transaction_ids)
{
  order_ids.clear ();
  coin_abbreviations.clear ();
  quantities.clear ();
  addresses.clear ();
  string error;
  if (exchange == bittrex_get_exchange ()) {
    bittrex_deposithistory (order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
  };
  if (exchange == cryptopia_get_exchange ()) {
    cryptopia_deposithistory (order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
  };
  if (exchange == bl3p_get_exchange ()) {
    bl3p_deposithistory (coin, order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
  };
  if (exchange == bitfinex_get_exchange ()) {
    bitfinex_deposithistory (coin, order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
  };
  if (exchange == yobit_get_exchange ()) {
    yobit_deposithistory (order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
  };
  if (exchange == poloniex_get_exchange ()) {
    poloniex_deposithistory (coin, order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    tradesatoshi_deposithistory (coin, order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
  };
  to_stdout (exchange_format_error (error, __FUNCTION__, exchange, "", coin, ""));
}


// The main purpose of this is to condense a long error or json message into a shorter one.
string exchange_condense_message (string message)
{
  // At times the API returns a while web page, in error.
  // This may get long.
  // Often this web page contains a title.
  // Example:
  // <title>api.kraken.com | 525: SSL handshake failed</title>
  // If this is the case, extract the information between the <title>...</title>
  // This information will become the new error text.
  vector <string> titles = { "title", "TITLE" };
  for (auto title : titles) {
    size_t pos = message.find ("<" + title + ">");
    if (pos != string::npos) {
      message.erase (0, pos + 7);
      pos = message.find ("</" + title + ">");
      if (pos != string::npos) {
        message.erase (pos);
      }
    }
  }

  // Truncate a long message.
  if (message.size () > 2000) {
    message.erase (2000);
    message.append (" ...");
  }

  // Done.
  return message;
}


vector <string> exchange_format_error (string error, string func, string exchange, string market, string coin, string satellite)
{
  // The formatted message.
  vector <string> message;
  
  // Proceed if there's an error.
  if (!error.empty ()) {
    message = { "API call error", func, exchange, market, coin, satellite, exchange_condense_message (error) };
  }
  
  // Done.
  return message;
}


// Returns the fee as a factor of the trade size.
float exchange_get_trade_fee (string exchange)
{
  if (exchange == bittrex_get_exchange ()) {
    // https://bittrex.com/fees
    return 0.25 / 100;
  };
  if (exchange == cryptopia_get_exchange ()) {
    // https://www.cryptopia.co.nz/Forum/Thread/1885
    return 0.2 / 100;
  };
  if (exchange == bl3p_get_exchange ()) {
    // https://bl3p.eu/nl/fees
    return 0.25 / 100;
  };
  if (exchange == bitfinex_get_exchange ()) {
    // https://www.bitfinex.com/fees
    return 0.2 / 100;
  };
  if (exchange == yobit_get_exchange ()) {
    // http://yobit.net/en/faq/
    return 0.2 / 100;
  };
  if (exchange == poloniex_get_exchange ()) {
    // https://poloniex.com/fees/
    return 0.25 / 100;
  };
  // Unknown exchange: 0.3%
  return 0.3 / 100;
}


// Returns the increase percentage on the price offered to buy coins for.
// Returns the decrease percentage on the price willing to sell coins for.
// The increase should be applied to the ask price of the exchange.
// The decrease should be applied to the bid price of the exchange.
// The reason for applying the increase is as follows:
// It has been noticed that many buy orders don't get fulfilled for a longer time.
// These buy orders keep accumulating, and don't get fulfulled within a reasonable time.
// To place limit buy orders for a slightly higher price, that would lead to fewer open orders
// The reason for applying the decrease is as follows:
// It has been noticed that many sell orders don't get fulfilled for a longer time.
// These sell orders keep accumulating, and don't get fulfulled within a reasonable time.
// To place limit sell orders for a slightly lower price, that would lead to fewer open orders
// It has been observed that, if the ease factor is too low,
// that leads to losses in arbitrage trading.
float exchange_get_trade_order_ease_percentage (string exchange)
{
  if (exchange == bittrex_get_exchange ()) {
  };
  if (exchange == cryptopia_get_exchange ()) {
    // Cryptopia has more open orders than is desirable.
    return 0.15;
  };
  if (exchange == bl3p_get_exchange ()) {
  };
  if (exchange == bitfinex_get_exchange ()) {
    // There's few limit orders placed on Bitfinex that remain open for a longer time.
    // So there's no need for any corrections on the limit price.
    // After this value was set, the number of open orders grew again.
    // So now set a value of 0.05%.
    // This was done on 29 December 2017.
    return 0.05;
  };
  if (exchange == yobit_get_exchange ()) {
  };
  // General increase: 0.1%
  return 0.1;
}


// This returns the factor of the available balance that can be sold,
// without the exchange giving an error.
// For example, if Cryptopia has a balance 2702.7028808594 trezarcoin,
// trying to sell that exact amount, gives an error:
// {"Success":false,"Error":"Insufficient Funds.","Data":null}.
// When selling 2700 trezarcoin, the sale succeeds.
float exchange_get_sell_balance_factor (const string & exchange)
{
  if (exchange == cryptopia_get_exchange ()) {
    return 0.9999;
  };
  return 0.9999;
}


// Returns the markets that trade on the given $exchange.
vector <string> exchange_get_supported_markets (string exchange)
{
  if (exchange == bittrex_get_exchange ()) {
    return { bitcoin_id (), ethereum_id () };
  };
  if (exchange == cryptopia_get_exchange ()) {
    return { bitcoin_id (), usdtether_id (), newzealanddollartoken_id (), litecoin_id (), dogecoin_id () };
  };
  if (exchange == bl3p_get_exchange ()) {
    return { euro_id () };
  };
  if (exchange == bitfinex_get_exchange ()) {
    return { bitcoin_id (), ethereum_id (), usdollar_id (), euro_id () };
  };
  if (exchange == yobit_get_exchange ()) {
    return { bitcoin_id (), ethereum_id (), dogecoin_id (), waves_id (), usdollar_id (), russianrubble_id () };
  };
  if (exchange == kraken_get_exchange ()) {
    return { bitcoin_id (), ethereum_id (), euro_id () };
  };
  if (exchange == poloniex_get_exchange ()) {
    return { bitcoin_id (), ethereum_id (), usdtether_id (), monero_id () };
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    return { bitcoin_id (), litecoin_id (), dogecoin_id (), bitcoincash_id (), usdtether_id () };
  };
  // No markets
  return {};
}


// Returns the URL of a good blockchain explorer for the $coin identifier.
string exchange_get_blockchain_explorer (string coin)
{
  if (coin == bitcoin_id ()) return "https://www.blocktrail.com/BTC";
  if (coin == "verge") return "https://verge-blockchain.info";
  if (coin == "dash") return "https://chainz.cryptoid.info/dash";
  if (coin == "bitcoingold") return "https://btgexplorer.com";
  if (coin == ethereum_id ()) return "https://etherscan.io";
  if (coin == "ethereumclassic") return "https://gastracker.io";
  if (coin == "zcash") return "https://zcash.blockexplorer.com";
  return "The code is not yet aware of a blockchain explorer for this coin";
}


// Parses the dates returned by the exchange's API calls.
// It returns the seconds since the Unix Epoch.
int exchange_parse_date (string exchange, string date)
{
  bool year_month_day_hour_minute_time = false;
  bool seconds_since_apoch = false;
  if (exchange == bittrex_get_exchange ()) {
    // Sample date: 2017-12-09T18:01:29.68
    year_month_day_hour_minute_time = true;
  };
  if (exchange == cryptopia_get_exchange ()) {
    // Sample date: 2017-12-08T04:07:21.4592266
    year_month_day_hour_minute_time = true;
  };
  if (exchange == bl3p_get_exchange ()) {
    // Sample date: 1520959773
    seconds_since_apoch = true;
  };
  if (exchange == bitfinex_get_exchange ()) {
    // Sample date: 1512579268.0
    seconds_since_apoch = true;
  };
  if (exchange == yobit_get_exchange ()) {
    // Sample date: 1512579268
    seconds_since_apoch = true;
  };
  if (exchange == kraken_get_exchange ()) {
    // Sample date: 1517161139.7076
    seconds_since_apoch = true;
  };
  if (exchange == poloniex_get_exchange ()) {
    // Sample date: 2018-01-09 19:05:43
    year_month_day_hour_minute_time = true;
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    // Sample date: 2018-03-15T19:18:34.433
    year_month_day_hour_minute_time = true;
  };
  if (year_month_day_hour_minute_time) {
    // Parsing this: 2017-12-09T18:01:29...
    // Or this     : 2018-01-09 19:05:43
    string year = date.substr (0, 4);
    string month = date.substr (5, 2);
    string day = date.substr (8, 2);
    int seconds = get_seconds_since_epoch (stoi (year), stoi (month), stoi (day));
    string hour = date.substr (11, 2);
    seconds += (stoi (hour) * 3600);
    return seconds;
  }
  if (seconds_since_apoch) {
    // Parsing this: 1512579268...
    int seconds = stoi (date);
    return seconds;
  }
  return 0;
}


// Get the minimum trade amounts from the exchange / market / coins.
void exchange_get_minimum_trade_amounts (const string & exchange,
                                         const string & market,
                                         const vector <string> & coins,
                                         map <string, float> & market_amounts,
                                         map <string, float> & coin_amounts)
{
  string error;
  market_amounts.clear ();
  coin_amounts.clear ();
  if (exchange == bittrex_get_exchange ()) {
    bittrex_get_minimum_trade_amounts (market, coins, coin_amounts, error);
  };
  if (exchange == cryptopia_get_exchange ()) {
    cryptopia_get_minimum_trade_amounts (market, coins, market_amounts, coin_amounts, error);
  };
  if (exchange == bl3p_get_exchange ()) {
  };
  if (exchange == bitfinex_get_exchange ()) {
    bitfinex_get_minimum_trade_amounts (market, coins, market_amounts, coin_amounts, error);
  };
  if (exchange == yobit_get_exchange ()) {
  };
  if (exchange == kraken_get_exchange ()) {
  };
  if (exchange == poloniex_get_exchange ()) {
    // The API does not provide this information.
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    // The API does not provide this information.
  };
  to_stdout (exchange_format_error (error, __FUNCTION__, exchange, "", "", ""));
}


int exchange_get_hurried_call_passes (const string & exchange)
{
  exchange_hurried_mutex.lock ();
  int this_pass_count = exchange_hurried_call_pass [exchange];
  exchange_hurried_mutex.unlock ();
  int that_pass_count = 0;
  if (exchange == bittrex_get_exchange ()) {
    that_pass_count = bittrex_get_hurried_call_passes ();
  };
  if (exchange == cryptopia_get_exchange ()) {
    that_pass_count = cryptopia_get_hurried_call_passes ();
  };
  if (exchange == bl3p_get_exchange ()) {
    that_pass_count = bl3p_get_hurried_call_passes ();
  };
  if (exchange == bitfinex_get_exchange ()) {
    that_pass_count = bitfinex_get_hurried_call_passes ();
  };
  if (exchange == yobit_get_exchange ()) {
    that_pass_count = yobit_get_hurried_call_passes ();
  };
  if (exchange == kraken_get_exchange ()) {
    that_pass_count = kraken_get_hurried_call_passes ();
  };
  if (exchange == poloniex_get_exchange ()) {
    that_pass_count = poloniex_get_hurried_call_passes ();
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    that_pass_count = tradesatoshi_get_hurried_call_passes ();
  };
  return this_pass_count + that_pass_count;
}


int exchange_get_hurried_call_fails (const string & exchange)
{
  exchange_hurried_mutex.lock ();
  int this_fail_count = exchange_hurried_call_fail [exchange];
  exchange_hurried_mutex.unlock ();
  int that_fail_count = 0;
  if (exchange == bittrex_get_exchange ()) {
    that_fail_count = bittrex_get_hurried_call_fails ();
  };
  if (exchange == cryptopia_get_exchange ()) {
    that_fail_count = cryptopia_get_hurried_call_fails ();
  };
  if (exchange == bl3p_get_exchange ()) {
    that_fail_count = bl3p_get_hurried_call_fails ();
  };
  if (exchange == bitfinex_get_exchange ()) {
    that_fail_count = bitfinex_get_hurried_call_fails ();
  };
  if (exchange == yobit_get_exchange ()) {
    that_fail_count = yobit_get_hurried_call_fails ();
  };
  if (exchange == kraken_get_exchange ()) {
    that_fail_count = kraken_get_hurried_call_fails ();
  };
  if (exchange == poloniex_get_exchange ()) {
    that_fail_count = poloniex_get_hurried_call_fails ();
  };
  if (exchange == tradesatoshi_get_exchange ()) {
    that_fail_count = tradesatoshi_get_hurried_call_fails ();
  };
  return this_fail_count + that_fail_count;
}


/*

 Lists of other possible exchanges:
 https://arbitrage.coincheckup.com

 https://bitexlive.com
 The API looks very incomplete in August 2018.
 
 https://www.meanxtrade.com
 The API is very incomplete in August 2018.

 https://www.allcoin.com
 September 2018
 Partially implemented API.
 
 https://www.binance.com
 Septemper 2018
 Does not have deposit and withdrawal API.

 https://indodax.com
 September 2018
 Indonesian exchange.
 The API is there, but not so complete to be useful.
 
 https://bitcoinsnorway.com
 September 2018
 Has no known API.
 
 https://bitebtc.com
 September 2018
 Does not have API for deposits and withdrawals.
 
 https://bitfex.trade
 September 2018
 Does not have a good API for deposits and withdrawals.
 
 https://bitflyer.com
 September 2018
 Does not have a good API for deposits and withdrawals.
 
 https://www.bithumb.com
 September 2018
 Could not find the English web interface.
 
 https://bitkonan.com
 September 2018
 Does not have API for withdrawals and deposits.
 
 https://bitso.com
 September 2018
 Only trades against the Mexican Peso.
 
 https://www.bitstamp.net
 September 2018
 Only trades BTC USD XRP LTC and the like.
 Seems to have a good API.

 Bitx
 Bleutrade
 Btc-E
 Btcchina
 Btcmarkets
 C-Cex
 Cexio
 Coinbase
 Coinexchange
 Coinfloor
 Coinmate
 Coinone
 Cryptonit
 Exmoney
 Exx
 Gatecoin
 Gateio
 Gemini
 Hitbtc
 Huobi
 Idex
 Itbit
 Korbit
 Kucoin
 Lakebtc
 Liqui
 Livecoin
 Okcoin
 Quadrigacx
 Quoine
 Therocktrading
 Tidex
 Tokenstore

*/
