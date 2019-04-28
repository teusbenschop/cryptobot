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


#include "bl3p.h"


#define bl3p_api_id "--enter--your--value--here--"
#define bl3p_api_key "--enter--your--value--here--"


// Hurried call counters.
atomic <int> bl3p_hurried_call_pass (0);
atomic <int> bl3p_hurried_call_fail (0);


// See https://github.com/BitonicNL/bl3p-api/blob/master/examples/php/example.php
string get_bl3p_api_call_result (const string & path,
                                 vector <pair <string, string> > parameters,
                                 string & error,
                                 bool hurry)
{
  // The PHP sample gives this nonce, e.g.:
  // 1523184707476923
  // It is 16 characters long.
  string nonce = increasing_nonce ();
  nonce.erase (16);
  parameters.push_back (make_pair ("nonce", nonce));

  // The data to POST.
  string postdata;
  for (auto parameter : parameters) {
    if (!postdata.empty ()) postdata.append ("&");
    postdata.append (parameter.first);
    postdata.append ("=");
    postdata.append (parameter.second);
  }
  
  // The body to sign.
  string body (path);
  body += '\0';
  body.append (postdata);

  // Build signature for Rest-Sign.
  string decoded_key = base64_decode (bl3p_api_key);
  string raw_hash_hmac = hmac_sha512_raw (decoded_key, body);
  string sign = base64_encode (raw_hash_hmac);
  
  // Combine the BL3P url and the desired path.
  string fullpath = "https://api.bl3p.eu/1/" + path;
  
  // Set extra headers.
  vector <pair <string, string> > headers = { make_pair ("Rest-Key", bl3p_api_id), make_pair ("Rest-Sign", sign) };
  
  // Call API.
  string json = http_call (fullpath, error, "", false, true, postdata, headers, hurry, true, "");
  if (!error.empty ()) {
    error.append (" ");
    error.append (fullpath);
    if (hurry) bl3p_hurried_call_fail++;
    return "";
  }
  // Done.
  if (hurry) bl3p_hurried_call_pass++;
  return json;
}


string bl3p_get_exchange ()
{
  return "bl3p";
}


typedef struct
{
  const char * identifier;
  const char * abbreviation;
  const char * name;
}
bl3p_coin_record;


bl3p_coin_record bl3p_coins_table [] =
{
  { "euro",    "EUR", "Euro" },
  { "bitcoin", "BTC", "Bitcoin" },
};


vector <string> bl3p_get_coin_ids ()
{
  vector <string> ids;
  unsigned int data_count = sizeof (bl3p_coins_table) / sizeof (*bl3p_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    ids.push_back (bl3p_coins_table[i].identifier);
  }
  return ids;
}


string bl3p_get_coin_id (string coin)
{
  unsigned int data_count = sizeof (bl3p_coins_table) / sizeof (*bl3p_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == bl3p_coins_table[i].identifier) return bl3p_coins_table[i].identifier;
    if (coin == bl3p_coins_table[i].abbreviation) return bl3p_coins_table[i].identifier;
    if (coin == bl3p_coins_table[i].name) return bl3p_coins_table[i].identifier;
  }
  return "";
}


string bl3p_get_coin_abbrev (string coin)
{
  unsigned int data_count = sizeof (bl3p_coins_table) / sizeof (*bl3p_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == bl3p_coins_table[i].identifier) return bl3p_coins_table[i].abbreviation;
    if (coin == bl3p_coins_table[i].abbreviation) return bl3p_coins_table[i].abbreviation;
    if (coin == bl3p_coins_table[i].name) return bl3p_coins_table[i].abbreviation;
  }
  return "";
}


string bl3p_get_coin_name (string coin)
{
  unsigned int data_count = sizeof (bl3p_coins_table) / sizeof (*bl3p_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == bl3p_coins_table[i].identifier) return bl3p_coins_table[i].name;
    if (coin == bl3p_coins_table[i].abbreviation) return bl3p_coins_table[i].name;
    if (coin == bl3p_coins_table[i].name) return bl3p_coins_table[i].name;
  }
  return "";
}


void bl3p_get_coins (vector <string> & currencies,
                     vector <string> & coin_ids,
                     vector <string> & names,
                     string & error)
{
  currencies.push_back ("EUR");
  coin_ids.push_back ("euro");
  names.push_back ("Euro");
  currencies.push_back ("BTC");
  coin_ids.push_back ("bitcoin");
  names.push_back ("Bitcoin");
  (void) error;
}


void bl3p_get_market_summaries (string market,
                                vector <string> & coin_ids,
                                vector <float> & bid,
                                vector <float> & ask,
                                string & error)
{
  // No support for any other market than the Euro market.
  (void) market;
  // Call the API.
  string json = get_bl3p_api_call_result ("BTCEUR/ticker", {}, error, false);
  // Check on error.
  if (!error.empty ()) {
    return;
  }
  // Parse the result.
  Object main;
  main.parse (json);
  if (main.has <Number> ("bid")) {
    if (main.has <Number> ("ask")) {
      coin_ids.push_back ("bitcoin");
      float value = main.get <Number> ("bid");
      bid.push_back (value);
      value = main.get <Number> ("ask");
      ask.push_back (value);
      return;
    }
  }
  // Error handler.
  error.append (json);
}


void bl3p_get_open_orders (vector <string> & identifiers,
                           vector <string> & markets,
                           vector <string> & coin_abbrevs,
                           vector <string> & coin_ids,
                           vector <string> & buys_sells,
                           vector <float> & quantities,
                           vector <float> & rates,
                           vector <string> & dates,
                           string & error)
{
  // Call the API.
  string json = get_bl3p_api_call_result ("BTCEUR/money/orders", {}, error, false);
  
  // Check on error.
  if (!error.empty ()) {
    return;
  }
  
  /*
  {
    "result": "success",
    "data": {"orders": [{
                        "date": 1535732087,
                        "item": "BTC",
                        "amount_funds": {
                          "value_int": "3327769000",
                          "display_short": "33277.69 EUR",
                          "display": "33277.69000 EUR",
                          "currency": "EUR",
                          "value": "33277.69000"
                        },
                        "price": {
                          "value_int": "570000000",
                          "display_short": "5700.00 EUR",
                          "display": "5700.00000 EUR",
                          "currency": "EUR",
                          "value": "5700.00000"
                        },
                        "amount_funds_executed": {
                          "value_int": "0",
                          "display_short": "0.00 EUR",
                          "display": "0.00000 EUR",
                          "currency": "EUR",
                          "value": "0.00000"
                        },
                        "currency": "EUR",
                        "label": "bl3p.eu",
                        "type": "bid",
                        "order_id": 39684967,
                        "status": "open",
                        "amount_executed": {
                          "value_int": "0",
                          "display_short": "0.00 BTC",
                          "display": "0.00000000 BTC",
                          "currency": "BTC",
                          "value": "0.00000000"
                        }
          }]}
  }

   Or another type:
   
  {
    "result": "success",
    "data": {"orders": [{
    "date": 1536259673,
    "item": "BTC",
    "amount": {
      "value_int": "539550592",
      "display_short": "5.40 BTC",
      "display": "5.39550592 BTC",
      "currency": "BTC",
      "value": "5.39550592"
    },
    "price": {
      "value_int": "577000000",
      "display_short": "5770.00 EUR",
      "display": "5770.00000 EUR",
      "currency": "EUR",
      "value": "5770.00000"
    },
    "currency": "EUR",
    "label": "bl3p.eu",
    "type": "ask",
    "order_id": 40144381,
    "status": "open",
    "amount_executed": {
      "value_int": "0",
      "display_short": "0.00 BTC",
      "display": "0.00000000 BTC",
      "currency": "BTC",
      "value": "0.00000000"
    }
   }]}
 }

   */
  
  
  Object main;
  main.parse (json);
  if (main.has<String>("result")) {
    string result = main.get<String>("result");
    if (result == "success") {
      if (main.has<Object>("data")) {
        Object data = main.get <Object> ("data");
        if (data.has<Array>("orders")) {
          Array orders = data.get<Array>("orders");
          for (size_t i = 0; i < orders.size(); i++) {
            Object values = orders.get <Object> (i);

            if (!values.has<Number>("order_id")) continue;
            int order_id = values.get<Number>("order_id");

            if (!values.has<String>("currency")) continue;
            string currency = values.get<String>("currency");
            string market = bl3p_get_coin_id (currency);

            if (!values.has<String>("item")) continue;
            string item = values.get<String>("item");
            string coin_id = bl3p_get_coin_id (item);

            if (!values.has<String>("type")) continue;
            string type = values.get<String>("type");
            string buysell (type);
            if (type == "ask") buysell = "sell";
            if (type == "bid") buysell = "buy";

            float amount_value = 0;
            if (values.has<Object>("amount_funds")) {
              Object amount_funds = values.get <Object> ("amount_funds");
              amount_value = str2float (amount_funds.get <String> ("value"));
            }
            if (values.has<Object>("amount")) {
              Object amount = values.get <Object> ("amount");
              amount_value = str2float (amount.get <String> ("value"));
            }

            Object price = values.get <Object> ("price");
            float price_value = str2float (price.get <String> ("value"));

            if (!values.has<Number>("date")) continue;
            int date = values.get<Number>("date");

            identifiers.push_back (to_string(order_id));
            markets.push_back (market);
            coin_abbrevs.push_back (item);
            coin_ids.push_back (coin_id);
            buys_sells.push_back (buysell);
            quantities.push_back (amount_value);
            rates.push_back (price_value);
            dates.push_back (to_string(date));
          }
        }
        return;
      }
    }
  }
  error.append (json);
}


// Get the $total balance that includes also the following two amounts:
// 1. The balances $reserved for trade orders.
// 2. The $unconfirmed balance: What is being deposited right now.
// It also gives all matching coin abbreviations.
void bl3p_get_balances (vector <string> & coin_abbrev,
                        vector <string> & coin_ids,
                        vector <float> & total,
                        vector <float> & reserved,
                        vector <float> & unconfirmed,
                        string & error)
{
  // Call the API.
  string json = get_bl3p_api_call_result ("GENMKT/money/info", {}, error, false);
  
  // Check on error.
  if (!error.empty ()) {
    return;
  }

  /*
  {
    "result": "success",
    "data": {
      "user_id": 3143,
      "wallets": {
        "BTC": {
          "balance": {
            "value_int": "0",
            "display_short": "0.00 BTC",
            "display": "0.00000000 BTC",
            "currency": "BTC",
            "value": "0.00000000"
          },
          "available": {
            "value_int": "0",
            "display_short": "0.00 BTC",
            "display": "0.00000000 BTC",
            "currency": "BTC",
            "value": "0.00000000"
          }
        },
        "EUR": {
          "balance": {
            "value_int": "130",
            "display_short": "0.00 EUR",
            "display": "0.00130 EUR",
            "currency": "EUR",
            "value": "0.00130"
          },
          "available": {
            "value_int": "130",
            "display_short": "0.00 EUR",
            "display": "0.00130 EUR",
            "currency": "EUR",
            "value": "0.00130"
          }
        },
        "BCH": {
          "balance": {
            "value_int": "0",
            "display_short": "0.00 BCH",
            "display": "0.00000000 BCH",
            "currency": "BCH",
            "value": "0.00000000"
          },
          "available": {
            "value_int": "0",
            "display_short": "0.00 BCH",
            "display": "0.00000000 BCH",
            "currency": "BCH",
            "value": "0.00000000"
          }
        }
      },
      "trade_fee": 0.25
    }
  }
  */
  Object main;
  main.parse (json);
  if (main.has<String>("result")) {
    string result = main.get<String>("result");
    if (result == "success") {
      if (main.has<Object>("data")) {
        Object data = main.get <Object> ("data");
        if (data.has<Object>("wallets")) {
          Object wallets = data.get<Object> ("wallets");
          vector <string> names = { "BTC", "EUR" };
          for (auto currency : names) {
            if (wallets.has<Object> (currency)) {
              Object btc = wallets.get<Object>(currency);
              Object balance = btc.get<Object>("balance");
              float value_int = str2float (balance.get<String> ("value_int"));
              if (value_int == 0) continue;
              string display = balance.get<String> ("display");
              string value = balance.get<String> ("value");
              coin_abbrev.push_back (currency);
              string coin_id = bl3p_get_coin_id (currency);
              if (coin_id.empty ()) coin_id = currency;
              coin_ids.push_back (coin_id);
              total.push_back (str2float (value));
              reserved.push_back (0);
              unconfirmed.push_back (0);
            }
          }
        }
        return;
      }
    }
  }
  error.append (json);
}


mutex bl3p_balance_mutex;
map <string, float> bl3p_balance_total;
map <string, float> bl3p_balance_available;
map <string, float> bl3p_balance_reserved;
map <string, float> bl3p_balance_unconfirmed;


void bl3p_get_balance (const string & coin, float * total, float * available, float * reserved, float * unconfirmed)
{
  bl3p_balance_mutex.lock ();
  if (total) * total = bl3p_balance_total [coin];
  if (available) * available = bl3p_balance_available [coin];
  if (reserved) * reserved = bl3p_balance_reserved [coin];
  if (unconfirmed) * unconfirmed = bl3p_balance_unconfirmed [coin];
  bl3p_balance_mutex.unlock ();
}


void bl3p_set_balance (const string & coin, float total, float available, float reserved, float unconfirmed)
{
  bl3p_balance_mutex.lock ();
  if (total != 0) bl3p_balance_total [coin] = total;
  if (available != 0) bl3p_balance_available [coin] = available;
  if (reserved != 0) bl3p_balance_reserved [coin] = reserved;
  if (unconfirmed != 0) bl3p_balance_unconfirmed [coin] = unconfirmed;
  bl3p_balance_mutex.unlock ();
}


vector <string> bl3p_get_coins_with_balances ()
{
  vector <string> coins;
  bl3p_balance_mutex.lock ();
  for (auto & element : bl3p_balance_total) {
    coins.push_back (element.first);
  }
  bl3p_balance_mutex.unlock ();
  return coins;
}


void bl3p_get_wallet_details (string coin, string & json, string & error,
                              string & address, string & payment_id)
{
  (void) payment_id;
  string coin_abbrev = bl3p_get_coin_abbrev (coin);
  json = get_bl3p_api_call_result (coin_abbrev + "EUR/money/deposit_address", {}, error, false);
  if (!error.empty ()) {
    return;
  }
  Object main;
  main.parse (json);
  if (main.has<String>("result")) {
    string result = main.get<String>("result");
    if (result == "success") {
      if (main.has<Object>("data")) {
        Object data = main.get <Object> ("data");
        if (data.has <String> ("address")) {
          address = data.get <String> ("address");
          return;
        }
      }
    }
  }
  error.append (json);
  return;
}


void bl3p_get_orderbook (const string & type,
                         const string & market,
                         const string & coin,
                         vector <float> & quantity,
                         vector <float> & rate,
                         string & error,
                         bool hurry)
{
  string market_abbrev = bl3p_get_coin_abbrev (market);
  string coin_abbrev = bl3p_get_coin_abbrev (coin);
  string json = get_bl3p_api_call_result (coin_abbrev + market_abbrev + "/orderbook", {}, error, hurry);
  if (!error.empty ()) {
    return;
  }
  Object main;
  main.parse (json);
  if (main.has<String>("result")) {
    string result = main.get<String>("result");
    if (result == "success") {
      if (main.has<Object>("data")) {
        Object data = main.get <Object> ("data");
        if (data.has <Array> (type)) {
          Array entries = data.get <Array> (type);
          for (size_t i = 0; i < entries.size(); i++) {
            Object values = entries.get <Object> (i);
            if (!values.has<Number>("count")) continue;
            if (!values.has<Number>("price_int")) continue;
            if (!values.has<Number>("amount_int")) continue;
            long int count = values.get<Number>("count");
            // The limit price in EUR (*1e5)
            long int price_int = values.get<Number>("price_int");
            // The amount in BTC (*1e8)
            long int amount_int = values.get<Number>("amount_int");
            for (int i = 0; i < count; i++) {
              float price = price_int / 1e5;
              float amount = amount_int / 1e8;
              quantity.push_back (amount * count);
              rate.push_back (price);
            }
          }
          return;
        }
      }
    }
  }
  // Error handler.
  error.append (json);
}


void bl3p_get_buyers (string market,
                      string coin,
                      vector <float> & quantity,
                      vector <float> & rate,
                      string & error,
                      bool hurry)
{
  bl3p_get_orderbook ("bids", market, coin, quantity, rate, error, hurry);
}


void bl3p_get_sellers (string market,
                       string coin,
                       vector <float> & quantity,
                       vector <float> & rate,
                       string & error,
                       bool hurry)
{
  bl3p_get_orderbook ("asks", market, coin, quantity, rate, error, hurry);
}


void bl3p_deposithistory (string coin,
                          vector <string> & order_ids,
                          vector <string> & coin_abbreviations,
                          vector <string> & coin_ids,
                          vector <float> & quantities,
                          vector <string> & addresses,
                          vector <string> & transaction_ids,
                          string & error)
{
  (void) order_ids;
  (void) coin_abbreviations;
  (void) coin_ids;
  (void) quantities;
  (void) addresses;
  (void) transaction_ids;
  (void) error;
  return;
  string coin_abbrev = bl3p_get_coin_abbrev (coin);
  vector <pair <string, string> > parameters = {
    make_pair ("currency", coin_abbrev),
    make_pair ("type", "deposit")
  };
  string json = get_bl3p_api_call_result ("GENMKT/money/wallet/history", parameters, error, false);
  if (error.empty ()) {
    Object main;
    main.parse (json);
    if (main.has<String>("result")) {
      string result = main.get<String>("result");
      if (result == "success") {
        if (main.has<Object>("data")) {
          Object data = main.get <Object> ("data");
          if (data.has<Array>("orders")) {
            Array orders = data.get<Array>("orders");
            for (size_t i = 0; i < orders.size(); i++) {
              Object values = orders.get <Object> (i);
              if (!values.has<String>("type")) continue;
              if (!values.has<String>("item")) continue;
              int order_id = values.get<Number>("order_id");
              string item = values.get<String>("item");
              string type = values.get<String>("type");
              Object price = values.get <Object> ("price");
              string currency = price.get<String>("currency");
              float price_display = str2float (price.get <String> ("display_short"));
              Object amount_funds = values.get <Object> ("amount_funds");
              float amount_funds_value = str2float (amount_funds.get <String> ("value"));
              (void) order_id;
              (void) price_display;
              (void) amount_funds_value;
            }
          }
          return;
        }
      }
    }
  }
  error.append (json);
}


int bl3p_get_hurried_call_passes ()
{
  return bl3p_hurried_call_pass;
}


int bl3p_get_hurried_call_fails ()
{
  return bl3p_hurried_call_fail;
}


bool bl3p_cancel_order (string order_id, string & error)
{
  vector <pair <string, string> > parameters = {
    make_pair ("order_id", order_id)
  };
  string json = get_bl3p_api_call_result ("BTCEUR/money/order/cancel", parameters, error, false);
  // { "result": "success", "data": {} }
  if (error.empty ()) {
    Object main;
    main.parse (json);
    if (main.has<String>("result")) {
      string result = main.get<String>("result");
      if (result == "success") {
        return true;
      }
    }
  }
  error.append (json);
  return false;
}


string bl3p_limit_trade (const string & market,
                         const string & coin,
                         const string & type,
                         float quantity,
                         float rate,
                         string & json,
                         string & error)
{
  int amount_int = quantity * 1e8;
  int price_int = rate * 1e5;
  string market_abbrev = bl3p_get_coin_abbrev (market);
  string coin_abbrev = bl3p_get_coin_abbrev (coin);
  vector <pair <string, string> > parameters = {
    make_pair ("type", type),
    make_pair ("amount_int", to_string (amount_int)),
    make_pair ("price_int", to_string (price_int)),
    make_pair ("fee_currency", market_abbrev),
  };
  json = get_bl3p_api_call_result (coin_abbrev + market_abbrev + "/money/order/add", parameters, error, false);
  // { "result": "success", "data": {"order_id": 26919224} }
  if (error.empty ()) {
    Object main;
    main.parse (json);
    if (main.has<String>("result")) {
      string result = main.get<String>("result");
      if (result == "success") {
        Object data = main.get<Object>("data");
        int order_id = data.get <Number> ("order_id");
        return to_string (order_id);
      }
    }
  }
  error.append (json);
  return "";
}


string bl3p_limit_buy (const string & market,
                       const string & coin,
                       float quantity,
                       float rate,
                       string & json,
                       string & error)
{
  return bl3p_limit_trade (market, coin, "bid", quantity, rate, json, error);
}


string bl3p_limit_sell (const string & market,
                        const string & coin,
                        float quantity,
                        float rate,
                        string & json,
                        string & error)
{
  return bl3p_limit_trade (market, coin, "ask", quantity, rate, json, error);
}
