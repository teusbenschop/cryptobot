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


#include "tradesatoshi.h"
#include "proxy.h"


#define api_key "--enter--your--value--here--"
#define api_secret "--enter--your--value--here--"


// Hurried call counters.
atomic <int> tradesatoshi_hurried_call_pass (0);
atomic <int> tradesatoshi_hurried_call_fail (0);


typedef struct
{
  const char * identifier;
  const char * abbreviation;
  const char * name;
}
tradesatoshi_coin_record;


tradesatoshi_coin_record tradesatoshi_coins_table [] =
{
  { "bitcoin", "BTC", "Bitcoin" },
  { "elite", "1337", "Elite" },
  { "42", "42", "42" },
  { "808", "808", "808" },
  { "8bit", "8BIT", "8Bit" },
  { "acchain", "ACC", "Acchain" },
  { "advancedinternetblock", "AIB", "Advanced Internet Block" },
  { "bitblocks", "BBK", "BitBlocks" },
  { "bitconnect", "BCC", "Bitconnect" },
  { "bitcoincash", "BCH", "Bitcoin Cash" },
  { "bitcoinstake", "BCS", "BitcoinStake" },
  { "betcoin", "BETC", "Betcoin" },
  { "blackcoin", "BLK", "Blackcoin" },
  { "doubloon", "BOAT", "Doubloon " },
  { "bolivarcoin", "BOLI", "Bolivarcoin" },
  { "bitraam", "BRM", "BitRaam" },
  { "bitradio", "BRO", "Bitradio" },
  { "bitsend", "BSD", "BitSend " },
  { "bitstradescoin", "BSS", "BitstradesCoin" },
  { "bata", "BTA", "BATA" },
  { "btcruble", "BTCR", "BtcRuble" },
  { "bitcoinscrypt", "BTCS", "Bitcoin Scrypt" },
  { "bitcoinz", "BTCZ", "BitcoinZ" },
  { "bitcoingold", "BTG", "BitcoinGold" },
  { "bitmark", "BTM", "Bitmark" },
  { "bitcore", "BTX", "BitCore" },
  { "bumbacoin", "BUMBA", "Bumbacoin" },
  { "bunnycoin", "BUN", "BunnyCoin" },
  { "buzzcoin", "BUZZ", "BUZZcoin" },
  { "bottlecaps", "CAP", "Bottlecaps" },
  { "canadaecoin", "CDN", "Canada eCoin" },
  { "ceocoin", "CEO", "Ceocoin" },
  { "clam", "CLAM", "Clam" },
  { "cryptolion", "CLC", "Crypto Lion " },
  { "cloakcoin", "CLOAK", "CloakCoin " },
  { "colossuscoinxt", "COLX", "ColossusCoinXT" },
  { "compoundcoin", "COMP", "Compound Coin" },
  { "contcoin", "CONT", "CONTCOIN" },
  { "capricoin", "CPC", "Capricoin" },
  { "crave", "CRAVE", "Crave" },
  { "cryptonium", "CTC", "Cryptonium" },
  { "curecoin", "CURE", "Curecoin" },
  { "conspiracycoin", "CYC", "Conspiracycoin" },
  { "dash", "DASH", "Dash" },
  { "debitcoin", "DBTC", "Debitcoin" },
  { "deutscheemark", "DEM", "Deutsche eMark" },
  { "deepfuturecoin", "DFC", "Deepfuture Coin" },
  { "digibyte", "DGB", "DigiByte" },
  { "digigems", "DGMS", "Digigems" },
  { "dinero", "DIN", "Dinero" },
  { "dogecoin", "DOGE", "Dogecoin" },
  { "divotycoin", "DTC", " DivotyCoin" },
  { "ebookcoin", "EBC", "Ebookcoin" },
  { "evoblu", "EBLU", "Evoblu" },
  { "einsteinium", "EMC2", "Einsteinium " },
  { "eternity", "ENT", "Eternity" },
  { "espers", "ESP2", "Espers" },
  { "futuredigitalcurrency", "FDC", "FutureDigitalCurrency" },
  { "gamecredits", "GAME", "GameCredits " },
  { "goldblocks", "GB", "GoldBlocks" },
  { "goldfishcoin", "GFISH", "GoldFishCoin" },
  { "goldcoin", "GLD", "GoldCoin" },
  { "giantbirdcoin", "GNB", "GiantBirdCoin" },
  { "generationchanger", "GNC", "Generationchanger" },
  { "grimcoin", "GRIM", "GRIM Coin" },
  { "garlicoin", "GRLC", "Garlicoin" },
  { "groestlcoin", "GRS", "Groestlcoin" },
  { "gsave", "GSAVE", "GSAVE" },
  { "htmlcoin", "HTML", "HTML Coin" },
  { "htmlcoin", "HTML5", "HTMLCOIN" },
  { "hush", "HUSH", "HUSH" },
  { "hyper", "HYPER", "Hyper" },
  { "indmoneysystem", "IMS", "Ind Money system" },
  { "insanecoin", "INSN", "InsaneCoin" },
  { "joincoin", "J", "Joincoin" },
  { "jesuscoin", "JESUS", "JesusCoin" },
  { "joystickcoin", "JOY", "Joystick Coin" },
  { "kaicoin", "KAI", "KaiCoin" },
  { "knolix", "KNLX", "Knolix" },
  { "korecoin", "KORE", "KoreCoin" },
  { "kzcash", "KZC", "Kzcash" },
  { "lbrycredits", "LBC", "LBRY Credits" },
  { "litedoge", "LDOGE", "LiteDoge" },
  { "litecoin", "LTC", "Litecoin" },
  { "litkoin", "LTK", "Litkoin" },
  { "mergecoin", "MGC", "MergeCoin" },
  { "mincoin", "MNC", "MinCoin" },
  { "monacoin", "MONA", "Monacoin" },
  { "needlecoin", "NDC", "NeedleCoin" },
  { "netcoin", "NET", "NetCoin" },
  { "nevacoin", "NEVA", "Nevacoin" },
  { "incakoin", "NKA", "Incakoin" },
  { "nobtcoin", "NOBT", "Nobtcoin" },
  { "novacoin", "NVC", "Novacoin" },
  { "nexus", "NXS", "Nexus " },
  { "nyancoin", "NYAN", "NyanCoin" },
  { "newyorkcoin", "NYC", "New York Coin" },
  { "okcash", "OK", "Okcash" },
  { "deeponion", "ONION", "DeepOnion" },
  { "orbitcoin", "ORB", "Orbitcoin " },
  { "pakcoin", "PAK", "Pakcoin" },
  { "pioneercoin", "PCOIN", "Pioneer Coin" },
  { "piggycoin", "PIGGY", "Piggycoin" },
  { "pinkcoin", "PINK", "Pinkcoin" },
  { "plusonecoin", "PLUS1", "PlusOneCoin" },
  { "popularcoin", "POP", "PopularCoin" },
  { "potcoin", "POT", "PotCoin " },
  { "peercoin", "PPC", "Peercoin" },
  { "prux", "PRUX", "Prux" },
  { "pesetacoin", "PTC", "Pesetacoin" },
  { "putincoin2", "PUT", "PutinCoin2" },
  { "russiacoin", "RC", "RussiaCoin " },
  { "reddcoin", "RDD", "Reddcoin" },
  { "safeexchangecoin", "SAFEX", "Safe Exchange Coin" },
  { "beachcoin", "SAND", "Beachcoin" },
  { "minutecoin", "SCF", "MinuteCoin" },
  { "scoobycoin", "SCOBY", "ScoobyCoin" },
  { "sficoin", "SFI", "SFI Coin" },
  { "shacoin", "SHA", "Shacoin" },
  { "stronghands", "SHND", "Stronghands" },
  { "sibcoin", "SIB", "Sibcoin" },
  { "socialcoin", "SLC", "SocialCoin" },
  { "slothcoin", "SLOTH", "Slothcoin" },
  { "solar", "SOLAR", "Solar" },
  { "sparks", "SPK", "Sparks" },
  { "startcoin", "START", "Startcoin" },
  { "staxcoin2", "STAX", "Staxcoin2" },
  { "stratis", "STRAT", "STRATIS" },
  { "sync", "SYNC", "SYNC" },
  { "topcash", "TCSH", "Top cash" },
  { "teacoin", "TEA", "TeaCoin" },
  { "tomorrowcoin", "TMRW", "TomorrowCoin " },
  { "trollplay", "TPAY", "TrollPlay" },
  { "terracoin", "TRC", "Terracoin" },
  { "truckcoin", "TRK", "Truckcoin" },
  { "trumpcoin", "TRUMP", "Trumpcoin" },
  { "unobtanium", "UNO", "Unobtanium " },
  { "usdtether", "USDT", "Tether" },
  { "vclassiccoin", "VCC", "Vclassiccoin" },
  { "vaultcoin", "VLTC", "Vault Coin" },
  { "votecoin", "VOT", "VoteCoin" },
  { "vericoin", "VRC", "VeriCoin " },
  { "vsync", "VSX", "Vsync " },
  { "virtacoin", "VTA", "Virtacoin" },
  { "worldbtc", "WBTC", "WorldBTC" },
  { "womencoin", "WOMEN", "WomenCoin" },
  { "bitcoinplus", "XBC", "Bitcoin Plus" },
  { "xtrabytes", "XBY", "XtraBYtes" },
  { "xgox", "XGOX", "Xgox" },
  { "magi", "XMG", "Magi" },
  { "myriad", "XMY", "Myriad " },
  { "experiencepoints", "XP", "ExperiencePoints" },
  { "shield", "XSH", "Shield" },
  { "spectrecoin", "XSPEC", "spectrecoin" },
  { "verge", "XVG", "Verge" },
  { "virtacoinplus", "XVP", "VirtacoinPlus" },
  { "zcoin", "XZC", "Zcoin" },
  { "zclassic", "ZCL", "Zclassic" },
  { "zcash", "ZEC", "Zcash" },
  { "zeitcoin", "ZEIT", "Zeitcoin" },
  { "zencash", "ZEN", "Zencash" },
  { "zero", "ZER", "ZERO" },
  { "bitzeny", "ZNY", "bitZeny" },
  { "dollarpac", "$PAC", "Dollar PAC" },
  { "aureus", "AURS", "Aureus" },
  { "bbccoin", "BBC", "BBCCOIN" },
  { "bitcoininterest", "BCI", "Bitcoin Interest" },
  { "bitcoinfor", "BFC", "BitcoinFor" },
  { "businessnetworkincubator", "BNI", "Business Network Incubator" },
  { "bitcoinprivate", "BTCP", "Bitcoin Private" },
  { "bottlecaps", "CAPo", "Bottlecaps" },
  { "crycoin", "CRY", "Crycoin" },
  { "dekadocoin", "DKD", "Dekadocoin" },
  { "diamondcoin", "DMCC", "Diamondcoin" },
  { "eboost", "EBST", "eBoost" },
  { "forexcoin", "FRX", "ForexCoin" },
  { "internetofpeople", "IOP", "InternetOfPeople" },
  { "shekel", "JEW", "Shekel" },
  { "b3coin", "KB3", "B3Coin" },
  { "litecoincash", "LCC", "Litecoin Cash" },
  { "opcoin", "OPC", "OP Coin" },
  { "procurrency", "PROC", "ProCurrency" },
  { "speedcash", "SCS", "Speedcash" },
  { "smileycoin", "SMLY", "Smileycoin" },
  { "stake", "STAKE", "Stake" },
  { "shopzcoin", "SZC", "Shopzcoin" },
  { "ititaniumcoin", "TIC", "Ititaniumcoin" },
  { "toacoin", "TOA", "TOA Coin" },
  { "turtlecoin", "TRTL", "TurtleCoin" },
  { "uscoin", "USDC", "USCOIN" },
  { "unifiedsociety", "USX", "Unified Society" },
  { "volcoin", "VOL", "VolCoin" },
  { "yenten", "YTN", "Yenten" },
  { "gmccoin", "GMC", "Gmccoin" },
  { "emaratcoin", "AEC", "EmaratCoin" },
  { "stationcoin", "STS", "Stationcoin" },
  { "ditcoin", "DIT", "DitCoin" },
  { "juliancoin", "JLN", "Julian Coin" },
  { "neblio", "NEBL", "Neblio" },
  { "quasarcoin", "QAC", "Quasarcoin" },
  { "smartcash", "SMART", "SmartCash" },
  { "monero", "XMR", "Monero" },
  { "clubcoin", "CLUB", "ClubCoin" },
  { "npcoin", "NPC", "Npcoin" },
  { "qredit", "QRT", "Qredit" },
  { "sabrcoin", "SABR", "SABR Coin" },
  { "worldwidetradecoin", "WWTC", "World Wide Trade Coin" },
  { "atcc", "ATCC", "ATCC" },
  { "bitcoinsupreme", "BTCSP", "Bitcoin Supreme" },
  { "cheesecoin", "CHEESE", "Cheese Coin" },
  { "flapx", "FLAPX", "FLAPX" },
  { "greenshilling", "GSL", "GreenShilling" },
  { "motacoin", "MOTA", "MotaCoin" },
  { "rupayacoin", "RUPX", "RupayaCoin" },
  { "wincoin", "WC", "WinCoin" },
  { "nimiq", "NIM", "Nimiq" },
  { "audiocoin", "ADC", "Audiocoin" },
  { "aristocoin", "ARISTO", "Aristocoin" },
  { "interstellarholdings", "HOLD", "Interstellar Holdings" },
  { "smartfva", "SVA", "Smartfva" },
  { "bitcoinincognito", "XBI", "Bitcoin Incognito" },
  { "nodium", "XN", "Nodium" },
  { "hexx", "HXX", "Hexx" },
  { "peepcoin", "PCN", "PeepCoin" },
  { "zelcash", "ZEL", "Zelcash" },
  { "ethereum", "ETH", "Ethereum" },
  { "electronero", "ETNX", "Electronero" },
  { "mirai", "MRI", "Mirai" },
  { "tron", "TRX", "Tron" },
  { "bfxcoin", "BFX", "BFX COIN" },
  { "eurescoin", "EURES", "Eurescoin" },
  { "ivntoken", "IVN", "IVN Token" },
  { "kodcoin", "KOD", "Kodcoin" },
  { "sat3coin", "SAT3", "SAT3 coin" },
  { "binancecoin", "BNB", "Binance Coin" },
  { "bezanttoken", "BZNT", "BezantToken" },
  { "future1coin", "F1C", "Future1coin" },
  { "gold", "GOLD", "Gold" },
  { "chainlink", "LINK", "ChainLink" },
  { "riocoin", "RIOC", "Rio Coin" },
  { "smartmesh", "SMT", "SmartMesh" },
  { "tokenofhope", "TOH", "Token of Hope" },
  { "veroneum", "VRN", "Veroneum" },
  { "zminetoken", "ZMN", "ZMINE Token" },
  { "atccoinpluscoin", "ATCP", "ATC Coin Plus Coin" },
  { "bitcoinimprove", "BCIM", "Bitcoin Improve" },
  { "bitcoinsleek", "BTSL", "Bitcoin Sleek" },
  { "coinlegacy", "CLY", "CoinLegacY" },
  { "italianlira", "ITL", "Italian Lira" },
  { "lumaxcoin", "LMX", "Lumax Coin" },
  { "oen", "OEN", "OEN" },
  { "stronghandsmasternode", "SHMN", "StrongHandsMasterNode" },
  { "wabnetwork", "WAB", "WABnetwork" },

};


vector <string> tradesatoshi_get_coin_ids ()
{
  vector <string> ids;
  unsigned int data_count = sizeof (tradesatoshi_coins_table) / sizeof (*tradesatoshi_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    ids.push_back (tradesatoshi_coins_table[i].identifier);
  }
  return ids;
}


string tradesatoshi_get_coin_id (string coin)
{
  unsigned int data_count = sizeof (tradesatoshi_coins_table) / sizeof (*tradesatoshi_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == tradesatoshi_coins_table[i].identifier) return tradesatoshi_coins_table[i].identifier;
    if (coin == tradesatoshi_coins_table[i].abbreviation) return tradesatoshi_coins_table[i].identifier;
    if (coin == tradesatoshi_coins_table[i].name) return tradesatoshi_coins_table[i].identifier;
  }
  return "";
}


string tradesatoshi_get_coin_abbrev (string coin)
{
  unsigned int data_count = sizeof (tradesatoshi_coins_table) / sizeof (*tradesatoshi_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == tradesatoshi_coins_table[i].identifier) return tradesatoshi_coins_table[i].abbreviation;
    if (coin == tradesatoshi_coins_table[i].abbreviation) return tradesatoshi_coins_table[i].abbreviation;
    if (coin == tradesatoshi_coins_table[i].name) return tradesatoshi_coins_table[i].abbreviation;
  }
  return "";
}


string tradesatoshi_get_coin_name (string coin)
{
  unsigned int data_count = sizeof (tradesatoshi_coins_table) / sizeof (*tradesatoshi_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == tradesatoshi_coins_table[i].identifier) return tradesatoshi_coins_table[i].name;
    if (coin == tradesatoshi_coins_table[i].abbreviation) return tradesatoshi_coins_table[i].name;
    if (coin == tradesatoshi_coins_table[i].name) return tradesatoshi_coins_table[i].name;
  }
  return "";
}


// https://tradesatoshi.com/Home/Api
string get_tradesatoshi_public_api_url (const vector <string> & endpoints,
                                        const vector <pair <string, string> > & parameters)
{
  // The url of the API website.
  string url ("https://tradesatoshi.com/api");
  for (auto part : endpoints) {
    url.append ("/");
    url.append (part);
  }
  // Add the query parameters, if any.
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


// Kind of ensure always increasing nonces to be received by the TradeSatoshi API.
// The sequencer is used for properly spacing the subsequent private API calls.
atomic <long> tradesatoshi_private_api_call_sequencer (0);


// https://tradesatoshi.com/support/api/
// Sample API wrapper: https://github.com/WorldBot/Trade-Satoshi-API
string get_tradesatoshi_private_api_call_result (const vector <string> & endpoints,
                                                 const vector <pair <string, string> > & parameters,
                                                 string & error)
{
  // Ensure there's enough time between this private API call and the most recent private API call.
  api_call_sequencer (tradesatoshi_private_api_call_sequencer);
  
  // The url to POST to.
  string url = get_tradesatoshi_public_api_url (endpoints, {});
  
  // Nonce: Strictly increasing.
  // The PHP sample given by TradeSatoshi used a nonce like this:
  // 1521130318451447
  // The length is 16 bytes.
  string nonce = increasing_nonce ();
  nonce.erase (16);

  // The payload to POST.
  Object post_payload;
  for (auto parameter : parameters) {
    post_payload << parameter.first << parameter.second;
  }
  string post_data = post_payload.json ();

  // The raw signature.
  // PHP sample:
  // $SIGNATURE = $API_PUBLIC_KEY.'POST'.strtolower(urlencode($URL)).$NONCE.base64_encode($REQ);
  string signature = string (api_key) + "POST" + str2lower (url_encode (url)) + nonce + base64_encode (post_data);
  
  // The HMAC signature.
  // PHP sample:
  // $HMAC_SIGN = base64_encode(hash_hmac('sha512',$SIGNATURE,base64_decode($API_SECRET_KEY),true));
  string hmac_sign = base64_encode (hmac_sha512_raw (base64_decode (api_secret), signature));
  
  // The authorization header.
  // PHP sample:
  // $HEADER = 'Basic '.$API_PUBLIC_KEY.':'.$HMAC_SIGN.':'.$NONCE;
  string header = "Basic " + string (api_key) + ":" + hmac_sign + ":" + nonce;
  
  // The security headers sent off.
  vector <pair <string, string> > headers = {
    make_pair ("Content-Type", "application/json; charset=utf-8"),
    make_pair ("Authorization", header)
  };
  
  // Call API.
  // Do not call it via a proxy,
  // rather use a fixed IP address for accessing the trading API at TradeSatoshi.
  // If a proxy is configured incorrectly,
  // other people could access the TradeSatoshi trading API from that proxy.
  string json = http_call (url, error, "", false, true, post_data, headers, false, true, "");
  
  // Done.
  return json;
}


string tradesatoshi_get_exchange ()
{
  return "tradesatoshi";
}


void tradesatoshi_get_coins (const string & market,
                             vector <string> & coin_abbrevs,
                             vector <string> & coin_ids,
                             vector <string> & names,
                             vector <string> & statuses,
                             string & error)
{
  // API does not use the market.
  (void) market;
  
  // Call the API to get the exchange's currencies.
  string url = get_tradesatoshi_public_api_url ({"public", "getcurrencies"}, { });
  string json = http_call (url, error, "", false, false, "", {}, false, false, "");
  if (!error.empty ()) {
    return;
  }

  // {"success":true,"message":null,"result":[{{"currency":"BBK","currencyLong":"BitBlocks","minConfirmation":25,"txFee":0.10000000,"status":"OK"},{"currency":"BCC","currencyLong":"Bitconnect","minConfirmation":18,"txFee":0.00010000,"status":"OK"},.......,{"currency":"ZYD","currencyLong":"ZayedCoin ","minConfirmation":6,"txFee":0.00010000,"status":"OK"}]}
  
  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          if (!values.has<String>("currency")) continue;
          if (!values.has<String>("currencyLong")) continue;
          if (!values.has<String>("status")) continue;
          string currency = values.get<String>("currency");
          string currencyLong = values.get<String>("currencyLong");
          string status = values.get<String>("status");
          coin_abbrevs.push_back (currency);
          string coin_id = tradesatoshi_get_coin_id (currency);
          coin_ids.push_back (coin_id);
          names.push_back (currencyLong);
          statuses.push_back (status);
        }
        return;
      }
    }
  }

  // Error handler.
  error.append (json);
}


void tradesatoshi_get_market_summaries (string market,
                                        vector <string> & coins,
                                        vector <float> & bids,
                                        vector <float> & asks,
                                        string & error)
{
  // Call the API to get the exchange's tickers.
  // The tickers also provide the markets-coin pairs.
  string url = get_tradesatoshi_public_api_url ({"public", "getmarketsummaries" }, { });
  string json = http_call (url, error, "", false, false, "", {}, false, false, satellite_get_interface (url));
  if (!error.empty ()) {
    return;
  }

  // {"success":true,"message":null,"result":[{"market":"PAK_BTC","high":0.00000000,"low":0.00000000,"volume":0.00000000,"baseVolume":0.00000000,"last":0.00000278,"bid":0.00000130,"ask":0.00000278,"openBuyOrders":45,"openSellOrders":30,"change":0.0},{"market":"BOLI_BTC","high":0.00000749,"low":0.00000600,"volume":14026.04902513,"baseVolume":0.08421745,"last":0.00000600,"bid":0.00000600,"ask":0.00000650,"openBuyOrders":34,"openSellOrders":111,"change":0.0},{"market":"DOGE_BTC","high":0.00000058,"low":0.00000050,"volume":7400155.17196666,"baseVolume":3.98662339,"last":0.00000056,"bid":0.00000056,"ask":0.00000057,"openBuyOrders":158,"openSellOrders":364,"change":0.0},...,{"market":"GFISH_BCH","high":0.00000000,"low":0.00000000,"volume":0.00000000,"baseVolume":0.00000000,"last":0.00000000,"bid":0.0,"ask":0.0,"openBuyOrders":0,"openSellOrders":0,"change":0.0}]}

  
  // The abbreviation of the market, e.g.: "_BTC".
  string market_abbrev = "_" + tradesatoshi_get_coin_abbrev (market);
  
  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          if (!values.has<String>("market")) continue;
          if (!values.has<Number>("bid")) continue;
          if (!values.has<Number>("ask")) continue;
          string market = values.get<String>("market");
          float bid = values.get<Number>("bid");
          float ask = values.get<Number>("ask");
          size_t pos = market.find (market_abbrev);
          if (pos != string::npos) {
            string coin_abbrev = market.substr (0, pos);
            string coin_id = tradesatoshi_get_coin_id (coin_abbrev);
            if (coin_id.empty ()) continue;
            coins.push_back (coin_id);
            bids.push_back (bid);
            asks.push_back (ask);
          }
        }
        return;
      }
    }
  }
  
  // Error handler.
  error.append (json);
}


void tradesatoshi_return_order_book (string market,
                                     string coin,
                                     string direction,
                                     vector <float> & quantities,
                                     vector <float> & rates,
                                     string & error,
                                     bool hurry)
{
  // Get the currency pair, e.g. "LTC_BTC".
  string coin_abbrev = tradesatoshi_get_coin_abbrev (coin);
  string market_abbrev = tradesatoshi_get_coin_abbrev (market);
  string coin_market_pair = coin_abbrev + "_" + market_abbrev;
  
  // Call the API to get the exchange's order book for the currency at the requested base market.
  // https://tradesatoshi.com/api/public/getorderbook?market=LTC_BTC&type=both&depth=20
  string url = get_tradesatoshi_public_api_url ({"public", "getorderbook"}, { make_pair ("market", coin_market_pair), make_pair ("type", direction) });
  string json = http_call (url, error, "", false, false, "", {}, hurry, false, "");
  if (!error.empty ()) {
    if (hurry) tradesatoshi_hurried_call_fail++;
    return;
  }
  
  // {"success":true,"message":null,"result":{"buy":[],"sell":[{"quantity":0.43000000,"rate":0.01300000},{"quantity":0.63935675,"rate":0.01429998},{"quantity":0.11362242,"rate":0.01429999},{"quantity":1.33810531,"rate":0.01430000},{"quantity":0.25000000,"rate":0.01437999},{"quantity":5.71353728,"rate":0.01438999},{"quantity":0.50000000,"rate":0.01456999},{"quantity":0.30000000,"rate":0.01457773},{"quantity":0.30124641,"rate":0.01457774},{"quantity":0.70000000,"rate":0.01457794},{"quantity":0.53465068,"rate":0.01457796},{"quantity":0.15212380,"rate":0.01477799},{"quantity":0.15212380,"rate":0.01477899},{"quantity":0.99799999,"rate":0.01478000},{"quantity":0.25266325,"rate":0.01478800},{"quantity":0.87606980,"rate":0.01478880},{"quantity":1.84573660,"rate":0.01478889},{"quantity":0.32462321,"rate":0.01479970},{"quantity":0.10127245,"rate":0.01484994},{"quantity":0.01839732,"rate":0.01487000}]}}
  
  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Object>("result")) {
        Object result = main.get<Object>("result");
        if (result.has<Array>(direction)) {
          Array orders = result.get<Array>(direction);
          for (size_t i = 0; i < orders.size(); i++) {
            Object values = orders.get <Object> (i);
            if (!values.has<Number>("quantity")) continue;
            if (!values.has<Number>("rate")) continue;
            float quantity = values.get<Number>("quantity");
            float rate = values.get<Number>("rate");
            quantities.push_back (quantity);
            rates.push_back (rate);
          }
          if (hurry) tradesatoshi_hurried_call_pass++;
          return;
        }
      }
    }
  }
  
  // Error handler.
  if (hurry) tradesatoshi_hurried_call_fail++;
  error.append (json);
}


void tradesatoshi_get_buyers (string market,
                              string coin,
                              vector <float> & quantities,
                              vector <float> & rates,
                              string & error,
                              bool hurry)
{
  tradesatoshi_return_order_book (market, coin, "buy", quantities, rates, error, hurry);
}


void tradesatoshi_get_sellers (string market,
                               string coin,
                               vector <float> & quantities,
                               vector <float> & rates,
                               string & error,
                               bool hurry)
{
  tradesatoshi_return_order_book (market, coin, "sell", quantities, rates, error, hurry);
}


// As of August 2018 the API call related to the wallet only is able to generate a new address.
// It cannot yet get the existing wallet address.
void tradesatoshi_get_wallet_details (string coin, string & json, string & error,
                                      string & address, string & payment_id)
{
  // Get the currency pair, e.g. "LTC".
  string coin_abbrev = tradesatoshi_get_coin_abbrev (coin);

  // The parameter(s) to POST.
  vector <pair <string, string> > parameters = {
    make_pair ("Currency", coin_abbrev)
  };

  // Call the private API.
  json = get_tradesatoshi_private_api_call_result ( {"private", "generateaddress",}, parameters, error);
  if (!error.empty ()) {
    return;
  }
  
  // The result looks like this:
  // {"success":true,"message":null,"result":{"currency":"ETH","address":"0xd74f4459e24f6f29ad442ab28f43440a3ee12a87","paymentId":""}}

  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Object>("result")) {
        Object result = main.get<Object>("result");
        if (result.has<String>("address")) {
          address = result.get<String>("address");
          if (result.has<String>("paymentId")) {
            payment_id = result.get<String>("paymentId");
          }
          return;
        }
      }
    }
  }

  // Error handler.
  error.append (json);
}


void tradesatoshi_get_balances (vector <string> & coin_abbrev,
                                vector <string> & coin_ids,
                                vector <float> & total,
                                vector <float> & reserved,
                                vector <float> & unconfirmed,
                                string & error)
{
  // The parameters to post.
  vector <pair <string, string> > parameters = {
    //make_pair ("Currency", "BTC")
  };
  
  // Call the private API.
  string json = get_tradesatoshi_private_api_call_result ( {"private", "getbalances",}, parameters, error);
  if (!error.empty ()) {
    return;
  }
  
  // Sample JSON:
  // {"success":true,"message":null,"result":[{"currency":"DTC","currencyLong":" DivotyCoin","available":0.0,"total":0.0,"heldForTrades":0.0,"unconfirmed":0.0,"pendingWithdraw":0.0,"address":null},...,{"currency":"ZBC","currencyLong":"Zilbercoin","available":0.0,"total":0.0,"heldForTrades":0.0,"unconfirmed":0.0,"pendingWithdraw":0.0,"address":null}]}

  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          // Get coin abbreviation.
          if (!values.has<String>("currency")) continue;
          string coin_abbreviation = values.get<String>("currency");
          // Get total balances.
          if (!values.has<Number>("total")) continue;
          float total_balance = values.get<Number>("total");
          // Skip zero balances.
          if (total_balance <= 0) continue;
          // Get balances somehow tied up.
          if (!values.has<Number>("heldForTrades")) continue;
          float heldForTrades = values.get<Number>("heldForTrades");
          if (!values.has<Number>("unconfirmed")) continue;
          float unconfirmed_balance = values.get<Number>("unconfirmed");
          if (!values.has<Number>("pendingWithdraw")) continue;
          float pendingWithdraw = values.get<Number>("pendingWithdraw");
          // Store all values.
          coin_abbrev.push_back (coin_abbreviation);
          string coin_id = tradesatoshi_get_coin_id (coin_abbreviation);
          coin_ids.push_back (coin_id);
          total.push_back (total_balance);
          reserved.push_back (heldForTrades + pendingWithdraw);
          unconfirmed.push_back (unconfirmed_balance);
        }
        return;
      }
    }
  }
  error.append (json);
}


mutex tradesatoshi_balance_mutex;
map <string, float> tradesatoshi_balance_total;
map <string, float> tradesatoshi_balance_available;
map <string, float> tradesatoshi_balance_reserved;
map <string, float> tradesatoshi_balance_unconfirmed;


void tradesatoshi_get_balance (const string & coin, float * total, float * available, float * reserved, float * unconfirmed)
{
  tradesatoshi_balance_mutex.lock ();
  if (total) * total = tradesatoshi_balance_total [coin];
  if (available) * available = tradesatoshi_balance_available [coin];
  if (reserved) * reserved = tradesatoshi_balance_reserved [coin];
  if (unconfirmed) * unconfirmed = tradesatoshi_balance_unconfirmed [coin];
  tradesatoshi_balance_mutex.unlock ();
}


void tradesatoshi_set_balance (const string & coin, float total, float available, float reserved, float unconfirmed)
{
  tradesatoshi_balance_mutex.lock ();
  if (total != 0) tradesatoshi_balance_total [coin] = total;
  if (available != 0) tradesatoshi_balance_available [coin] = available;
  if (reserved != 0) tradesatoshi_balance_reserved [coin] = reserved;
  if (unconfirmed != 0) tradesatoshi_balance_unconfirmed [coin] = unconfirmed;
  tradesatoshi_balance_mutex.unlock ();
}


vector <string> tradesatoshi_get_coins_with_balances ()
{
  vector <string> coins;
  tradesatoshi_balance_mutex.lock ();
  for (auto & element : tradesatoshi_balance_total) {
    coins.push_back (element.first);
  }
  tradesatoshi_balance_mutex.unlock ();
  return coins;
}


string tradesatoshi_withdrawal_order (const string & coin,
                                      float quantity,
                                      const string & address,
                                      const string & paymentid,
                                      string & json,
                                      string & error)
{
  string coin_abbrev = tradesatoshi_get_coin_abbrev (coin);
  vector <pair <string, string> > parameters;
  parameters.push_back (make_pair ("Currency", coin_abbrev));
  parameters.push_back (make_pair ("Address", address));
  parameters.push_back (make_pair ("Amount", float2string (quantity)));
  if (!paymentid.empty ()) {
    // This was not tested, and likely won't work.
    parameters.push_back (make_pair ("PaymentId", paymentid));
  }

  json = get_tradesatoshi_private_api_call_result ({"private", "submitwithdraw"}, parameters, error);
  if (!error.empty ()) {
    return "";
  }
  
  // Sample JSON:
  // {"success":true,"message":null,"result":{"withdrawalId":416181}}
  
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Object>("result")) {
        Object result = main.get<Object>("result");
        if (result.has<Number>("withdrawalId")) {
          int withdrawalId = result.get<Number>("withdrawalId");
          return to_string (withdrawalId);
        }
      }
    }
  }

  error.append (json);
  return "";
}


void tradesatoshi_get_open_orders (vector <string> & identifiers,
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
  parameters.push_back (make_pair ("Count", "100"));
  string json = get_tradesatoshi_private_api_call_result ({ "private", "getorders" }, parameters, error);
  if (!error.empty ()) {
    return;
  }
  
  // Sample JSON:
  // {"success":true,"message":null,"result":[{"id":12085865,"market":"DOGE_BTC","type":"Sell","amount":200.00000000,"rate":0.00000044,"remaining":200.00000000,"total":0.00008800,"status":"Pending","timestamp":"2018-03-15T19:19:00.147","isApi":true},{"id":12085837,"market":"DOGE_BTC","type":"Sell","amount":200.00000000,"rate":0.00000044,"remaining":200.00000000,"total":0.00008800,"status":"Pending","timestamp":"2018-03-15T19:18:34.433","isApi":true},{"id":12085686,"market":"DOGE_BTC","type":"Sell","amount":200.00000000,"rate":0.00000044,"remaining":200.00000000,"total":0.00008800,"status":"Pending","timestamp":"2018-03-15T19:15:43.323","isApi":true},{"id":12084108,"market":"DOGE_BTC","type":"Buy","amount":200.00000000,"rate":0.00000022,"remaining":200.00000000,"total":0.00004400,"status":"Pending","timestamp":"2018-03-15T18:43:49.29","isApi":true},{"id":12083861,"market":"DOGE_BTC","type":"Buy","amount":200.00000000,"rate":0.00000022,"remaining":200.00000000,"total":0.00004400,"status":"Pending","timestamp":"2018-03-15T18:38:32.253","isApi":true},{"id":12083832,"market":"DOGE_BTC","type":"Buy","amount":200.00000000,"rate":0.00000022,"remaining":200.00000000,"total":0.00004400,"status":"Pending","timestamp":"2018-03-15T18:37:51.657","isApi":true}]}
  
  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          if (!values.has<Number>("id")) continue;
          int id = values.get<Number> ("id");
          if (!values.has<String>("market")) continue;
          string coin_market_pair = values.get<String> ("market");
          if (!values.has<String>("type")) continue;
          string type = values.get<String> ("type");
          if (!values.has<Number>("amount")) continue;
          float amount = values.get<Number> ("amount");
          if (!values.has<Number>("rate")) continue;
          float rate = values.get<Number> ("rate");
          if (!values.has<String>("timestamp")) continue;
          string timestamp = values.get<String> ("timestamp");
          vector <string> coin_market = explode (coin_market_pair, '_');
          if (coin_market.size () != 2) continue;
          identifiers.push_back (to_string (id));
          markets.push_back (tradesatoshi_get_coin_id (coin_market[1]));
          coin_abbreviations.push_back (coin_market[0]);
          coin_ids.push_back (tradesatoshi_get_coin_id (coin_market[0]));
          buy_sells.push_back (type);
          amounts.push_back (amount);
          rates.push_back (rate);
          dates.push_back (timestamp);
        }
        return;
      }
    }
  }
  error.append (json);
}


bool tradesatoshi_cancel_order (const string & order_id,
                                string & error)
{
  vector <pair <string, string> > parameters = {
    make_pair ("Type", "Single"),
    make_pair ("OrderId", order_id)
  };
  string json = get_tradesatoshi_private_api_call_result ({ "private", "cancelorder" }, parameters, error);
  if (!error.empty ()) {
    return false;
  }
  // Sample JSON:
  // {"success":true,"message":null,"result":{"canceledOrders":[12085865]}}
  // Or:
  // {"success":true,"message":null,"result":{"canceledOrders":[]}}
  Object main;
  main.parse (json);
  if (main.get<Boolean>("success")) {
    if (main.has<Object>("result")) {
      Object result = main.get<Object>("result");
      if (result.has<Array>("canceledOrders")) {
        Array canceled = result.get<Array>("canceledOrders");
        if (canceled.size() == 1) {
          return true;
        }
      }
    }
  }
  error.append (json);
  return false;
}


string tradesatoshi_limit_trade (const string & type,
                                 const string & market,
                                 const string & coin,
                                 float quantity,
                                 float rate,
                                 string & json,
                                 string & error)
{
  string coin_abbrev = tradesatoshi_get_coin_abbrev (coin);
  string market_abbrev = tradesatoshi_get_coin_abbrev (market);
  string coin_market = coin_abbrev + "_" + market_abbrev;
  vector <pair <string, string> > parameters;
  parameters.push_back (make_pair ("Market", coin_market));
  parameters.push_back (make_pair ("Type", type));
  parameters.push_back (make_pair ("Amount", float2string (quantity)));
  parameters.push_back (make_pair ("Price", float2string (rate)));
  json = get_tradesatoshi_private_api_call_result ({ "private", "submitorder" }, parameters, error);
  if (!error.empty ()) {
    return "";
  }

  // Sample JSON:
  // {"success":true,"message":null,"result":{"orderId":12083861,"filled":[]}}
  // Or:
  // {"success":true,"message":null,"result":{"orderId":null,"filled":[4187553]}}

  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Object>("result")) {
        Object result = main.get<Object>("result");
        if (result.has<Number>("orderId")) {
          int orderId = result.get<Number>("orderId");
          return to_string (orderId);
        }
        if (result.has<Array>("filled")) {
          Array FilledOrders = result.get<Array>("filled");
          return trade_order_fulfilled ();
        }
      }
    }
  }
  error.append (json);
  return "";
}


string tradesatoshi_limit_buy (const string & market,
                               const string & coin,
                               float quantity,
                               float rate,
                               string & json,
                               string & error)
{
  return tradesatoshi_limit_trade ("Buy", market, coin, quantity, rate, json, error);
}


string tradesatoshi_limit_sell (const string & market,
                                const string & coin,
                                float quantity,
                                float rate,
                                string & json,
                                string & error)
{
  return tradesatoshi_limit_trade ("Sell", market, coin, quantity, rate, json, error);
}


void tradesatoshi_withdrawalhistory (const string & coin,
                                     vector <string> & order_ids,
                                     vector <string> & coin_abbreviations,
                                     vector <string> & coin_ids,
                                     vector <float> & quantities,
                                     vector <string> & addresses,
                                     vector <string> & transaction_ids,
                                     string & error)
{
  string coin_abbrev = tradesatoshi_get_coin_abbrev (coin);
  vector <pair <string, string> > parameters;
  parameters.push_back (make_pair ("Currency", coin_abbrev));
  
  string json = get_tradesatoshi_private_api_call_result ({"private", "getwithdrawals"}, parameters, error);
  if (!error.empty ()) {
    return;
  }
  
  // Sample JSON:
  // {"success":true,"message":null,"result":[{"id":389478,"currency":"BTC","currencyLong":"Bitcoin","amount":0.21298074,"fee":0.00200000,"address":"16wRKm75Hd9uy1VYx76UA6rogPWwzg7trK","status":"Complete","txid":"96afda8eb23ddad4196af11592c9f3eb5bbd62a998aef424c3cdedaeb0121c8f","confirmations":8,"timeStamp":"2018-07-19T16:14:20.737","isApi":false},{"id":317537,"currency":"BTC","currencyLong":"Bitcoin","amount":0.25000000,"fee":0.00200000,"address":"1LgQxFtxdLYu7t1JS4tt4BhbX63CV6G55T","status":"Complete","txid":"5d8fe62c370121fd8431e74101b997e46b112b8596a8602008ee8d58a113944a","confirmations":6,"timeStamp":"2018-05-01T15:01:36.77","isApi":false}]}
  
  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          if (!values.has<Number>("id")) continue;
          int id = values.get<Number> ("id");
          if (!values.has<String>("currency")) continue;
          string currency = values.get<String> ("currency");
          if (!values.has<Number>("amount")) continue;
          float amount = values.get<Number> ("amount");
          if (!values.has<String>("address")) continue;
          string address = values.get<String> ("address");
          if (!values.has<String>("txid")) continue;
          string txid = values.get<String> ("txid");
          order_ids.push_back (to_string (id));
          coin_abbreviations.push_back (currency);
          coin_ids.push_back (tradesatoshi_get_coin_id (currency));
          quantities.push_back (amount);
          addresses.push_back (address);
          transaction_ids.push_back (txid);
        }
        return;
      }
    }
  }
  error.append (json);
}


void tradesatoshi_deposithistory (const string & coin,
                                  vector <string> & order_ids,
                                  vector <string> & coin_abbreviations,
                                  vector <string> & coin_ids,
                                  vector <float> & quantities,
                                  vector <string> & addresses,
                                  vector <string> & transaction_ids,
                                  string & error)
{
  string coin_abbrev = tradesatoshi_get_coin_abbrev (coin);
  vector <pair <string, string> > parameters;
  parameters.push_back (make_pair ("Currency", coin_abbrev));
  
  string json = get_tradesatoshi_private_api_call_result ({"private", "getdeposits"}, parameters, error);
  if (!error.empty ()) {
    return;
  }
  
  // Sample JSON:
  // {"success":true,"message":null,"result":[{"id":6772238,"currency":"ETH","currencyLong":"Ethereum","amount":0.75925900,"status":"Confirmed","txid":"0xc4d1aa7b298e27135cb1eef63681b9140a86c0c126b8323691b894d7afdf2b48","confirmations":100,"timeStamp":"2018-08-24T05:31:59"},{"id":6760051,"currency":"ETH","currencyLong":"Ethereum","amount":0.73287605,"status":"Confirmed","txid":"0x96441615c86435d17aa0ec205fe4ddd28afe429e9a97a8208538f950f60e4a99","confirmations":100,"timeStamp":"2018-08-21T19:03:19"},{"id":6749454,"currency":"ETH","currencyLong":"Ethereum","amount":0.76423077,"status":"Confirmed","txid":"0x594104c6134c7c409abb9d6e2b4c291e83cd9abd28206a90a2fcde7796ecb4c1","confirmations":100,"timeStamp":"2018-08-20T13:35:15"}]}
  
  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has<Boolean>("success")) {
    if (main.get<Boolean>("success")) {
      if (main.has<Array>("result")) {
        Array result = main.get<Array>("result");
        for (size_t i = 0; i < result.size(); i++) {
          Object values = result.get <Object> (i);
          if (!values.has<Number>("id")) continue;
          int id = values.get<Number> ("id");
          if (!values.has<String>("currency")) continue;
          string currency = values.get<String> ("currency");
          if (!values.has<Number>("amount")) continue;
          float amount = values.get<Number> ("amount");
          string address = "tradesatoshi";
          if (!values.has<String>("txid")) continue;
          string txid = values.get<String> ("txid");
          order_ids.push_back (to_string (id));
          coin_abbreviations.push_back (currency);
          coin_ids.push_back (tradesatoshi_get_coin_id (currency));
          quantities.push_back (amount);
          addresses.push_back (address);
          transaction_ids.push_back (txid);
        }
        return;
      }
    }
  }
  error.append (json);
}


int tradesatoshi_get_hurried_call_passes ()
{
  return tradesatoshi_hurried_call_pass;
}


int tradesatoshi_get_hurried_call_fails ()
{
  return tradesatoshi_hurried_call_fail;
}
