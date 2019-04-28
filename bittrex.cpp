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


#include "bittrex.h"
#include "proxy.h"


#define api_key "--enter--your--value--here--"
#define api_secret "--enter--your--value--here--"


atomic <long> bittrex_private_api_call_sequencer (0);


// Hurried call counters.
atomic <int> bittrex_hurried_call_pass (0);
atomic <int> bittrex_hurried_call_fail (0);


string get_bittrex_api_call (string category, string method, vector <pair <string, string> > parameters)
{
  string url ("https://bittrex.com/api/v1.1/");
  url.append (category);
  url.append ("/");
  url.append (method);
  for (auto element : parameters) {
    string key = element.first;
    string value = element.second;
    if (url.find ("?") == string::npos) url.append ("?");
    else url.append ("&");
    url.append (key);
    url.append ("=");
    url.append (value);
  }
  return url;
}


string bittrex_get_exchange ()
{
  return "bittrex";
}


typedef struct
{
  const char * identifier;
  const char * abbreviation;
  const char * name;
}
bittrex_coin_record;


bittrex_coin_record bittrex_coins_table [] =
{
  { "bitcoin", "BTC", "Bitcoin" },
  { "litecoin", "LTC", "Litecoin" },
  { "dogecoin", "DOGE", "Dogecoin" },
  { "vertcoin", "VTC", "Vertcoin" },
  { "peercoin", "PPC", "Peercoin" },
  { "feathercoin", "FTC", "Feathercoin" },
  { "reddcoin", "RDD", "ReddCoin" },
  { "nxt", "NXT", "NXT" },
  { "dash", "DASH", "Dash" },
  { "potcoin", "POT", "PotCoin" },
  { "blackcoin", "BLK", "BlackCoin" },
  { "einsteinium", "EMC2", "Einsteinium" },
  { "myriad", "XMY", "Myriad" },
  { "auroracoin", "AUR", "AuroraCoin" },
  { "electronicgulden", "EFL", "ElectronicGulden" },
  { "goldcoin", "GLD", "GoldCoin" },
  { "solarcoin", "SLR", "SolarCoin" },
  { "pesetacoin", "PTC", "PesetaCoin " },
  { "groestlcoin", "GRS", "Groestlcoin" },
  { "gulden", "NLG", "Gulden" },
  { "rubycoin", "RBY", "RubyCoin" },
  { "whitecoin", "XWC", "WhiteCoin" },
  { "monacoin", "MONA", "MonaCoin" },
  { "hempcoin", "THC", "HempCoin" },
  { "energycoin", "ENRG", "EnergyCoin" },
  { "europecoin", "ERC", "EuropeCoin" },
  { "vericoin", "VRC", "VeriCoin" },
  { "curecoin", "CURE", "CureCoin" },
  { "monero", "XMR", "Monero" },
  { "cloakcoin", "CLOAK", "CloakCoin" },
  { "kore", "KORE", "Kore" },
  { "digitalnote", "XDN", "DigitalNote" },
  { "trustplus", "TRUST", "TrustPlus" },
  { "navcoin", "NAV", "NAVCoin" },
  { "stealthcoin", "XST", "StealthCoin" },
  { "viacoin", "VIA", "ViaCoin " },
  { "pinkcoin", "PINK", "PinkCoin" },
  { "iocoin", "IOC", "I/OCoin" },
  { "cannabiscoin", "CANN", "CannabisCoin" },
  { "syscoin", "SYS", "SysCoin" },
  { "neoscoin", "NEOS", "NeosCoin" },
  { "digibyte", "DGB", "Digibyte" },
  { "burstcoin", "BURST", "BURSTCoin" },
  { "exclusivecoin", "EXCL", "ExclusiveCoin" },
  { "dopecoin", "DOPE", "DopeCoin" },
  { "blocknet", "BLOCK", "BlockNet" },
  { "artbyte", "ABY", "ArtByte" },
  { "bytecent", "BYC", "Bytecent" },
  { "magi", "XMG", "Magi" },
  { "bitbay", "BAY", "BitBay" },
  { "spreadcoin", "SPR", "SpreadCoin" },
  { "ripple", "XRP", "Ripple" },
  { "gamecredits", "GAME", "GameCredits" },
  { "circuitsofvalue", "COVAL", "Circuits of Value" },
  { "nexus", "NXS", "Nexus" },
  { "counterparty", "XCP", "Counterparty" },
  { "bitbean", "BITB", "BitBean" },
  { "geocoin", "GEO", "GeoCoin" },
  { "foldingcoin", "FLDC", "FoldingCoin" },
  { "gridcoin", "GRC", "GridCoin" },
  { "florin", "FLO", "Florin" },
  { "nubits", "NBT", "Nubits" },
  { "monetaryunit", "MUE", "MonetaryUnit" },
  { "neweconomymovement", "XEM", "NewEconomyMovement" },
  { "clams", "CLAM", "CLAMs" },
  { "diamond", "DMD", "Diamond" },
  { "sphere", "SPHR", "Sphere" },
  { "okcash", "OK", "OkCash" },
  { "aeon", "AEON", "Aeon" },
  { "ethereum", "ETH", "Ethereum" },
  { "transfercoin", "TX", "TransferCoin" },
  { "bitcrystals", "BCY", "BitCrystals" },
  { "expanse", "EXP", "Expanse" },
  { "omni", "OMNI", "OMNI" },
  { "amp", "AMP", "AMP" },
  { "lumen", "XLM", "Lumen" },
  { "emercoin", "EMC", "EmerCoin" },
  { "factom", "FCT", "Factom" },
  { "evergreencoin", "EGC", "EverGreenCoin" },
  { "salus", "SLS", "SaluS" },
  { "radium", "RADS", "Radium" },
  { "decred", "DCR", "Decred" },
  { "bitsend", "BSD", "BitSend" },
  { "verge", "XVG", "Verge" },
  { "pivx", "PIVX", "Pivx" },
  { "memetic", "MEME", "Memetic" },
  { "steem", "STEEM", "STEEM" },
  { "2give", "2GIVE", "2GIVE" },
  { "lisk", "LSK", "Lisk" },
  { "breakout", "BRK", "Breakout" },
  { "waves", "WAVES", "Waves" },
  { "lbrycredits", "LBC", "LBRY Credits" },
  { "steemdollars", "SBD", "SteemDollars" },
  { "breakoutstake", "BRX", "Breakout Stake" },
  { "ethereumclassic", "ETC", "Ethereum Classic" },
  { "stratis", "STRAT", "Stratis" },
  { "unbreakablecoin", "UNB", "UnbreakableCoin" },
  { "syndicate", "SYNX", "Syndicate" },
  { "eboost", "EBST", "eBoost" },
  { "verium", "VRM", "Verium" },
  { "sequence", "SEQ", "Sequence" },
  { "augur", "REP", "Augur" },
  { "shift", "SHIFT", "Shift" },
  { "zcoin", "XZC", "ZCoin" },
  { "neo", "NEO", "Neo" },
  { "zcash", "ZEC", "Zcash" },
  { "zclassic", "ZCL", "Zclassic" },
  { "internetofpeople", "IOP", "Internet Of People" },
  { "golos", "GOLOS", "Golos" },
  { "ubiq", "UBQ", "Ubiq" },
  { "komodo", "KMD", "Komodo" },
  { "gbg", "GBG", "GBG" },
  { "siberianchervonets", "SIB", "Siberian Chervonets" },
  { "ion", "ION", "Ion" },
  { "lomocoin", "LMC", "Lomocoin" },
  { "qwark", "QWARK", "Qwark" },
  { "crown", "CRW", "Crown" },
  { "swarmcitytoken", "SWT", "Swarm City Token" },
  { "melon", "MLN", "Melon" },
  { "ark", "ARK", "Ark" },
  { "dynamic", "DYN", "Dynamic" },
  { "tokes", "TKS", "Tokes" },
  { "musicoin", "MUSIC", "Musicoin" },
  { "databits", "DTB", "Databits" },
  { "bytes", "GBYTE", "Bytes" },
  { "golem", "GNT", "Golem" },
  { "nexium", "NXC", "Nexium" },
  { "edgeless", "EDG", "Edgeless" },
  { "wingsdao", "WINGS", "Wings DAO" },
  { "iex.ec", "RLC", "iEx.ec" },
  { "gnosis", "GNO", "Gnosis" },
  { "guppy", "GUP", "Guppy" },
  { "lunyr", "LUN", "Lunyr" },
  { "humaniq", "HMQ", "Humaniq" },
  { "aragon", "ANT", "Aragon" },
  { "siacoin", "SC", "Siacoin" },
  { "basicattentiontoken", "BAT", "Basic Attention Token" },
  { "zencash", "ZEN", "Zencash" },
  { "quantumresistantledger", "QRL", "Quantum Resistant Ledger" },
  { "creditbit", "CRB", "CreditBit" },
  { "patientory", "PTOY", "Patientory" },
  { "cofoundit", "CFI", "Cofound.it" },
  { "bancor", "BNT", "Bancor" },
  { "numeraire", "NMR", "Numeraire" },
  { "statusnetworktoken", "SNT", "Status Network Token" },
  { "decent", "DCT", "DECENT" },
  { "elastic", "XEL", "Elastic" },
  { "monaco", "MCO", "Monaco" },
  { "adtoken", "ADT", "adToken" },
  { "tenxpaytoken", "PAY", "TenX Pay Token" },
  { "storj", "STORJ", "Storj" },
  { "adex", "ADX", "AdEx" },
  { "omisego", "OMG", "OmiseGO" },
  { "civic", "CVC", "Civic" },
  { "particl", "PART", "Particl" },
  { "qtum", "QTUM", "Qtum" },
  { "bitcoincash", "BCH", "Bitcoin Cash" },
  { "district0x", "DNT", "district0x" },
  { "ada", "ADA", "Ada" },
  { "decentraland", "MANA", "Decentraland" },
  { "salt", "SALT", "Salt" },
  { "blocktix", "TIX", "Blocktix" },
  { "ripiocreditnetwork", "RCN", "Ripio Credit Network" },
  { "viberate", "VIB", "Viberate" },
  { "mercury", "MER", "Mercury" },
  { "powerledger", "POWR", "PowerLedger" },
  { "bitcoingold", "BTG", "Bitcoin Gold" },
  { "enigma", "ENG", "Enigma" },
  { "incent", "INCNT", "Incent" },
  { "unikoingold", "UKG", "UnikoinGold" },
  { "gambit", "GAM", "Gambit" },
  { "ardor", "ARDR", "Ardor" },
  { "ignis", "IGNIS", "Ignis" },
  { "revolutionvr", "RVR", "RevolutionVR" },
  { "sirintoken", "SRN", "Sirin Token" },
  { "worldwideassetexchange", "WAX", "Worldwide Asset Exchange" },
  { "0xprotocol", "ZRX", "0x Protocol" },
  { "blockv", "VEE", "BLOCKv" },
  { "blockmasoncreditprotocol", "BCPT", "BlockMason Credit Protocol" },
  { "tron", "TRX", "TRON" },
  { "trueusd", "TUSD", "TrueUSD" },
  { "loopring", "LRC", "Loopring" },
  { "uptoken", "UP", "UpToken" },
  { "dmarket", "DMT", "DMarket" },
  { "polymath", "POLY", "Polymath" },
  { "propy", "PRO", "Propy" },
  { "bloom", "BLT", "Bloom" },
  { "storm", "STORM", "Storm" },
  { "aidcoin", "AID", "AidCoin" },
  { "gifto", "GTO", "Gifto" },
  { "naga", "NGC", "Naga" },
  { "odyssey", "OCN", "Odyssey" },
  { "bittube", "TUBE", "BitTube" },
  { "cashbet", "CBC", "CashBet" },
  { "bitswift", "BITS", "Bitswift" },
  { "crowdmachine", "CMCT", "Crowd Machine" },
  { "more", "MORE", "More" },
  { "nolimitcoin", "NLC2", "NoLimitCoin" },
  { "bankex", "BKX", "Bankex" },
  { "mainframe", "MFT", "Mainframe" },
  { "loomnetwork", "LOOM", "Loom Network" },
  { "refereum", "RFR", "Refereum" },
  { "ravencoin", "RVN", "RavenCoin" },

};


vector <string> bittrex_get_coin_ids ()
{
  vector <string> ids;
  unsigned int data_count = sizeof (bittrex_coins_table) / sizeof (*bittrex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    ids.push_back (bittrex_coins_table[i].identifier);
  }
  return ids;
}


string bittrex_get_coin_id (string coin)
{
  unsigned int data_count = sizeof (bittrex_coins_table) / sizeof (*bittrex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == bittrex_coins_table[i].identifier) return bittrex_coins_table[i].identifier;
    if (coin == bittrex_coins_table[i].abbreviation) return bittrex_coins_table[i].identifier;
    if (coin == bittrex_coins_table[i].name) return bittrex_coins_table[i].identifier;
  }
  return "";
}


string bittrex_get_coin_abbrev (string coin)
{
  unsigned int data_count = sizeof (bittrex_coins_table) / sizeof (*bittrex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == bittrex_coins_table[i].identifier) return bittrex_coins_table[i].abbreviation;
    if (coin == bittrex_coins_table[i].abbreviation) return bittrex_coins_table[i].abbreviation;
    if (coin == bittrex_coins_table[i].name) return bittrex_coins_table[i].abbreviation;
  }
  return "";
}


string bittrex_get_coin_name (string coin)
{
  unsigned int data_count = sizeof (bittrex_coins_table) / sizeof (*bittrex_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == bittrex_coins_table[i].identifier) return bittrex_coins_table[i].name;
    if (coin == bittrex_coins_table[i].abbreviation) return bittrex_coins_table[i].name;
    if (coin == bittrex_coins_table[i].name) return bittrex_coins_table[i].name;
  }
  return "";
}


void bittrex_get_coins (const string & market,
                        vector <string> & coin_abbrevs,
                        vector <string> & coin_ids,
                        vector <string> & names,
                        string & error)
{
  string market_abbrev = bittrex_get_coin_abbrev (market);
  string url = get_bittrex_api_call ("public", "getmarkets", {});
  string json = http_call (url, error, "", false, false, "", {}, false, false, "");
  if (!error.empty ()) {
    return;
  }
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          if (!values.has<String>("BaseCurrency")) continue;
          string base_currency = values.get<String>("BaseCurrency");
          if (base_currency != market_abbrev) continue;
          if (!values.has<String>("MarketCurrency")) continue;
          string market_currency = values.get<String>("MarketCurrency");
          if (!values.has<String>("MarketCurrencyLong")) continue;
          string market_currency_long = values.get<String>("MarketCurrencyLong");
          coin_abbrevs.push_back (market_currency);
          string coin_id = bittrex_get_coin_id (market_currency);
          coin_ids.push_back (coin_id);
          names.push_back (market_currency_long);
        }
        return;
      }
    }
  }
  error.append (json);
}


void bittrex_get_market_summaries (string market,
                                   vector <string> & coin_ids,
                                   vector <float> & bid,
                                   vector <float> & ask,
                                   string & error)
{
  string url = get_bittrex_api_call ("public", "getmarketsummaries", {});
  string json = http_call (url, error, "", false, false, "", {}, false, false, satellite_get_interface (url));
  if (!error.empty ()) {
    return;
  }
  string market_abbrev = bittrex_get_coin_abbrev (market);
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          if (!values.has<String>("MarketName")) continue;
          string market_name = values.get<String>("MarketName");
          // Example: BTC-NLG
          if (market_name.find (market_abbrev) == 0) {
            market_name.erase (0, 4);
            if (!values.has<Number>("Bid")) continue;
            if (!values.has<Number>("Ask")) continue;
            float Bid = values.get<Number>("Bid");
            float Ask = values.get<Number>("Ask");
            string coin_id = bittrex_get_coin_id (market_name);
            if (coin_id.empty ()) continue;
            coin_ids.push_back (coin_id);
            bid.push_back (Bid);
            ask.push_back (Ask);
          }
        }
        return;
      }
    }
  }
  error.append (json);
}


void bittrex_get_open_orders (vector <string> & identifier,
                              vector <string> & market,
                              vector <string> & coin_abbreviation,
                              vector <string> & coin_ids,
                              vector <string> & buy_sell,
                              vector <float> & quantity,
                              vector <float> & rate,
                              vector <string> & date,
                              string & error)
{
  api_call_sequencer (bittrex_private_api_call_sequencer);
  string nonce = increasing_nonce ();
  string url = get_bittrex_api_call ("market", "getopenorders", { make_pair ("apikey", api_key), make_pair ("nonce", nonce) });
  string hexits = hmac_sha512_hexits (api_secret, url);
  string json = http_call (url, error, "", false, false, "", { make_pair ("Apisign", hexits)}, false, false, "");
  if (!error.empty ()) {
    return;
  }
  // {"success":true,"message":"","result":[{"Uuid":null,"OrderUuid":"341b7c7e-b068-4100-8d3a-825caa11b8a3","Exchange":"BTC-ZEC","OrderType":"LIMIT_SELL","Quantity":0.10000000,"QuantityRemaining":0.10000000,"Limit":0.03265317,"CommissionPaid":0.00000000,"Price":0.00000000,"PricePerUnit":null,"Opened":"2017-12-09T18:01:29.68","Closed":null,"CancelInitiated":false,"ImmediateOrCancel":false,"IsConditional":false,"Condition":"NONE","ConditionTarget":null}]}
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          if (!values.has<String>("OrderUuid")) continue;
          if (!values.has<String>("Exchange")) continue;
          if (!values.has<String>("OrderType")) continue;
          if (!values.has<Number>("QuantityRemaining")) continue;
          if (!values.has<Number>("Limit")) continue;
          if (!values.has<String>("Opened")) continue;
          string OrderUuid = values.get<String>("OrderUuid");
          string Exchange = values.get<String>("Exchange");
          vector <string> market_coin_pair = explode (Exchange, '-');
          if (market_coin_pair.size () != 2) continue;
          string market_abbrev = market_coin_pair [0];
          string coin_abbrev = market_coin_pair [1];
          string OrderType = values.get<String>("OrderType");
          float QuantityRemaining = values.get<Number>("QuantityRemaining");
          float Limit = values.get<Number>("Limit");
          string Opened = values.get<String>("Opened");
          identifier.push_back (OrderUuid);
          string market_id = bittrex_get_coin_id (market_abbrev);
          market.push_back (market_id);
          coin_abbreviation.push_back (coin_abbrev);
          coin_ids.push_back (bittrex_get_coin_id (coin_abbrev));
          buy_sell.push_back (OrderType);
          quantity.push_back (QuantityRemaining);
          rate.push_back (Limit);
          date.push_back (Opened);
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
void bittrex_get_balances (vector <string> & coin_abbrev,
                           vector <string> & coin_ids,
                           vector <float> & total,
                           vector <float> & reserved,
                           vector <float> & unconfirmed,
                           string & error)
{
  api_call_sequencer (bittrex_private_api_call_sequencer);
  string nonce = increasing_nonce ();
  string url = get_bittrex_api_call ("account", "getbalances", { make_pair ("apikey", api_key), make_pair ("nonce", nonce) });
  string hexits = hmac_sha512_hexits (api_secret, url);
  string json = http_call (url, error, "", false, false, "", {make_pair ("Apisign", hexits)}, false, true, "");
  if (!error.empty ()) {
    return;
  }
  /*
  {"success":true,"message":"","result":[
   {"Currency":"ARK","Balance":0.00000000,"Available":0.00000000,"Pending":0.00000000,"CryptoAddress":"AJMAQRfEzEVF88WQkyZmHJe9VqvedcWASM"},
   {"Currency":"BCC","Balance":0.00000000,"Available":0.00000000,"Pending":0.00000000,"CryptoAddress":"1FS6Z6ZU8xRw6TgvDmzX2yuN1qYU82m3bA"},
   {"Currency":"BTA","Balance":0.00000000,"Available":0.00000000,"Pending":0.00000000,"CryptoAddress":"BAVCgTsd38geUbt1fTWtnHGFygM19rgX6m"},
   {"Currency":"BTC","Balance":0.63185633,"Available":0.62658042,"Pending":0.00000000,"CryptoAddress":"16wRKm75Hd9uy1VYx76UA6rogPWwzg7trK"},
   ...
   */
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          if (!values.has<String>("Currency")) continue;
          if (!values.has<Number>("Balance")) continue;
          if (!values.has<Number>("Available")) continue;
          if (!values.has<Number>("Pending")) continue;
          string Currency = values.get<String>("Currency");
          // The freely available balance plus the balance reserved for orders.
          float Balance = values.get<Number>("Balance");
          // The freely available balance.
          float Available = values.get<Number>("Available");
          // The balance that does not yet have sufficient confirmations.
          float Pending = values.get<Number>("Pending");
          if (Balance == 0) continue;
          coin_abbrev.push_back (Currency);
          // Get the coin ID.
          // If there's a balance and the coin ID is unknown, give the coin abbreviation instead.
          // The result will be that any unknown balance will be reported at least.
          string coin_id = bittrex_get_coin_id (Currency);
          if (coin_id.empty ()) coin_id = Currency;
          coin_ids.push_back (coin_id);
          // Total balance includes also the reserved and unconfirmed balance.
          total.push_back (Balance + Pending);
          // Balance reserved for orders.
          reserved.push_back (Balance - Available);
          // The unconformed deposited balance.
          unconfirmed.push_back (Pending);
        }
        return;
      }
    }
  }
  error.append (json);
}


mutex bittrex_balance_mutex;
map <string, float> bittrex_balance_total;
map <string, float> bittrex_balance_available;
map <string, float> bittrex_balance_reserved;
map <string, float> bittrex_balance_unconfirmed;


void bittrex_get_balance (const string & coin, float * total, float * available, float * reserved, float * unconfirmed)
{
  bittrex_balance_mutex.lock ();
  if (total) * total = bittrex_balance_total [coin];
  if (available) * available = bittrex_balance_available [coin];
  if (reserved) * reserved = bittrex_balance_reserved [coin];
  if (unconfirmed) * unconfirmed = bittrex_balance_unconfirmed [coin];
  bittrex_balance_mutex.unlock ();
}


void bittrex_set_balance (const string & coin, float total, float available, float reserved, float unconfirmed)
{
  bittrex_balance_mutex.lock ();
  if (total != 0) bittrex_balance_total [coin] = total;
  if (available != 0) bittrex_balance_available [coin] = available;
  if (reserved != 0) bittrex_balance_reserved [coin] = reserved;
  if (unconfirmed != 0) bittrex_balance_unconfirmed [coin] = unconfirmed;
  bittrex_balance_mutex.unlock ();
}


vector <string> bittrex_get_coins_with_balances ()
{
  vector <string> coins;
  bittrex_balance_mutex.lock ();
  for (auto & element : bittrex_balance_total) {
    coins.push_back (element.first);
  }
  bittrex_balance_mutex.unlock ();
  return coins;
}


void bittrex_get_wallet_details (string coin, string & json, string & error,
                                 string & address, string & payment_id)
{
  api_call_sequencer (bittrex_private_api_call_sequencer);
  string nonce = increasing_nonce ();
  string coin_abbrev = bittrex_get_coin_abbrev (coin);
  string url = get_bittrex_api_call ("account", "getdepositaddress", { make_pair ("apikey", api_key), make_pair ("currency", coin_abbrev), make_pair ("nonce", nonce) });
  string hexits = hmac_sha512_hexits (api_secret, url);
  json = http_call (url, error, "", false, false, "", {make_pair ("Apisign", hexits)}, false, false, "");
  if (!error.empty ()) {
    return;
  }
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Object>("result")) {
        Object result = main.get<Object>("result");
        if (result.has<String>("Currency")) {
          if (result.has<String>("Address")) {
            string Currency = result.get<String>("Currency");
            string Address = result.get<String>("Address");
            if (Currency == coin_abbrev) {
              address = Address;
              if (coin_abbrev == "XRP") {
                // In the case of Ripple, the API returns the payment ID rather than the address.
                payment_id = address;
                // The Bittrex Ripple address:
                address = "rPVMhWBsfF9iMXYj3aAzJVkPDTFNSyWdKy";
                // Bittrex was asked to get the Ripple address via the API.
                // The reason was that it had changed the Bittrex pool Ripple address.
                // So automatic balancing became inpossible to do.
                // Here's the link to the feature request:
                // https://support.bittrex.com/hc/en-us/requests/455703
                //  The feature request was forwarded to the developers.
              }
              if (coin_abbrev == "XEM") {
                // In the case of NewEconomyMovement, the API returns the message rather than the address.
                payment_id = address;
                // The Bittrex NewEconomyMovement pool address:
                address = "ND2JRPQIWXHKAA26INVGA7SREEUMX5QAI6VU7HNR";
              }
              return;
            }
          }
        }
      }
    }
  }
  error.append (json);
  return;
}


void bittrex_get_order_book (string market,
                             string coin,
                             string type,
                             vector <float> & quantity,
                             vector <float> & rate,
                             string & error,
                             bool hurry)
{
  string market_abbrev = bittrex_get_coin_abbrev (market);
  string currency_abbrev = bittrex_get_coin_abbrev (coin);
  // Bittrex returns the whole orderbook, and that might include more than 500 entries.
  // The API documentation describes no way to limit the number of entries it returns.
  string url = get_bittrex_api_call ("public", "getorderbook", { make_pair ("market", market_abbrev + "-" + currency_abbrev), make_pair ("type", type)});
  string json = http_call (url, error, "", false, false, "", {}, hurry, false, "");
  if (!error.empty ()) {
    if (hurry) bittrex_hurried_call_fail++;
    return;
  }
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          if (!values.has<Number>("Quantity")) continue;
          if (!values.has<Number>("Rate")) continue;
          float Quantity = values.get<Number>("Quantity");
          float Rate = values.get<Number>("Rate");
          quantity.push_back (Quantity);
          rate.push_back (Rate);
        }
        if (hurry) bittrex_hurried_call_pass++;
        return;
      }
    }
  }
  // Error handler.
  if (hurry) bittrex_hurried_call_fail++;
  error.append (json);
}



void bittrex_get_buyers (string market,
                         string coin,
                         vector <float> & quantity,
                         vector <float> & rate,
                         string & error,
                         bool hurry)
{
  bittrex_get_order_book (market, coin, "buy", quantity, rate, error, hurry);
}


void bittrex_get_sellers (string market,
                          string coin,
                          vector <float> & quantity,
                          vector <float> & rate,
                          string & error,
                          bool hurry)
{
  bittrex_get_order_book (market, coin, "sell", quantity, rate, error, hurry);
}


// Place a limit buy order for a $quantity of $coin abbreviation at $rate.
// It returns the order identifier.
string bittrex_limit_buy (string market,
                          string coin,
                          float quantity,
                          float rate,
                          string & json,
                          string & error)
{
  api_call_sequencer (bittrex_private_api_call_sequencer);
  string nonce = increasing_nonce ();
  string market_abbreviation = bittrex_get_coin_abbrev (market);
  string coin_abbreviation = bittrex_get_coin_abbrev (coin);
  string url = get_bittrex_api_call ("market", "buylimit", { make_pair ("apikey", api_key), make_pair ("nonce", nonce), make_pair ("market", market_abbreviation + "-" + coin_abbreviation), make_pair ("quantity", float2string (quantity)), make_pair ("rate", float2string (rate)) });
  string hexits = hmac_sha512_hexits (api_secret, url);
  json = http_call (url, error, "", false, false, "", { make_pair ("Apisign", hexits)}, false, true, "");
  if (!error.empty ()) {
    return "";
  }
  string order_id;
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Object>("result")) {
        Object result = main.get<Object>("result");
        if (result.has<String>("uuid")) {
          return result.get<String>("uuid");
        }
      }
    }
  }
  return "";
}


// Place a limit sell order for a $quantity of $coin abbreviation at $rate.
// It returns the order identifier.
string bittrex_limit_sell (string market,
                           string coin,
                           float quantity,
                           float rate,
                           string & json,
                           string & error)
{
  api_call_sequencer (bittrex_private_api_call_sequencer);
  string nonce = increasing_nonce ();
  string market_abbreviation = bittrex_get_coin_abbrev (market);
  string coin_abbreviation = bittrex_get_coin_abbrev (coin);
  string url = get_bittrex_api_call ("market", "selllimit", { make_pair ("apikey", api_key), make_pair ("nonce", nonce), make_pair ("market", market_abbreviation + "-" + coin_abbreviation), make_pair ("quantity", float2string (quantity)), make_pair ("rate", float2string (rate)) });
  string hexits = hmac_sha512_hexits (api_secret, url);
  json = http_call (url, error, "", false, false, "", { make_pair ("Apisign", hexits)}, false, true, "");
  if (!error.empty ()) {
    return "";
  }
  string order_id;
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Object>("result")) {
        Object result = main.get<Object>("result");
        if (result.has<String>("uuid")) {
          return result.get<String>("uuid");
        }
      }
    }
  }
  return "";
}


// Cancel $order id.
bool bittrex_cancel_order (string order_id,
                           string & error)
{
  api_call_sequencer (bittrex_private_api_call_sequencer);
  string nonce = increasing_nonce ();
  string url = get_bittrex_api_call ("market", "cancel", { make_pair ("apikey", api_key), make_pair ("nonce", nonce), make_pair ("uuid", order_id) });
  string hexits = hmac_sha512_hexits (api_secret, url);
  string json = http_call (url, error, "", false, false, "", { make_pair ("Apisign", hexits)}, false, false, "");
  if (!error.empty ()) {
    return false;
  }
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      return true;
    }
  }
  error.append (json);
  return false;
}


// Places an order to withdraw funds.
// It returns the order ID.
string bittrex_withdrawal_order (string coin,
                                 float quantity,
                                 string address,
                                 string paymentid,
                                 string & json,
                                 string & error)
{
  api_call_sequencer (bittrex_private_api_call_sequencer);
  string nonce = increasing_nonce ();
  string coin_abbrev = bittrex_get_coin_abbrev (coin);
  // Normally the bot uses "float2string" to convert the quantity to a string.
  // But here it just uses "to_string".
  // The reason is that "float2string" would change e.g. 10.025 to 10.0249996185.
  // And that would upset a withdrawal of Neo from Bittrex.
  vector <pair <string, string> > parameters = { make_pair ("apikey", api_key), make_pair ("nonce", nonce), make_pair ("currency", coin_abbrev), make_pair ("quantity", to_string (quantity)), make_pair ("address", address) };
  // If a currency does not use a Payment ID, and this call adds an empty Payment ID,
  // Bittrex responds with:
  // {"success":false,"message":"PAYMENTID_NOT_SUPPORTED","result":null}
  // So add it only in case it is given.
  if (!paymentid.empty ()) parameters.push_back (make_pair ("paymentid", paymentid));
  string url = get_bittrex_api_call ("account", "withdraw", parameters);
  string hexits = hmac_sha512_hexits (api_secret, url);
  json = http_call (url, error, "", false, false, "", { make_pair ("Apisign", hexits)}, false, true, "");
  if (!error.empty ()) {
    return "";
  }
  // In case a withdrawal order is placed successfully, Bittrex will say:
  // {"success":true,"message":"","result":{"uuid":"06eb1ee1-56c6-47da-94a6-37c304686c2e"}}
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Object>("result")) {
        Object result = main.get<Object>("result");
        if (result.has<String>("uuid")) {
          return result.get<String>("uuid");
        }
      }
    }
  }
  error.append (json);
  return "";
}


void bittrex_withdrawalhistory (vector <string> & order_ids,
                                vector <string> & coin_abbreviations,
                                vector <string> & coin_ids,
                                vector <float> & quantities,
                                vector <string> & addresses,
                                vector <string> & transaction_ids,
                                string & error)
{
  api_call_sequencer (bittrex_private_api_call_sequencer);
  string nonce = increasing_nonce ();
  string url = get_bittrex_api_call ("account", "getwithdrawalhistory", { make_pair ("apikey", api_key), make_pair ("nonce", nonce) });
  string hexits = hmac_sha512_hexits (api_secret, url);
  string json = http_call (url, error, "", false, false, "", { make_pair ("Apisign", hexits)}, false, false, "");
  if (!error.empty ()) {
    return;
  }
  /*
   {"success" : true, "message" : "", "result" : [
   {
			"PaymentUuid" : "b52c7a5c-90c6-4c6e-835c-e16df12708b1",
			"Currency" : "BTC",
			"Amount" : 17.00000000,
			"Address" : "1DeaaFBdbB5nrHj87x3NHS4onvw1GPNyAu",
			"Opened" : "2014-07-09T04:24:47.217",
			"Authorized" : true,
			"PendingPayment" : false,
			"TxCost" : 0.00020000,
			"TxId" : null,
			"Canceled" : true,
			"InvalidAddress" : false
   }, {
			"PaymentUuid" : "f293da98-788c-4188-a8f9-8ec2c33fdfcf",
			"Currency" : "XC",
			"Amount" : 7513.75121715,
			"Address" : "XVnSMgAd7EonF2Dgc4c9K14L12RBaW5S5J",
			"Opened" : "2014-07-08T23:13:31.83",
			"Authorized" : true,
			"PendingPayment" : false,
			"TxCost" : 0.00002000,
			"TxId" : "b4a575c2a71c7e56d02ab8e26bb1ef0a2f6cf2094f6ca2116476a569c1e84f6e",
			"Canceled" : false,
			"InvalidAddress" : false
   }
   ]}
   */
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          if (!values.has <String> ("PaymentUuid")) continue;
          string PaymentUuid = values.get <String>("PaymentUuid");
          if (!values.has <String> ("Currency")) continue;
          string Currency = values.get <String>("Currency");
          if (!values.get <Number> ("Amount")) continue;
          float Amount = values.get <Number>("Amount");
          if (!values.has<String>("Address")) continue;
          string Address = values.get <String>("Address");
          string TxId;
          if (values.has<String>("TxId")) {
            TxId = values.get <String>("TxId");
          }
          order_ids.push_back (PaymentUuid);
          coin_abbreviations.push_back (Currency);
          coin_ids.push_back (bittrex_get_coin_id (Currency));
          quantities.push_back (Amount);
          addresses.push_back (Address);
          transaction_ids.push_back (TxId);
        }
        return;
      }
    }
  }
  error.append (json);
}


void bittrex_deposithistory (vector <string> & order_ids,
                             vector <string> & coin_abbreviations,
                             vector <string> & coin_ids,
                             vector <float> & quantities,
                             vector <string> & addresses,
                             vector <string> & transaction_ids,
                             string & error)
{
  api_call_sequencer (bittrex_private_api_call_sequencer);
  string nonce = increasing_nonce ();
  string url = get_bittrex_api_call ("account", "getdeposithistory", { make_pair ("apikey", api_key), make_pair ("nonce", nonce) });
  string hexits = hmac_sha512_hexits (api_secret, url);
  string json = http_call (url, error, "", false, false, "", { make_pair ("Apisign", hexits)}, false, false, "");
  if (!error.empty ()) {
    return;
  }
  /*
   { "success" : true, "message" : "", "result" : [
   {
     "Id":29012182,
     "Amount":123.82573112,
     "Currency":"UBQ",
     "Confirmations":80,
     "LastUpdated":"2017-08-30T16:19:15.703",
     "TxId":"0x372da9977487cb038293e21103e111f2ab255c867a1f36fc1b7a35a9914632c4",
     "CryptoAddress":"0x84aa4946f69fb0d3dba355350f69532d3a327491"
   }, {
     "Id":28988832,
     "Amount":121.44559082,
     "Currency":"UBQ",
     "Confirmations":81,
     "LastUpdated":"2017-08-30T13:02:47.547",
     "TxId":"0xd226936240bd71f9bfadc55da64700aeb597d560060444cb3688a01648e04ddc",
     "CryptoAddress":"0x84aa4946f69fb0d3dba355350f69532d3a327491"
   }
   ]}
   */

  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          if (!values.has <Number> ("Id")) continue;
          int Id = values.get <Number>("Id");
          if (!values.has <String> ("Currency")) continue;
          string Currency = values.get <String>("Currency");
          if (!values.get <Number> ("Amount")) continue;
          float Amount = values.get <Number>("Amount");
          if (!values.has<String>("CryptoAddress")) continue;
          string CryptoAddress = values.get <String>("CryptoAddress");
          string TxId;
          if (values.has<String>("TxId")) {
            TxId = values.get <String>("TxId");
          }
          order_ids.push_back (to_string (Id));
          coin_abbreviations.push_back (Currency);
          coin_ids.push_back (bittrex_get_coin_id (Currency));
          quantities.push_back (Amount);
          addresses.push_back (CryptoAddress);
          transaction_ids.push_back (TxId);
        }
        return;
      }
    }
  }
  error.append (json);
}


void bittrex_get_minimum_trade_amounts (const string & market,
                                        const vector <string> & coins,
                                        map <string, float> & amounts,
                                        string & error)
{
  string url = get_bittrex_api_call ("public", "getmarkets", {});
  string json = http_call (url, error, "", false, false, "", {}, false, false, "");
  if (!error.empty ()) {
    return;
  }
  // {"success":true,"message":"","result":[{"MarketCurrency":"LTC","BaseCurrency":"BTC","MarketCurrencyLong":"Litecoin","BaseCurrencyLong":"Bitcoin","MinTradeSize":0.02931955,"MarketName":"BTC-LTC","IsActive":true,"Created":"2014-02-13T00:00:00","Notice":null,"IsSponsored":null,"LogoUrl":"https://bittrexblobstorage.blob.core.windows.net/public/6defbc41-582d-47a6-bb2e-d0fa88663524.png"},{"MarketCurrency":"DOGE","BaseCurrency":"BTC","MarketCurrencyLong":"Dogecoin","BaseCurrencyLong":"Bitcoin","MinTradeSize":877.19298246,"MarketName":"BTC-DOGE","IsActive":true,"Created":"2014-02-13T00:00:00","Notice":null,"IsSponsored":null,"LogoUrl":"https://bittrexblobstorage.blob.core.windows.net/public/a2b8eaee-2905-4478-a7a0-246f212c64c6.png"},{"MarketCurrency":"VTC","BaseCurrency":"BTC","MarketCurrencyLong":"Vertcoin","BaseCurrencyLong":"Bitcoin","MinTradeSize":1.05289757,"MarketName":"BTC-VTC","IsActive":true,"Created":"2014-02-13T00:00:00","Notice":null,"IsSponsored":null,"LogoUrl":"https://bittrexblobstorage.blob.core.windows.net/public/1f0317bc-c44b-4ea4-8a89-b9a71f3349c8.png"},... {"MarketCurrency":"UKG","BaseCurrency":"ETH","MarketCurrencyLong":"UnikoinGold","BaseCurrencyLong":"Ethereum","MinTradeSize":0.00000001,"MarketName":"ETH-UKG","IsActive":true,"Created":"2017-12-29T22:41:56.017","Notice":null,"IsSponsored":null,"LogoUrl":null}]}
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          if (!values.has<String>("BaseCurrency")) continue;
          string base_currency = values.get<String>("BaseCurrency");
          string market_id = bittrex_get_coin_id (base_currency);
          if (market != market_id) continue;
          if (!values.has<String>("MarketCurrency")) continue;
          string market_currency = values.get<String>("MarketCurrency");
          string coin_id = bittrex_get_coin_id (market_currency);
          if (!in_array (coin_id, coins)) continue;
          if (!values.has<Number>("MinTradeSize")) continue;
          float MinTradeSize = values.get<Number>("MinTradeSize");
          amounts [coin_id] = MinTradeSize;
        }
        return;
      }
    }
  }
  error.append (json);
}


int bittrex_get_hurried_call_passes ()
{
  return bittrex_hurried_call_pass;
}


int bittrex_get_hurried_call_fails ()
{
  return bittrex_hurried_call_fail;
}
