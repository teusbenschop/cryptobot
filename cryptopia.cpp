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


#include "cryptopia.h"
#include "proxy.h"


#define api_key "--enter--your--value--here--"
#define api_secret "--enter--your--value--here--"


// Hurried call counters.
atomic <int> cryptopia_hurried_call_pass (0);
atomic <int> cryptopia_hurried_call_fail (0);


string cryptopia_get_public_api_call (vector <string> methods)
{
  // Assemble the URL.
  string url ("https://www.cryptopia.co.nz/api");
  for (auto method : methods) url.append ("/" + method);
  // Done.
  return url;
}


string cryptopia_get_private_api_call (vector <string> methods)
{
  // Assemble the URL.
  string url ("https://www.cryptopia.co.nz/Api");
  for (auto method : methods) url.append ("/" + method);

  // The private calls regularly returns this error:
  // {"Success":false,"Error":"Nonce has already been used for this request."}
  // It does not necessarily mean that this nonce has already been used, but what it often means is this:
  // The nonce use is smaller than a previously used nonce.
  // The bot generates always increasing nonces.
  // But the API calls the bot makes to Cryptopia may arrive out of sync.
  // The result is that a later call may arrive before an earlier call.
  // And that leads to a smaller nonce being received by Cryptopia after a bigger nonce has been receive already.
  // This is what Cryptopia Support says about the issue:
  // "Hi. What you are experiencing is the result of duplicate requests to the api. The first request has succeeded, hence the order being placed, the second request was identified as a duplicate, which is why you receive the response: {"Success":false,"Error":"Nonce has already been used for this request."}. This is protection put in place to avoid duplicate orders being placed and processed through the api. Hope this explains what you are experiencing. Thanks, Cryptopia Support".
  
  // Done.
  return url;
}


// If API calls are sent to the network with correct increasing nonces,
// the calls may arrive out of order at the Cryptopia exchange, due to the network.
// This would lead to nonces no longer to be increasing, but out of order.
// This mutex should prevent that more or less.
// mutex cryptopia_private_api_call_mutex;
// When the mutex appears as too rigid as related to timing,
// another way of ensuring increasing nonces is to use no longer mutexes but to use timers.
// There's a timer delay of part of a second,
// that ensures pending requests are sent off in the correct sequence.
// This system no longer waits for the response to come in.
// It just ensures the requests go out in a timely fashion and in an orderly fashion.
// This new method is now used as being better in case of an overloaded exchange that responds after a long delay.
atomic <long> cryptopia_private_api_call_sequencer (0);


string cryptopia_get_exchange ()
{
  return "cryptopia";
}


typedef struct
{
  const char * identifier;
  const char * abbreviation;
  const char * name;
}
cryptopia_coin_record;


cryptopia_coin_record cryptopia_coins_table [] =
{
  { "bitcoin", "BTC", "Bitcoin" },
  { "dotcoin", "DOT", "Dotcoin" },
  { "litecoin", "LTC", "Litecoin" },
  { "dogecoin", "DOGE", "Dogecoin" },
  { "potcoin", "POT", "Potcoin" },
  { "feathercoin", "FTC", "Feathercoin" },
  { "wearesatoshi", "WSX", "WeAreSatoshi" },
  { "reddcoin", "RDD", "Reddcoin" },
  { "dark", "DARK", "DARK" },
  { "digibyte", "DGB", "DigiByte" },
  { "aricoin", "ARI", "Aricoin" },
  { "bitcoinfast", "BCF", "Bitcoinfast" },
  { "songcoin", "SONG", "Songcoin" },
  { "sexcoin", "SXC", "Sexcoin" },
  { "magi", "XMG", "Magi" },
  { "redcoin", "RED", "RedCoin" },
  { "paycon", "CON", "PayCon" },
  { "money", "$$$", "Money" },
  { "quatloo", "QTL", "Quatloo" },
  { "bitsend", "BSD", "BitSend" },
  { "okcash", "OK", "OKCash" },
  { "geocoin", "GEO", "GeoCoin" },
  { "globalboost-y", "BSTY", "GlobalBoost-Y" },
  { "titcoin", "TIT", "TitCoin" },
  { "verge", "XVG", "Verge" },
  { "topcoin", "TOP", "TopCoin" },
  { "orbitcoin", "ORB", "OrbitCoin" },
  { "cryptobullion", "CBX", "CryptoBullion" },
  { "guncoin", "GUN", "GunCoin" },
  { "motocoin", "MOTO", "MotoCoin" },
  { "fonziecoin", "FONZ", "FonzieCoin" },
  { "tittiecoin", "TTC", "TittieCoin" },
  { "squallcoin", "SQL", "SquallCoin" },
  { "hobonickels", "HBN", "HoboNickels" },
  { "phoenixcoin", "PXC", "PhoenixCoin" },
  { "unicoin", "UNIC", "UniCoin" },
  { "unobtanium", "UNO", "Unobtanium" },
  { "piggycoin", "PIGGY", "PiggyCoin" },
  { "emercoin", "EMC", "EmerCoin" },
  { "cthulhuofferings", "OFF", "Cthulhu Offerings" },
  { "sooncoin", "SOON", "SoonCoin" },
  { "8bit", "8BIT", "8Bit" },
  { "hyperstake", "HYP", "HyperStake" },
  { "deutscheemark", "DEM", "Deutsche eMark" },
  { "shacoin2", "SHA", "SHACoin2" },
  { "irishcoin", "IRL", "IrishCoin" },
  { "skeincoin", "SKC", "SkeinCoin" },
  { "asiacoin", "AC", "Asiacoin" },
  { "mars", "MARS", "Mars" },
  { "megacoin", "MEC", "MegaCoin" },
  { "teslacoin", "TES", "TeslaCoin" },
  { "russiacoin", "RC", "RussiaCoin" },
  { "i0coin", "I0C", "I0Coin" },
  { "flaxscript", "FLAX", "Flaxscript" },
  { "petrodollar", "XPD", "PetroDollar" },
  { "animecoin", "ANI", "AnimeCoin" },
  { "parallelcoin", "DUO", "Parallelcoin" },
  { "growthcoin", "GRW", "GrowthCoin" },
  { "polcoin", "PLC", "Polcoin" },
  { "version", "V", "Version" },
  { "audiocoin", "ADC", "AudioCoin" },
  { "bnrtxcoin", "BNX", "BnrtxCoin" },
  { "chaincoin", "CHC", "ChainCoin" },
  { "truckcoin", "TRK", "TruckCoin" },
  { "compucoin", "CPN", "CompuCoin" },
  { "gamecredits", "GAME", "GameCredits" },
  { "coffeecoin", "CFC", "CoffeeCoin" },
  { "elacoin", "ELC", "ElaCoin" },
  { "spots", "SPT", "Spots" },
  { "worldcoin", "WDC", "WorldCoin" },
  { "prototanium", "PR", "Prototanium" },
  { "startcoin", "START", "StartCoin" },
  { "triangles", "TRI", "Triangles" },
  { "rubycoin", "RBY", "RubyCoin" },
  { "goldpieces", "GP", "GoldPieces" },
  { "peercoin", "PPC", "PeerCoin" },
  { "eurocoin", "EUC", "Eurocoin" },
  { "novacoin", "NVC", "NovaCoin" },
  { "blackcoin", "BLK", "BlackCoin" },
  { "pakcoin", "PAK", "PakCoin" },
  { "vericoin", "VRC", "VeriCoin" },
  { "pesetacoin", "PTC", "PesetaCoin" },
  { "polishcoin", "PCC", "PolishCoin" },
  { "clamcoin", "CLAM", "ClamCoin" },
  { "influxcoin", "INFX", "Influxcoin" },
  { "cannabiscoin", "CANN", "Cannabiscoin" },
  { "primecoin", "XPM", "PrimeCoin" },
  { "canadaecoin", "CDN", "Canada eCoin" },
  { "netcoin", "NET", "NetCoin" },
  { "bolivarcoin", "BOLI", "BolivarCoin" },
  { "bitcoinscrypt", "BTCS", "Bitcoin Scrypt" },
  { "nyancoin", "NYAN", "NyanCoin" },
  { "embargocoin", "EBG", "EmbargoCoin" },
  { "goldcoin", "GLD", "GoldCoin" },
  { "sterlingcoin", "SLG", "Sterlingcoin" },
  { "bumbacoin", "BUMBA", "Bumbacoin" },
  { "bata", "BTA", "BATA" },
  { "auroracoin", "AUR", "AuroraCoin" },
  { "rimbit", "RBT", "Rimbit" },
  { "sativacoin", "STV", "Sativacoin" },
  { "myriad", "XMY", "Myriad" },
  { "evotion", "EVO", "Evotion" },
  { "evergreencoin", "EGC", "EverGreenCoin" },
  { "bitbar", "BTB", "BitBar" },
  { "philosopherstone", "PHS", "PhilosopherStone" },
  { "opensourcecoin", "OSC", "Open Source Coin" },
  { "emeraldcrypto", "EMD", "Emerald Crypto" },
  { "swingcoin", "SWING", "Swingcoin" },
  { "mintcoin", "MINT", "Mintcoin" },
  { "digitalcoin", "DGC", "Digitalcoin" },
  { "quark", "QRK", "Quark" },
  { "evilcoin", "EVIL", "Evilcoin" },
  { "catcoin", "CAT", "Catcoin" },
  { "fastcoin", "FST", "FastCoin" },
  { "namecoin", "NMC", "NameCoin" },
  { "universalmolecule", "UMO", "UniversalMolecule" },
  { "unitus", "UIS", "Unitus" },
  { "transfercoin", "TX", "TransferCoin" },
  { "terracoin", "TRC", "TerraCoin" },
  { "c-bit", "XCT", "C-bit" },
  { "monero", "XMR", "Monero" },
  { "halcyon", "HAL", "Halcyon" },
  { "navcoin", "NAV", "NavCoin" },
  { "decred", "DCR", "Decred" },
  { "ronpaulcoin", "RPC", "RonPaulCoin" },
  { "litebar", "LTB", "LiteBar" },
  { "beezercoin", "BEEZ", "BeezerCoin" },
  { "neutron", "NTRN", "Neutron" },
  { "moin", "MOIN", "MOIN" },
  { "fuzzballs", "FUZZ", "FuzzBalls" },
  { "darsek", "KED", "Darsek" },
  { "ixcoin", "IXC", "IXCoin" },
  { "bitcoinplus", "XBC", "BitcoinPlus" },
  { "yobitcoin", "YOVI", "YobitCoin" },
  { "acoin", "ACOIN", "ACoin" },
  { "francs", "FRN", "Francs" },
  { "edrcoin", "EDRC", "EDRcoin" },
  { "lemoncoin", "LEMON", "LemonCoin" },
  { "lithiumcoin", "LIT", "Lithiumcoin" },
  { "blakecoin", "BLC", "Blakecoin" },
  { "sakuracoin", "SKR", "SakuraCoin" },
  { "goldpressedlatinum", "GPL", "GoldPressedLatinum " },
  { "argentum", "ARG", "Argentum" },
  { "tigercoin", "TGC", "TigerCoin" },
  { "berncash", "BERN", "BERNcash" },
  { "trumpcoin", "TRUMP", "Trumpcoin" },
  { "ratecoin", "XRA", "RateCoin" },
  { "cloakcoin", "CLOAK", "CloakCoin" },
  { "revolvercoin", "XRE", "Revolvercoin" },
  { "mineum", "MNM", "Mineum" },
  { "smartcoin", "SMC", "SmartCoin" },
  { "coolindarkcoin", "CC", "CoolInDarkCoin" },
  { "karbowanec", "KRB", "Karbowanec" },
  { "joulecoin", "XJO", "JouleCoin" },
  { "zetacoin", "ZET", "ZetaCoin" },
  { "aurumcoin", "AU", "AurumCoin" },
  { "octocoin", "888", "OctoCoin" },
  { "cubits", "QBT", "Cubits" },
  { "nexus", "NXS", "Nexus" },
  { "cryptojacks", "CJ", "CryptoJacks" },
  { "klondikecoin", "KDC", "KlondikeCoin" },
  { "educoinv", "EDC", "EducoinV" },
  { "granite", "GRN", "Granite" },
  { "ethereumclassic", "ETC", "Ethereum Classic" },
  { "expanse", "EXP", "Expanse" },
  { "cometcoin", "CMT", "CometCoin" },
  { "bitgem", "BTG", "Bitgem" },
  { "machinecoin", "MAC", "MachineCoin" },
  { "postcoin", "POST", "PostCoin" },
  { "ladacoin", "LDC", "LADACoin" },
  { "opalcoin", "OPAL", "Opalcoin" },
  { "bvbcoin", "BVB", "BVBCoin" },
  { "fujicoin", "FJC", "FujiCoin" },
  { "e-gulden", "EFL", "E-Gulden" },
  { "gapcoin", "GAP", "Gapcoin" },
  { "lbrycredits", "LBC", "LBRY Credits" },
  { "factom", "FCT", "Factom" },
  { "benjirolls", "BENJI", "BenjiRolls" },
  { "kashcoin", "KASH", "KashCoin" },
  { "spacecoin", "SPACE", "SpaceCoin" },
  { "verium", "VRM", "Verium" },
  { "groestlcoin", "GRS", "Groestlcoin" },
  { "pivx", "PIVX", "PIVX" },
  { "cannacoin", "CCN", "CannaCoin" },
  { "fluttercoin", "FLT", "FlutterCoin" },
  { "bitcoal", "COAL", "Bitcoal" },
  { "zcoin", "XZC", "ZCoin" },
  { "atomiccoin", "ATOM", "Atomiccoin" },
  { "mustangcoin", "MST", "Mustangcoin" },
  { "zcash", "ZEC", "ZCash" },
  { "ur", "UR", "UR" },
  { "arcticcoin", "ARC", "ArcticCoin" },
  { "qubitcoin", "Q2C", "Qubitcoin" },
  { "bipcoin", "BIP", "BipCoin" },
  { "wayawolfcoin", "WW", "WayaWolfCoin" },
  { "zclassic", "ZCL", "Zclassic" },
  { "zoin", "ZOI", "Zoin" },
  { "vadercorpcoin", "VCC", "VaderCorpCoin" },
  { "stratis", "STRAT", "Stratis" },
  { "hush", "HUSH", "Hush" },
  { "conquestcoin", "CQST", "ConquestCoin" },
  { "siberianchervonets", "SIB", "SiberianChervonets" },
  { "aquariuscoin", "ARCO", "AquariusCoin" },
  { "incoin", "IN", "InCoin" },
  { "chesscoin", "CHESS", "ChessCoin" },
  { "careercoin", "CAR", "CareerCoin" },
  { "kobocoin", "KOBO", "KoboCoin" },
  { "selencoin", "SEL", "Selencoin" },
  { "om", "OOO", "Om" },
  { "42-coin", "42", "42-coin" },
  { "donationcoin", "DON", "DonationCoin" },
  { "eryllium", "ERY", "Eryllium" },
  { "nevacoin", "NEVA", "NevaCoin" },
  { "lanacoin", "LANA", "LanaCoin" },
  { "spectrecoin", "XSPEC", "SpectreCoin" },
  { "beatcoin", "XBTS", "BeatCoin" },
  { "tajcoin", "TAJ", "Tajcoin" },
  { "marxcoin", "MARX", "MarxCoin" },
  { "prime-xi", "PXI", "Prime-XI" },
  { "warcoin", "WRC", "WarCoin" },
  { "solarflarecoin", "SFC", "SolarflareCoin" },
  { "komodo", "KMD", "Komodo" },
  { "kushcoin", "KUSH", "KushCoin" },
  { "marijuanacoin", "MAR", "MarijuanaCoin" },
  { "ultracoin", "UTC", "Ultracoin" },
  { "alexandrite", "ALEX", "Alexandrite" },
  { "zero", "ZER", "Zero" },
  { "melite", "MLITE", "Melite" },
  { "arguscoin", "ARGUS", "ArgusCoin" },
  { "netko", "NETKO", "Netko" },
  { "renoscoin", "RNS", "RenosCoin" },
  { "allion", "ALL", "Allion" },
  { "mojocoin", "MOJO", "MojoCoin" },
  { "geertcoin", "GEERT", "GeertCoin" },
  { "pascallite", "PASL", "PascalLite" },
  { "ubiq", "UBQ", "Ubiq" },
  { "musicoin", "MUSIC", "Musicoin" },
  { "hexxcoin", "HXX", "HexxCoin" },
  { "synereoamp", "AMP", "Synereo AMP" },
  { "maidsafecoin", "MAID", "MaidSafeCoin" },
  { "iticoin", "ITI", "ItiCoin" },
  { "ark", "ARK", "Ark" },
  { "zsecoin", "ZSE", "ZSEcoin" },
  { "altcoin", "ALT", "AltCoin" },
  { "wirelesscoin", "WLC", "WirelessCoin" },
  { "dash", "DASH", "Dash" },
  { "eclipse", "EC", "Eclipse" },
  { "putincoin", "PUT", "PutinCoin" },
  { "ecocoin", "ECO", "ECOcoin" },
  { "skycoin", "SKY", "Skycoin" },
  { "noblecoin", "NOBL", "NobleCoin" },
  { "bitstar", "BITS", "Bitstar" },
  { "pepecoin", "PEPE", "PepeCoin" },
  { "pinkcoin", "PINK", "PinkCoin" },
  { "procurrency", "PROC", "ProCurrency" },
  { "bitcore", "BTX", "BitCore" },
  { "crave", "CRAVE", "Crave" },
  { "condensate", "RAIN", "Condensate" },
  { "sumokoin", "SUMO", "Sumokoin" },
  { "coin2", "C2", "Coin2" },
  { "creativecoin", "CREA", "CreativeCoin" },
  { "coinonat", "CXT", "Coinonat" },
  { "insane", "INSN", "Insane" },
  { "xtrabytes", "XBY", "XTRABYTES" },
  { "terranova", "TER", "TerraNova" },
  { "neweconomymovement", "XEM", "NewEconomyMovement" },
  { "ethereum", "ETH", "Ethereum" },
  { "gnosis", "GNO", "Gnosis" },
  { "golem", "GNT", "Golem" },
  { "cachecoin", "CACH", "CacheCoin" },
  { "beachcoin", "SAND", "BeachCoin" },
  { "augur", "REP", "Augur" },
  { "dinastycoin", "DCY", "DinastyCoin" },
  { "daxxcoin", "DAXX", "DaxxCoin" },
  { "einsteinium", "EMC2", "Einsteinium" },
  { "flash", "FLASH", "FLASH" },
  { "ecobit", "ECOB", "EcoBit" },
  { "minereum", "MNE", "Minereum" },
  { "toacoin", "TOA", "TOACoin" },
  { "soilcoin", "SOIL", "SoilCoin" },
  { "lindacoin", "LINDA", "LindaCoin" },
  { "magnetcoin", "MAGN", "MagnetCoin" },
  { "denarius", "DNR", "Denarius" },
  { "xcoin", "XCO", "Xcoin" },
  { "300token", "300", "300 Token" },
  { "platinumbar", "XPTX", "PlatinumBar" },
  { "sixeleven", "611", "SixEleven" },
  { "kekcoin", "KEK", "Kekcoin" },
  { "bitbay", "BAY", "BitBay" },
  { "minex", "MINEX", "Minex" },
  { "mothership", "MSP", "Mothership" },
  { "sphre", "XID", "Sphre" },
  { "mobilego", "MGO", "MobileGo" },
  { "bravenewcoin", "BNC", "BraveNewCoin" },
  { "dopecoin", "DOPE", "DopeCoin" },
  { "nolimitcoin", "NLC2", "NoLimitCoin" },
  { "securecoin", "SRC", "SecureCoin" },
  { "shrooms", "SHRM", "Shrooms" },
  { "adcoin", "ACC", "Adcoin" },
  { "growersintl", "GRWI", "Growers Intl" },
  { "linx", "LINX", "Linx" },
  { "bitcoincash", "BCH", "BitcoinCash" },
  { "weed", "WEED", "Weed" },
  { "droxne", "DRXNE", "Droxne" },
  { "virtauniquecoin", "VUC", "VirtaUniqueCoin" },
  { "pillar", "PLR", "Pillar" },
  { "adshares", "ADST", "Adshares" },
  { "artbyte", "ABY", "ArtByte" },
  { "coinonatx", "XCXT", "CoinonatX" },
  { "etheriya", "RIYA", "Etheriya" },
  { "da$", "DAS", "DA$" },
  { "alphabit", "ABC", "Alphabit" },
  { "dalecoin", "DALC", "DALECOIN" },
  { "investfeed", "IFT", "investFeed" },
  { "stealthcoin", "XST", "StealthCoin" },
  { "kronecoin", "KRONE", "Kronecoin" },
  { "litebitcoin", "LBTC", "LiteBitcoin" },
  { "litecoinultra", "LTCU", "LitecoinUltra" },
  { "skincoin", "SKIN", "SkinCoin" },
  { "campuscoin", "CMPCO", "CampusCoin" },
  { "omisego", "OMG", "OmiseGo" },
  { "tenx", "PAY", "TenX" },
  { "dubaicoin", "DBIX", "DubaiCoin" },
  { "neo", "NEO", "Neo" },
  { "masternodecoin", "MTNC", "MasterNodeCoin" },
  { "embers", "MBRS", "Embers" },
  { "royalkingdomcoin", "RKC", "RoyalKingdomCoin" },
  { "athenianwarriortoken", "ATH", "Athenian Warrior Token" },
  { "creamcoin", "CRM", "CREAMcoin" },
  { "bitdeal", "BDL", "BitDeal" },
  { "zencash", "ZEN", "ZenCash" },
  { "spreadcoin", "SPR", "SpreadCoin" },
  { "starcredits", "STRC", "StarCredits" },
  { "leviarcoin", "XLC", "LeviarCoin" },
  { "wildcrypto", "WILD", "WildCrypto" },
  { "metal", "MTL", "Metal" },
  { "ormeuscoin", "ORME", "OrmeusCoin" },
  { "blockcat", "BKCAT", "BlockCat" },
  { "neblio", "NEBL", "Neblio" },
  { "bytom", "BTM", "Bytom" },
  { "monacocoin", "XMCC", "MonacoCoin" },
  { "equitrade", "EQT", "Equitrade" },
  { "digitalprice", "DP", "DigitalPrice" },
  { "iquantchain", "IQT", "Iquant Chain" },
  { "billionairetoken", "XBL", "Billionaire Token" },
  { "musiconomi", "MCI", "Musiconomi" },
  { "corion", "COR", "Corion" },
  { "hackspace", "HAC", "Hackspace" },
  { "hodlbucks", "HDLB", "HodlBucks" },
  { "kybernetworkcrystal", "KNC", "KyberNetworkCrystal" },
  { "qwark", "QWARK", "Qwark" },
  { "apx", "APX", "APX" },
  { "king93", "KING", "King93" },
  { "obsidian", "ODN", "Obsidian" },
  { "hshare", "HSR", "HSHARE" },
  { "gocoin", "XGOX", "GoCoin" },
  { "revain", "R", "Revain" },
  { "picklericks", "RICKS", "PickleRicks" },
  { "cryptopiafeeshare", "CEFS", "CryptopiaFeeShare" },
  { "bitcloud", "BTDX", "Bitcloud" },
  { "elements", "ELM", "Elements" },
  { "everus", "EVR", "Everus" },
  { "senderon", "SDRN", "Senderon" },
  { "blocktix", "TIX", "Blocktix" },
  { "alis", "ALIS", "ALIS" },
  { "coimatic3", "CTIC3", "Coimatic 3" },
  { "social", "SCL", "Social" },
  { "enjincoin", "ENJ", "Enjin Coin" },
  { "coppercoin", "COPPER", "CopperCoin" },
  { "opentradingnetwork", "OTN", "Open Trading Network" },
  { "pura", "PURA", "Pura" },
  { "dapowerplay", "DPP", "DA Power Play" },
  { "sbccoin", "SBC", "SBC Coin" },
  { "luxcoin", "LUX", "Luxcoin" },
  { "pirl", "PIRL", "Pirl" },
  { "coino", "CNO", "Coino" },
  { "deeponion", "ONION", "DeepOnion" },
  { "vivo", "VIVO", "Vivo" },
  { "trezarcoin", "TZC", "Trezarcoin" },
  { "electroneum", "ETN", "Electroneum" },
  { "powerledger", "POWR", "PowerLedger" },
  { "wincoin", "WC", "Wincoin" },
  { "izecoin", "IZE", "IZEcoin" },
  { "innova", "INN", "Innova" },
  { "havecoin", "HAV", "Havecoin" },
  { "eddiecoin", "EDDIE", "Eddiecoin" },
  { "litecoinplus", "LCP", "litecoinPlus" },
  { "unify", "UNIFY", "Unify" },
  { "blockpool", "BPL", "Blockpool" },
  { "interstellarholdings", "HOLD", "InterstellarHoldings" },
  { "ethereumdark", "ETHD", "Ethereum Dark" },
  { "zap", "ZAP", "Zap" },
  { "megax", "MGX", "MegaX" },
  { "phore", "PHR", "Phore" },
  { "deuscoin", "DEUS", "Deuscoin" },
  { "harvestcoin", "HC", "HarvestCoin" },
  { "gobyte", "GBX", "GoByte" },
  { "digipulse", "DGPT", "DigiPulse" },
  { "clearpoll", "POLL", "ClearPoll" },
  { "blockmason", "BCPT", "BlockMason" },
  { "decisiontoken", "HST", "Decision Token" },
  { "opcoin", "OPC", "OP Coin" },
  { "universalcurrency", "UNIT", "UniversalCurrency" },
  { "monkeyproject", "MONK", "MonkeyProject" },
  { "voise", "VOISE", "VOISE" },
  { "diviexchangetoken", "DIVX", "Divi Exchange Token" },
  { "publica", "PBL", "Publica" },
  { "bonpay", "BON", "Bonpay" },
  { "heatledger", "HEAT", "HeatLedger" },
  { "tokugawacoin", "TOK", "TokugawaCoin" },
  { "kubera", "KBR", "Kubera" },
  { "genechain", "DNA", "GeneChain" },
  { "socialsend", "SEND", "SocialSend" },
  { "ellaism", "ELLA", "Ellaism" },
  { "bulwark", "BWK", "Bulwark" },
  { "pioneercoin", "PCOIN", "PioneerCoin" },
  { "bismuth", "BIS", "Bismuth" },
  { "magiccoin", "MAGE", "MagicCoin" },
  { "sagacoin", "SAGA", "SagaCoin" },
  { "matrixcoin", "MATRX", "MatrixCoin" },
  { "crowdcoin", "CRC", "CrowdCoin" },
  { "numus", "NMS", "Numus" },
  { "spankchain", "SPANK", "SpankChain" },
  { "steneum", "STN", "Steneum" },
  { "mywishtoken", "WISH", "MyWishToken" },
  { "cappasity", "CAPP", "Cappasity" },
  { "upfiring", "UFR", "Upfiring" },
  { "oysterpearl", "PRL", "OysterPearl" },
  { "pandacoin", "PND", "PandaCoin" },
  { "byteballbytes", "GBYTE", "Byteball Bytes" },
  { "cryptcoin", "CRYPT", "CryptCoin" },
  { "kayicoin", "KAYI", "Kayicoin" },
  { "helium", "HLM", "Helium" },
  { "cryptoclub", "CCB", "CryptoClub" },
  { "geocoin", "GEO/Old", "GeoCoin" },
  { "trinity", "TTY", "Trinity" },
  { "groincoin", "GXG", "GroinCoin" },
  { "feathercoinclassic", "FTCC", "FeatherCoinClassic" },
  { "gcoin", "GCN", "GCoin" },
  { "spartancoin", "SPN", "SpartanCoin" },
  { "zeitcoin", "ZEIT", "ZeitCoin" },
  { "ozziecoin", "OZC", "OzzieCoin" },
  { "leacoin", "LEA", "LeaCoin" },
  { "cypherfunks", "FUNK", "Cypherfunks" },
  { "blazecoin", "BLZ", "BlazeCoin" },
  { "litedoge", "LDOGE", "LiteDoge" },
  { "sjwcoin", "SJW", "SJWCoin" },
  { "incakoin", "NKA", "Incakoin" },
  { "bongger", "BGR", "Bongger" },
  { "tekcoin", "TEK", "TEKcoin" },
  { "grandcoin", "GDC", "GrandCoin" },
  { "turbostake", "TRBO", "TurboStake" },
  { "kumacoin", "KUMA", "KumaCoin" },
  { "leafcoin", "LEAF", "Leafcoin" },
  { "batcoin", "BAT", "BatCoin" },
  { "fireflycoin", "FFC", "FireFlyCoin" },
  { "rabbitcoin", "RBBT", "Rabbitcoin" },
  { "borgcoin", "BRG", "BorgCoin" },
  { "photon", "PHO", "Photon" },
  { "lycancoin", "LYC", "Lycancoin" },
  { "808", "808", "808" },
  { "vaperscoin", "VPRC", "VapersCoin" },
  { "amigacoin", "AGA", "AmigaCoin" },
  { "1337", "1337", "1337" },
  { "royalties", "XRY", "Royalties" },
  { "inflationcoin", "IFLT", "InflationCoin" },
  { "dentacoin", "DCN", "Dentacoin" },
  { "namocoin", "NAMO", "NAMO COIN" },
  { "compoundcoin", "COMP", "Compound Coin" },
  { "lottocoin", "LOT", "Lottocoin" },
  { "penguin", "PENG", "Penguin" },
  { "bunnycoin", "BUN", "BunnyCoin" },
  { "floripacoin", "FLN", "FloripaCoin" },
  { "slothcoin", "SLOTH", "SlothCoin" },
  { "dimecoin", "DIME", "DimeCoin" },
  { "birdcoin", "BIRD", "BirdCoin" },
  { "experiencecoin", "EPC", "ExperienceCoin" },
  { "coinlancer", "CL", "Coinlancer" },
  { "tronix", "TRX", "Tronix" },
  { "soma", "SCT", "Soma" },
  { "canya", "CAN", "CanYa" },
  { "usdtether", "USDT", "Tether" },
  { "newzealanddollartoken", "NZDT", "New Zealand Dollar Token" },
  { "lynx", "LYNX", "Lynx" },
  { "beancash", "BEAN", "Bean Cash" },
  { "fuelcoin", "FC2", "FuelCoin" },
  { "chancoin", "CHAN", "ChanCoin" },
  { "ac3", "AC3", "AC3" },
  { "ignitioncoin", "IC", "IgnitionCoin" },
  { "$pac", "$PAC", "$PAC" },
  { "centrality", "CENNZ", "Centrality" },
  { "wispr", "WSP", "Wispr" },
  { "gainer", "GNR", "Gainer" },
  { "galactrum", "ORE", "Galactrum" },
  { "polis", "POLIS", "Polis" },
  { "colossusxt", "COLX", "ColossusXT" },
  { "vade", "VADE", "Vade" },
  { "inpay", "INPAY", "InPay" },
  { "bitzeny", "ZNY", "BitZeny" },
  { "magnet", "MAG", "Magnet" },
  { "electra", "ECA", "Electra" },
  { "bitmark", "MARKS", "Bitmark" },
  { "blocknet", "BLOCK", "Blocknet" },
  { "experiencepoints", "XP", "Experience Points" },
  { "viceindustrytoken", "VIT", "Vice Industry Token" },
  { "deviant", "DEV", "Deviant" },
  { "skrilla", "SKRL", "Skrilla" },
  { "bitcoingreen", "BITG", "Bitcoin Green" },
  { "nullex", "NLX", "NulleX" },
  { "fantasycash", "FANS", "Fantasy Cash" },
  { "lina", "LINA", "Lina" },
  { "stakenet", "XSN", "StakeNet" },
  { "indahash", "IDH", "indaHash" },
  { "eos", "EOS", "EOS" },
  { "livestarstoken", "LIVE", "Live Stars Token" },
  { "lisk", "LSK", "Lisk" },
  { "amon", "AMN", "Amon" },
  { "atmos", "ATMOS", "Atmos" },
  { "lwf", "LWF", "LWF" },
  { "oxycoin", "OXY", "Oxycoin" },
  { "globalcryptocurrency", "GCC", "Global Cryptocurrency" },
  { "etherzero", "ETZ", "EtherZero" },
  { "sirinlabstoken", "SRN", "Sirin Labs Token" },
  { "cardano", "ADA", "Cardano" },
  { "boson", "BOSON", "Boson" },
  { "thechiefcoin", "CHIEF", "TheChiefCoin" },
  { "embercoin", "EMB", "EmberCoin" },
  { "eztoken", "EZT", "EZToken" },
  { "lookcoin", "LOOK", "LookCoin" },
  { "trueusd", "TUSD", "TrueUSD" },
  { "fabrictoken", "FT", "Fabric Token" },
  { "loki", "LOKI", "Loki" },
  { "loyalcoin", "LYL", "LoyalCoin" },
  { "citadel", "CTL", "Citadel" },
  { "qtum", "QTUM", "Qtum" },
  { "sirius", "SIRX", "Sirius" },
  { "syndicate", "SYNX", "Syndicate" },
  { "gincoin", "GIN", "GINcoin" },
  { "maza", "MAZA", "Maza" },
  { "proof", "PF", "PROOF" },
  { "quasarcoin", "QAC", "Quasarcoin" },
  { "blitzpredict", "XBP", "BlitzPredict" },
  { "zest", "ZEST", "Zest" },
  { "newpowercoin", "NPW", "New Power Coin" },
  { "peepcoin", "PCN", "Peepcoin" },
  { "tokenpay", "TPAY", "TokenPay" },
  { "twist", "TWIST", "TWIST" },
  { "budbotoken", "BUBO", "Budbo Token" },
  { "gazecoin", "GZE", "GazeCoin" },
  { "energi", "NRG", "Energi" },
  { "shivers", "SHVR", "Shivers" },
  { "whalecoin", "WHL", "WhaleCoin" },
  { "zixx", "XZX", "Zixx" },
  { "bcshop.iotoken", "BCS", "BCShop.io Token" },
  { "boxx", "BOXX", "Boxx" },
  { "bottlecaps", "CAP", "BottleCaps" },
  { "condominium", "CDM", "Condominium" },
  { "dimecoindirty", "DIRTY", "DimeCoinDirty" },
  { "drputility", "DRPU", "DRP Utility" },
  { "encryptotel", "ETT", "EncryptoTel" },
  { "bifrost", "FROST", "BiFrost" },
  { "iqcash", "IQ", "IQ Cash" },
  { "ivy", "IVY", "IVY" },
  { "lftccoin", "LFTC", "LFTCCoin" },
  { "overpowercoinx", "OPCX", "OverPowerCoinX" },
  { "popularcoin", "POP", "PopularCoin" },
  { "proudmoney", "PROUD", "ProudMoney" },
  { "ryo", "RYO", "Ryo" },
  { "whitecoin", "XWC", "WhiteCoin" },
  { "contractnet", "CNET", "ContractNet" },
  { "dimcoin", "DIM", "DIMCOIN" },
  { "graft", "GRFT", "GRAFT" },
  { "blockpass", "PASS", "Blockpass" },
  { "vitae", "VITAE", "Vitae" },
  { "adultchain", "XXX", "AdultChain" },
  { "blocknode", "BND", "BLOCKNODE" },
  { "ravencoin", "RVN", "RAVENCOIN" },
  { "lightpaycoin", "LPC", "Light Pay Coin" },
  { "xdna", "XDNA", "XDNA" },

};


vector <string> cryptopia_get_coin_ids ()
{
  vector <string> ids;
  unsigned int data_count = sizeof (cryptopia_coins_table) / sizeof (*cryptopia_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    ids.push_back (cryptopia_coins_table[i].identifier);
  }
  return ids;
}


string cryptopia_get_coin_id (string coin)
{
  unsigned int data_count = sizeof (cryptopia_coins_table) / sizeof (*cryptopia_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == cryptopia_coins_table[i].identifier) return cryptopia_coins_table[i].identifier;
    if (coin == cryptopia_coins_table[i].abbreviation) return cryptopia_coins_table[i].identifier;
    if (coin == cryptopia_coins_table[i].name) return cryptopia_coins_table[i].identifier;
  }
  return "";
}


string cryptopia_get_coin_abbrev (string coin)
{
  unsigned int data_count = sizeof (cryptopia_coins_table) / sizeof (*cryptopia_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == cryptopia_coins_table[i].identifier) return cryptopia_coins_table[i].abbreviation;
    if (coin == cryptopia_coins_table[i].abbreviation) return cryptopia_coins_table[i].abbreviation;
    if (coin == cryptopia_coins_table[i].name) return cryptopia_coins_table[i].abbreviation;
  }
  return "";
}


string cryptopia_get_coin_name (string coin)
{
  unsigned int data_count = sizeof (cryptopia_coins_table) / sizeof (*cryptopia_coins_table);
  for (unsigned int i = 0; i < data_count; i++) {
    if (coin == cryptopia_coins_table[i].identifier) return cryptopia_coins_table[i].name;
    if (coin == cryptopia_coins_table[i].abbreviation) return cryptopia_coins_table[i].name;
    if (coin == cryptopia_coins_table[i].name) return cryptopia_coins_table[i].name;
  }
  return "";
}


void cryptopia_json_check (string & json, string & error)
{
  if (json.find ("maintenance mode") != string::npos) {
    json.clear ();
    error = "Cryptopia is in maintenance mode";
    to_stdout ({error});
  }
}


void cryptopia_get_coins (const string & market,
                          vector <string> & coin_abbrevs,
                          vector <string> & coin_ids,
                          vector <string> & names,
                          string & error)
{
  string market_abbrev = cryptopia_get_coin_abbrev (market);
  string url = cryptopia_get_public_api_call ({ "GetTradePairs" });
  string json = http_call (url, error, "", false, false, "", {}, false, false, "");
  cryptopia_json_check (json, error);
  if (!error.empty ()) {
    return;
  }
  Object main;
  main.parse (json);
  if (main.has<Boolean>("Success")) {
    if (main.get<Boolean>("Success")) {
      if (main.has<Array>("Data")) {
        Array data = main.get<Array>("Data");
        for (size_t i = 0; i < data.size(); i++) {
          Object values = data.get <Object> (i);
          if (!values.has <String> ("Label")) continue;
          string label = values.get <String> ("Label");
          size_t pos = label.find ("/" + market_abbrev);
          if (pos == string::npos) continue;
          label.erase (pos);
          if (!values.has <String> ("Symbol")) continue;
          string symbol = values.get <String> ("Symbol");
          if (!values.has <String> ("Currency")) continue;
          string currency = values.get <String> ("Currency");
          coin_abbrevs.push_back (label);
          coin_ids.push_back (cryptopia_get_coin_id (label));
          names.push_back (currency);
        }
        return;
      }
    }
  }
  error.append (json);
}


void cryptopia_get_market_summaries (string market,
                                     vector <string> & coin_ids,
                                     vector <float> & bid,
                                     vector <float> & ask,
                                     string & error)
{
  string market_abbrev = cryptopia_get_coin_abbrev (market);
  // Gets the summaries for the currencies in the requested base $market.
  string url = cryptopia_get_public_api_call ({ "GetMarkets", market_abbrev });
  string json = http_call (url, error, "", false, false, "", {}, false, false, satellite_get_interface (url));
  // At times markets other than Bitcoin regularly return "null" as the JSON value.
  // This is specific to Cryptopia.
  // Check on that here.
  // Repeat the call if needed.
  if (json == "null") {
    json = http_call (url, error, "", false, false, "", {}, false, false, satellite_get_interface (url));
  }
  if (json == "null") {
    json = http_call (url, error, "", false, false, "", {}, false, false, satellite_get_interface (url));
  }
  cryptopia_json_check (json, error);
  if (!error.empty ()) {
    return;
  }
  Object main;
  main.parse (json);
  if (main.has<Boolean>("Success")) {
    if (main.get<Boolean>("Success")) {
      if (main.has<Array>("Data")) {
        Array data = main.get<Array>("Data");
        for (size_t i = 0; i < data.size(); i++) {
          Object values = data.get <Object> (i);
          if (!values.has<String>("Label")) continue;
          string Label = values.get<String>("Label");
          // Example: LTC/BTC (the Litecoin at the Bitcoin market).
          size_t pos = Label.find ("/" + market_abbrev);
          if (pos == string::npos) continue;
          Label.erase (pos);
          if (!values.has<Number>("BidPrice")) continue;
          if (!values.has<Number>("AskPrice")) continue;
          float BidPrice = values.get<Number>("BidPrice");
          float AskPrice = values.get<Number>("AskPrice");
          string coin_id = cryptopia_get_coin_id (Label);
          if (coin_id.empty ()) continue;
          coin_ids.push_back (coin_id);
          bid.push_back (BidPrice);
          ask.push_back (AskPrice);
        }
        return;
      }
    }
  }
  // Error handler.
  error.append (json);
}


string cryptopia_get_authorization (string url, string post_data)
{
  // The Cryptopia API gives an error on a duplicate nonce.
  // So the nonce cannot be derived from the current second.
  // Add the get_microseconds to it.
  string nonce = increasing_nonce ();

  string m = md5_raw (post_data);
  
  string requestContentBase64String = base64_encode (m);
  
  string lowerurlencoded = str2lower (url_encode (url));
  string signature = string (api_key) + "POST" + lowerurlencoded + nonce + requestContentBase64String;
  
  string hmacsignature = base64_encode (hmac_sha256_raw (base64_decode (api_secret), signature));
  
  string authorization = "amx " + string (api_key) + ":" + hmacsignature + ":" + nonce;
  
  return authorization;
}


void cryptopia_get_open_orders (vector <string> & identifier,
                                vector <string> & market,
                                vector <string> & coin_abbreviation,
                                vector <string> & coin_ids,
                                vector <string> & buy_sell,
                                vector <float> & amount,
                                vector <float> & rate,
                                vector <string> & date,
                                string & error)
{
  api_call_sequencer (cryptopia_private_api_call_sequencer);
  // cryptopia_private_api_call_mutex.lock ();

  string url = cryptopia_get_private_api_call ({ "GetOpenOrders" });
  
  //string post_data = "{\"Market\":\"DOT/BTC\"}";
  string post_data = "{\"Market\":\"\"}";
  
  vector <pair <string, string> > headers;
  headers.push_back (make_pair ("Content-Type", "application/json; charset=utf-8"));
  headers.push_back (make_pair ("Authorization", cryptopia_get_authorization (url, post_data)));
  
  string json = http_call (url, error, "", false, true, post_data, headers, false, false, "");
  
  //cryptopia_private_api_call_mutex.unlock ();

  cryptopia_json_check (json, error);
  if (!error.empty ()) {
    return;
  }

  // {"Success":true,"Error":null,"Data":[{"OrderId":188537897,"TradePairId":4032,"Market":"ETC/BTC","Type":"Sell","Rate":0.00192282,"Amount":2.42228365,"Total":0.00465762,"Remaining":2.42228365,"TimeStamp":"2017-12-09T19:13:24.9989013"},...{"OrderId":184469433,"TradePairId":4032,"Market":"ETC/BTC","Type":"Buy","Rate":0.00142258,"Amount":5.93603754,"Total":0.00844449,"Remaining":5.93603754,"TimeStamp":"2017-12-08T04:07:21.4592266"}]}

  Object main;
  main.parse (json);
  if (main.has<Boolean>("Success")) {
    if (main.get<Boolean>("Success")) {
      if (main.has<Array>("Data")) {
        Array data = main.get<Array>("Data");
        for (size_t i = 0; i < data.size(); i++) {
          Object values = data.get <Object> (i);
          if (!values.has<Number>("OrderId")) continue;
          if (!values.has<String>("Market")) continue;
          if (!values.has<String>("Type")) continue;
          if (!values.has<Number>("Rate")) continue;
          if (!values.has<Number>("Remaining")) continue;
          if (!values.has<String>("TimeStamp")) continue;
          int OrderId = values.get<Number>("OrderId");
          string Market = values.get<String>("Market");
          vector <string> coin_market_pair = explode (Market, '/');
          if (coin_market_pair.size() != 2) continue;
          string coin_abbrev = coin_market_pair [0];
          string market_abbreviation = coin_market_pair [1];
          string market_id = cryptopia_get_coin_id (market_abbreviation);
          string Type = values.get<String>("Type");
          float Rate = values.get<Number>("Rate");
          float Remaining = values.get<Number>("Remaining");
          string TimeStamp = values.get<String>("TimeStamp");
          identifier.push_back (to_string (OrderId));
          market.push_back (market_id);
          coin_abbreviation.push_back (coin_abbrev);
          coin_ids.push_back (cryptopia_get_coin_id (coin_abbrev));
          buy_sell.push_back (Type);
          amount.push_back (Remaining);
          rate.push_back (Rate);
          date.push_back (TimeStamp);
        }
        return;
      }
    }
  }
  
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
}


// Get the $total balance that includes also the following two amounts:
// 1. The balances $reserved for trade orders and for withdrawals.
// 2. The $unconfirmed balance: What is being deposited right now.
// It also gives all matching coin abbreviations.
void cryptopia_get_balances (vector <string> & coin_abbrev,
                             vector <string> & coin_ids,
                             vector <float> & total,
                             vector <float> & reserved,
                             vector <float> & unconfirmed,
                             string & error)
{
  api_call_sequencer (cryptopia_private_api_call_sequencer);
  //cryptopia_private_api_call_mutex.lock ();

  string url = cryptopia_get_private_api_call ({ "GetBalance" });
  
  //string post_data = "{\"Currency\":\"EFL\"}";
  string post_data = "{\"Currency\":\"\"}";
  
  vector <pair <string, string> > headers;
  headers.push_back (make_pair ("Content-Type", "application/json; charset=utf-8"));
  headers.push_back (make_pair ("Authorization", cryptopia_get_authorization (url, post_data)));
  
  string json = http_call (url, error, "", false, true, post_data, headers, false, true, "");
  
  //cryptopia_private_api_call_mutex.unlock ();
  
  cryptopia_json_check (json, error);
  if (!error.empty ()) {
    return;
  }
  
  /*
  {"Success":true,"Error":null,"Data":[
   {"CurrencyId":331,"Symbol":"1337","Total":0.00000000,"Available":0.00000000,"Unconfirmed":0.00000000,"HeldForTrades":0.00000000,"PendingWithdraw":0.00000000,"Address":null,"Status":"Maintenance","StatusMessage":"Investigating network Fork.","BaseAddress":null},
   {"CurrencyId":573,"Symbol":"21M","Total":0.00000000,"Available":0.00000000,"Unconfirmed":0.00000000,"HeldForTrades":0.00000000,"PendingWithdraw":0.00000000,"Address":null,"Status":"OK","StatusMessage":null,"BaseAddress":""},
   {"CurrencyId":99,"Symbol":"CHAO","Total":0.00000000,"Available":0.00000000,"Unconfirmed":0.00000000,"HeldForTrades":0.00000000,"PendingWithdraw":0.00000000,"Address":null,"Status":"OK","StatusMessage":"network has been 51%'d","BaseAddress":null}, ...
   */

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
          float Total = values.get<Number>("Total");
          //float Available = values.get<Number>("Available");
          float Unconfirmed = values.get<Number>("Unconfirmed");
          float HeldForTrades = values.get<Number>("HeldForTrades");
          //float PendingWithdraw = values.get<Number>("PendingWithdraw");
          if (Total == 0) continue;
          coin_abbrev.push_back (Symbol);
          // Get the coin ID.
          // If there's a balance and the coin ID is unknown, give the coin abbreviation instead.
          // The result will be that any unknown balance will be reported at least.
          string coin_id = cryptopia_get_coin_id (Symbol);
          if (coin_id.empty ()) coin_id = Symbol;
          coin_ids.push_back (coin_id);
          total.push_back (Total);
          reserved.push_back (HeldForTrades);
          unconfirmed.push_back (Unconfirmed);
        }
        return;
      }
    }
  }
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
}


mutex cryptopia_balance_mutex;
map <string, float> cryptopia_balance_total;
map <string, float> cryptopia_balance_available;
map <string, float> cryptopia_balance_reserved;
map <string, float> cryptopia_balance_unconfirmed;


void cryptopia_get_balance (const string & coin, float * total, float * available, float * reserved, float * unconfirmed)
{
  cryptopia_balance_mutex.lock ();
  if (total) * total = cryptopia_balance_total [coin];
  if (available) * available = cryptopia_balance_available [coin];
  if (reserved) * reserved = cryptopia_balance_reserved [coin];
  if (unconfirmed) * unconfirmed = cryptopia_balance_unconfirmed [coin];
  cryptopia_balance_mutex.unlock ();
}


void cryptopia_set_balance (const string & coin, float total, float available, float reserved, float unconfirmed)
{
  cryptopia_balance_mutex.lock ();
  if (total != 0) cryptopia_balance_total [coin] = total;
  if (available != 0) cryptopia_balance_available [coin] = available;
  if (reserved != 0) cryptopia_balance_reserved [coin] = reserved;
  if (unconfirmed != 0) cryptopia_balance_unconfirmed [coin] = unconfirmed;
  cryptopia_balance_mutex.unlock ();
}


vector <string> cryptopia_get_coins_with_balances ()
{
  vector <string> coins;
  cryptopia_balance_mutex.lock ();
  for (auto & element : cryptopia_balance_total) {
    coins.push_back (element.first);
  }
  cryptopia_balance_mutex.unlock ();
  return coins;
}


void cryptopia_get_wallet_details (string coin, string & json, string & error,
                                   string & address, string & payment_id)
{
  api_call_sequencer (cryptopia_private_api_call_sequencer);
  //cryptopia_private_api_call_mutex.lock ();

  string url = cryptopia_get_private_api_call ({ "GetDepositAddress" });
  
  string coin_abbrev = cryptopia_get_coin_abbrev (coin);
  
  string post_data = "{\"Currency\":\"" + coin_abbrev + "\"}";
  
  vector <pair <string, string> > headers;
  headers.push_back (make_pair ("Content-Type", "application/json; charset=utf-8"));
  headers.push_back (make_pair ("Authorization", cryptopia_get_authorization (url, post_data)));
  
  json = http_call (url, error, "", false, true, post_data, headers, false, false, "");
  
  //cryptopia_private_api_call_mutex.unlock ();

  cryptopia_json_check (json, error);
  if (!error.empty ()) {
    return;
  }

  Object main;
  main.parse (json);
  if (main.has<Boolean>("Success")) {
    if (main.get<Boolean>("Success")) {
      if (main.has<Object>("Data")) {
        Object data = main.get<Object>("Data");
        if (data.has<String>("Currency")) {
          if (data.has<String>("Address")) {
            string Currency = data.get<String>("Currency");
            string Address = data.get<String>("Address");
            string BaseAddress;
            if (data.has<String>("BaseAddress")) {
              BaseAddress = data.get <String> ("BaseAddress");
            }
            if (Currency == coin_abbrev) {
              address = Address;
              payment_id.clear ();
              if (coin_abbrev == "XEM") {
                // This currency has a base address, so values are slightly different from the usual.
                payment_id = address;
                address = BaseAddress;
              }
              return;
            }
          }
        }
      }
    }
  }

  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
  return;
}


void cryptopia_get_market_orders (string market,
                                  string coin,
                                  string direction,
                                  vector <float> & quantity,
                                  vector <float> & rate,
                                  string & error,
                                  bool hurry)
{
  // Translate the market and the coin.
  string market_abbrev = cryptopia_get_coin_abbrev (market);
  string coin_abbrev = cryptopia_get_coin_abbrev (coin);
  // Call the API.
  string url = cryptopia_get_public_api_call ({ "GetMarketOrders", coin_abbrev + "_" + market_abbrev });
  string json = http_call (url, error, "", false, false, "", {}, hurry, false, "");
  // Error handling.
  // Check on errors specific to Cryptopia.
  cryptopia_json_check (json, error);
  if (!error.empty ()) {
    if (hurry) cryptopia_hurried_call_fail++;
    return;
  }
  // Parse the JSON.
  Object main;
  main.parse (json);
  if (main.has<Boolean>("Success")) {
    if (main.get<Boolean>("Success")) {
      if (main.has<Object>("Data")) {
        Object data = main.get<Object>("Data");
        if (data.has<Array>(direction)) {
          Array buy = data.get<Array>(direction);
          for (size_t i = 0; i < buy.size(); i++) {
            Object values = buy.get <Object> (i);
            if (!values.has<Number>("Price")) continue;
            if (!values.has<Number>("Volume")) continue;
            float Price = values.get<Number>("Price");
            float Volume = values.get<Number>("Volume");
            quantity.push_back (Volume);
            rate.push_back (Price);
          }
        }
        if (hurry) cryptopia_hurried_call_pass++;
        return;
      }
    }
  }
  // Error handling.
  if (hurry) cryptopia_hurried_call_fail++;
  error.append (json);
}


void cryptopia_get_buyers (string market,
                           string coin,
                           vector <float> & quantity,
                           vector <float> & rate,
                           string & error,
                           bool hurry)
{
  cryptopia_get_market_orders (market, coin, "Buy", quantity, rate, error, hurry);
}


void cryptopia_get_sellers (string market,
                            string coin,
                            vector <float> & quantity,
                            vector <float> & rate,
                            string & error,
                            bool hurry)
{
  cryptopia_get_market_orders (market, coin, "Sell", quantity, rate, error, hurry);
}


string cryptopia_limit_trade (string market, string coin, string type, float quantity, float rate, string & json, string & error)
{
  api_call_sequencer (cryptopia_private_api_call_sequencer);
  //cryptopia_private_api_call_mutex.lock ();

  string url = cryptopia_get_private_api_call ({ "SubmitTrade" });
  
  string market_abbrev = cryptopia_get_coin_abbrev (market);
  string coin_abbrev = cryptopia_get_coin_abbrev (coin);

  Object post;
  post << "Market" << coin_abbrev + "/" + market_abbrev;
  post << "Type" << type;
  post << "Rate" << rate;
  post << "Amount" << quantity;
  string post_data = post.json ();
  
  vector <pair <string, string> > headers;
  headers.push_back (make_pair ("Content-Type", "application/json; charset=utf-8"));
  headers.push_back (make_pair ("Authorization", cryptopia_get_authorization (url, post_data)));
  
  json = http_call (url, error, "", false, true, post_data, headers, false, true, "");
  
  //cryptopia_private_api_call_mutex.unlock ();

  cryptopia_json_check (json, error);
  if (!error.empty ()) {
    return "";
  }
  
  // Parse the JSON.
  string order_id = cryptopia_limit_trade_json_parser (json);
  if (!order_id.empty ()) return order_id;
  
  // Error handling.
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
  return "";
}


string cryptopia_limit_buy (string market,
                            string coin,
                            float quantity,
                            float rate,
                            string & json,
                            string & error)
{
  return cryptopia_limit_trade (market, coin, "Buy", quantity, rate, json, error);
}


string cryptopia_limit_sell (string market,
                             string coin,
                             float quantity,
                             float rate,
                             string & json,
                             string & error)
{
  return cryptopia_limit_trade (market, coin, "Sell", quantity, rate, json, error);
}


bool cryptopia_cancel_order (string order_id,
                             string & error)
{
  api_call_sequencer (cryptopia_private_api_call_sequencer);
  //cryptopia_private_api_call_mutex.lock ();

  string url = cryptopia_get_private_api_call ({ "CancelTrade" });
  
  Object post;
  post << "Type" << "Trade";
  post << "OrderId" << stoi (order_id);
  string post_data = post.json ();
  
  vector <pair <string, string> > headers;
  headers.push_back (make_pair ("Content-Type", "application/json; charset=utf-8"));
  headers.push_back (make_pair ("Authorization", cryptopia_get_authorization (url, post_data)));
  
  string json = http_call (url, error, "", false, true, post_data, headers, false, false, "");
  
  //cryptopia_private_api_call_mutex.unlock ();

  cryptopia_json_check (json, error);
  if (!error.empty ()) {
    return false;
  }

  Object main;
  main.parse (json);
  if (main.has<Boolean>("Success")) {
    if (main.get<Boolean>("Success")) {
      return true;
    }
  }
  
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
  return false;
}


string cryptopia_withdrawal_order (string coin,
                                   float quantity,
                                   string address,
                                   string paymentid,
                                   string & json,
                                   string & error)
{
  api_call_sequencer (cryptopia_private_api_call_sequencer);
  //cryptopia_private_api_call_mutex.lock ();

  string url = cryptopia_get_private_api_call ({ "SubmitWithdraw" });
  
  string coin_abbrev = cryptopia_get_coin_abbrev (coin);
  
  Object post;
  post << "Currency" << coin_abbrev;
  post << "Address" << address;
  if (!paymentid.empty ()) {
    post << "PaymentId" << paymentid;
  }
  post << "Amount" << quantity;
  string post_data = post.json ();
  
  vector <pair <string, string> > headers;
  headers.push_back (make_pair ("Content-Type", "application/json; charset=utf-8"));
  headers.push_back (make_pair ("Authorization", cryptopia_get_authorization (url, post_data)));
  
  json = http_call (url, error, "", false, true, post_data, headers, false, true, "");
  
  //cryptopia_private_api_call_mutex.unlock ();

  cryptopia_json_check (json, error);
  if (!error.empty ()) {
    return "";
  }

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
  
  // There have been cases that the Cryptopia API returned this:
  // {"Success":false,"Error":"Nonce has already been used for this request."}
  // One would expect that if this is returned, the limit trade order has not been placed.
  // But in some cases it returns the above error, and also places the limit trade order.
  // So every error of this type needs investigation whether the order was placed or not.
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
  return "";
}


void cryptopia_get_transactions (string type,
                                 vector <string> & order_ids,
                                 vector <string> & coin_abbreviations,
                                 vector <string> & coin_ids,
                                 vector <float> & quantities,
                                 vector <string> & addresses,
                                 vector <string> & transaction_ids,
                                 string & error)
{
  api_call_sequencer (cryptopia_private_api_call_sequencer);
  //cryptopia_private_api_call_mutex.lock ();

  string url = cryptopia_get_private_api_call ({ "GetTransactions" });
  
  Object post;
  post << "Type" << type;
  // Output plenty of transactions since these transaction cover all coins.
  // So a low value may not provide enough history of one particular coin.
  post << "Count" << 1000;
  string post_data = post.json ();
  
  vector <pair <string, string> > headers;
  headers.push_back (make_pair ("Content-Type", "application/json; charset=utf-8"));
  headers.push_back (make_pair ("Authorization", cryptopia_get_authorization (url, post_data)));
  
  string json = http_call (url, error, "", false, true, post_data, headers, false, false, "");
  
  //cryptopia_private_api_call_mutex.unlock ();

  cryptopia_json_check (json, error);
  if (!error.empty ()) {
    return;
  }

  /*
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
   */
  
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
          coin_ids.push_back (cryptopia_get_coin_id (Currency));
          quantities.push_back (Amount);
          addresses.push_back (Address);
          transaction_ids.push_back (TxId);
        }
      }
      return;
    }
  }
  
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
}


void cryptopia_withdrawalhistory (vector <string> & order_ids,
                                  vector <string> & coin_abbreviations,
                                  vector <string> & coin_ids,
                                  vector <float> & quantities,
                                  vector <string> & addresses,
                                  vector <string> & transaction_ids,
                                  string & error)
{
  cryptopia_get_transactions ("Withdraw", order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
}


void cryptopia_deposithistory (vector <string> & order_ids,
                               vector <string> & coin_abbreviations,
                               vector <string> & coin_ids,
                               vector <float> & quantities,
                               vector <string> & addresses,
                               vector <string> & transaction_ids,
                               string & error)
{
  cryptopia_get_transactions ("Deposit", order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids, error);
}


// This is for testing repeating a call after it failed.
void cryptopia_unit_test (int test, string & error)
{
  string call = "GetBalance";
  if (test == 1) call = "CancelTrade";
  
  string url = cryptopia_get_private_api_call ({ call });
  
  string post_data = "{\"Currency\":\"\"}";
  
  vector <pair <string, string> > headers;
  headers.push_back (make_pair ("Content-Type", "application/json; charset=utf-8"));
  headers.push_back (make_pair ("Authorization", cryptopia_get_authorization (url, post_data)));
  
  error.clear ();
  string json = http_call (url, error, "", false, true, post_data, headers, false, false, "");
  cryptopia_json_check (json, error);
  if (!error.empty ()) {
    return;
  }
  
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
  error.append (__FUNCTION__);
  error.append (" ");
  error.append (json);
}


string cryptopia_nonce_has_already_been_used ()
{
  string nonce_error = R"({"Success":false,"Error":"Nonce has already been used for this request."})";
  return nonce_error;
}


string cryptopia_limit_trade_json_parser (const string & json)
{
  Object main;
  main.parse (json);
  if (main.has<Boolean>("Success")) {
    if (main.get<Boolean>("Success")) {
      if (main.has<Object>("Data")) {
        Object data = main.get<Object>("Data");
        // The output can be this:
        // {"Success":true,"Error":null,"Data":{"OrderId":62049648,"FilledOrders":[]}}
        if (data.has<Number>("OrderId")) {
          int order_id = data.get<Number>("OrderId");
          return to_string (order_id);
        }
        // The output can also be this:
        // {"Success":true,"Error":null,"Data":{"OrderId":null,"FilledOrders":[15096400]}}
        if (data.has<Array>("FilledOrders")) {
          Array FilledOrders = data.get<Array>("FilledOrders");
          return trade_order_fulfilled ();
        }
      }
    }
  }

  // The output could be this:
  // {"Success":false,"Error":"Nonce has already been used for this request."}
  // Cryptopia says:
  // What you are experiencing is the result of duplicate requests to the api. The first request has succeeded, hence the order being placed, the second request was identified as a duplicate, which is why you receive the response: {"Success":false,"Error":"Nonce has already been used for this request."}
  // This is protection put in place to avoid duplicate orders being placed and processed through the api.
  // Hope this explains what you are experiencing.
  // Thanks, Cryptopia Support

  // Failed to parse a desired response.
  return "";
}


void cryptopia_get_minimum_trade_amounts (const string & market,
                                          const vector <string> & coins,
                                          map <string, float> & market_amounts,
                                          map <string, float> & coin_amounts,
                                          string & error)
{
  string url = cryptopia_get_public_api_call ({ "GetTradePairs" });
  string json = http_call (url, error, "", false, false, "", {}, false, false, "");
  cryptopia_json_check (json, error);
  if (!error.empty ()) {
    return;
  }
  // {"Success":true,"Message":null,"Data":[{"Id":4909,"Label":"BTC/USDT","Currency":"Bitcoin","Symbol":"BTC","BaseCurrency":"Tether","BaseSymbol":"USDT","Status":"OK","StatusMessage":null,"TradeFee":0.20000000,"MinimumTrade":0.00000001,"MaximumTrade":100000000.0,"MinimumBaseTrade":1.00000000,"MaximumBaseTrade":100000000.0,"MinimumPrice":0.00000001,"MaximumPrice":100000000.0},{"Id":5082,"Label":"BTC/NZDT","Currency":"Bitcoin","Symbol":"BTC","BaseCurrency":"NZed","BaseSymbol":"NZDT","Status":"Paused","StatusMessage":"NZDT MARKETS TEMPORARILY PAUSED","TradeFee":0.20000000,"MinimumTrade":0.00000001,"MaximumTrade":100000000.0,"MinimumBaseTrade":1.00000000,"MaximumBaseTrade":100000000.0,"MinimumPrice":0.00000001,"MaximumPrice":100000000.0},{"Id":100,"Label":"DOT/BTC","Currency":"Dotcoin","Symbol":"DOT","BaseCurrency":"Bitcoin","BaseSymbol":"BTC","Status":"OK","StatusMessage":null,"TradeFee":0.20000000,"MinimumTrade":0.00000001,"MaximumTrade":100000000.0,"MinimumBaseTrade":0.00050000,"MaximumBaseTrade":100000000.0,"MinimumPrice":0.00000001,"MaximumPrice":100000000.0},{"Id":300,"Label":"DOT/LTC","Currency":"Dotcoin","Symbol":"DOT","BaseCurrency":"Litecoin","BaseSymbol":"LTC","Status":"Paused","StatusMessage":"Temporarily Paused","TradeFee":0.20000000,"MinimumTrade":0.00000001,"MaximumTrade":100000000.0,"MinimumBaseTrade":0.01000000,"MaximumBaseTrade":100000000.0,"MinimumPrice":0.00000001,"MaximumPrice":100000000.0},...],"Error":null}
  Object main;
  main.parse (json);
  if (main.has<Boolean>("Success")) {
    if (main.get<Boolean>("Success")) {
      if (main.has<Array>("Data")) {
        Array data = main.get<Array>("Data");
        for (size_t i = 0; i < data.size(); i++) {
          Object values = data.get <Object> (i);
          if (!values.has <String> ("BaseSymbol")) continue;
          string BaseSymbol = values.get <String> ("BaseSymbol");
          string market_id = cryptopia_get_coin_id (BaseSymbol);
          if (market != market_id) continue;
          if (!values.has <String> ("Symbol")) continue;
          string Symbol = values.get <String> ("Symbol");
          string coin_id = cryptopia_get_coin_id (Symbol);
          if (!in_array (coin_id, coins)) continue;
          if (!values.has <Number> ("MinimumTrade")) continue;
          float MinimumTrade = values.get <Number> ("MinimumTrade");
          coin_amounts [coin_id] = MinimumTrade;
          if (!values.has <Number> ("MinimumBaseTrade")) continue;
          float MinimumBaseTrade = values.get <Number> ("MinimumBaseTrade");
          market_amounts [coin_id] = MinimumBaseTrade;
        }
        return;
      }
    }
  }
  error.append (json);
}


int cryptopia_get_hurried_call_passes ()
{
  return cryptopia_hurried_call_pass;
}


int cryptopia_get_hurried_call_fails ()
{
  return cryptopia_hurried_call_fail;
}
