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


#include "bitfinex.h"
#include "proxy.h"


#define two_factor_authentication_account_token "--enter--your--value--here--"
#define api_key "--enter--your--value--here--"
#define api_secret "--enter--your--value--here--"


// Hurried call counters.
atomic <int> bitfinex_hurried_call_pass (0);
atomic <int> bitfinex_hurried_call_fail (0);


typedef struct
{
  const char * bot_identifier;
  const char * bot_abbreviation;
  const char * name;
  const char * bitfinex_method;
  const char * bitfinex_abbreviation;
}
bitfinex_coin_record;


bitfinex_coin_record bitfinex_coins_table [] =
{
  { "bitcoin", "BTC", "Bitcoin", "bitcoin", "btc" },
  { "bitcoingold", "BTG", "Bitcoin Gold", "bitcoingold", "btg" },
  { "litecoin", "LTC", "Litecoin", "litecoin", "ltc" },
  { "ethereum", "ETH", "Ethereum", "ethereum", "eth" },
  { "ethereumclassic", "ETC", "Ethereum Classic", "ethereumc", "etc" },
  { "recoveryrighttokens", "RRT", "Recovery Right Tokens", "recoveryrighttokens", "rrt" },
  { "zcash", "ZEC", "Zcash", "zcash", "zec" },
  { "monero", "XMR", "Monero", "monero", "xmr" },
  { "dash", "DSH", "Dash", "dash", "dsh" },
  { "ripple", "XRP", "Ripple", "ripple", "xrp" },
  { "iota", "IOT", "Iota", "iota", "iot" },
  { "eos", "EOS", "EOS", "eos", "eos" },
  { "santiment", "SAN", "Santiment", "santiment", "san" },
  { "omisego", "OMG", "OmiseGO", "omisego", "omg" },
  { "bcash", "BCH", "Bcash", "bcash", "bch" },
  { "neo", "NEO", "NEO", "neo", "neo" },
  { "etp", "ETP", "ETP", "etp", "etp" },
  { "qtum", "QTUM","Qtum", "qtm", "qtm" },
  { "aventus", "AVT", "Aventus", "avt", "avt" },
  { "eidoo", "EDO", "Eidoo", "edo", "edo" },
  { "streamr", "DAT", "Streamr", "dat", "dat" },
  { "qash", "QSH", "QASH", "qsh", "qsh" },
  { "yoyow", "YYW", "YOYOW", "yyw", "yyw" },
  { "usdollar", "USD", "US Dollar", "usd", "usd" },
  { "euro", "EUR", "Euro", "eur", "eur" },
  { "golem", "GNT", "Golem", "gnt", "gnt" },
  { "status", "SNT", "Status", "snt", "snt" },
  { "basicattentiontoken", "BAT", "Basic Attention Token", "bat", "bat" },
  { "decentraland", "MNA", "Decentraland", "mna", "mna" },
  { "funfair", "FUN", "FunFair", "fun", "fun" },
  { "0x", "ZRX", "0x", "zrx", "zrx" },
  { "timenewbank", "TNB", "Time New Bank", "tnb", "tnb" },
  { "spankchain", "SPK", "SpankChain", "spk", "spk" },
  { "tron", "TRX", "TRON", "trx", "trx" },
  { "iexec", "RLC", "iExec", "rlc", "rlc" },
  { "aidcoin", "AID", "AidCoin", "aid", "aid" },
  { "singulardtv", "SNG", "SingularDTV", "sng", "sng" },
  { "augur", "REP", "Augur", "rep", "rep" },
  { "aelf", "ELF", "aelf", "elf", "elf" },
  { "iostoken", "IOS", "IOSToken", "ios", "ios" },
  { "aion", "AIO", "Aion", "aio", "aio" },
  { "requestnetwork", "REQ", "Request Network", "req", "req" },
  { "raiden", "RDN", "Raiden", "rdn", "rdn" },
  { "loopring", "LRC", "Loopring", "lrc", "lrc" },
  { "daistablecoin", "DAI", "Dai Stablecoin", "dai", "dai" },
  { "cofoundit", "CFI", "Cofound.it", "cfi", "cfi" },
  { "singularitynet", "AGI", "SingularityNET", "agi", "agi" },
  { "bnktothefuture", "BFT", "BnkToTheFuture", "bft", "bft" },
  { "medicalchain", "MTN", "Medicalchain", "mtn", "mtn" },
  { "odem", "ODE", "Odem", "ode", "ode" },
  { "aragon", "ANT", "Aragon", "ant", "ant" },
  { "dether", "DTH", "Dether", "dth", "dth" },
  { "mithril", "MIT", "Mithril", "mit", "mit" },
  { "storj", "STJ", "Storj", "stj", "stj" },
  { "rcn", "RCN", "RCN", "rcn", "rcn" },
  { "wax", "WAX", "WAX", "wax", "wax" },
  { "stellerlumen", "XLM", "Stellar Lumen", "xlm", "xlm" },
  { "verge", "XVG", "Verge", "xvg", "xvg" },
  { "bitcoininterest", "BCI", "BitcoinInterest", "bci", "bci" },
  { "maker", "MKR", "Maker", "mkr", "mkr" },
  { "vechain", "VEN", "VeChain", "ven", "ven" },
  { "kyber", "KNC", "Kyber", "knc", "knc" },
  { "poanetwork", "POA", "POA Network", "poa", "poa" },
  { "auctus", "AUC", "Auctus", "auc", "auc" },
  { "commerceblock", "CBT", "CommerceBlock", "cbt", "cbt" },
  { "cindicator", "CND", "Cindicator", "cnd", "cnd" },
  { "dadi", "DADI", "DADI", "dad", "dad" },
  { "fusion", "FSN", "Fusion", "fsn", "fsn" },
  { "lympo", "LYM", "Lympo", "lym", "lym" },
  { "orsgroup", "ORS", "ORS Group", "ors", "ors" },
  { "polymath", "POY", "Polymath", "poy", "poy" },
  { "consensusai", "SEN", "Consensus AI", "sen", "sen" },
  { "utrust", "UTK", "UTRUST", "utk", "utk" },
  { "blockv", "VEE", "BLOCKv", "vee", "vee" },
  { "0chain", "ZCN", "0chain", "zcn", "zcn" },
  { "theabyss", "ABS", "The Abyss", "abs", "abs" },
  { "atonomi", "ATM", "Atonomi", "atm", "atm" },
  { "bancor", "BNT", "Bancor", "bnt", "bnt" },
  { "cortex", "CTX", "Cortex", "ctx", "ctx" },
  { "data", "DTA", "Data", "dta", "dta" },
  { "essentia", "ESS", "Essentia", "ess", "ess" },
  { "hydroprotocol", "HOT", "Hydro Protocol", "hot", "hot" },
  { "everipedia", "IQX", "Everipedia", "iqx", "iqx" },
  { "matrix", "MAN", "Matrix", "man", "man" },
  { "nucleusvision", "NCA", "Nucleus Vision", "nca", "nca" },
  { "paiproject", "PAI", "PAI Project", "pai", "pai" },
  { "seer", "SEE", "Seer", "see", "see" },
  { "wepower", "WPR", "WePower", "wpr", "wpr" },
  { "xriba", "XRA", "Xriba", "xra", "xra" },
  { "zilliqa", "ZIL", "Zilliqa", "zil", "zil" },
  
};


vector <string> bitfinex_get_coin_ids ()
{
  vector <string> ids;
  unsigned int data_count = sizeof (bitfinex_coins_table) / sizeof (*bitfinex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    ids.push_back (bitfinex_coins_table[i].bot_identifier);
  }
  return ids;
}


string bitfinex_get_coin_id (string coin)
{
  unsigned int data_count = sizeof (bitfinex_coins_table) / sizeof (*bitfinex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == bitfinex_coins_table[i].bot_identifier) return bitfinex_coins_table[i].bot_identifier;
    if (coin == bitfinex_coins_table[i].bot_abbreviation) return bitfinex_coins_table[i].bot_identifier;
    if (coin == bitfinex_coins_table[i].name) return bitfinex_coins_table[i].bot_identifier;
    if (coin == bitfinex_coins_table[i].bitfinex_method) return bitfinex_coins_table[i].bot_identifier;
    if (coin == bitfinex_coins_table[i].bitfinex_abbreviation) return bitfinex_coins_table[i].bot_identifier;
  }
  return "";
}


string bitfinex_get_coin_abbrev (string coin)
{
  unsigned int data_count = sizeof (bitfinex_coins_table) / sizeof (*bitfinex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == bitfinex_coins_table[i].bot_identifier) return bitfinex_coins_table[i].bot_abbreviation;
    if (coin == bitfinex_coins_table[i].bot_abbreviation) return bitfinex_coins_table[i].bot_abbreviation;
    if (coin == bitfinex_coins_table[i].name) return bitfinex_coins_table[i].bot_abbreviation;
    if (coin == bitfinex_coins_table[i].bitfinex_method) return bitfinex_coins_table[i].bot_abbreviation;
    if (coin == bitfinex_coins_table[i].bitfinex_abbreviation) return bitfinex_coins_table[i].bot_abbreviation;
  }
  return "";
}


string bitfinex_get_coin_name (string coin)
{
  unsigned int data_count = sizeof (bitfinex_coins_table) / sizeof (*bitfinex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == bitfinex_coins_table[i].bot_identifier) return bitfinex_coins_table[i].name;
    if (coin == bitfinex_coins_table[i].bot_abbreviation) return bitfinex_coins_table[i].name;
    if (coin == bitfinex_coins_table[i].name) return bitfinex_coins_table[i].name;
    if (coin == bitfinex_coins_table[i].bitfinex_method) return bitfinex_coins_table[i].name;
    if (coin == bitfinex_coins_table[i].bitfinex_abbreviation) return bitfinex_coins_table[i].name;
  }
  return "";
}


// Get the "method" used by Bitfinex.
// It was tried whether replacing the "method" with the bot abbreviation works,
// when requesting wallet details.
// But Bitfinex said: {"message":"Unknown method"}
string bitfinex_get_coin_bitfinex_method (string coin)
{
  unsigned int data_count = sizeof (bitfinex_coins_table) / sizeof (*bitfinex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == bitfinex_coins_table[i].bot_identifier) return bitfinex_coins_table[i].bitfinex_method;
    if (coin == bitfinex_coins_table[i].bot_abbreviation) return bitfinex_coins_table[i].bitfinex_method;
    if (coin == bitfinex_coins_table[i].name) return bitfinex_coins_table[i].bitfinex_method;
    if (coin == bitfinex_coins_table[i].bitfinex_method) return bitfinex_coins_table[i].bitfinex_method;
    if (coin == bitfinex_coins_table[i].bitfinex_abbreviation) return bitfinex_coins_table[i].bitfinex_method;
  }
  return "";
}


string bitfinex_get_coin_bitfinex_abbrev (string coin)
{
  unsigned int data_count = sizeof (bitfinex_coins_table) / sizeof (*bitfinex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == bitfinex_coins_table[i].bot_identifier) return bitfinex_coins_table[i].bitfinex_abbreviation;
    if (coin == bitfinex_coins_table[i].bot_abbreviation) return bitfinex_coins_table[i].bitfinex_abbreviation;
    if (coin == bitfinex_coins_table[i].name) return bitfinex_coins_table[i].bitfinex_abbreviation;
    if (coin == bitfinex_coins_table[i].bitfinex_method) return bitfinex_coins_table[i].bitfinex_abbreviation;
    if (coin == bitfinex_coins_table[i].bitfinex_abbreviation) return bitfinex_coins_table[i].bitfinex_abbreviation;
  }
  return "";
}


/*
 Bitfinex says:
 If an IP address exceeds 90 requests per minute to the REST APIs,
 the requesting IP address will be blocked for 10-60 seconds
 and the JSON response {"error": "ERR_RATE_LIMIT"} will be returned.
 Please note the exact logic and handling for such DDoS defenses may change over time
 to further improve reliability.
*/


string get_bitfinex_public_api_call (const string & version,
                                     vector <string> endpoint,
                                     vector <pair <string, string> > parameters,
                                     string * request = nullptr)
{
  // The url of the API website.
  string url ("https://api.bitfinex.com");
  // The GET or POST request.
  string get ("/v" + version);
  for (auto part : endpoint) {
    get.append ("/");
    get.append (part);
  }
  url.append (get);
  // Optionally give the caller the POST request.
  if (request) request->assign (get);
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


// If API calls are sent to the network with correct increasing nonces,
// the calls may arrive out of order at the exchange, due to network latency.
// This would lead to nonces no longer to be increasing, but out of order.
// This mutex should prevent that more or less.
// mutex bitfinex_private_api_call_mutex;
// When the mutex appears as too rigid as related to timing,
// another way of ensuring increasing nonces is to use no longer mutexes but to use timers.
// There's a timer delay of about a second,
// that ensures pending requests are sent off in the correct sequence.
// This system no longer waits for the response to come in.
// It just ensures the requests go out in a timely fashion and in an orderly fashion.
// This is implemented below.
atomic <long> bitfinex_private_api_call_sequencer (0);


string get_bitfinex_private_api_call_result (vector <string> endpoint, Object & post_payload, string & error)
{
  // The request, and the url to POST.
  string request;
  string url = get_bitfinex_public_api_call ("1", endpoint, {}, &request);

  // Ensure strictly increasing nonces.
  // bitfinex_private_api_call_mutex.lock (); No longer needed with the timed sequencer.
  api_call_sequencer (bitfinex_private_api_call_sequencer);
  
  // Nonce: Strictly increasing.
  string nonce = increasing_nonce ();
  
  // The payload to POST.
  post_payload << "request" << request;
  post_payload << "nonce" << nonce;
  string json_post_payload = post_payload.json ();

  // The security headers.
  string payload = base64_encode (json_post_payload);
  string signature = hmac_sha384_hexits (api_secret, payload);
  vector <pair <string, string> > headers = {
    make_pair ("X-BFX-APIKEY", api_key),
    make_pair ("X-BFX-PAYLOAD", payload),
    make_pair ("X-BFX-SIGNATURE", signature)
  };
  
  // Call API.
  string json = http_call (url, error, "", false, true, json_post_payload, headers, false, true, "");

  // Unlock so further calls can be made.
  // No longer needed with the timed sequencer.
  // bitfinex_private_api_call_mutex.unlock ();

  // Done.
  return json;
}


string bitfinex_get_exchange ()
{
  return "bitfinex";
}


// Caches to reduce the Bitfinex API call rate.
mutex bitfinex_markets_mutex;
map <string, vector <string> > bitfinex_markets_ids;
map <string, vector <string> > bitfinex_markets_abbrevs;
map <string, vector <string> > bitfinex_markets_names;


void bitfinex_get_coins (string market,
                         vector <string> & coin_abbrevs,
                         vector <string> & coin_ids,
                         vector <string> & names,
                         string & error)
{
  // If the markets have been fetched before, take those.
  if (!bitfinex_markets_ids[market].empty ()) {
    bitfinex_markets_mutex.lock ();
    coin_abbrevs = bitfinex_markets_abbrevs[market];
    coin_ids = bitfinex_markets_ids[market];
    names = bitfinex_markets_names[market];
    bitfinex_markets_mutex.unlock ();
    return;
  }
  // Call the API to get the exchange's symbols.
  // It provides this:
  // ["btcusd","ltcusd","ltcbtc",...]
  string url = get_bitfinex_public_api_call ("1", {"symbols"}, { });
  string json = http_call (url, error, "", false, false, "", {}, false, false, "");
  if (!error.empty ()) {
    return;
  }
  // Get the abbreviation to filter the desired market out from the above JSON.
  string bitfinex_abbreviation = bitfinex_get_coin_bitfinex_abbrev (market);
  // Parse the JSON.
  Array main;
  main.parse (json);
  if (main.size ()) {
    for (size_t i = 0; i < main.size(); i++) {
      if (!main.has <String> (i)) continue;
      string pair = main.get <String> (i);
      // Should be six bytes long.
      if (pair.size () < 6) continue;
      // Take the Bitcoin markets only, for just now.
      string second = pair.substr (3);
      if (second != bitfinex_abbreviation) continue;
      // Get the Bitfinex currency abbreviation.
      string bitfinex_abbreviation = pair.substr (0, 3);
      // Find the appropriate bot identifier, abbreviation and name for it.
      string coin_id = bitfinex_get_coin_id (bitfinex_abbreviation);
      string bot_abbreviation = bitfinex_get_coin_abbrev (bitfinex_abbreviation);
      string name = bitfinex_get_coin_name (bitfinex_abbreviation);
      if (bot_abbreviation.empty()) bot_abbreviation = bitfinex_abbreviation;
      // If no names are given, just pass the abbreviation found.
      // It is essential to recording the coin names that this be done, rather than to give an empty string.
      if (name.empty ()) name = bitfinex_abbreviation;
      if (!name.empty () && !bot_abbreviation.empty()) {
        // Store them locally.
        coin_ids.push_back (coin_id);
        coin_abbrevs.push_back (bot_abbreviation);
        names.push_back (name);
        // Store them globally.
        bitfinex_markets_mutex.lock ();
        bitfinex_markets_ids[market].push_back (coin_id);
        bitfinex_markets_abbrevs[market].push_back (bot_abbreviation);
        bitfinex_markets_names[market].push_back (name);
        bitfinex_markets_mutex.unlock ();
      }
    }
  } else {
    error.append (json);
  }
}


// This gets the data through the REST API.
// Bitfinex limits the number of calls per time unit.
// This leads to { "error": "ERR_RATE_LIMIT"}.
void bitfinex_get_market_summaries (string market,
                                    vector <string> & coins,
                                    vector <float> & bids,
                                    vector <float> & asks,
                                    string & error)
{
  // Bitfinex needs all trade pairs passed to this call.
  // Sample: https://api.bitfinex.com/v2/tickers?symbols=tBTCUSD,tLTCUSD
  // So fetch those first.
  vector <string> coin_abbrevs;
  vector <string> coin_ids;
  vector <string> coin_names;
  bitfinex_get_coins (market, coin_abbrevs, coin_ids, coin_names, error);
  if (!error.empty ()) return;

  // Assemble the query to pass to the API.
  string market_abbrev = bitfinex_get_coin_abbrev (market);
  vector <string> pairs;
  for (auto coin_abbrev : coin_abbrevs) {
    pairs.push_back ("t" + coin_abbrev + market_abbrev);
  }
  string query = "?symbols=" + implode (pairs, ",");
  
  // The URL to call.
  string url = get_bitfinex_public_api_call ("2", {"tickers", query}, {});
  
  // Call the API.
  string json = http_call (url, error, "", false, false, "", {}, false, false, satellite_get_interface (url));
  if (!error.empty ()) {
    return;
  }
  
  // Sample JSON:
  // [
  //   ["tLTCBTC",0.017938,2834.80377611,0.017939,825.93738622,0.000005,0.0003,0.017939,99551.72517577,0.018443,0.017251],
  //   ...
  //   ["tSNTBTC",0.00002433,319049.37126025,0.00002497,183450.43949121,-0.00000102,-0.0102,0.00002465,1580836.00427209,0.00002792,0.00002332]
  // ]
  
  // Explanation:
  // [
  // 0  SYMBOL,
  // 1  BID,
  // 2  BID_SIZE,
  // 3  ASK,
  // 4  ASK_SIZE,
  // 5  DAILY_CHANGE,
  // 6  DAILY_CHANGE_PERC,
  // 7  LAST_PRICE,
  // 8  VOLUME,
  // 9  HIGH,
  // 10 LOW
  // ]

  Array main;
  main.parse (json);
  for (size_t i = 0; i < main.size(); i++) {
    if (main.has<Array>(i)) {
      Array values = main.get<Array>(i);
      if (values.size() == 11) {
        string tpair = values.get<String>(0);
        float bid = values.get<Number>(1);
        float ask = values.get<Number>(3);
        if (!tpair.empty ()) {
          tpair.erase (0, 1);
          size_t pos = tpair.find (market_abbrev);
          if (pos != string::npos) {
            tpair.erase (pos);
            string coin_id = bitfinex_get_coin_id (tpair);
            if (!coin_id.empty ()) {
              coins.push_back (coin_id);
              bids.push_back (bid);
              asks.push_back (ask);
            }
          }
        }
      }
    }
  }

  // Error handler.
  if (coins.empty ()) error.append (json);
}


void bitfinex_get_open_orders (vector <string> & identifier,
                               vector <string> & market,
                               vector <string> & coin_abbreviation,
                               vector <string> & coin_ids,
                               vector <string> & buy_sell,
                               vector <float> & quantity,
                               vector <float> & rate,
                               vector <string> & date,
                               string & error)
{
  // The payload to POST.
  Object post_payload;
  // Call authenticated API.
  string json = get_bitfinex_private_api_call_result ({"orders"}, post_payload, error);
  // Error handling.
  if (!error.empty ()) {
    error.append (" ");
    error.append (__FUNCTION__);
    return;
  }
  /*
  [{
   "id":448411365,
   "symbol":"omgbtc",
   "price":"0.02",
   "avg_execution_price":"0.0",
   "side":"buy",
   "type":"exchange limit",
   "timestamp":"1444276597.0",
   "is_live":true,
   "is_cancelled":false,
   "is_hidden":false,
   "was_forced":false,
   "original_amount":"0.02",
   "remaining_amount":"0.02",
   "executed_amount":"0.0"
   }]
  */
  Array main;
  main.parse (json);
  for (size_t i = 0; i < main.size(); i++) {
    Object values = main.get <Object> (i);
    if (!values.has<Number>("id")) continue;
    if (!values.has<String>("symbol")) continue;
    if (!values.has<String>("side")) continue;
    if (!values.has<String>("remaining_amount")) continue;
    if (!values.has<String>("price")) continue;
    if (!values.has<String>("timestamp")) continue;
    long int id = values.get<Number>("id");
    string symbol = values.get<String>("symbol");
    string market_id = bitfinex_get_coin_id (symbol.substr (3, 3));
    string bitfinex_abbreviation = symbol.erase (3);
    string coin_abbrev = bitfinex_get_coin_abbrev (bitfinex_abbreviation);
    string side = values.get<String>("side");
    float remaining_amount = str2float (values.get<String>("remaining_amount"));
    float price = str2float (values.get<String>("price"));
    string timestamp = values.get<String>("timestamp");
    identifier.push_back (to_string(id));
    market.push_back (market_id);
    coin_abbreviation.push_back (coin_abbrev);
    coin_ids.push_back (bitfinex_get_coin_id (coin_abbrev));
    buy_sell.push_back (side);
    quantity.push_back (remaining_amount);
    rate.push_back (price);
    date.push_back (timestamp);
  }
}


// Get the $total balance that includes also the following two amounts:
// 1. The balances $reserved for trade orders.
// 2. The $unconfirmed balance: What is being deposited right now.
// It also gives all matching coin abbreviations.
void bitfinex_get_balances (vector <string> & coin_abbrev,
                            vector <string> & coin_ids,
                            vector <float> & total,
                            vector <float> & reserved,
                            vector <float> & unconfirmed,
                            string & error)
{
  // The payload to POST.
  Object post_payload;
  // Call authenticated API.
  string json = get_bitfinex_private_api_call_result ({"balances"}, post_payload, error);
  // Error handling.
  if (!error.empty ()) {
    error.append (" ");
    error.append (__FUNCTION__);
    return;
  }
  /*
  [{
   "type":"deposit",
   "currency":"btc",
   "amount":"0.0",
   "available":"0.0"
   },{
   "type":"deposit",
   "currency":"usd",
   "amount":"1.0",
   "available":"1.0"
   },{
   "type":"exchange",
   "currency":"btc",
   "amount":"1",
   "available":"1"
   },{
   "type":"exchange",
   "currency":"usd",
   "amount":"1",
   "available":"1"
   },{
   "type":"trading",
   "currency":"btc",
   "amount":"1",
   "available":"1"
   },{
   "type":"trading",
   "currency":"usd",
   "amount":"1",
   "available":"1"
   }]
  */
  Array main;
  main.parse (json);
  for (size_t i = 0; i < main.size(); i++) {
    Object values = main.get <Object> (i);
    // Either “trading”, or “deposit” or “exchange”.
    if (!values.has<String>("type")) continue;
    string type = values.get<String>("type");
    if (type != "exchange") continue;
    // The currency.
    if (!values.has<String>("currency")) continue;
    string currency = values.get<String>("currency");
    // The total balance of this currency in this wallet.
    if (!values.has<String>("amount")) continue;
    string amount = values.get<String>("amount");
    // The balance in this wallet that is available to trade with.
    if (!values.has<String>("available")) continue;
    string available = values.get<String>("available");
    // Skip zero balances.
    float Total = str2float (amount);
    if (Total == 0) continue;
    string bot_coin_abbreviation = bitfinex_get_coin_abbrev (currency);
    float Available = str2float (available);
    float Reserved = Total - Available;
    coin_abbrev.push_back (bot_coin_abbreviation);
    // Get the coin ID.
    // If there's a balance and the coin ID is unknown, give the coin abbreviation instead.
    // The result will be that any unknown balance will be reported at least.
    string coin_id = bitfinex_get_coin_id (currency);
    if (coin_id.empty ()) coin_id = currency;
    coin_ids.push_back (coin_id);
    // Total balance includes also the reserved and unconfirmed balance.
    total.push_back (Total);
    reserved.push_back (Reserved);
    unconfirmed.push_back (0);
  }
  // Handle cases of an error message, e.g.:
  // {"message":"Nonce is too small."}
  // Since this is not an array, the array size above will be 0.
  if (main.size() == 0) {
    error.append (__FUNCTION__);
    error.append (" ");
    error.append (json);
  }
}


mutex bitfinex_balance_mutex;
map <string, float> bitfinex_balance_total;
map <string, float> bitfinex_balance_available;
map <string, float> bitfinex_balance_reserved;
map <string, float> bitfinex_balance_unconfirmed;


void bitfinex_get_balance (const string & coin, float * total, float * available, float * reserved, float * unconfirmed)
{
  bitfinex_balance_mutex.lock ();
  if (total) * total = bitfinex_balance_total [coin];
  if (available) * available = bitfinex_balance_available [coin];
  if (reserved) * reserved = bitfinex_balance_reserved [coin];
  if (unconfirmed) * unconfirmed = bitfinex_balance_unconfirmed [coin];
  bitfinex_balance_mutex.unlock ();
}


void bitfinex_set_balance (const string & coin, float total, float available, float reserved, float unconfirmed)
{
  bitfinex_balance_mutex.lock ();
  if (total != 0) bitfinex_balance_total [coin] = total;
  if (available != 0) bitfinex_balance_available [coin] = available;
  if (reserved != 0) bitfinex_balance_reserved [coin] = reserved;
  if (unconfirmed != 0) bitfinex_balance_unconfirmed [coin] = unconfirmed;
  bitfinex_balance_mutex.unlock ();
}


vector <string> bitfinex_get_coins_with_balances ()
{
  vector <string> coins;
  bitfinex_balance_mutex.lock ();
  for (auto & element : bitfinex_balance_total) {
    coins.push_back (element.first);
  }
  bitfinex_balance_mutex.unlock ();
  return coins;
}


void bitfinex_get_wallet_details (string coin, string & json, string & error,
                                  string & address, string & payment_id)
{
  // The payload to POST.
  Object post_payload;
  string bitfinex_coin_method = bitfinex_get_coin_bitfinex_method (coin);
  post_payload << "method" << bitfinex_coin_method;
  post_payload << "wallet_name" << "exchange";
  post_payload << "renew" << 0;
  // Call authenticated API.
  json = get_bitfinex_private_api_call_result ({"deposit", "new"}, post_payload, error);
  // Error handling.
  if (!error.empty ()) {
    error.append (" ");
    error.append (__FUNCTION__);
    return;
  }
  // Parse the resulting JSON.
  Object main;
  main.parse (json);
  if (main.has<String>("result")) {
    if (main.has<String>("address")) {
      address = main.get<String>("address");
      if (bitfinex_coin_method == "ripple") {
        // The API returns the payment ID for the Ripple coin.
        // Move it to the correct place.
        payment_id = address;
        // Bitfinex's Ripple pool deposit address:
        if (main.has<String>("address_pool")) {
          string address_pool = main.get<String>("address_pool");
          address = address_pool;
        }
      }
      return;
    }
  }
  // Error handling.
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
}


// This gets the bids through the REST API.
// Bitfinex limits the number of calls per time unit.
// This leads to { "error": "ERR_RATE_LIMIT"}.
void bitfinex_get_buyers_rest (string market,
                               string coin,
                               vector <float> & quantity,
                               vector <float> & rate,
                               string & error,
                               bool hurry)
{
  quantity.clear ();
  rate.clear ();
  string market_abbrev = bitfinex_get_coin_abbrev (market);
  string coin_abbrev = bitfinex_get_coin_abbrev (coin);
  string url = get_bitfinex_public_api_call ("1", {"book", coin_abbrev + market_abbrev}, { make_pair ("limit_bids", "15"), make_pair ("limit_asks", "0") });
  string json = http_call (url, error, "", false, false, "", {}, hurry, false, "");
  if (!error.empty ()) {
    if (hurry) bitfinex_hurried_call_fail++;
    return;
  }
  // {
  // "bids":[
  //  {"price":"0.0022284","amount":"0.68","timestamp":"1506018540.0"},
  //  {"price":"0.0022336","amount":"91.522","timestamp":"1506018540.0"},
  //  {"price":"0.002234","amount":"634.067","timestamp":"1506018540.0"},
  //  ...
  // ],
  // "asks":[]
  // }

  Object main;
  main.parse (json);
  if (main.has<Array>("bids")) {
    Array asks = main.get<Array>("bids");
    for (size_t i = 0; i < asks.size(); i++) {
      Object values = asks.get <Object> (i);
      if (!values.has<String>("price")) continue;
      if (!values.has<String>("amount")) continue;
      float price = str2float (values.get<String>("price"));
      float amount = str2float (values.get<String>("amount"));
      quantity.push_back (amount);
      rate.push_back (price);
    }
    if (hurry) bitfinex_hurried_call_pass++;
    return;
  }
  if (hurry) bitfinex_hurried_call_fail++;
  error.append (json);
}


// This gets the asks through the REST API.
// Bitfinex limits the number of calls per time unit.
// This leads to { "error": "ERR_RATE_LIMIT"}.
void bitfinex_get_sellers_rest (string market,
                                string coin,
                                vector <float> & quantity,
                                vector <float> & rate,
                                string & error,
                                bool hurry)
{
  quantity.clear ();
  rate.clear ();
  string market_abbrev = bitfinex_get_coin_abbrev (market);
  string coin_abbrev = bitfinex_get_coin_abbrev (coin);
  string url = get_bitfinex_public_api_call ("1", {"book", coin_abbrev + market_abbrev}, { make_pair ("limit_bids", "0"), make_pair ("limit_asks", "15") });
  string json = http_call (url, error, "", false, false, "", {}, hurry, false, "");
  if (!error.empty ()) {
    if (hurry) bitfinex_hurried_call_fail++;
    return;
  }

  // {
  //   "bids":[],
  //   "asks":[
  //    {"price":"0.0022284","amount":"0.68","timestamp":"1506018540.0"},
  //    {"price":"0.0022336","amount":"91.522","timestamp":"1506018540.0"},
  //    {"price":"0.002234","amount":"634.067","timestamp":"1506018540.0"},
  //    ...
  //   ]
  // }

  Object main;
  main.parse (json);
  if (main.has<Array>("asks")) {
    Array asks = main.get<Array>("asks");
    for (size_t i = 0; i < asks.size(); i++) {
      Object values = asks.get <Object> (i);
      if (!values.has<String>("price")) continue;
      if (!values.has<String>("amount")) continue;
      float price = str2float (values.get<String>("price"));
      float amount = str2float (values.get<String>("amount"));
      quantity.push_back (amount);
      rate.push_back (price);
    }
    if (hurry) bitfinex_hurried_call_pass++;
    return;
  }
  if (hurry) bitfinex_hurried_call_fail++;
  error.append (json);
}
 

string bitfinex_limit_trade (string market,
                             string coin,
                             float quantity,
                             float rate,
                             string side,
                             string & json,
                             string & error)
{
  // The payload to POST.
  Object post_payload;
  // The symbol, e.g. "omgbtc" means: Trade OmiseGo on the Bitcoin market.
  string bitfinex_coin_abbreviation = bitfinex_get_coin_bitfinex_abbrev (coin);
  string bitfinex_market_abbreviation = bitfinex_get_coin_bitfinex_abbrev (market);
  string symbol = bitfinex_coin_abbreviation + bitfinex_market_abbreviation;
  post_payload << "symbol" << symbol;
  // How much to buy or sell. Should be a decimal string.
  post_payload << "amount" << float2string (quantity);
  // Price to buy or sell at. Must be positive. Should be a decimal string.
  post_payload << "price" << float2string (rate);
  // Either “buy” or “sell”.
  post_payload << "side" << side;
  // The order type: Exchange limit.
  post_payload << "type" << "exchange limit";
  // Further required fields.
  post_payload << "ocoorder" << false;
  post_payload << "buy_price_oco" << 0.0;
  post_payload << "sell_price_oco" << 0.0;
  // Call authenticated API.
  json = get_bitfinex_private_api_call_result ({"order", "new"}, post_payload, error);
  // Error handling.
  if (!error.empty ()) {
    error.append (" ");
    error.append (__FUNCTION__);
    return "";
  }
  /*
   {
   "id":4158993443,
   "cid":70846754704,
   "cid_date":"2017-10-05",
   "gid":null,
   "symbol":"omgbtc",
   "exchange":"bitfinex",
   "price":"0.000139","
   avg_execution_price":"0.0",
   "side":"buy",
   "type":"exchange limit",
   "timestamp":"1507232446.789312663",
   "is_live":true,
   "is_cancelled":false,
   "is_hidden":false,
   "oco_order":null,
   "was_forced":false,
   "original_amount":"1.0",
   "remaining_amount":"1.0",
   "executed_amount":"0.0",
   "src":"api",
   "order_id":4158993443
   }
   */
  Object main;
  main.parse (json);
  if (main.has<Number>("id")) {
    long int id = main.get<Number>("id");
    return to_string (id);
  }
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
  return "";
}


// Place a limit buy order for a $quantity of $coin abbreviation at $rate.
// It returns the order identifier.
string bitfinex_limit_buy (string market,
                           string coin,
                           float quantity,
                           float rate,
                           string & json,
                           string & error)
{
  return bitfinex_limit_trade (market, coin, quantity, rate, "buy", json, error);
}


// Place a limit sell order for a $quantity of $coin abbreviation at $rate.
// It returns the order identifier.
string bitfinex_limit_sell (string market,
                            string coin,
                            float quantity,
                            float rate,
                            string & json,
                            string & error)
{
  return bitfinex_limit_trade (market, coin, quantity, rate, "sell", json, error);
}


// Cancel $order id.
bool bitfinex_cancel_order (string order_id,
                            string & error)
{
  // The payload to POST.
  Object post_payload;
  long int id = stoull (order_id);
  post_payload << "order_id" << id;
  // Call authenticated API.
  string json = get_bitfinex_private_api_call_result ({"order", "cancel"}, post_payload, error);
  // Error handling.
  if (!error.empty ()) {
    error.append (" ");
    error.append (__FUNCTION__);
    return false;
  }
  /*
   {
   "id":4158993443,
   "cid":70846754704,
   "cid_date":"2017-10-05",
   "gid":null,
   "symbol":"omgbtc",
   "exchange":"bitfinex",
   "price":"0.000139","
   avg_execution_price":"0.0",
   "side":"buy",
   "type":"exchange limit",
   "timestamp":"1507232446.789312663",
   "is_live":true,
   "is_cancelled":false,
   "is_hidden":false,
   "oco_order":null,
   "was_forced":false,
   "original_amount":"1.0",
   "remaining_amount":"1.0",
   "executed_amount":"0.0",
   "src":"api",
   "order_id":4158993443
   }
   */
  Object main;
  main.parse (json);
  if (main.has<Number>("id")) {
    long int id = main.get<Number>("id");
    if (id == stol (order_id)) {
      return true;
    }
  }
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
  return false;
}


// Places an order to withdraw funds.
// It returns the order ID.
string bitfinex_withdrawal_order (string coin,
                                  float quantity,
                                  string address,
                                  string paymentid,
                                  string & json,
                                  string & error)
{
  // The payload to POST.
  Object post_payload;
  // The withdrawal type, e.g. 'bitcoin', 'litecoin', 'ethereum', 'ethereumc', 'mastercoin', 'zcash', 'monero', 'wire', 'dash', 'ripple', 'eos', 'neo' ...
  string bitfinex_method = bitfinex_get_coin_bitfinex_method (coin);
  post_payload << "withdraw_type" << bitfinex_method;
  // The wallet to withdraw from, can be “trading”, “exchange”, or “deposit”.
  post_payload << "walletselected" << "exchange";
  // The quantity to withdraw.
  post_payload << "amount" << float2string (quantity);
  // The destination address.
  post_payload << "address" << address;
  // Optional payment ID for a Monero transaction.
  if (!paymentid.empty ()) {
    post_payload << "payment_id" << paymentid;
  }
  // Call authenticated API.
  json = get_bitfinex_private_api_call_result ({"withdraw"}, post_payload, error);
  // Error handling.
  if (!error.empty ()) {
    error.append (" ");
    error.append (__FUNCTION__);
    return "";
  }
  /*
  [{"status":"success","message":"Your withdrawal request has been successfully submitted.","withdrawal_id":4147892,"fees":"0.1"}]
  */
  Array main;
  main.parse (json);
  for (size_t i = 0; i < main.size(); i++) {
    Object values = main.get <Object> (i);
    if (!values.has <String> ("status")) continue;
    if (!values.has <Number> ("withdrawal_id")) continue;
    string status = values.get <String>("status");
    if (status != "success") continue;
    long int withdrawal_id = values.get <Number>("withdrawal_id");
    if (values.has <String> ("message")) {
      to_stdout ({values.get <String>("message")});
    }
    return to_string (withdrawal_id);
  }
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
  return "";
}


void bitfinex_movement_history (string coin,
                                string deposit_or_withdrawal,
                                vector <string> & order_ids,
                                vector <string> & coin_abbreviations,
                                vector <string> & coin_ids,
                                vector <float> & quantities,
                                vector <string> & addresses,
                                vector <string> & transaction_ids,
                                string & error)
{
  // Translate the coin to the bitfinex abbreviation.
  string bitfinex_abbreviation = bitfinex_get_coin_bitfinex_abbrev (coin);
  bitfinex_abbreviation = str2upper (bitfinex_abbreviation);
  // The payload to POST.
  Object post_payload;
  post_payload << "currency" << bitfinex_abbreviation;
  // Call authenticated API.
  string json = get_bitfinex_private_api_call_result ({"history", "movements"}, post_payload, error);
  // Error handling.
  if (!error.empty ()) {
    error.append (" ");
    error.append (__FUNCTION__);
    return;
  }
  /*
   [
   {"id":4110384,
   "currency":"BTC",
   "method":"BITCOIN",
   "type":"DEPOSIT",
   "amount":"0.01396",
   "description":"215ae65a9c530fa506932cf52a6d0b48d9a86d4330818f35eae668659852b6d1",
   "address":"1GNsEAJ1oBmtnmASeUD7iAXsyvbu2gDqXf",
   "status":"COMPLETED",
   "timestamp":"1507218365.0",
   "timestamp_created":"1507211826.0",
   "txid":"215ae65a9c530fa506932cf52a6d0b48d9a86d4330818f35eae668659852b6d1",
   "fee":"0.0"}
   ...]
   */
  Array main;
  main.parse (json);
  for (size_t i = 0; i < main.size(); i++) {
    Object values = main.get <Object> (i);
    if (!values.has <Number> ("id")) continue;
    long int id = values.get <Number>("id");
    if (!values.has <String> ("currency")) continue;
    string currency = values.get <String>("currency");
    // Translate the currency.
    currency = str2lower (currency);
    string bot_abbreviation = bitfinex_get_coin_abbrev (currency);
    if (!values.has <String> ("type")) continue;
    string movement_type = values.get <String>("type");
    if (movement_type != deposit_or_withdrawal) continue;
    if (!values.has <String> ("amount")) continue;
    string amount = values.get <String>("amount");
    if (!values.has<String>("address")) continue;
    string address = values.get <String>("address");
    string status = values.get <String>("status");
    string txid;
    if (values.has<String>("txid")) {
      txid = values.get <String>("txid");
    }
    order_ids.push_back (to_string (id));
    coin_abbreviations.push_back (bot_abbreviation);
    coin_ids.push_back (bitfinex_get_coin_id (bot_abbreviation));
    quantities.push_back (str2float (amount));
    addresses.push_back (address);
    transaction_ids.push_back (txid);
  }
  // Error handling.
  if (main.size() == 0) {
    error.append (__FUNCTION__);
    error.append (" ");
    error.append (json);
  }
}


void bitfinex_withdrawalhistory (string coin,
                                 vector <string> & order_ids,
                                 vector <string> & coin_abbreviations,
                                 vector <string> & coin_ids,
                                 vector <float> & quantities,
                                 vector <string> & addresses,
                                 vector <string> & transaction_ids,
                                 string & error)
{
  bitfinex_movement_history (coin,
                             "WITHDRAWAL",
                             order_ids,
                             coin_abbreviations,
                             coin_ids,
                             quantities,
                             addresses,
                             transaction_ids,
                             error);
}


void bitfinex_deposithistory (string coin_abbreviation,
                              vector <string> & order_ids,
                              vector <string> & coin_abbreviations,
                              vector <string> & coin_ids,
                              vector <float> & quantities,
                              vector <string> & addresses,
                              vector <string> & transaction_ids,
                              string & error)
{
  bitfinex_movement_history (coin_abbreviation,
                             "DEPOSIT",
                             order_ids,
                             coin_abbreviations,
                             coin_ids,
                             quantities,
                             addresses,
                             transaction_ids,
                             error);
}


void bitfinex_get_minimum_trade_amounts (const string & market,
                                         const vector <string> & coins,
                                         map <string, float> & market_amounts,
                                         map <string, float> & coin_amounts,
                                         string & error)
{
  // Call the API to get the exchange's symbol details.
  string url = get_bitfinex_public_api_call ("1", {"symbols_details"}, { });
  string json = http_call (url, error, "", false, false, "", {}, false, false, "");
  if (!error.empty ()) {
    return;
  }
  // [{"pair":"btcusd","price_precision":5,"initial_margin":"30.0","minimum_margin":"15.0","maximum_order_size":"2000.0","minimum_order_size":"0.002","expiration":"NA"},{"pair":"ltcusd","price_precision":5,"initial_margin":"30.0","minimum_margin":"15.0","maximum_order_size":"5000.0","minimum_order_size":"0.08","expiration":"NA"},...]
  // Get the abbreviation to filter the desired market out from the above JSON.
  string bitfinex_market_abbreviation = bitfinex_get_coin_bitfinex_abbrev (market);
  // Parse the JSON.
  Array main;
  main.parse (json);
  if (main.size ()) {
    for (size_t i = 0; i < main.size(); i++) {
      Object values = main.get <Object> (i);
      string pair = values.get <String> ("pair");
      // Should be six bytes long.
      if (pair.size () < 6) continue;
      // Take the desired markets only, for just now.
      string second = pair.substr (3);
      if (second != bitfinex_market_abbreviation) continue;
      // Get the Bitfinex coin abbreviation.
      string bitfinex_coin_abbreviation = pair.substr (0, 3);
      string coin_id = bitfinex_get_coin_id (bitfinex_coin_abbreviation);
      string minimum_order_size = values.get <String> ("minimum_order_size");
      if (!minimum_order_size.empty ()) {
        float amount = str2float (minimum_order_size);
        coin_amounts [coin_id] = amount;
      }
    }
  } else {
    error.append (json);
  }
  (void) coins;
  (void) market_amounts;
}


int bitfinex_get_hurried_call_passes ()
{
  return bitfinex_hurried_call_pass;
}


int bitfinex_get_hurried_call_fails ()
{
  return bitfinex_hurried_call_fail;
}
