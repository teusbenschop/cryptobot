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


#include "yobit.h"
#include "proxy.h"


#define api_key "--enter--your--value--here--"
#define api_secret "--enter--your--value--here--"


// Hurried call counters.
atomic <int> yobit_hurried_call_pass (0);
atomic <int> yobit_hurried_call_fail (0);


// See here: https://yobit.net/en/api/
string yobit_get_public_api_call (vector <string> methods)
{
  string url ("https://yobit.net/api/3");
  for (auto method : methods) url.append ("/" + method);
  return url;
}


// Return an increasing nonce for the Yobit trading API.
string yobit_increasing_nonce ()
{
  // These are Yobit's specifications for the nonce:
  // * Parameter nonce (1 minimum to 2147483646 maximum).
  // * The nonce in a subsequent request should exceed that in the previous request.
  // At cryptobot startup, set the nonce to the current seconds since the Unix epoch.
  // Each subsequent trade API request, increases this by one.
  // Since there's only relatively few private calls, this should work till the next minute.
  // This way there's enough nonces for about 60 private calls per minute.
  // That should do.
  static int nonce = seconds_since_epoch ();
  nonce++;
  return to_string (nonce);
  // The current seconds since the Epoch at the time of writing this is "1514134259".
  // So there's no space to add microseconds to the seconcs since Epoch.
  // Else it would become too long, it would exceed the maximum specified by Yobit.
  // The way it is now, manual authenticated commands, like ./orders,
  // always fail when Yobit has been called during that minute.
  // Improved nonce generation should fix this.
  // But how to do that, this is the question.
}


// If API calls are sent to the network with correct increasing nonces,
// the calls may arrive out of order at the Yobit exchange, due to network latency.
// This would lead to nonces no longer to be increasing, but out of order.
// This mutex should prevent that more or less.
// When the mutex appears as too rigid as related to timing,
// another way of ensuring increasing nonces is to use no longer mutexes but to use timers.
// There's a timer delay of about a second,
// that ensures pending requests are sent off in the correct sequence.
// This system no longer waits for the response to come in.
// It just ensures the requests go out in a timely fashion and in an orderly fashion.
// The new method is timed sequencing of these private API calls.
atomic <long> yobit_private_api_call_sequencer (0);


// See here: https://yobit.net/en/api/
string yobit_get_trading_api_call (vector <pair <string, string> > parameters, string & error)
{
  /* Working PHP example for trading API call.
   
   <?php
   
   $api_key    = 'your-key';
   $api_secret = 'your-secret';
   
   $req = array();
   $req['method'] = 'GetDepositAddress';
   $req['coinName'] = 'BTC';
   $req['nonce'] = time();
   $post_data = http_build_query($req, '', '&');
   $sign = hash_hmac("sha512", $post_data, $api_secret);
   $headers = array(
   'Sign: '.$sign,
   'Key: '.$api_key,
   );
   
   $ch = null;
   $ch = curl_init();
   curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
   curl_setopt($ch, CURLOPT_USERAGENT, 'Mozilla/4.0 (compatible; SMART_API PHP client; '.php_uname('s').'; PHP/'.phpversion().')');
   curl_setopt($ch, CURLOPT_URL, 'https://yobit.net/tapi/');
   curl_setopt($ch, CURLOPT_POSTFIELDS, $post_data);
   curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
   curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
   curl_setopt($ch, CURLOPT_ENCODING , 'gzip');
   $res = curl_exec($ch);
   curl_close($ch);
   
   echo $res;
   
   ?>
   
   */

  api_call_sequencer (yobit_private_api_call_sequencer);
  
  string url ("https://yobit.net/tapi");
  
  string postdata;
  parameters.push_back (make_pair ("nonce", yobit_increasing_nonce ()));
  for (auto parameter : parameters) {
    if (!postdata.empty ()) postdata.append ("&");
    postdata.append (parameter.first);
    postdata.append ("=");
    postdata.append (parameter.second);
  }

  string sign = hmac_sha512_hexits (api_secret, postdata);

  vector <pair <string, string> > headers;
  headers.push_back (make_pair ("Sign", sign));
  headers.push_back (make_pair ("Key", api_key));

  string json = http_call (url, error, "", false, true, postdata, headers, false, true, "");

  return json;
}


string yobit_get_exchange ()
{
  return "yobit";
}


typedef struct
{
  const char * identifier;
  const char * abbreviation;
  const char * name;
}
yobit_coin_record;


yobit_coin_record yobit_coins_table [] =
{
  { "bitcoin", "BTC", "Bitcoin" },
  { "2give", "2GIVE", "2GIVE" },
  { "octocoin", "888", "OctoCoin" },
  { "8bit", "8BIT", "8Bit" },
  { "artbyte", "ABY", "ArtByte" },
  { "acoin", "ACOIN", "ACoin" },
  { "audiocoin", "ADC", "AudioCoin" },
  { "adtoken", "ADT", "adToken" },
  { "alexandrite", "ALEX", "Alexandrite" },
  { "alis", "ALIS", "ALIS" },
  { "allion", "ALL", "Allion" },
  { "animecoin", "ANI", "AnimeCoin" },
  { "aragon", "ANT", "Aragon" },
  { "aquariuscoin", "ARCO", "AquariusCoin" },
  { "arguscoin", "ARGUS", "ArgusCoin" },
  { "atomiccoin", "ATOM", "Atomiccoin" },
  { "auroracoin", "AUR", "AuroraCoin" },
  { "basicattentiontoken", "BAT", "Basic Attention Token" },
  { "beezercoin", "BEEZ", "BeezerCoin" },
  { "benjirolls", "BENJI", "BenjiRolls" },
  { "berncash", "BERN", "BERNcash" },
  { "bitbean", "BITB", "BitBean" },
  { "bitstar", "BITS", "Bitstar" },
  { "bancor", "BNT", "Bancor" },
  { "bolivarcoin", "BOLI", "BolivarCoin" },
  { "globalboost-y", "BSTY", "GlobalBoost-Y" },
  { "bata", "BTA", "BATA" },
  { "bitcoingold", "BTG", "Bitcoin Gold" },
  { "swagbucks", "BUCKS", "SwagBucks" },
  { "coin2", "C2", "Coin2" },
  { "cannabiscoin", "CANN", "CannabisCoin" },
  { "cryptobullion", "CBX", "CryptoBullion" },
  { "coolindarkcoin", "CC", "CoolInDarkCoin" },
  { "coffeecoin", "CFC", "CoffeeCoin" },
  { "chesscoin", "CHESS", "ChessCoin" },
  { "cryptojacks", "CJ", "CryptoJacks" },
  { "clams", "CLAM", "CLAMs" },
  { "cloakcoin", "CLOAK", "CloakCoin" },
  { "cometcoin", "CMT", "CometCoin" },
  { "paycon", "CON", "PayCon" },
  { "crave", "CRAVE", "Crave" },
  { "creamcoin", "CRM", "CREAMcoin" },
  { "crown", "CRW", "Crown" },
  { "chronoscoin", "CRX", "ChronosCoin" },
  { "coinonat", "CXT", "Coinonat" },
  { "dalecoin", "DALC", "DALECOIN" },
  { "dash", "DASH", "Dash" },
  { "decred", "DCR", "Decred" },
  { "decent", "DCT", "DECENT" },
  { "deutscheemark", "DEM", "Deutsche eMark" },
  { "digibyte", "DGB", "Digibyte" },
  { "dogecoin", "DOGE", "Dogecoin" },
  { "parallelcoin", "DUO", "Parallelcoin" },
  { "ecocoin", "ECO", "ECOcoin" },
  { "ecobit", "ECOB", "EcoBit" },
  { "educoinv", "EDC", "EducoinV" },
  { "edrcoin", "EDRC", "EDRcoin" },
  { "evergreencoin", "EGC", "EverGreenCoin" },
  { "elacoin", "ELC", "ElaCoin" },
  { "emercoin", "EMC", "EmerCoin" },
  { "equitrade", "EQT", "Equitrade" },
  { "ethereumclassic", "ETC", "Ethereum Classic" },
  { "ethereum", "ETH", "Ethereum" },
  { "eurocoin", "EUC", "Eurocoin" },
  { "evilcoin", "EVIL", "Evilcoin" },
  { "evotion", "EVO", "Evotion" },
  { "expanse", "EXP", "Expanse" },
  { "fujicoin", "FJC", "FujiCoin" },
  { "fonziecoin", "FONZ", "FonzieCoin" },
  { "francs", "FRN", "Francs" },
  { "fuzzballs", "FUZZ", "FuzzBalls" },
  { "gamecredits", "GAME", "GameCredits" },
  { "geocoin", "GEO", "GeoCoin" },
  { "gnosis", "GNO", "Gnosis" },
  { "golem", "GNT", "Golem" },
  { "goldpieces", "GP", "GoldPieces" },
  { "guppy", "GUP", "Guppy" },
  { "hackspace", "HAC", "Hackspace" },
  { "humaniq", "HMQ", "Humaniq" },
  { "hexxcoin", "HXX", "HexxCoin" },
  { "investfeed", "IFT", "investFeed" },
  { "incoin", "IN", "InCoin" },
  { "influxcoin", "INFX", "InfluxCoin" },
  { "iocoin", "IOC", "I/OCoin" },
  { "iticoin", "ITI", "ItiCoin" },
  { "ixcoin", "IXC", "IXCoin" },
  { "kybernetworkcrystal", "KNC", "KyberNetworkCrystal" },
  { "kobocoin", "KOBO", "KoboCoin" },
  { "kushcoin", "KUSH", "KushCoin" },
  { "lanacoin", "LANA", "LanaCoin" },
  { "litebitcoin", "LBTC", "LiteBitcoin" },
  { "lindacoin", "LINDA", "LindaCoin" },
  { "lisk", "LSK", "Lisk" },
  { "litecoin", "LTC", "Litecoin" },
  { "litecoinultra", "LTCU", "LitecoinUltra" },
  { "lunyr", "LUN", "Lunyr" },
  { "luxcoin", "LUX", "Luxcoin" },
  { "marxcoin", "MARX", "MarxCoin" },
  { "monaco", "MCO", "Monaco" },
  { "mobilego", "MGO", "MobileGo" },
  { "melite", "MLITE", "Melite" },
  { "mineum", "MNM", "Mineum" },
  { "moin", "MOIN", "MOIN" },
  { "mojocoin", "MOJO", "MojoCoin" },
  { "motocoin", "MOTO", "MotoCoin" },
  { "mustangcoin", "MST", "Mustangcoin" },
  { "monetaryunit", "MUE", "MonetaryUnit" },
  { "navcoin", "NAV", "NAVCoin" },
  { "netcoin", "NET", "NetCoin" },
  { "netko", "NETKO", "Netko" },
  { "nevacoin", "NEVA", "NevaCoin" },
  { "nolimitcoin", "NLC2", "NoLimitCoin" },
  { "gulden", "NLG", "Gulden" },
  { "namecoin", "NMC", "NameCoin" },
  { "numeraire", "NMR", "Numeraire" },
  { "neutron", "NTRN", "Neutron" },
  { "novacoin", "NVC", "NovaCoin" },
  { "okcash", "OK", "OkCash" },
  { "omisego", "OMG", "OmiseGO" },
  { "opalcoin", "OPAL", "Opalcoin" },
  { "opentradingnetwork", "OTN", "Open Trading Network" },
  { "pakcoin", "PAK", "PakCoin" },
  { "tenxpaytoken", "PAY", "TenX Pay Token" },
  { "phore", "PHR", "Phore" },
  { "pivx", "PIVX", "Pivx" },
  { "postcoin", "POST", "PostCoin" },
  { "peercoin", "PPC", "Peercoin" },
  { "procurrency", "PROC", "ProCurrency" },
  { "prime-xi", "PXI", "Prime-XI" },
  { "revain", "R", "Revain" },
  { "rimbit", "RBT", "Rimbit" },
  { "rubycoin", "RBY", "RubyCoin" },
  { "ripiocreditnetwork", "RCN", "Ripio Credit Network" },
  { "reddcoin", "RDD", "ReddCoin" },
  { "augur", "REP", "Augur" },
  { "ronpaulcoin", "RPC", "RonPaulCoin" },
  { "rupee", "RUP", "Rupee" },
  { "social", "SCL", "Social" },
  { "selencoin", "SEL", "Selencoin" },
  { "shrooms", "SHRM", "Shrooms" },
  { "siberianchervonets", "SIB", "Siberian Chervonets" },
  { "salus", "SLS", "SaluS" },
  { "smartcoin", "SMC", "SmartCoin" },
  { "songcoin", "SONG", "Songcoin" },
  { "spacecoin", "SPACE", "SpaceCoin" },
  { "spreadcoin", "SPR", "SpreadCoin" },
  { "spots", "SPT", "Spots" },
  { "squallcoin", "SQL", "SquallCoin" },
  { "startcoin", "START", "StartCoin" },
  { "storj", "STORJ", "Storj" },
  { "sativacoin", "STV", "Sativacoin" },
  { "swingcoin", "SWING", "Swingcoin" },
  { "swarmcitytoken", "SWT", "Swarm City Token" },
  { "sexcoin", "SXC", "Sexcoin" },
  { "syndicate", "SYNX", "Syndicate" },
  { "syscoin", "SYS", "SysCoin" },
  { "tajcoin", "TAJ", "Tajcoin" },
  { "teslacoin", "TES", "TeslaCoin" },
  { "titcoin", "TIT", "TitCoin" },
  { "trumpcoin", "TRUMP", "Trumpcoin" },
  { "tittiecoin", "TTC", "TittieCoin" },
  { "transfercoin", "TX", "TransferCoin" },
  { "unitus", "UIS", "Unitus" },
  { "unify", "UNIFY", "Unify" },
  { "viacoin", "VIA", "ViaCoin " },
  { "vertcoin", "VTC", "Vertcoin" },
  { "waves", "WAVES", "Waves" },
  { "beatcoin", "XBTS", "BeatCoin" },
  { "xtrabytes", "XBY", "XTRABYTES" },
  { "xcoin", "XCO", "Xcoin" },
  { "neweconomymovement", "XEM", "NewEconomyMovement" },
  { "joulecoin", "XJO", "JouleCoin" },
  { "magi", "XMG", "Magi" },
  { "petrodollar", "XPD", "PetroDollar" },
  { "platinumbar", "XPTX", "PlatinumBar" },
  { "ratecoin", "XRA", "RateCoin" },
  { "verge", "XVG", "Verge" },
  { "yobitcoin", "YOVI", "YobitCoin" },
  { "zcash", "ZEC", "Zcash" },
  { "zetacoin", "ZET", "ZetaCoin" },
  { "bonpay", "BON", "Bonpay" },
  { "universalcurrency", "UNIT", "UniversalCurrency" },
  { "bitcoinscrypt", "BTCS", "Bitcoin Scrypt" },
  { "catcoin", "CAT", "Catcoin" },
  { "crowdcoin", "CRC", "CrowdCoin" },
  { "decentbet", "DBET", "DecentBet" },
  { "pandacoin", "PND", "PandaCoin" },
  { "warcoin", "WRC", "WarCoin" },
  { "1337", "1337", "1337" },
  { "cryptoclub", "CCB", "CryptoClub" },
  { "coinlancer", "CL", "Coinlancer" },
  { "compoundcoin", "COMP", "Compound Coin" },
  { "dimecoin", "DIME", "DimeCoin" },
  { "experiencecoin", "EPC", "ExperienceCoin" },
  { "fireflycoin", "FFC", "FireFlyCoin" },
  { "cypherfunks", "FUNK", "Cypherfunks" },
  { "inflationcoin", "IFLT", "InflationCoin" },
  { "litedoge", "LDOGE", "LiteDoge" },
  { "leacoin", "LEA", "LeaCoin" },
  { "leafcoin", "LEAF", "Leafcoin" },
  { "incakoin", "NKA", "Incakoin" },
  { "ozziecoin", "OZC", "OzzieCoin" },
  { "clearpoll", "POLL", "ClearPoll" },
  { "rabbitcoin", "RBBT", "Rabbitcoin" },
  { "soma", "SCT", "Soma" },
  { "sjwcoin", "SJW", "SJWCoin" },
  { "statusnetworktoken", "SNT", "Status Network Token" },
  { "tekcoin", "TEK", "TEKcoin" },
  { "tronix", "TRX", "Tronix" },
  { "upfiring", "UFR", "Upfiring" },
  { "unikoingold", "UKG", "UnikoinGold" },
  { "vaperscoin", "VPRC", "VapersCoin" },
  { "mywishtoken", "WISH", "MyWishToken" },
  { "zeitcoin", "ZEIT", "ZeitCoin" },
  { "loopring", "LRC", "Loopring" },
  { "polcoin", "PLC", "Polcoin" },
  { "tron", "TRON", "TRON" },
  { "bytom", "BTM", "Bytom" },
  { "dmarket", "DMT", "DMarket" },
  { "0xprotocol", "ZRX", "0x Protocol" },
  { "electra", "ECA", "Electra" },
  { "inpay", "INPAY", "InPay" },
  { "polymath", "POLY", "Polymath" },
  { "usdtether", "USDT", "Tether" },
  { "eos", "EOS", "EOS" },
  { "nubits", "NBT", "Nubits" },
  { "sirintoken", "SRN", "Sirin Token" },
  { "etherzero", "ETZ", "EtherZero" },
  { "globalcryptocurrency", "GCC", "Global Cryptocurrency" },
  { "storm", "STORM", "Storm" },
  { "boson", "BOSON", "Boson" },
  { "embercoin", "EMB", "EmberCoin" },
  { "lookcoin", "LOOK", "LookCoin" },
  { "usdollar", "USD", "US Dollar" },
  { "russianrubble", "RUR", "Russian Rubble" },
  { "lina", "LINA", "Lina" },
  { "citadel", "CTL", "Citadel" },
  { "trueusd", "TUSD", "TrueUSD" },
  { "twist", "TWIST", "TWIST" },

};


vector <string> yobit_get_coin_ids ()
{
  vector <string> ids;
  unsigned int data_count = sizeof (yobit_coins_table) / sizeof (*yobit_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    ids.push_back (yobit_coins_table[i].identifier);
  }
  return ids;
}


vector <string> yobit_get_coin_abbrevs ()
{
  vector <string> abbrevs;
  unsigned int data_count = sizeof (yobit_coins_table) / sizeof (*yobit_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    abbrevs.push_back (yobit_coins_table[i].abbreviation);
  }
  return abbrevs;
}


string yobit_get_coin_id (string coin)
{
  unsigned int data_count = sizeof (yobit_coins_table) / sizeof (*yobit_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == yobit_coins_table[i].identifier) return yobit_coins_table[i].identifier;
    if (coin == yobit_coins_table[i].abbreviation) return yobit_coins_table[i].identifier;
    if (coin == yobit_coins_table[i].name) return yobit_coins_table[i].identifier;
  }
  return "";
}


string yobit_get_coin_abbrev (string coin)
{
  unsigned int data_count = sizeof (yobit_coins_table) / sizeof (*yobit_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == yobit_coins_table[i].identifier) return yobit_coins_table[i].abbreviation;
    if (coin == yobit_coins_table[i].abbreviation) return yobit_coins_table[i].abbreviation;
    if (coin == yobit_coins_table[i].name) return yobit_coins_table[i].abbreviation;
  }
  return "";
}


string yobit_get_coin_name (string coin)
{
  unsigned int data_count = sizeof (yobit_coins_table) / sizeof (*yobit_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == yobit_coins_table[i].identifier) return yobit_coins_table[i].name;
    if (coin == yobit_coins_table[i].abbreviation) return yobit_coins_table[i].name;
    if (coin == yobit_coins_table[i].name) return yobit_coins_table[i].name;
  }
  return "";
}


void yobit_get_coins (const string & market, vector <string> & coin_abbreviations, string & error)
{
  string market_abbrev = str2lower (yobit_get_coin_abbrev (market));
  string url = yobit_get_public_api_call ({ "info" });
  string json = http_call (url, error, "", false, false, "", {}, false, false, "");
  if (!error.empty ()) {
    return;
  }
  // {
  //   "server_time":1418654531,
  //   "pairs":{
  //     "ltc_btc":{
  //       "decimal_places":8,
  //       "min_price":0.00000001,
  //       "max_price":10000,
  //       "min_amount":0.0001,
  //       "hidden":0,
  //       "fee":0.2
  //     }
  //     ...
  //   }
  // }
  Object main;
  main.parse (json);
  if (main.has<Object>("pairs")) {
    Object pairs = main.get<Object>("pairs");
    map <string, Value*> json_map = pairs.kv_map ();
    for (auto key_value : json_map) {
      string pair = key_value.first;
      // Object values = key_value.second->get<Object>();
      // Take coins only from the desired market.
      size_t pos = pair.find ("_" + market_abbrev);
      if (pos != string::npos) {
        string lowercase_abbreviation = pair.substr (0, pos);
        string uppercase_abbreviation = str2upper (lowercase_abbreviation);
        coin_abbreviations.push_back (uppercase_abbreviation);
        // The full name of the coin is not available through the API, in 2017.
      }
    }
    return;
  }
  error.append (json);
}


void yobit_get_market_summaries (string market,
                                 vector <string> & coin_ids,
                                 vector <float> & bid,
                                 vector <float> & ask,
                                 string & error)
{
  // Storage for all JSON responses added together.
  string accumulated_json;
  string market_abbrev = str2lower (yobit_get_coin_abbrev (market));
  // Gets the summaries for the currencies in the requested base $market.
  // Several pairs can be added to the call, separated by a hyphen.
  // Add a parameter also, to ignore possible invalid pairs, so the server does not return an error.
  // E.g.: https://yobit.net/api/3/ticker/ltc_btc-bern_btc?ignore_invalid=1
  // If there's too many pairs in the URL,
  // or if the total URL length becomes too long,
  // Yobit will give this response:
  // {"success":0,"error":"Empty pair list"}
  // To get all pairs, multiple calls to Yobit are needed.
  // Each calls only requests a limited number of pairs, not all of them.
  // The maximum number of pairs one call takes was established by trial and error.
  vector <string> abbrevs = yobit_get_coin_abbrevs ();
  while (!abbrevs.empty ()) {
    string pairs;
    for (size_t i = 0; i < 40; i++) {
      if (!abbrevs.empty ()) {
        if (!pairs.empty ()) pairs.append ("-");
        pairs.append (str2lower (abbrevs.front ()));
        abbrevs.erase (abbrevs.begin());
        pairs.append ("_");
        pairs.append (market_abbrev);
      }
    }
    string url = yobit_get_public_api_call ({ "ticker", pairs });
    url.append ("?ignore_invalid=1");
    string json = http_call (url, error, "", false, false, "", {}, false, false, satellite_get_interface (url));
    if (!error.empty ()) {
      return;
    }
    accumulated_json.append (json);
    // {
    //   "ltc_btc":{
    //     "high":105.41,
    //     "low":104.67,
    //     "avg":105.04,
    //     "vol":43398.22251455,
    //     "vol_cur":4546.26962359,
    //     "last":105.11,
    //     "buy":104.2,
    //     "sell":105.11,
    //     "updated":1418654531
    //   }
    //   ...
    // }
    Object main;
    main.parse (json);
    // In case of failure, it gives JSON like this: {"success":0,"error":"Empty pair list"}
    if (!main.has<Number>("success")) {
      map <string, Value*> json_map = main.kv_map ();
      for (auto key_value : json_map) {
        // The pair: E.g.: ltc_btc.
        string pair = key_value.first;
        // The values, among them bid (buy) and ask (sell) prices.
        Object values = key_value.second->get<Object>();
        // Get the coin abbreviation from e.g. ltc_btc.
        size_t pos = pair.find ("_");
        if (pos != string::npos) {
          string lowercase_coin_abbreviation = pair.substr (0, pos);
          string uppercase_coin_abbreviation = str2upper (lowercase_coin_abbreviation);
          if (values.has<Number>("buy")) {
            float buy = values.get<Number>("buy");
            if (values.has<Number>("sell")) {
              float sell = values.get<Number>("sell");
              string coin_id = yobit_get_coin_id (uppercase_coin_abbreviation);
              if (coin_id.empty ()) continue;
              coin_ids.push_back (coin_id);
              bid.push_back (buy);
              ask.push_back (sell);
            }
          }
        }
      }
    }
  }
  if (coin_ids.empty ()) {
    error.append (accumulated_json);
    return;
  }
}


void yobit_get_depth (string market,
                      string coin,
                      string direction,
                      vector <float> & quantity,
                      vector <float> & rate,
                      string & error,
                      bool hurry)
{
  // Gets the depth, that is, the sellers and the buyers, for the currency at the market.
  string market_abbrev = str2lower (yobit_get_coin_abbrev (market));
  string coin_abbrev = str2lower (yobit_get_coin_abbrev (coin));
  string pair = coin_abbrev + "_" + market_abbrev;
  string url = yobit_get_public_api_call ({ "depth", pair });
  url.append ("?limit=20");
  string json = http_call (url, error, "", false, false, "", {}, hurry, false, "");
  if (!error.empty ()) {
    if (hurry) yobit_hurried_call_fail++;
    return;
  }
  // {
  //   "dash_doge":{
  //     "asks":[
  //       [309901.53552,0.00125376],
  //       [311243.72994486,0.01],
  //       [311661.38,0.00159171],
  //       ...
  //     ],
  //     "bids":[
  //       [301482.56,0.01727109],
  //       [300326,0.00033297],
  //       [291481.58,0.00779725],
  //       ...
  //     ]
  //   }
  // }
  Object main;
  main.parse (json);
  if (main.has <Object>(pair)) {
    Object coin_market = main.get <Object>(pair);
    if (coin_market.has <Array>(direction)) {
      Array values = coin_market.get <Array> (direction);
      for (size_t i = 0; i < values.size(); i++) {
        Array price_quantity = values.get <Array> (i);
        float price = price_quantity.get<Number>(0);
        float amount = price_quantity.get<Number>(1);
        quantity.push_back (amount);
        rate.push_back (price);
      }
    }
  }
  // Error handler.
  if (quantity.empty ()) {
    error.append (json);
    if (hurry) yobit_hurried_call_fail++;
  } else {
    if (hurry) yobit_hurried_call_pass++;
  }
}


void yobit_get_buyers (string market,
                       string coin,
                       vector <float> & quantity,
                       vector <float> & rate,
                       string & error,
                       bool hurry)
{
  yobit_get_depth (market, coin, "bids", quantity, rate, error, hurry);
}


void yobit_get_sellers (string market,
                        string coin,
                        vector <float> & quantity,
                        vector <float> & rate,
                        string & error,
                        bool hurry)
{
  yobit_get_depth (market, coin, "asks", quantity, rate, error, hurry);
}


void yobit_get_wallet_details (string coin, string & json, string & error,
                               string & address, string & payment_id)
{
  string coin_abbrev = yobit_get_coin_abbrev (coin);
  vector <pair <string, string> > parameters = {
    make_pair ("method", "GetDepositAddress"),
    make_pair("coinName", coin_abbrev)
  };
  json = yobit_get_trading_api_call (parameters, error);
  if (!error.empty ()) {
    error.append (" ");
    error.append (__FUNCTION__);
    return;
  }
 
  // {"success":1,"return":{"address":"19RyPTjwvwH4BrZ2wxPsSSRoZRG9B3tTu3","processed_amount":0.00000000,"server_time":1512061501}}
  
  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has<Number>("success")) {
    if (main.get<Number>("success") == 1) {
      if (main.has<Object>("return")) {
        Object data = main.get<Object>("return");
        if (data.has<String>("address")) {
          address = data.get<String>("address");
          payment_id.clear ();
          /* No longer used.
          if (data.has<Number> ("processed_amount")) {
            // The processed amount here only includes the processed amount in deposits.
            // The value does not increase after Yobit has processed a withdrawal.
            processed_quantity = data.get<Number> ("processed_amount");
          }
           */
          if (coin_abbrev == "XEM") {
            // In the case of NewEconomyMovement, it returns the message rather than the address.
            payment_id = address;
            // The Yobit pool address for NewEconomyMovement:
            address = "NBRT3YQTVHLTYBDUXH2HHURI5KCYDWRWJ63YWIGG";
          }
          // Done.
          return;
        }
      }
    }
  }
  
  // Error handler.
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
  return;
}


// Get the $total balance that includes also the following two amounts:
// 1. The balances $reserved for trade orders and for withdrawals.
// 2. The $unconfirmed balance: What is being deposited right now.
// It also gives all matching coin abbreviations.
void yobit_get_balances (vector <string> & coin_abbrev,
                         vector <string> & coin_ids,
                         vector <float> & total,
                         vector <float> & reserved,
                         vector <float> & unconfirmed,
                         string & error)
{
  vector <pair <string, string> > parameters = {
    make_pair ("method", "getInfo")
  };
  string json = yobit_get_trading_api_call (parameters, error);
  if (!error.empty ()) {
    error.append (" ");
    error.append (__FUNCTION__);
    return;
  }
  
  // {
  //   "success":1,
  //   "return":{
  //     "rights":{"info":1,"trade":1,"deposit":1,"withdraw":1},
  //     "funds":{"btc":0.148,"ltc":22,"nvc":423.998,"ppc":10, ...},
  //     "funds_incl_orders":{"btc":0.148,"ltc":32,"nvc":523.998,"ppc":20, ...},
  //     "transaction_count":0,
  //     "open_orders":0,
  //     "server_time":1512066873
  //   }
  // }

  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has<Number>("success")) {
    if (main.get<Number>("success") == 1) {
      if (main.has<Object>("return")) {
        Object data = main.get<Object>("return");
        // Store the distinct coins from the funds and the funds including the orders.
        // The reason is this: It has not been checked that they match all of the time.
        set <string> distinct_lowercase_coin_abbreviations;
        if (data.has<Object>("funds")) {
          // funds: available account balance (does not include money on open orders)
          Object funds = data.get <Object> ("funds");
          map <string, Value*> json_map_funds = funds.kv_map ();
          for (auto key_value : json_map_funds) {
            string lowercase_coin_abbrev = key_value.first;
            distinct_lowercase_coin_abbreviations.insert (lowercase_coin_abbrev);
          }
          if (data.has<Object>("funds_incl_orders")) {
            // funds_incl_orders: available account balance (includes money on open orders)
            Object funds_incl_orders = data.get <Object> ("funds_incl_orders");
            map <string, Value*> json_map_funds_incl_orders = funds_incl_orders.kv_map ();
            for (auto key_value : json_map_funds_incl_orders) {
              string lowercase_coin_abbrev = key_value.first;
              distinct_lowercase_coin_abbreviations.insert (lowercase_coin_abbrev);
            }
            // Iterate over all coins encountered, and fetch their balances from the JSON object.
            for (auto lowercase_coin_abbrev : distinct_lowercase_coin_abbreviations) {
              float available_balance = funds.get <Number> (lowercase_coin_abbrev);
              float available_balance_including_orders = funds_incl_orders.get <Number> (lowercase_coin_abbrev);
              if (available_balance_including_orders > 0) {
                // Translate the balances from Yobit to the balance types as used by the bot.
                coin_abbrev.push_back (str2upper (lowercase_coin_abbrev));
                // Get the coin ID.
                // If there's a balance and the coin ID is unknown, give the coin abbreviation instead.
                // The result will be that any unknown balance will be reported at least.
                string coin_id = yobit_get_coin_id (str2upper (lowercase_coin_abbrev));
                if (coin_id.empty ()) coin_id = str2upper (lowercase_coin_abbrev);
                coin_ids.push_back (coin_id);
                total.push_back (available_balance_including_orders);
                reserved.push_back (available_balance_including_orders - available_balance);
                // The Yobit API does not currently provide any unconfirmed balances.
                unconfirmed.push_back (0.0);
              }
            }
          }
          // Done.
          return;
        }
      }
    }
  }

  // Error handling.
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
}


mutex yobit_balance_mutex;
map <string, float> yobit_balance_total;
map <string, float> yobit_balance_available;
map <string, float> yobit_balance_reserved;
map <string, float> yobit_balance_unconfirmed;


void yobit_get_balance (const string & coin, float * total, float * available, float * reserved, float * unconfirmed)
{
  yobit_balance_mutex.lock ();
  if (total) * total = yobit_balance_total [coin];
  if (available) * available = yobit_balance_available [coin];
  if (reserved) * reserved = yobit_balance_reserved [coin];
  if (unconfirmed) * unconfirmed = yobit_balance_unconfirmed [coin];
  yobit_balance_mutex.unlock ();
}


void yobit_set_balance (const string & coin, float total, float available, float reserved, float unconfirmed)
{
  yobit_balance_mutex.lock ();
  if (total != 0) yobit_balance_total [coin] = total;
  if (available != 0) yobit_balance_available [coin] = available;
  if (reserved != 0) yobit_balance_reserved [coin] = reserved;
  if (unconfirmed != 0) yobit_balance_unconfirmed [coin] = unconfirmed;
  yobit_balance_mutex.unlock ();
}


vector <string> yobit_get_coins_with_balances ()
{
  vector <string> coins;
  yobit_balance_mutex.lock ();
  for (auto & element : yobit_balance_total) {
    coins.push_back (element.first);
  }
  yobit_balance_mutex.unlock ();
  return coins;
}


string yobit_limit_trade (string market, string coin, string type, float quantity, float rate, string & json, string & error)
{
  string market_abbreviation = yobit_get_coin_abbrev (market);
  string coin_abbreviation = yobit_get_coin_abbrev (coin);
  string coin_market_pair = str2lower (coin_abbreviation) + "_" + market_abbreviation;
  vector <pair <string, string> > parameters = {
    make_pair ("method", "Trade"),
    make_pair ("type", type),
    make_pair ("pair", coin_market_pair),
    make_pair ("rate", float2string (rate)),
    make_pair ("amount", float2string (quantity))
  };
  json = yobit_get_trading_api_call (parameters, error);
  if (!error.empty ()) {
    error.append (" ");
    error.append (__FUNCTION__);
    return "";
  }
  
  // {
  //   "success":1,
  //   "return":{
  //     "received":0.1,
  //     "remains":0,
  //     "order_id":12345,
  //     "funds":{
  //       "btc":15,
  //       "ltc":51.82,
  //       "nvc":0,
  //       ...
  //     }
  //   }
  // }

  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has<Number>("success")) {
    if (main.get<Number>("success") == 1) {
      if (main.has<Object>("return")) {
        Object data = main.get<Object>("return");
        if (data.has<Number> ("order_id")) {
          long int order_id = data.get<Number> ("order_id");
          return to_string (order_id);
        }
      }
    }
  }

  // Error handler.
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);

  // Done.
  return "";
}


string yobit_limit_buy (string market,
                        string coin,
                        float quantity,
                        float rate,
                        string & json,
                        string & error)
{
  return yobit_limit_trade (market, coin, "buy", quantity, rate, json, error);
}


string yobit_limit_sell (string market,
                         string coin,
                         float quantity,
                         float rate,
                         string & json,
                         string & error)
{
  return yobit_limit_trade (market, coin, "sell", quantity, rate, json, error);
}


void yobit_get_open_orders (vector <string> & identifier,
                            vector <string> & market,
                            vector <string> & coin_abbreviation,
                            vector <string> & coin_ids,
                            vector <string> & buy_sell,
                            vector <float> & amount,
                            vector <float> & rate,
                            vector <string> & date,
                            string & error)
{
  // The Yobit API returns the open order per coin/market pair.
  // So first ask the balances, which will give the coins that have funds.
  // Then ask the open orders for each of those coins.
  vector <string> coin_abbrevs_with_balance;
  vector <float> dummy;
  vector <string> coin_ids_with_balance;
  yobit_get_balances (coin_abbrevs_with_balance, coin_ids_with_balance, dummy, dummy, dummy, error);
  if (!error.empty ()) return;
  
  // Iterate over the coins.
  for (auto coin_abbrev : coin_abbrevs_with_balance) {

    // Request active orders for this coin at the Bitcoin market.
    string market_abbrev = "btc";
    coin_abbrev = str2lower (coin_abbrev);
    if (coin_abbrev == market_abbrev) continue;
    string coin_market_pair = coin_abbrev + "_" + market_abbrev;
    vector <pair <string, string> > parameters = {
      make_pair ("method", "ActiveOrders"),
      make_pair ("pair", coin_market_pair)
    };
    string json = yobit_get_trading_api_call (parameters, error);
    if (!error.empty ()) {
      error.append (" ");
      error.append (__FUNCTION__);
      return;
    }
    
    // {
    //   "success":1,
    //   "return":{
    //     "100001320501550":{
    //       "pair":"ltc_btc",
    //       "type":"sell",
    //       "amount":21.615,
    //       "rate":0.258,
    //       "timestamp_created":"1512845671",
    //       "status":0
    //     },
    //     ...
    //   }
    // }

    // Parse the JSON.
    Object main;
    main.parse (json);
    if (main.has<Number>("success")) {
      if (main.get<Number>("success") == 1) {
        if (main.has<Object>("return")) {
          Object data = main.get<Object>("return");
          map <string, Value*> json_map = data.kv_map ();
          for (auto key_value : json_map) {
            string order_id = key_value.first;
            Object values = key_value.second->get<Object>();
            string pair = values.get <String> ("pair");
            string type = values.get <String> ("type");
            float quantity = values.get <Number> ("amount");
            float price = values.get <Number> ("rate");
            string timestamp_created = values.get <String> ("timestamp_created");
            vector <string> coin_market_pair = explode (pair, '_');
            if (coin_market_pair.size () == 2) {
              string coin_abbrev = coin_market_pair [0];
              coin_abbrev = str2upper (coin_abbrev);
              string market_abbrev = coin_market_pair [1];
              market_abbrev = str2upper (market_abbrev);
              string market_id = yobit_get_coin_id (market_abbrev);
              identifier.push_back (order_id);
              market.push_back (market_id);
              coin_abbreviation.push_back (coin_abbrev);
              coin_ids.push_back (yobit_get_coin_id (coin_abbrev));
              buy_sell.push_back (type);
              amount.push_back (quantity);
              rate.push_back (price);
              date.push_back (timestamp_created);
            }
          }
        }
        continue;
      }
    }
    
    // Error handler.
    error.append (__FUNCTION__);
    error.append (" ");
    error.append (json);
  }
}


bool yobit_cancel_order (string order_id,
                         string & error)
{
  vector <pair <string, string> > parameters = {
    make_pair ("method", "CancelOrder"),
    make_pair ("order_id", order_id)
  };
  string json = yobit_get_trading_api_call (parameters, error);
  if (!error.empty ()) {
    error.append (" ");
    error.append (__FUNCTION__);
    return false;
  }
  
  // {
  //   "success":1,
  //   "return":{
  //     "order_id":100025362,
  //     "funds":{
  //       "btc":15,
  //       "ltc":51.82,
  //       "nvc":0,
  //       ...
  //     }
  //   }
  // }
  
  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has<Number>("success")) {
    if (main.get<Number>("success") == 1) {
      if (main.has<Object>("return")) {
        return true;
      }
    }
  }
  
  // Error handler.
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
  return false;
}


string yobit_withdrawal_order (string coin,
                               float quantity,
                               string address,
                               string paymentid,
                               string & json,
                               string & error)
{
  string coin_abbrev = yobit_get_coin_abbrev (coin);
  vector <pair <string, string> > parameters = {
    make_pair ("method", "WithdrawCoinsToAddress"),
    make_pair ("coinName", coin_abbrev),
    make_pair ("amount", float2string (quantity)),
    make_pair ("address", address)
  };
  (void) paymentid;
  json = yobit_get_trading_api_call (parameters, error);
  if (!error.empty ()) {
    error.append (" ");
    error.append (__FUNCTION__);
    return "";
  }

  // {
  //   "success":1,
  //   "return":{
  //     "server_time": 1437146228
  //   }
  // }
  
  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has<Number>("success")) {
    if (main.get<Number>("success") == 1) {
      // Provide some distinct order identifier.
      return "YobiT" + coin + to_string (seconds_since_epoch ());
    }
  }

  // Error handler.
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
  return "";
}


void yobit_withdrawalhistory (vector <string> & order_ids,
                              vector <string> & coin_abbreviations,
                              vector <string> & coin_ids,
                              vector <float> & quantities,
                              vector <string> & addresses,
                              vector <string> & transaction_ids,
                              string & error)
{
  // The Yobit API does not provide this information (2017).
  // A request was put in to support this.
  
  // The following was tried:
  // Need to access the web interface of Yobit via curl.
  // https://askubuntu.com/questions/161778/how-do-i-use-wget-curl-to-download-from-a-site-i-am-logged-into
  // wget --load-cookies=cookies.txt http://en.wikipedia.org/wiki/User:A
  // The reason is to verify, automatically, all withdrawals off Yobit.
  // To check whether their TxIDs are found on the public block chain.
  // https://curl.haxx.se/docs/http-cookies.html
  // If cookie exported from Firefox, to be sent to the coins manually, not as part of the code.
  // curl --cookie-jar ~/Desktop/cookies.txt --header "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.13; rv:57.0) Gecko/20100101 Firefox/57.0" https://yobit.net/en/history/withdrawals/ > ~/Desktop/out.html
  // It failed to work, because Yobit returned another page to curl, one not logged-in.
  (void) order_ids;
  (void) coin_abbreviations;
  (void) coin_ids;
  (void) quantities;
  (void) addresses;
  (void) transaction_ids;
  (void) error;
  
  // A request was submitted to Yobit support for the deposit and withdrawal history via the API.
  // See https://yobit.net/en/support/a9854a72f693311db17ca5d5c7309ceebe038056aba4541a71a7f11b7a578a5b
  // How to find out via the Yobit API if a withdrawal has been completed?
  // A ticket was opened about this.
  // https://yobit.net/en/support/4f5c9444689bd6d6f39a76bd3a0092ecb4d2df3fbbd23fc599eeae4a85b105fd
}


void yobit_deposithistory (vector <string> & order_ids,
                           vector <string> & coin_abbreviations,
                           vector <string> & coin_ids,
                           vector <float> & quantities,
                           vector <string> & addresses,
                           vector <string> & transaction_ids,
                           string & error)
{
  // The Yobit API does not provide this information (2017).
  (void) order_ids;
  (void) coin_abbreviations;
  (void) coin_ids;
  (void) quantities;
  (void) addresses;
  (void) transaction_ids;
  (void) error;

  // A request was submitted to Yobit support for the deposit and withdrawal history via the API.
  // See https://yobit.net/en/support/a9854a72f693311db17ca5d5c7309ceebe038056aba4541a71a7f11b7a578a5b
  // How to find out via the Yobit API if a withdrawal has been completed?
  // A ticket was opened about this.
  // https://yobit.net/en/support/4f5c9444689bd6d6f39a76bd3a0092ecb4d2df3fbbd23fc599eeae4a85b105fd
}


int yobit_get_hurried_call_passes ()
{
  return yobit_hurried_call_pass;
}


int yobit_get_hurried_call_fails ()
{
  return yobit_hurried_call_fail;
}


/*
 
 Yobit really was troublesome.
 See the following proposed work-around, below.
 This work-around is no longer needed because Yobit was ditched as too troublesome.
 
 If there's a critical operation via the satellite,
 like getting the order book for a real trade operation,
 it will be helpful to do that through a dedicated satellite,
 rather than through all the satellites,
 because like Yobit at times fails saying "Just a moment...",
 and if that does that during a real trade, this trade would fail.
 Another way of resolving this possible point of failure would be
 to then repeat the request for the order book,
 in case of a defined response, like "Just a moment...".
 Or rather than using a satellite for that,
 one could also use no satellite at all,
 just send it straight from the front bot.
 
 */
