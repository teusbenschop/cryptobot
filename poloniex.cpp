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


#include "poloniex.h"
#include "proxy.h"


#define api_key "--enter--your--value--here--"
#define api_secret "--enter--your--value--here--"


// Hurried call counters.
atomic <int> poloniex_hurried_call_pass (0);
atomic <int> poloniex_hurried_call_fail (0);


typedef struct
{
  const char * identifier;
  const char * abbreviation;
  const char * name;
}
poloniex_coin_record;


poloniex_coin_record poloniex_coins_table [] =
{
  { "bitcoin", "BTC", "Bitcoin" },
  { "synereoamp", "AMP", "Synereo AMP" },
  { "bitcoincash", "BCH", "BitcoinCash" },
  // { "bitcrystals", "BCY", "BitCrystals" }, delist
  // { "blackcoin", "BLK", "BlackCoin" }, delist
  { "bitmark", "BTM", "Bitmark" },
  { "burstcoin", "BURST", "BURSTCoin" },
  { "clamcoin", "CLAM", "ClamCoin" },
  { "civic", "CVC", "Civic" },
  { "dash", "DASH", "Dash" },
  { "decred", "DCR", "Decred" },
  { "digibyte", "DGB", "DigiByte" },
  { "dogecoin", "DOGE", "Dogecoin" },
  { "einsteinium", "EMC2", "Einsteinium" },
  { "ethereumclassic", "ETC", "Ethereum Classic" },
  { "ethereum", "ETH", "Ethereum" },
  { "expanse", "EXP", "Expanse" },
  { "factom", "FCT", "Factom" },
  { "gamecredits", "GAME", "GameCredits" },
  { "gnosis", "GNO", "Gnosis" },
  { "golem", "GNT", "Golem" },
  { "gridcoin", "GRC", "GridCoin" },
  { "lbrycredits", "LBC", "LBRY Credits" },
  { "lisk", "LSK", "Lisk" },
  { "litecoin", "LTC", "Litecoin" },
  { "maidsafecoin", "MAID", "MaidSafeCoin" },
  { "navcoin", "NAV", "NavCoin" },
  { "neoscoin", "NEOS", "NeosCoin" },
  { "namecoin", "NMC", "NameCoin" },
  // { "nexium", "NXC", "Nexium" }, delist
  { "nxt", "NXT", "NXT" },
  { "omisego", "OMG", "OmiseGo" },
  { "omni", "OMNI", "OMNI" },
  { "potcoin", "POT", "Potcoin" },
  { "peercoin", "PPC", "PeerCoin" },
  // { "radium", "RADS", "Radium" }, delist
  { "augur", "REP", "Augur" },
  { "steemdollars", "SBD", "SteemDollars" },
  { "siacoin", "SC", "Siacoin" },
  { "steem", "STEEM", "STEEM" },
  { "storj", "STORJ", "Storj" },
  { "stratis", "STRAT", "Stratis" },
  { "syscoin", "SYS", "SysCoin" },
  { "viacoin", "VIA", "ViaCoin " },
  { "vericoin", "VRC", "VeriCoin" },
  { "vertcoin", "VTC", "Vertcoin" },
  { "bitcoinplus", "XBC", "BitcoinPlus" },
  { "counterparty", "XCP", "Counterparty" },
  { "neweconomymovement", "XEM", "NewEconomyMovement" },
  { "monero", "XMR", "Monero" },
  { "primecoin", "XPM", "PrimeCoin" },
  { "ripple", "XRP", "Ripple" },
  { "zcash", "ZEC", "ZCash" },
  { "ardor", "ARDR", "Ardor" },
  { "0xprotocol", "ZRX", "0x Protocol" },
  { "usdtether", "USDT", "USD Tether" },
  { "eos", "EOS", "EOS" },
  { "kybernetworkcrystal", "KNC", "KyberNetworkCrystal" },
  { "statusnetworktoken", "SNT", "Status Network Token" },

};


vector <string> poloniex_get_coin_ids ()
{
  vector <string> ids;
  unsigned int data_count = sizeof (poloniex_coins_table) / sizeof (*poloniex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    ids.push_back (poloniex_coins_table[i].identifier);
  }
  return ids;
}


string poloniex_get_coin_id (string coin)
{
  unsigned int data_count = sizeof (poloniex_coins_table) / sizeof (*poloniex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == poloniex_coins_table[i].identifier) return poloniex_coins_table[i].identifier;
    if (coin == poloniex_coins_table[i].abbreviation) return poloniex_coins_table[i].identifier;
    if (coin == poloniex_coins_table[i].name) return poloniex_coins_table[i].identifier;
  }
  return "";
}


string poloniex_get_coin_abbrev (string coin)
{
  unsigned int data_count = sizeof (poloniex_coins_table) / sizeof (*poloniex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == poloniex_coins_table[i].identifier) return poloniex_coins_table[i].abbreviation;
    if (coin == poloniex_coins_table[i].abbreviation) return poloniex_coins_table[i].abbreviation;
    if (coin == poloniex_coins_table[i].name) return poloniex_coins_table[i].abbreviation;
  }
  return "";
}


string poloniex_get_coin_name (string coin)
{
  unsigned int data_count = sizeof (poloniex_coins_table) / sizeof (*poloniex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == poloniex_coins_table[i].identifier) return poloniex_coins_table[i].name;
    if (coin == poloniex_coins_table[i].abbreviation) return poloniex_coins_table[i].name;
    if (coin == poloniex_coins_table[i].name) return poloniex_coins_table[i].name;
  }
  return "";
}


// https://poloniex.com/support/api/
string get_poloniex_public_api_call (vector <string> endpoints, vector <pair <string, string> > parameters)
{
  // The url of the API website.
  string url ("https://poloniex.com");
  for (auto part : endpoints) {
    url.append ("/");
    url.append (part);
  }
  // Add the query parameters.
  for (auto element : parameters) {
    string key = element.first;
    string value = element.second;
    if (url.find ("?") == string::npos) url.append ("?");
    else url.append ("&");
    url.append (key);
    url.append ("=");
    url.append (value);
  }
  // Done.
  return url;
}


// Kind of ensure always increasing nonces to be received by the Poloniex API.
// The sequencer is used for properly spacing the subsequent private API calls.
atomic <long> poloniex_private_api_call_sequencer (0);


// https://poloniex.com/support/api/
string get_poloniex_private_api_call_result (string command,
                                             vector <pair <string, string> > parameters,
                                             string & error)
{
  // Ensure there's enough time between this private API call and the most recent private API call.
  api_call_sequencer (poloniex_private_api_call_sequencer);

  // The url to POST to.
  string url = "https://poloniex.com/tradingApi";

  // Nonce: Strictly increasing.
  // The PHP sample given by Poloniex used, for example, this nonce:
  // 1514272672575483
  // So the length is 16 bytes.
  string nonce = increasing_nonce ();
  nonce.erase (16);
  
  // The payload to POST.
  string postdata;
  parameters.push_back (make_pair ("command", command));
  parameters.push_back (make_pair ("nonce", nonce));
  for (auto parameter : parameters) {
    if (!postdata.empty ()) postdata.append ("&");
    postdata.append (parameter.first);
    postdata.append ("=");
    postdata.append (parameter.second);
  }

  // The security headers.
  string signature = hmac_sha512_hexits (api_secret, postdata);
  vector <pair <string, string> > headers = {
    make_pair ("Key", api_key),
    make_pair ("Sign", signature)
  };
  
  // Call API.
  // Do not call it via a proxy,
  // because the trading API at Poloniex is restricted to certain trusted IP addresses,
  // and the addresses of the proxies are not trusted.
  // If a proxy is incorrectly configured,
  // other people could access the Poloniex trading API from those proxies.
  string json = http_call (url, error, "", false, true, postdata, headers, false, true, "");

  // Done.
  return json;
}


string poloniex_get_exchange ()
{
  return "poloniex";
}


void poloniex_get_coins (const string & market,
                         vector <string> & coin_abbrevs,
                         string & error)
{
  // Call the API to get the exchange's tickers.
  // The tickers also provide the markets-coin pairs.
  string url = get_poloniex_public_api_call ({"public"}, { make_pair ("command", "returnTicker") });
  string json = http_call (url, error, "", false, false, "", {}, false, false, "");
  if (!error.empty ()) {
    return;
  }
  // {
  //   "BTC_BCN":{"id":7,"last":"0.00000019","lowestAsk":"0.00000019","highestBid":"0.00000018", ...},
  //   "BTC_BELA":{"id":8,"last":"0.00001178","lowestAsk":"0.00001178","highestBid":"0.00001174", ...},
  //   ...
  // }
  // This is about getting the available coins at the exchange.
  // Most coins will be given, just now, in the Bitcoin market.
  // So look for those, and remove the BTC abbreviaton.
  // This results in the available coin abbreviations at Poloniex.

  // Parse the JSON.
  string market_abbrev = poloniex_get_coin_abbrev (market);
  Object main;
  main.parse (json);
  map <string, Value*> json_map = main.kv_map ();
  for (auto key_value : json_map) {
    string market_coin_pair = key_value.first;
    size_t pos = market_coin_pair.find (market_abbrev + "_");
    if (pos == 0) {
      string coin_abbrev = market_coin_pair.substr (market_abbrev.size () + 1);
      coin_abbrevs.push_back (coin_abbrev);
    }
  }
  
  // Error handler.
  if (coin_abbrevs.empty ()) {
    error.append (json);
  }
}


void poloniex_get_market_summaries (string market,
                                    vector <string> & coin_ids,
                                    vector <float> & bid,
                                    vector <float> & ask,
                                    string & error)
{
  // Call the API to get the exchange's tickers.
  // The tickers also provide the markets-coin pairs.
  string url = get_poloniex_public_api_call ({"public"}, { make_pair ("command", "returnTicker") });
  string json = http_call (url, error, "", false, false, "", {}, false, false, satellite_get_interface (url));
  if (!error.empty ()) {
    return;
  }
  // {
  //   "BTC_BCN":{"id":7,"last":"0.00000019","lowestAsk":"0.00000019","highestBid":"0.00000018", ...},
  //   "BTC_BELA":{"id":8,"last":"0.00001178","lowestAsk":"0.00001178","highestBid":"0.00001174", ...},
  //   ...
  // }
  
  // The abbreviation of the market, e.g.: "BTC_".
  string market_abbreviation = poloniex_get_coin_abbrev (market) +  "_";
  
  // Parse the JSON.
  Object main;
  main.parse (json);
  map <string, Value*> json_map = main.kv_map ();
  for (auto key_value : json_map) {
    string market_coin_pair = key_value.first;
    size_t pos = market_coin_pair.find (market_abbreviation);
    if (pos == 0) {
      // From e.g. ""BTC_BELA", get the last component: "BELA".
      string coin_abbrev = market_coin_pair.substr (market_abbreviation.size());
      Object values = key_value.second->get<Object>();
      if (values.has<String>("lowestAsk")) {
        if (values.has<String>("highestBid")) {
          string lowestAsk = values.get<String>("lowestAsk");
          string highestBid = values.get<String>("highestBid");
          string coin_id = poloniex_get_coin_id (coin_abbrev);
          if (coin_id.empty ()) continue;
          coin_ids.push_back (coin_id);
          bid.push_back (str2float (highestBid));
          ask.push_back (str2float (lowestAsk));
        }
      }
    }
  }
  
  // Error handler.
  if (coin_ids.empty ()) {
    error.append (json);
  }
}


void poloniex_return_order_book (string market,
                                 string coin,
                                 string direction,
                                 vector <float> & quantity,
                                 vector <float> & rate,
                                 string & error,
                                 bool hurry)
{
  // Get the currency pair, e.g. "BTC_BELA".
  string market_abbrev = poloniex_get_coin_abbrev (market);
  string coin_abbrev = poloniex_get_coin_abbrev (coin);
  string market_coin_pair = market_abbrev + "_" + coin_abbrev;
  
  // Call the API to get the exchange's order book for the currency at the requested base market.
  string url = get_poloniex_public_api_call ({"public"}, { make_pair ("command", "returnOrderBook"), make_pair ("currencyPair", market_coin_pair) });
  string json = http_call (url, error, "", false, false, "", {}, hurry, false, "");
  if (!error.empty ()) {
    if (hurry) poloniex_hurried_call_fail++;
    return;
  }

  // {
  //   "asks":[
  //     ["0.06517196",17.4640791],
  //     ["0.06517197",15.47475326],
  //     ...
  //   ],
  //   "bids":[
  //     ["0.06500026",2.20891514],
  //     ["0.06500000",30.76905351],
  //     ...
  //   ],
  //   "isFrozen":"0",
  //   "seq":165934452
  // }

  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has <Array> (direction)) {
    Array values = main.get <Array> (direction);
    for (size_t i = 0; i < values.size(); i++) {
      Array price_quantity = values.get <Array> (i);
      float price = str2float (price_quantity.get<String>(0));
      float amount = price_quantity.get<Number>(1);
      quantity.push_back (amount);
      rate.push_back (price);
    }
  }

  // Error handler.
  if (quantity.empty ()) {
    if (hurry) poloniex_hurried_call_fail++;
    error.append (json);
  } else {
    if (hurry) poloniex_hurried_call_pass++;
  }
}


void poloniex_get_buyers (string market,
                          string coin,
                          vector <float> & quantity,
                          vector <float> & rate,
                          string & error,
                          bool hurry)
{
  poloniex_return_order_book (market, coin, "bids", quantity, rate, error, hurry);
}


void poloniex_get_sellers (string market,
                           string coin,
                           vector <float> & quantity,
                           vector <float> & rate,
                           string & error,
                           bool hurry)
{
  poloniex_return_order_book (market, coin, "asks", quantity, rate, error, hurry);
}


void poloniex_get_wallet_details (string coin, string & json, string & error,
                                  string & address, string & payment_id)
{
  json = get_poloniex_private_api_call_result ("returnDepositAddresses", {}, error);
  if (!error.empty ()) {
    return;
  }
  
  // Sample JSON:
  // {"BTC":"19YqztHmspv2egyD6jQM3yn81x5t5krVdJ","LTC":"LPgf9kjv9H1Vuh4XSaKhzBe8JHdou1WgUB", ... "ITC":"Press Generate.." ... }

  // Parse the JSON.
  string coin_abbrev = poloniex_get_coin_abbrev (coin);
  Object main;
  main.parse (json);
  if (main.has <String>(coin_abbrev)) {
    address = main.get <String>(coin_abbrev);
    return;
  }
  
  // Error handler.
  error.append (json);

  // Unused parameter.
  (void) payment_id;
}


void poloniex_get_balances (vector <string> & coin_abbrev,
                            vector <string> & coin_ids,
                            vector <float> & total,
                            vector <float> & reserved,
                            vector <float> & unconfirmed,
                            string & error) 
{
  string json = get_poloniex_private_api_call_result ("returnCompleteBalances", {}, error);
  if (!error.empty ()) {
    return;
  }

  // Sample JSON:
  // {"LTC":{"available":"0.00000000","onOrders":"0.00000000","btcValue":"0.00000000"}, ... "BTC":{"available":"0.00000000","onOrders":"0.00000000","btcValue":"0.00000000"},"ZEC":{"available":"0.00000000","onOrders":"0.00000000","btcValue":"0.00000000"},"ZRX":{"available":"0.00000000","onOrders":"0.00000000","btcValue":"0.00000000"}}

  Object main;
  main.parse (json);
  // In the event of an error, the response will always be of the following format:
  // {"error":"<error message>"}
  if (main.has<String>("error")) {
    error = json;
    return;
  }
  map <string, Value*> json_map = main.kv_map ();
  for (auto key_value : json_map) {
    string coin_abbreviation = key_value.first;
    Object values = key_value.second->get<Object>();
    if (!values.has <String> ("available")) continue;
    if (!values.has <String> ("onOrders")) continue;
    string available = values.get <String> ("available");
    string onOrders = values.get <String> ("onOrders");
    float Available = str2float (available);
    float OnOrders = str2float (onOrders);
    if ((Available == 0) && (OnOrders == 0)) continue;
    coin_abbrev.push_back (coin_abbreviation);
    string coin_id = poloniex_get_coin_id (coin_abbreviation);
    coin_ids.push_back (coin_id);
    total.push_back (Available + OnOrders);
    reserved.push_back (OnOrders);
    unconfirmed.push_back (0);
  }
  if (coin_abbrev.empty ()) {
    error.append (__FUNCTION__);
    error.append (" ");
    error.append (json);
  }
}


mutex poloniex_balance_mutex;
map <string, float> poloniex_balance_total;
map <string, float> poloniex_balance_available;
map <string, float> poloniex_balance_reserved;
map <string, float> poloniex_balance_unconfirmed;


void poloniex_get_balance (const string & coin, float * total, float * available, float * reserved, float * unconfirmed)
{
  poloniex_balance_mutex.lock ();
  if (total) * total = poloniex_balance_total [coin];
  if (available) * available = poloniex_balance_available [coin];
  if (reserved) * reserved = poloniex_balance_reserved [coin];
  if (unconfirmed) * unconfirmed = poloniex_balance_unconfirmed [coin];
  poloniex_balance_mutex.unlock ();
}


void poloniex_set_balance (const string & coin, float total, float available, float reserved, float unconfirmed)
{
  poloniex_balance_mutex.lock ();
  if (total != 0) poloniex_balance_total [coin] = total;
  if (available != 0) poloniex_balance_available [coin] = available;
  if (reserved != 0) poloniex_balance_reserved [coin] = reserved;
  if (unconfirmed != 0) poloniex_balance_unconfirmed [coin] = unconfirmed;
  poloniex_balance_mutex.unlock ();
}


vector <string> poloniex_get_coins_with_balances ()
{
  vector <string> coins;
  poloniex_balance_mutex.lock ();
  for (auto & element : poloniex_balance_total) {
    coins.push_back (element.first);
  }
  poloniex_balance_mutex.unlock ();
  return coins;
}


string poloniex_withdrawal_order (const string & coin,
                                  float quantity,
                                  const string & address,
                                  const string & paymentid,
                                  string & json,
                                  string & error)
{
  string coin_abbrev = poloniex_get_coin_abbrev (coin);
  vector <pair <string, string> > parameters;
  parameters.push_back (make_pair ("currency", coin_abbrev));
  parameters.push_back (make_pair ("amount", float2string (quantity)));
  parameters.push_back (make_pair ("address", address));
  if (!paymentid.empty ()) {
    parameters.push_back (make_pair ("paymentId", paymentid));
  }
  json = get_poloniex_private_api_call_result ("withdraw", parameters, error);
  if (!error.empty ()) {
    return "";
  }
  
  // Sample JSON:
  // {"response":"Withdrew 2398 NXT."}
  
  Object main;
  main.parse (json);
  if (main.has<String>("response")) {
    string message = main.get<String>("response");
    return poloniex_get_exchange ();
  }
  error.append (json);
  return "";
}


void poloniex_get_open_orders (vector <string> & identifiers,
                               vector <string> & markets,
                               vector <string> & coin_abbreviations,
                               vector <string> & coin_ids,
                               vector <string> & buy_sells,
                               vector <float> & amounts,
                               vector <float> & rates,
                               vector <string> & dates,
                               string & error)
{
  vector <pair <string, string> > parameters;
  parameters.push_back (make_pair ("currencyPair", "all"));
  string json = get_poloniex_private_api_call_result ("returnOpenOrders", parameters, error);
  if (!error.empty ()) {
    return;
  }

  // {"BTC_1CR":[],"BTC_XRP":[{"orderNumber":"110361867777","type":"sell","rate":"0.025","amount":"100","total":"2.5","date":"2018-01-09 19:05:43","margin":0},{"orderNumber":"110361867778","type":"sell","rate":"0.04","amount":"100","total":"4""date":"2018-01-09 19:05:43","margin":0}], ... }

  Object main;
  main.parse (json);
  map <string, Value*> json_map = main.kv_map ();
  for (auto & key_value : json_map) {
    string market_coin_pair = key_value.first;
    Array values = key_value.second->get<Array>();
    for (size_t i = 0; i < values.size(); i++) {
      Object orders = values.get <Object> (i);
      if (!orders.has<String>("orderNumber")) continue;
      string orderNumber = orders.get<String> ("orderNumber");
      if (!orders.has<String>("type")) continue;
      string type = orders.get<String> ("type");
      if (!orders.has<String>("amount")) continue;
      string amount = orders.get<String> ("amount");
      if (!orders.has<String>("rate")) continue;
      string rate = orders.get<String> ("rate");
      if (!orders.has<String>("date")) continue;
      string date = orders.get<String> ("date");
      vector <string> market_coin = explode (market_coin_pair, '_');
      if (market_coin.size () != 2) continue;
      identifiers.push_back (orderNumber);
      markets.push_back (poloniex_get_coin_id (market_coin[0]));
      coin_abbreviations.push_back (market_coin[1]);
      coin_ids.push_back (poloniex_get_coin_id (market_coin[1]));
      buy_sells.push_back (type);
      amounts.push_back (str2float (amount));
      rates.push_back (str2float (rate));
      dates.push_back (date);
    }
  }
  // Error handler.
  // If there's only a few market/coin pairs, there's a problem, obviously.
  // Because if there's no open orders, Poloniex still returns all its known market/coin pairs.
  if (main.size () < 10) {
    error.append (json);
  }
}


bool poloniex_cancel_order (const string & order_id,
                            string & error)
{
  vector <pair <string, string> > parameters;
  parameters.push_back (make_pair ("orderNumber", order_id));
  string json = get_poloniex_private_api_call_result ("cancelOrder", parameters, error);
  if (!error.empty ()) {
    return false;
  }
  // Sample JSON:
  // {"success":1}
  Object main;
  main.parse (json);
  if (main.has<Number>("success")) {
    int success = main.get<Number>("success");
    if (success > 0) {
      return true;
    }
  }
  error.append (json);
  return false;
}


string poloniex_limit_trade (const string & type,
                             const string & market,
                             const string & coin,
                             float quantity,
                             float rate,
                             string & json,
                             string & error)
{
  string market_abbrev = poloniex_get_coin_abbrev (market);
  string coin_abbrev = poloniex_get_coin_abbrev (coin);
  vector <pair <string, string> > parameters;
  parameters.push_back (make_pair ("currencyPair", market_abbrev + "_" + coin_abbrev));
  parameters.push_back (make_pair ("rate", float2string (rate)));
  parameters.push_back (make_pair ("amount", float2string (quantity)));
  json = get_poloniex_private_api_call_result (type, parameters, error);
  if (!error.empty ()) {
    return "";
  }
  
  // {"orderNumber":31226040,"resultingTrades":[{"amount":"338.8732","date":"2017-10-18 23:03:21","rate":"0.00000173","total":"0.00058625","tradeID":"16164","type":"buy"}]}
  
  Object main;
  main.parse (json);
  if (main.has<String>("orderNumber")) {
    string orderNumber = main.get<String>("orderNumber");
    return orderNumber;
  }
  error.append (json);
  return "";
}


string poloniex_limit_buy (const string & market,
                           const string & coin,
                           float quantity,
                           float rate,
                           string & json,
                           string & error)
{
  return poloniex_limit_trade ("buy", market, coin, quantity, rate, json, error);
}


string poloniex_limit_sell (const string & market,
                            const string & coin,
                            float quantity,
                            float rate,
                            string & json,
                            string & error)
{
  return poloniex_limit_trade ("sell", market, coin, quantity, rate, json, error);
}


void poloniex_deposits_withdrawals (const string & type,
                                    const string & coin,
                                    vector <string> & order_ids,
                                    vector <string> & coin_abbreviations,
                                    vector <string> & coin_ids,
                                    vector <float> & quantities,
                                    vector <string> & addresses,
                                    vector <string> & transaction_ids,
                                    string & error)
{
  int seconds = seconds_since_epoch ();
  vector <pair <string, string> > parameters;
  
  parameters.push_back (make_pair ("start", to_string (seconds - 30 * 24 * 3600)));
  parameters.push_back (make_pair ("end", to_string (seconds)));
  string json = get_poloniex_private_api_call_result ("returnDepositsWithdrawals", parameters, error);
  if (!error.empty ()) {
    return;
  }

  // {"deposits":[{"currency":"BTC","address":"1F5VSpUegTs9QTeMNHTkaNQhgFjjoZ2ng8","amount":"0.34470125","confirmations":3,"txid":"7d8ae6480a2109ffb9b45009caf88a1f9655c09f52a0b31e30ecca34aca4f77e","timestamp":1534517023,"status":"COMPLETE"},{"currency":"ETH","address":"0xcdec82bd7886112adbb62b6193f15ab1996bece1","amount":"0.66604740","confirmations":135,"txid":"0xefb23531bf0e13b24f355411bde08c1abced0e5ee7922d30ba2e91093a9acbc6","timestamp":1534554323,"status":"COMPLETE"}],"withdrawals":[{"withdrawalNumber":11822507,"currency":"ETH","address":"0x243b541a83935dced89d9e44c4dd0072ef14b0fd","amount":"0.77423077","fee":"0.01000000","timestamp":1534771806,"status":"COMPLETE: 0x594104c6134c7c409abb9d6e2b4c291e83cd9abd28206a90a2fcde7796ecb4c1","ipAddress":"185.87.186.173"},{"withdrawalNumber":11824509,"currency":"BTC","address":"15c6WiDEMUWPitpCpv9T5JrRG9Z1e2PwDM","amount":"0.29510371","fee":"0.00050000","timestamp":1534829221,"status":"COMPLETE: ffb17523875df0e81e2ba6565ba5649316755e925ca40bf728bfeded50cab905","ipAddress":"81.207.142.28"}]}
  
  string coin_abbrev = poloniex_get_coin_abbrev (coin);

  Object main;
  main.parse (json);
  if (main.has<Array>(type)) {
    Array values = main.get<Array>(type);
    for (size_t i = 0; i < values.size(); i++) {
      Object item = values.get <Object> (i);
      if (!item.has<String>("currency")) continue;
      string currency = item.get<String>("currency");
      if (coin_abbrev != currency) continue;
      string order_id = "Unknown";
      if (item.has<Number>("withdrawalNumber")) {
        int withdrawalNumber = item.get<Number> ("withdrawalNumber");
        order_id = to_string (withdrawalNumber);
      }
      if (!item.has<String>("amount")) continue;
      string amount = item.get<String>("amount");
      if (!item.has<String>("address")) continue;
      string address = item.get<String>("address");
      // The txid is given with deposits only.
      // With withdrawals it's given as part of the status field.
      string txid;
      if (item.has<String>("txid")) {
        txid = item.get<String>("txid");
      } else {
        if (item.has<String>("status")) {
          string status = item.get<String>("status");
          if (status.find ("COMPLETE") == 0) {
            txid = status.substr (10);
          }
        }
      }
      order_ids.push_back (order_id);
      coin_abbreviations.push_back (currency);
      coin_ids.push_back (coin);
      quantities.push_back (str2float (amount));
      addresses.push_back (address);
      transaction_ids.push_back (txid);
    }
    return;
  }
  
  // Error handler.
  error.append (json);
}


void poloniex_withdrawalhistory (const string & coin,
                                 vector <string> & order_ids,
                                 vector <string> & coin_abbreviations,
                                 vector <string> & coin_ids,
                                 vector <float> & quantities,
                                 vector <string> & addresses,
                                 vector <string> & transaction_ids,
                                 string & error)
{
  poloniex_deposits_withdrawals ("withdrawals",
                                 coin,
                                 order_ids,
                                 coin_abbreviations,
                                 coin_ids,
                                 quantities,
                                 addresses,
                                 transaction_ids,
                                 error);
}


void poloniex_deposithistory (const string & coin,
                              vector <string> & order_ids,
                              vector <string> & coin_abbreviations,
                              vector <string> & coin_ids,
                              vector <float> & quantities,
                              vector <string> & addresses,
                              vector <string> & transaction_ids,
                              string & error)
{
  poloniex_deposits_withdrawals ("deposits",
                                 coin,
                                 order_ids,
                                 coin_abbreviations,
                                 coin_ids,
                                 quantities,
                                 addresses,
                                 transaction_ids,
                                 error);
}


int poloniex_get_hurried_call_passes ()
{
  return poloniex_hurried_call_pass;
}


int poloniex_get_hurried_call_fails ()
{
  return poloniex_hurried_call_fail;
}
