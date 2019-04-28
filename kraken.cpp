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


#include "kraken.h"
#include "proxy.h"


#define api_key "--enter--your--value--here--"
#define api_secret "--enter--your--value--here--"


// Hurried call counters.
atomic <int> kraken_hurried_call_pass (0);
atomic <int> kraken_hurried_call_fail (0);


// See here: https://www.kraken.com/en-us/help/api
string kraken_get_api ()
{
  string url ("https://api.kraken.com");
  return url;
}


string kraken_get_path (const vector <string> & methods)
{
  string path ("/0");
  for (auto method : methods) path.append ("/" + method);
  return path;
}


// This ensures strictly increasing nonces.
atomic <long> kraken_private_api_call_sequencer (0);


// See here: https://www.kraken.com/en-us/help/api
string kraken_get_private_api_call (vector <string> methods,
                                    string & error,
                                    vector <pair <string, string> > parameters)
{
  // The path, e.g.:
  // /0/private/DepositAddresses
  string path = kraken_get_path (methods);

  // Sample URL for getting deposit addresses:
  // https://api.kraken.com/0/private/DepositAddresses
  string url = kraken_get_api () + path;
  
  // Ensure strictly increasing nonces.
  api_call_sequencer (kraken_private_api_call_sequencer);

  // Nonce: Strictly increasing.
  // The PHP sample given by Kraken used, for example, this nonce:
  // 1515003839819517
  // The length is 16 bytes.
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

  /*
   Sample PHP code for how to sign the request:

   $path = '/' . $version . '/private/' . $method;
   $sign = hash_hmac('sha512', $path . hash('sha256', $request['nonce'] . $postdata, true), base64_decode($secret), true);
   $headers = array('API-Key: ' . $key, 'API-Sign: ' . base64_encode($sign);
  */
 
  // Create the signature for the message.
  string sign = hmac_sha512_raw (base64_decode (api_secret), path + sha256_raw (nonce + postdata));
  
  // The security headers.
  vector <pair <string, string> > headers = {
    make_pair ("API-Key", api_key),
    make_pair ("API-Sign", base64_encode (sign))
  };

  // Private methods must use POST.
  string json = http_call (url, error, "", false, true, postdata, headers, false, true, "");

  // Done.
  return json;
}


string kraken_get_exchange ()
{
  return "kraken";
}


typedef struct
{
  const char * identifier;
  const char * abbreviation;
  const char * name;
  const char * kraken_asset;
}
kraken_coin_record;


kraken_coin_record kraken_coins_table [] =
{
  { "bitcoincash", "BCH", "BitcoinCash", "BCH" },
  { "dash", "DASH", "Dash", "DASH" },
  { "gnosis", "GNO", "Gnosis", "GNO" },
  { "ethereumclassic", "ETC", "Ethereum Classic", "XETC" },
  { "ethereum", "ETH", "Ethereum", "XETH" },
  { "litecoin", "LTC", "Litecoin", "XLTC" },
  { "melon", "MLN", "Melon", "XMLN" },
  { "namecoin", "NMC", "NameCoin", "XNMC" },
  { "augur", "REP", "Augur", "XREP" },
  { "bitcoin", "XBT", "Bitcoin", "XXBT" },
  { "lumen", "XLM", "Lumen", "XXLM" },
  { "monero", "XMR", "Monero", "XXMR" },
  { "ripple", "XRP", "Ripple", "XXRP" },
  { "zcash", "ZEC", "Zcash", "XZEC" },
  { "usdtether", "USDT", "Tether", "USDT" },
  { "euro", "EUR", "Euro", "ZEUR" },
  { "eos", "EOS", "EOS", "EOS" },
  
};


vector <string> kraken_get_coin_ids ()
{
  vector <string> ids;
  unsigned int data_count = sizeof (kraken_coins_table) / sizeof (*kraken_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    ids.push_back (kraken_coins_table[i].identifier);
  }
  return ids;
}


string kraken_get_coin_id (string coin)
{
  unsigned int data_count = sizeof (kraken_coins_table) / sizeof (*kraken_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == kraken_coins_table[i].identifier) return kraken_coins_table[i].identifier;
    if (coin == kraken_coins_table[i].abbreviation) return kraken_coins_table[i].identifier;
    if (coin == kraken_coins_table[i].name) return kraken_coins_table[i].identifier;
    if (coin == kraken_coins_table[i].kraken_asset) return kraken_coins_table[i].identifier;
  }
  return "";
}


string kraken_get_coin_abbrev (string coin)
{
  unsigned int data_count = sizeof (kraken_coins_table) / sizeof (*kraken_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == kraken_coins_table[i].identifier) return kraken_coins_table[i].abbreviation;
    if (coin == kraken_coins_table[i].abbreviation) return kraken_coins_table[i].abbreviation;
    if (coin == kraken_coins_table[i].name) return kraken_coins_table[i].abbreviation;
    if (coin == kraken_coins_table[i].kraken_asset) return kraken_coins_table[i].abbreviation;
  }
  return "";
}


string kraken_get_coin_name (string coin)
{
  unsigned int data_count = sizeof (kraken_coins_table) / sizeof (*kraken_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == kraken_coins_table[i].identifier) return kraken_coins_table[i].name;
    if (coin == kraken_coins_table[i].abbreviation) return kraken_coins_table[i].name;
    if (coin == kraken_coins_table[i].name) return kraken_coins_table[i].name;
    if (coin == kraken_coins_table[i].kraken_asset) return kraken_coins_table[i].name;
  }
  return "";
}


// Returns the Kraken asset name. E.g. "XXBT" for the Bitcoin.
string kraken_get_coin_asset (string coin)
{
  unsigned int data_count = sizeof (kraken_coins_table) / sizeof (*kraken_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == kraken_coins_table[i].identifier) return kraken_coins_table[i].kraken_asset;
    if (coin == kraken_coins_table[i].abbreviation) return kraken_coins_table[i].kraken_asset;
    if (coin == kraken_coins_table[i].name) return kraken_coins_table[i].kraken_asset;
    if (coin == kraken_coins_table[i].kraken_asset) return kraken_coins_table[i].kraken_asset;
  }
  return "";
}


void kraken_json_check (string & json)
{
  string error = "Web server is returning an unknown error";
  if (json.find (error) != string::npos) {
    json = error;
  }
}


void kraken_get_coins (const string & market,
                       vector <string> & asset_names,
                       vector <string> & alt_names,
                       string & error)
{
  (void) market;
  vector <string> methods = { "public", "Assets" };
  string url = kraken_get_api () + kraken_get_path (methods);
  // The GET request to the Kraken API is much faster than the POST request, so do a GET.
  string json = http_call (url, error, "", false, false, "", {}, false, false, "");
  if (!error.empty ()) {
    return;
  }
  kraken_json_check (json);
  // {
  //  "error":[],
  //  "result":{
  //   "BCH":{"aclass":"currency","altname":"BCH","decimals":10,"display_decimals":5},
  //   "DASH":{"aclass":"currency","altname":"DASH","decimals":10,"display_decimals":5},
  //   "EOS":{"aclass":"currency","altname":"EOS","decimals":10,"display_decimals":5},
  //   "XETH":{"aclass":"currency","altname":"ETH","decimals":10,"display_decimals":5},
  //   "XXBT":{"aclass":"currency","altname":"XBT","decimals":10,"display_decimals":5},
  //   .....
  //   }
  // }
  Object main;
  main.parse (json);
  if (main.has<Object>("result")) {
    Object assets = main.get<Object>("result");
    map <string, Value*> json_map = assets.kv_map ();
    for (auto key_value : json_map) {
      string asset = key_value.first;
      Object values = key_value.second->get<Object>();
      if (values.has<String>("altname")) {
        // The alternate name at times is slight different from the coin asset name.
        // This alternate name is more like the standard coin abbreviations.
        // See the JSON for examples.
        string altname = values.get<String>("altname");
        // Store the asset and the alternate name.
        asset_names.push_back (asset);
        alt_names.push_back (altname);
      }
    }
    return;
  }
  error.append (json);
}


void kraken_get_market_summaries (string market,
                                  vector <string> & coin_ids,
                                  vector <float> & bid,
                                  vector <float> & ask,
                                  string & error)
{
  // Start off with getting the tradable asset pairs at Kraken.
  vector <string> methods = { "public", "AssetPairs" };
  string url = kraken_get_api () + kraken_get_path (methods);
  // The GET requests to the Kraken API are much faster than the POST requests, so prefer GET..
  string json = http_call (url, error, "", false, false, "", {}, false, false, satellite_get_interface (url));
  if (!error.empty ()) {
    return;
  }
  kraken_json_check (json);
  // {
  //   "error":[],
  //   "result":{
  //     "BCHEUR":{"altname":"BCHEUR","aclass_base":"currency","base":"BCH",...},
  //     "BCHUSD":{"altname":"BCHUSD","aclass_base":"currency","base":"BCH",...},
  //     "XXBTZUSD":{"altname":"XBTUSD","aclass_base":"currency","base":"XXBT",...),
  //     "XXMRXXBT":{"altname":"XMRXBT","aclass_base":"currency","base":"XXMR",...),
  //     ...
  //   }
  // }

  // Filter out the assets tradable at the given market.
  string market_asset_name = kraken_get_coin_asset (market);
  vector <string> tradable_assets;
  
  // Parse the JSON that Kraken gave us.
  {
    Object main;
    main.parse (json);
    if (main.has<Object>("result")) {
      Object assets = main.get<Object>("result");
      map <string, Value*> json_map = assets.kv_map ();
      for (auto key_value : json_map) {
        string coin_market_pair = key_value.first;
        //Object values = key_value.second->get<Object>();
        size_t pos = coin_market_pair.find (market_asset_name);
        if (pos != string::npos) {
          if (pos >= 3) {
            // Skip items like "XETHXBT.d".
            if (coin_market_pair.find (".") == string::npos) {
              string tradable_asset = coin_market_pair.substr (0, pos);
              tradable_assets.push_back (tradable_asset);
            }
          }
        }
      }
    }
  }
  
  if (tradable_assets.empty ()) {
    error.append (json);
    return;
  }

  // Once the tradable assets at this market are known, proceed with getting their ticker information.
  // Several pairs can be added to the call, separated by a comma.
  string pairs;
  for (auto tradable_asset : tradable_assets) {
    if (!pairs.empty ()) pairs.append (",");
    pairs.append (tradable_asset);
    pairs.append (market_asset_name);
  }
  string postdata = "pair=" + pairs;
  methods = { "public", "Ticker" };
  url = kraken_get_api () + kraken_get_path (methods);
  json = http_call (url, error, "", false, true, postdata, {}, false, false, satellite_get_interface (url));
  if (!error.empty ()) {
    return;
  }
  kraken_json_check (json);
  // {
  //   "error":[],
  //   "result":{
  //     "XETCXXBT":{"a":["0.00243700","16","16.000"],"b":["0.00242100","2","2.000"],...},
  //     ...
  //    }
  // }

  // Parse the JSON.
  Object main2;
  main2.parse (json);
  if (main2.has<Object>("result")) {
    Object assets = main2.get<Object>("result");
    map <string, Value*> json_map = assets.kv_map ();
    for (auto key_value : json_map) {
      string coin_market_pair = key_value.first;
      size_t pos = coin_market_pair.find (market_asset_name);
      if (pos != string::npos) {
        // Get the tradable asset abbreviation.
        string kraken_asset_abbrev = coin_market_pair.substr (0, pos);
        string bot_coin_abbreviation = kraken_get_coin_abbrev (kraken_asset_abbrev);
        if (!bot_coin_abbreviation.empty ()) {
          Object values = key_value.second->get<Object>();
          if (values.has<Array>("a")) {
            if (values.has<Array>("b")) {
              Array a = values.get<Array>("a");
              Array b = values.get<Array>("b");
              float ask0 = str2float (a.get<String>(0));
              float bid0 = str2float (b.get<String>(0));
              string coin_id = kraken_get_coin_id (bot_coin_abbreviation);
              if (coin_id.empty ()) continue;
              coin_ids.push_back (coin_id);
              bid.push_back (bid0);
              ask.push_back (ask0);
            }
          }
        }
      }
    }
  }

  if (coin_ids.empty ()) {
    error.append (json);
  }
}


void kraken_get_wallet_details (string coin, string & json, string & error,
                                string & address, string & payment_id)
{
  vector <pair <string, string> > parameters;

  // Sample: string(47) "asset=XBT&method=Bitcoin&nonce=1515002803064857"
  string coin_abbrev = kraken_get_coin_abbrev (coin);
  parameters.push_back (make_pair ("asset", coin_abbrev));
  parameters.push_back (make_pair ("method", "Bitcoin"));

  json = kraken_get_private_api_call ({"private", "DepositAddresses"}, error, parameters);
  if (!error.empty ()) {
    return;
  }
  
  kraken_json_check (json);

  // {"error":[],"result":[{"address":"3FVm21y2UNRZMwFDi98KiZbVqbgh7bQbG5","expiretm":"0","new":true},{"address":"3DJFBgYs7dwVey2mAaZHhoLb17twWogiFJ","expiretm":"0","new":true}]}
  
  Object main;
  main.parse (json);
  if (main.has<Array>("result")) {
    Array result = main.get<Array>("result");
    for (size_t i = 0; i < result.size(); i++) {
      Object values = result.get <Object> (i);
      if (!values.has<String>("address")) continue;
      address = values.get<String>("address");
      return;
    }
  }
  
  error.append (json);
  
  (void) payment_id;
}


// Gets the $total balance that includes also the following two amounts:
// 1. The balances $reserved for trade orders and for withdrawals.
// 2. The $unconfirmed balance: What is being deposited right now.
// It also gives all matching coin abbreviations.
void kraken_get_balances (vector <string> & coin_abbrev,
                          vector <string> & coin_ids,
                          vector <float> & total,
                          vector <float> & reserved,
                          vector <float> & unconfirmed,
                          string & error)
{
  // Get the JSON for the balances.
  string json = kraken_get_private_api_call ({"private", "Balance"}, error, {});
  if (!error.empty ()) {
    return;
  }
  
  // Check on known issues.
  kraken_json_check (json);
  
  // Since the Kraken "Balance" call returns only the total balances,
  // and does not return the reserved or unconfirmed balances,
  // another call is needed for getting the reserved balances.
  // The reserved balances will be derived from the open orders.
  // The unconfirmed balances won't be obtained right now, as not needed, really.
  // The reserved balances will be stored as: container [coin] = reserved-balance.
  map <string, float> reserved_balances;
  {
    vector <string> dummy;
    vector <string> markets;
    vector <string> coin_ids;
    vector <string> buys_sells;
    vector <float> quantities;
    vector <float> rates;
    kraken_get_open_orders (dummy, markets, dummy, coin_ids, buys_sells, quantities, rates, dummy, error);
    if (!error.empty ()) {
      return;
    }
    for (unsigned int i = 0; i < coin_ids.size (); i++) {
      string market = markets [i];
      string coin = coin_ids[i];
      string buy_sell = buys_sells[i];
      float coin_quantity = quantities[i];
      float rate = rates[i];
      float market_quantity = coin_quantity * rate;
      if (buy_sell == "buy") {
        // Open order to buy the coin: The market coin has a reserved balance.
        reserved_balances [market] += market_quantity;
      } else {
        // Open order to sell the coin: The coin itself has a reserved balance.
        reserved_balances [coin] += coin_quantity;
      }
    }
  }
 
  // Sample JSON of the "Balance" call.
  // {"error":[],"result":{"XXBT":"0.4992000000"}}

  Object main;
  main.parse (json);
  if (main.has<Object>("result")) {
    Object result = main.get<Object>("result");
    map <string, Value*> json_map = result.kv_map ();
    for (auto key_value : json_map) {
      string coin_abbreviation = key_value.first;
      string balance = result.get <String>(coin_abbreviation);
      if (!balance.empty ()) {
        string coin_id = kraken_get_coin_id (coin_abbreviation);
        float value = str2float (balance);
        coin_abbrev.push_back (coin_abbreviation);
        coin_ids.push_back (coin_id);
        total.push_back (value);
        reserved.push_back (reserved_balances[coin_id]);
        unconfirmed.push_back (0);
      }
    }
    return;
  }

  error.append (json);
}


mutex kraken_balance_mutex;
map <string, float> kraken_balance_total;
map <string, float> kraken_balance_available;
map <string, float> kraken_balance_reserved;
map <string, float> kraken_balance_unconfirmed;


void kraken_get_balance (const string & coin, float * total, float * available, float * reserved, float * unconfirmed)
{
  kraken_balance_mutex.lock ();
  if (total) * total = kraken_balance_total [coin];
  if (available) * available = kraken_balance_available [coin];
  if (reserved) * reserved = kraken_balance_reserved [coin];
  if (unconfirmed) * unconfirmed = kraken_balance_unconfirmed [coin];
  kraken_balance_mutex.unlock ();
}


void kraken_set_balance (const string & coin, float total, float available, float reserved, float unconfirmed)
{
  kraken_balance_mutex.lock ();
  if (total != 0) kraken_balance_total [coin] = total;
  if (available != 0) kraken_balance_available [coin] = available;
  if (reserved != 0) kraken_balance_reserved [coin] = reserved;
  if (unconfirmed != 0) kraken_balance_unconfirmed [coin] = unconfirmed;
  kraken_balance_mutex.unlock ();
}


vector <string> kraken_get_coins_with_balances ()
{
  vector <string> coins;
  kraken_balance_mutex.lock ();
  for (auto & element : kraken_balance_total) {
    coins.push_back (element.first);
  }
  kraken_balance_mutex.unlock ();
  return coins;
}


void kraken_get_open_orders (vector <string> & identifier,
                             vector <string> & market,
                             vector <string> & coin_abbreviation,
                             vector <string> & coin_ids,
                             vector <string> & buy_sell,
                             vector <float> & quantity,
                             vector <float> & rate,
                             vector <string> & date,
                             string & error)
{
  string json = kraken_get_private_api_call ({"private", "OpenOrders"}, error, {});
  if (!error.empty ()) {
    return;
  }
  
  kraken_json_check (json);
  
  // Sample JSON for two identical open orders.
  //{
  //   "error":[
  //
  //   ],
  //   "result":{
  //      "open":{
  //         "O3ONGS-MWANT-ZXOU43":{
  //            "refid":null,
  //            "userref":0,
  //            "status":"open",
  //            "opentm":1517161139.7076,
  //            "starttm":0,
  //            "expiretm":0,
  //            "descr":{
  //               "pair":"XRPXBT",
  //               "type":"buy",
  //               "ordertype":"limit",
  //               "price":"0.00005000",
  //               "price2":"0",
  //               "leverage":"none",
  //               "order":"buy 100.00000000 XRPXBT @ limit 0.00005000",
  //               "close":""
  //            },
  //            "vol":"100.00000000",
  //            "vol_exec":"0.00000000",
  //            "cost":"0.000000000",
  //            "fee":"0.000000000",
  //            "price":"0.000000000",
  //            "stopprice":"0.000000000",
  //            "limitprice":"0.000000000",
  //            "misc":"",
  //            "oflags":"fciq"
  //         },
  //         "OELAFI-B7VOH-KISHT6":{
  //            "refid":null,
  //            "userref":0,
  //            "status":"open",
  //            "opentm":1517159031.2498,
  //            "starttm":0,
  //            "expiretm":0,
  //            "descr":{
  //               "pair":"XRPXBT",
  //               "type":"buy",
  //               "ordertype":"limit",
  //               "price":"0.00005000",
  //               "price2":"0",
  //               "leverage":"none",
  //               "order":"buy 100.00000000 XRPXBT @ limit 0.00005000",
  //               "close":""
  //            },
  //            "vol":"100.00000000",
  //            "vol_exec":"0.00000000",
  //            "cost":"0.000000000",
  //            "fee":"0.000000000",
  //            "price":"0.000000000",
  //            "stopprice":"0.000000000",
  //            "limitprice":"0.000000000",
  //            "misc":"",
  //            "oflags":"fciq"
  //         }
  //      }
  //   }
  //}

  Object main;
  main.parse (json);
  if (main.has<Object>("result")) {
    Object result = main.get<Object>("result");
    if (result.has<Object>("open")) {
      Object open = result.get<Object>("open");
      map <string, Value*> json_map = open.kv_map ();
      for (auto key_value : json_map) {
        string order_id = key_value.first;
        Object order_values = key_value.second->get<Object>();
        if (!order_values.has<Number>("opentm")) continue;
        int opentm = order_values.get<Number>("opentm");
        if (!order_values.has<String>("vol")) continue;
        string vol = order_values.get<String>("vol");
        if (order_values.has<Object>("descr")) {
          Object descr = order_values.get<Object>("descr");
          if (!descr.has<String>("pair")) continue;
          string coin_market_pair = descr.get<String>("pair");
          if (!descr.has<String>("type")) continue;
          string type = descr.get<String>("type");
          if (!descr.has<String>("price")) continue;
          string price = descr.get<String>("price");
          if (coin_market_pair.size () != 6) continue;
          identifier.push_back (order_id);
          string coin_abbrev = coin_market_pair.substr (0, 3);
          string market_abbrev = coin_market_pair.substr (3);
          market.push_back (kraken_get_coin_id (market_abbrev));
          coin_abbreviation.push_back (kraken_get_coin_abbrev (coin_abbrev));
          coin_ids.push_back (kraken_get_coin_id (coin_abbrev));
          buy_sell.push_back (type);
          quantity.push_back (str2float (vol));
          rate.push_back (str2float (price));
          date.push_back (to_string (opentm));
        }
      }
    }
    // Done.
    return;
  }
  
  // Error handler.
  error.append (json);
}


void kraken_get_market_orders (string market,
                               string coin,
                               string direction,
                               vector <float> & quantity,
                               vector <float> & rate,
                               string & error,
                               bool hurry)
{
  // Translate the market.
  string market_asset = kraken_get_coin_asset (market);

  // Translate the coin.
  string coin_asset = kraken_get_coin_asset (coin);
  
  // Call the API.
  string coin_market = coin_asset + market_asset;
  string postdata = "pair=" + coin_market;
  postdata.append ("&");
  postdata.append ("count=15");
  vector <string> methods = {"public", "Depth"};
  string url = kraken_get_api () + kraken_get_path (methods);
  string json = http_call (url, error, "", false, true, postdata, {}, hurry, false, "");
  // Error handling.
  if (!error.empty ()) {
    if (hurry) kraken_hurried_call_fail++;
    return;
  }
  
  // Check on errors specific to kraken.
  kraken_json_check (json);

  // {
  //   "error":[],
  //   "result":{
  //     "XETHXXBT":{
  //       "asks":[
  //         ["0.046750","180.076",1511893343],
  //         ["0.046850","88.818",1511893329],
  //         ["0.046860","18.478",1511893256],
  //         ...
  //       ],
  //       "bids":[
  //         ["0.046700","0.022",1511893253],
  //         ["0.046660","2.167",1511893306],
  //         ["0.046640","0.863",1511893243],
  //         ...
  //       ]
  //     }
  //   }
  // }

  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has <Object> ("result")) {
    Object result = main.get <Object> ("result");
    if (result.has <Object> (coin_market)) {
      Object asset = result.get <Object> (coin_market);
      if (asset.has <Array> (direction)) {
        Array values = asset.get <Array> (direction);
        for (size_t i = 0; i < values.size(); i++) {
          Array price_quantity = values.get <Array> (i);
          float price = str2float (price_quantity.get <String> (0));
          float amount = str2float (price_quantity.get <String> (1));
          quantity.push_back (amount);
          rate.push_back (price);
        }
      }
    }
  }

  // Error handling.
  if (quantity.empty ()) {
    if (hurry) kraken_hurried_call_fail++;
    error.append (json);
  } else {
    if (hurry) kraken_hurried_call_pass++;
  }
}


void kraken_get_buyers (string market,
                        string coin,
                        vector <float> & quantity,
                        vector <float> & rate,
                        string & error,
                        bool hurry)
{
  kraken_get_market_orders (market, coin, "bids", quantity, rate, error, hurry);
}


void kraken_get_sellers (string market,
                         string coin,
                         vector <float> & quantity,
                         vector <float> & rate,
                         string & error,
                         bool hurry)
{
  kraken_get_market_orders (market, coin, "asks", quantity, rate, error, hurry);
}


string kraken_limit_trade (const string & market,
                           const string & coin,
                           const string & type,
                           float quantity,
                           float rate,
                           string & json,
                           string & error)
{
  // Translate the market and coin.
  string market_asset = kraken_get_coin_asset (market);
  string coin_asset = kraken_get_coin_asset (coin);
  string coin_market = coin_asset + market_asset;

  // Set the parameters.
  vector <pair <string, string> > parameters;
  parameters.push_back (make_pair ("pair", coin_market));
  parameters.push_back (make_pair ("type", type));
  parameters.push_back (make_pair ("ordertype", "limit"));
  // Deal with these types of errors:
  // EOrder:Invalid price:XXMRXXBT price can only be specified up to 6 decimals.
  // EOrder:Invalid price:XZECXXBT price can only be specified up to 5 decimals.
  // EOrder:Invalid price:XXBTZEUR price can only be specified up to 1 decimals.
  int decimals = 5;
  if (market == euro_id ()) {
    decimals = 1;
  }
  parameters.push_back (make_pair ("price", float2string (rate, decimals)));
  parameters.push_back (make_pair ("volume", float2string (quantity, 6)));
  
  // Call the API.
  json = kraken_get_private_api_call ({"private", "AddOrder"}, error, parameters);
  if (!error.empty ()) {
    return "";
  }

  // Check on standard Kraken issues.
  kraken_json_check (json);

  // {"error":[],"result":{"descr":{"order":"buy 100.00000000 XRPXBT @ limit 0.00005000"},"txid":["OELAFI-B7VOH-KISHT6"]}}

  Object main;
  main.parse (json);
  if (main.has<Object>("result")) {
    Object result = main.get<Object>("result");
    if (result.has<Array>("txid")) {
      Array txid = result.get<Array>("txid");
      if (!txid.empty ()) {
        return txid.get<String> (0);
      }
    }
  }
  
  // Error handler.
  error.append (json);
  return "";
}


string kraken_limit_buy (const string & market,
                         const string & coin,
                         float quantity,
                         float rate,
                         string & json,
                         string & error)
{
  return kraken_limit_trade (market, coin, "buy", quantity, rate, json, error);
}


string kraken_limit_sell (const string & market,
                          const string & coin,
                          float quantity,
                          float rate,
                          string & json,
                          string & error)
{
  return kraken_limit_trade (market, coin, "sell", quantity, rate, json, error);
}


bool kraken_cancel_order (string order_id, string & error)
{
  // Set the parameters.
  vector <pair <string, string> > parameters;
  parameters.push_back (make_pair ("txid", order_id));
  
  // Call the API.
  string json = kraken_get_private_api_call ({"private", "CancelOrder"}, error, parameters);
  if (!error.empty ()) {
    return false;
  }
  
  // Check on standard Kraken issues.
  kraken_json_check (json);

  // Cancel failure: {"error":["EOrder:Unknown order"]}
  // Cancel success: {"error":[],"result":{"count":1}}

  Object main;
  main.parse (json);
  if (main.has<Object>("result")) {
    return true;
  }
  
  // Error handler.
  error.append (json);
  return false;
}


/*
string kraken_withdrawal_order (string coin_abbreviation,
                                   float quantity,
                                   string address,
                                   string paymentid,
                                   string & error)
{
  string url = kraken_get_private_api_call ({ "SubmitWithdraw" });
  
  Object post;
  post << "Currency" << coin_abbreviation;
  post << "Address" << address;
  if (!paymentid.empty ()) {
    post << "PaymentId" << paymentid;
  }
  post << "Amount" << quantity;
  string post_data = post.json ();
  
  vector <pair <string, string> > headers;
  headers.push_back (make_pair ("Content-Type", "application/json; charset=utf-8"));
  headers.push_back (make_pair ("Authorization", kraken_get_authorization (url, post_data)));
  
  string json = http_call (url, error, false, true, post_data, headers);
  if (!error.empty ()) {
    return "";
  }

  kraken_json_check (json);

  Object main;
  main.parse (json);
  if (main.has<Boolean>("Success")) {
    if (main.get<Boolean>("Success")) {
      if (main.has<Number>("Data")) {
        int data = main.get<Number>("Data");
        return to_string (data);
      }
    }
  }
  
  // There have been cases that the kraken API returned this:
  // {"Success":false,"Error":"Nonce has already been used for this request."}
  // One would expect that if this is returned, the limit trade order has not been placed.
  // But in some cases it returns the above error, and also places the limit trade order.
  // So every error of this type needs investigation whether the order was placed or not.
  error.append (json);
  return "";
}


void kraken_get_transactions (string type,
                                 vector <string> & order_ids,
                                 vector <string> & coin_abbreviations,
                                 vector <float> & quantities,
                                 vector <string> & addresses,
                                 vector <string> & transaction_ids,
                                 string & error)
{
  string url = kraken_get_private_api_call ({ "GetTransactions" });
  
  Object post;
  post << "Type" << type;
  string post_data = post.json ();
  
  vector <pair <string, string> > headers;
  headers.push_back (make_pair ("Content-Type", "application/json; charset=utf-8"));
  headers.push_back (make_pair ("Authorization", kraken_get_authorization (url, post_data)));
  
  string json = http_call (url, error, false, true, post_data, headers);
  if (!error.empty ()) {
    return;
  }

  kraken_json_check (json);

  //
   { "Success":true, "Error":null, "Data":[
     {
       "Id": 23467,
       "Currency": "DOT",
       "TxId": "6ddbaca454c97ba4e8a87a1cb49fa5ceace80b89eaced84b46a8f52c2b8c8ca3",
       "Type": "Deposit",
       "Amount": 145.98000000,
       "Fee": "0.00000000",
       "Status": "Confirmed",
       "Confirmations": "20",
       "TimeStamp":"2014-12-07T20:04:05.3947572",
       "Address": ""
     },
     {
       "Id": 23467,
       "Currency": "DOT",
       "TxId": "9281eacaad58335b884adc24be884c00200a4fc17b2e05c72e255976223de187",
       "Type": "Withdraw",
       "Amount": 1000.00000000,
       "Fee": "0.00004000",
       "Status": "Pending",
       "Confirmations": "20",
       "TimeStamp":"2014-12-07T20:04:05.3947572",
       "Address": "15wPaAegfKai51KK2yemgLP5vEg5UWzSkC"
     }
   ]}
 
  Object main;
  main.parse (json);
  if (main.has<Boolean>("Success")) {
    if (main.get<Boolean>("Success")) {
      if (main.has<Array>("Data")) {
        Array data = main.get<Array>("Data");
        for (size_t i = 0; i < data.size(); i++) {
          Object values = data.get <Object> (i);
          if (!values.has<Number>("Id")) continue;
          int Id = values.get<Number> ("Id");
          if (!values.has<String>("Currency")) continue;
          string Currency = values.get<String> ("Currency");
          if (!values.has<Number>("Amount")) continue;
          float Amount = values.get<Number> ("Amount");
          // The address will be null when requesting the deposit history.
          string Address;
          if (values.has<String>("Address")) {
            Address = values.get<String> ("Address");
          }
          // The transfer ID may be given or be absent.
          string TxId;
          if (values.has<String>("TxId")) {
            TxId = values.get <String>("TxId");
          }
          order_ids.push_back (to_string (Id));
          coin_abbreviations.push_back (Currency);
          quantities.push_back (Amount);
          addresses.push_back (Address);
          transaction_ids.push_back (TxId);
        }
      }
      return;
    }
  }
  
  error.append (json);
}


void kraken_withdrawalhistory (vector <string> & order_ids,
                                  vector <string> & coin_abbreviations,
                                  vector <float> & quantities,
                                  vector <string> & addresses,
                                  vector <string> & transaction_ids,
                                  string & error)
{
  kraken_get_transactions ("Withdraw", order_ids, coin_abbreviations, quantities, addresses, transaction_ids, error);
}


void kraken_deposithistory (vector <string> & order_ids,
                               vector <string> & coin_abbreviations,
                               vector <float> & quantities,
                               vector <string> & addresses,
                               vector <string> & transaction_ids,
                               string & error)
{
  kraken_get_transactions ("Deposit", order_ids, coin_abbreviations, quantities, addresses, transaction_ids, error);
}


// This is for testing repeating a call after it failed.
void kraken_unit_test (int test, string & error)
{
  string call = "GetBalance";
  if (test == 1) call = "CancelTrade";
  
  string url = kraken_get_private_api_call ({ call });
  
  string post_data = "{\"Currency\":\"\"}";
  
  vector <pair <string, string> > headers;
  headers.push_back (make_pair ("Content-Type", "application/json; charset=utf-8"));
  headers.push_back (make_pair ("Authorization", kraken_get_authorization (url, post_data)));
  
  error.clear ();
  string json = http_call (url, error, false, true, post_data, headers);
  if (!error.empty ()) {
    return;
  }
  
  kraken_json_check (json);
  
  Object main;
  main.parse (json);
  if (main.has<Boolean>("Success")) {
    if (main.get<Boolean>("Success")) {
      if (main.has<Array>("Data")) {
        Array data = main.get<Array>("Data");
        for (size_t i = 0; i < data.size(); i++) {
          Object values = data.get <Object> (i);
          if (!values.has<String>("Symbol")) continue;
          string Symbol = values.get<String>("Symbol");
          if (!values.has<Number>("Total")) continue;
          if (!values.has<Number>("Available")) continue;
          if (!values.has<Number>("Unconfirmed")) continue;
          if (!values.has<Number>("HeldForTrades")) continue;
          if (!values.has<Number>("PendingWithdraw")) continue;
        }
        return;
      }
    }
  }
  error.append (json);
}
*/


int kraken_get_hurried_call_passes ()
{
  return kraken_hurried_call_pass;
}


int kraken_get_hurried_call_fails ()
{
  return kraken_hurried_call_fail;
}
