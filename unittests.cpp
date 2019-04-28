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
#include "exchanges.h"
#include "models.h"
#include "bl3p.h"
#include "bittrex.h"
#include "cryptopia.h"
#include "bitfinex.h"
#include "controllers.h"
#include "yobit.h"
#include "poloniex.h"
#include "proxy.h"
#include "sqlite.h"
#include "kraken.h"
#include "traders.h"


void error_message (int line, string func, string desired, string actual)
{
  string difference;
  cout << "Line number:    " << line << endl;
  cout << "Function:       " << func << endl;
  cout << "Desired result: " << desired << endl;
  cout << "Actual result:  " << actual << endl;
}


bool evaluate (int line, string func, string desired, string actual)
{
  if (desired != actual) {
    error_message (line, func, desired, actual);
    return false;
  }
  return true;
}


void evaluate (int line, string func, int desired, int actual)
{
  if (desired != actual) error_message (line, func, to_string (desired), to_string (actual));
}


void evaluate (int line, string func, unsigned int desired, unsigned int actual)
{
  if (desired != actual) error_message (line, func, to_string ((size_t)desired), to_string ((size_t)actual));
}


void evaluate (int line, string func, float desired, float actual)
{
  bool ok = true;
  if (desired < (actual * 0.999999)) ok = false;
  if (desired > (actual * 1.000001)) ok = false;
  if (!ok) error_message (line, func, float2string (desired), float2string (actual));
}


void evaluate (int line, string func, bool desired, bool actual)
{
  if (desired != actual) error_message (line, func, desired ? "true" : "false", actual ? "true" : "false");
}


void evaluate (int line, string func, vector <string> desired, vector <string> actual)
{
  if (desired.size() != actual.size ()) {
    error_message (line, func, to_string ((int)desired.size ()), to_string ((int)actual.size()) + " size mismatch");
    return;
  }
  for (size_t i = 0; i < desired.size (); i++) {
    if (desired[i] != actual[i]) error_message (line, func, desired[i], actual[i] + " mismatch at offset " + to_string (i));
  }
}


void evaluate (int line, string func, vector <int> desired, vector <int> actual)
{
  if (desired.size() != actual.size ()) {
    error_message (line, func, to_string ((int)desired.size ()), to_string ((int)actual.size()) + " size mismatch");
    return;
  }
  for (size_t i = 0; i < desired.size (); i++) {
    if (desired[i] != actual[i]) error_message (line, func, to_string (desired[i]), to_string (actual[i]) + " mismatch at offset " + to_string (i));
  }
}


void evaluate (int line, string func, vector <float> desired, vector <float> actual)
{
  if (desired.size() != actual.size ()) {
    error_message (line, func, to_string ((int)desired.size ()), to_string ((int)actual.size()) + " size mismatch");
    return;
  }
  for (size_t i = 0; i < desired.size (); i++) {
    if (fabs (desired[i] - actual[i]) > 1e-10) {
      error_message (line, func, float2string (desired[i]), float2string (actual[i]) + " mismatch at offset " + to_string (i));
    }
  }
}


void evaluate (int line, string func, map <int, string> desired, map <int, string> actual)
{
  if (desired.size() != actual.size ()) {
    error_message (line, func, to_string ((int)desired.size ()), to_string ((int)actual.size()) + " size mismatch");
    return;
  }
  auto desirediterator = desired.begin ();
  auto actualiterator = actual.begin ();
  for (auto iterator = desired.begin(); iterator != desired.end(); iterator++) {
    evaluate (line, func, desirediterator->first, actualiterator->first);
    evaluate (line, func, desirediterator->second, actualiterator->second);
    desirediterator++;
    actualiterator++;
  }
}


void evaluate (int line, string func, map <string, int> desired, map <string, int> actual)
{
  if (desired.size() != actual.size ()) {
    error_message (line, func, to_string ((int)desired.size ()), to_string ((int)actual.size()) + " size mismatch");
    return;
  }
  auto desirediterator = desired.begin ();
  auto actualiterator = actual.begin ();
  for (auto iterator = desired.begin(); iterator != desired.end(); iterator++) {
    evaluate (line, func, desirediterator->first, actualiterator->first);
    evaluate (line, func, desirediterator->second, actualiterator->second);
    desirediterator++;
    actualiterator++;
  }
}


void evaluate (int line, string func, map <int, int> desired, map <int, int> actual)
{
  if (desired.size() != actual.size ()) {
    error_message (line, func, to_string ((int)desired.size ()), to_string ((int)actual.size()) + " size mismatch");
    return;
  }
  auto desirediterator = desired.begin ();
  auto actualiterator = actual.begin ();
  for (auto iterator = desired.begin(); iterator != desired.end(); iterator++) {
    evaluate (line, func, desirediterator->first, actualiterator->first);
    evaluate (line, func, desirediterator->second, actualiterator->second);
    desirediterator++;
    actualiterator++;
  }
}


void evaluate (int line, string func, map <string, string> desired, map <string, string> actual)
{
  if (desired.size() != actual.size ()) {
    error_message (line, func, to_string ((int)desired.size ()), to_string ((int)actual.size()) + " size mismatch");
    return;
  }
  auto desirediterator = desired.begin ();
  auto actualiterator = actual.begin ();
  for (auto iterator = desired.begin(); iterator != desired.end(); iterator++) {
    evaluate (line, func, desirediterator->first, actualiterator->first);
    evaluate (line, func, desirediterator->second, actualiterator->second);
    desirediterator++;
    actualiterator++;
  }
}


void evaluate (int line, string func, vector <pair<int, string>> desired, vector <pair<int, string>> actual)
{
  if (desired.size() != actual.size ()) {
    error_message (line, func, to_string ((int)desired.size ()), to_string ((int)actual.size()) + " size mismatch");
    return;
  }
  auto desirediterator = desired.begin ();
  auto actualiterator = actual.begin ();
  for (auto iterator = desired.begin(); iterator != desired.end(); iterator++) {
    evaluate (line, func, desirediterator->first, actualiterator->first);
    evaluate (line, func, desirediterator->second, actualiterator->second);
    desirediterator++;
    actualiterator++;
  }
}


void test_trade_quantities_rates ()
{
  string market, exchange;
  float standard = 0;
  bool too_low = false;
  SQL sql (front_bot_ip_address ());

  // Bittrex has a minimum trade value of 0.001 Bitcoins, currently.
  standard = 0.001;
  evaluate (__LINE__, __func__, true, is_dust_trade (bitcoin_id (), 1.0, standard - 0.0001));
  evaluate (__LINE__, __func__, false, is_dust_trade (bitcoin_id (), 1.0, standard + 0.0001));
  
  // Fixing dust trades in the order book.
  market = bitcoin_id ();
  exchange = bittrex_get_exchange ();

  vector <float> quantities = { 0.005, 0.2, 5 };
  vector <float> rates = { 0.01, 0.02, 0.03 };
  fix_dust_trades (market, quantities, rates);
  evaluate (__LINE__, __func__, { 0.205, 5 }, quantities);
  evaluate (__LINE__, __func__, { 0.02, 0.03 }, rates);
  quantities = { 0.06, 0.2, 5 };
  rates = { 0.02, 0.02, 0.03 };
  fix_dust_trades (market, quantities, rates);
  evaluate (__LINE__, __func__, { 0.06, 0.2, 5 }, quantities);
  evaluate (__LINE__, __func__, { 0.02, 0.02, 0.03 }, rates);
  quantities = { 0.0001, 0.0001, 0.0001 };
  rates = { 0.001, 0.002, 0.003 };
  fix_dust_trades (market, quantities, rates);
  evaluate (__LINE__, __func__, { 0.0003 }, quantities);
  evaluate (__LINE__, __func__, { 0.003 }, rates);
  
  // The exchange updates this value over time.
  float min_ripple_size = 14.0;
  
  map <string, float> minimum_trade_sizes = mintrade_get (sql);
  evaluate (__LINE__, __func__, min_ripple_size, minimum_trade_sizes ["bitfinexbitcoinripple"]);

  too_low = trade_size_too_low (minimum_trade_sizes, "bitfinex", "bitcoin", "ripple", min_ripple_size - 0.1);
  evaluate (__LINE__, __func__, true, too_low);
  too_low = trade_size_too_low (minimum_trade_sizes, "bitfinex", "bitcoin", "ripple", min_ripple_size);
  evaluate (__LINE__, __func__, false, too_low);

  quantities = { 2.2, 6.6, 9.9 };
  rates = { 0.01, 0.02, 0.03 };
  fix_too_low_trades (minimum_trade_sizes, "bitfinex", "bitcoin", "ripple", quantities, rates);
  evaluate (__LINE__, __func__, { 18.7 }, quantities);
  evaluate (__LINE__, __func__, { 0.03 }, rates);

  quantities = { 2.2, 6.6 };
  rates = { 0.01, 0.02 };
  fix_too_low_trades (minimum_trade_sizes, "bitfinex", "bitcoin", "ripple", quantities, rates);
  evaluate (__LINE__, __func__, { 8.8 }, quantities);
  evaluate (__LINE__, __func__, { 0.02 }, rates);
  
  quantities = { 10, 20, 40 };
  rates = { 0.1, 0.2, 0.4 };
  fix_rate_for_quantity (22, quantities, rates);
  evaluate (__LINE__, __func__, { 30, 40 }, quantities);
  evaluate (__LINE__, __func__, { 0.2, 0.4 }, rates);

  quantities = { };
  rates = { };
  fix_rate_for_quantity (22, quantities, rates);
  evaluate (__LINE__, __func__, { }, quantities);
  evaluate (__LINE__, __func__, { }, rates);

  quantities = { 10, 20, 40 };
  rates = { 0.1, 0.2, 0.4 };
  fix_rate_for_quantity (10, quantities, rates);
  evaluate (__LINE__, __func__, { 10, 20, 40 }, quantities);
  evaluate (__LINE__, __func__, { 0.1, 0.2, 0.4 }, rates);
  
  quantities = { 1, 2 };
  rates = { 1, 2 };
  evaluate (__LINE__, __func__, true, order_book_is_good (quantities, rates, false));
  evaluate (__LINE__, __func__, false, order_book_is_good (quantities, rates, true));
}


void test_exchange_deposit_history ()
{
  vector <string> order_ids;
  vector <string> coin_abbreviations;
  vector <string> coin_ids;
  vector <float> quantities;
  vector <string> addresses;
  vector <string> transaction_ids;
  exchange_deposithistory ("cryptopia", "", order_ids, coin_abbreviations, coin_ids, quantities, addresses, transaction_ids);
  if (addresses.empty ()) evaluate (__LINE__, __func__, "should have values", "no values");
}


void test_transfers_database ()
{
  // The data to test.
  string exchange_withdraw = "exchange1";
  string exchange_deposit = "exchange_deposit";
  string exchange3 = "exchange3";
  string exchange4 = "exchange4";
  string coin_abbreviation = "ABBREV";
  string order_id = "unittest-order";
  float quantity = 1000;
  float rxquantity = quantity - 0.11;
  string txid = "txid";
  string rxid = "rxid";
  
  SQL sql (front_bot_ip_address ());
  vector <string> order_ids;
  vector <string> exchanges;
  vector <string> coins;
  vector <float> quantities;
  vector <string> transaction_ids;
  vector <string> dummy;

  // There should be no active orders with the above data yet.
  int active_count = transfers_get_active_orders (sql, coin_abbreviation, {exchange_withdraw, exchange_deposit});
  evaluate (__LINE__, __func__, 0, active_count);
  active_count = transfers_get_active_orders (sql, coin_abbreviation, {exchange_withdraw, exchange_deposit, exchange3, exchange4});
  evaluate (__LINE__, __func__, 0, active_count);
  
  // The seconds since the most recently placed order should now be an hour.
  int elapsed_seconds = transfers_get_seconds_since_last_order (sql, coin_abbreviation, {exchange_withdraw, exchange_deposit});
  evaluate (__LINE__, __func__, 3600, elapsed_seconds);
  elapsed_seconds = transfers_get_seconds_since_last_order (sql, coin_abbreviation, {exchange_deposit, exchange_withdraw});
  evaluate (__LINE__, __func__, 3600, elapsed_seconds);
  elapsed_seconds = transfers_get_seconds_since_last_order (sql, coin_abbreviation, {exchange_deposit, exchange_withdraw, exchange4});
  evaluate (__LINE__, __func__, 3600, elapsed_seconds);
  
  // The known exchanges should not be in the non-executed orders.
  // Since this works with live data, there could be other non-executed orders.
  transfers_get_non_executed_orders (sql, order_ids, exchanges, coins, dummy, quantities);
  evaluate (__LINE__, __func__, false, in_array (exchange_withdraw, exchanges));
  evaluate (__LINE__, __func__, false, in_array (coin_abbreviation, coins));
  evaluate (__LINE__, __func__, false, in_array (quantity, quantities));

  // Enter the order, and check that it was entered.
  transfers_enter_order (sql, order_id, exchange_withdraw, coin_abbreviation, quantity, exchange_deposit, coin_abbreviation, "address", "");
  transfers_get_non_executed_orders (sql, order_ids, exchanges, coins, dummy, quantities);
  evaluate (__LINE__, __func__, true, in_array (exchange_withdraw, exchanges));
  evaluate (__LINE__, __func__, true, in_array (coin_abbreviation, coins));
  evaluate (__LINE__, __func__, true, in_array (quantity, quantities));

  // There should be an active order now.
  active_count = transfers_get_active_orders (sql, coin_abbreviation, {exchange_withdraw, exchange_deposit});
  evaluate (__LINE__, __func__, 1, active_count);
  // The order of the exchanges should make no difference.
  active_count = transfers_get_active_orders (sql, coin_abbreviation, {exchange_deposit, exchange_withdraw});
  evaluate (__LINE__, __func__, 1, active_count);
  
  // The seconds since the most recently placed order should be 0.
  elapsed_seconds = transfers_get_seconds_since_last_order (sql, coin_abbreviation, {exchange_withdraw, exchange_deposit});
  evaluate (__LINE__, __func__, 0, elapsed_seconds);
  elapsed_seconds = transfers_get_seconds_since_last_order (sql, coin_abbreviation, {exchange_deposit, exchange_withdraw});
  evaluate (__LINE__, __func__, 0, elapsed_seconds);
  
  // Move the seconds for this order back and check.
  int standard_seconds = 11;
  transfers_update_seconds_tests (sql, 0 - standard_seconds);
  elapsed_seconds = transfers_get_seconds_since_last_order (sql, coin_abbreviation, {exchange_withdraw, exchange_deposit});
  if ((elapsed_seconds < standard_seconds) || (elapsed_seconds > standard_seconds + 1)) {
    evaluate (__LINE__, __func__, 11, elapsed_seconds);
  }

  // Simulate the system setting this order as executed on the originating exchange.
  transfers_set_order_executed (sql, exchange_withdraw, order_id, txid);

  // There should be no non-executed orders anymore, with the above data.
  transfers_get_non_executed_orders (sql, order_ids, exchanges, coins, dummy, quantities);
  evaluate (__LINE__, __func__, false, in_array (exchange_withdraw, exchanges));
  evaluate (__LINE__, __func__, false, in_array (coin_abbreviation, coins));
  evaluate (__LINE__, __func__, false, in_array (quantity, quantities));

  // There's still an active transfer.
  active_count = transfers_get_active_orders (sql, coin_abbreviation, {exchange_withdraw, exchange_deposit});
  evaluate (__LINE__, __func__, 1, active_count);
  
  // Check the elapsed seconds since order placement.
  elapsed_seconds = transfers_get_seconds_since_last_order (sql, coin_abbreviation, {exchange_withdraw, exchange_deposit});
  if ((elapsed_seconds < standard_seconds) || (elapsed_seconds > standard_seconds + 1)) {
    evaluate (__LINE__, __func__, 11, elapsed_seconds);
  }
  
  // Simulate the system setting the order as arrived, initially.
  transfers_set_order_arrived (sql, exchange_deposit, order_id, rxid, rxquantity);
  
  // Check that the transaction ID has been recorded.
  transaction_ids = transfers_get_rx_ids (sql);
  evaluate (__LINE__, __func__, true, in_array (rxid, transaction_ids));
  
  // No active orders anymore, with the above data.
  active_count = transfers_get_active_orders (sql, coin_abbreviation, {exchange_withdraw, exchange_deposit});
  evaluate (__LINE__, __func__, 0, active_count);
  // Swapping exchanges makes no difference.
  active_count = transfers_get_active_orders (sql, coin_abbreviation, {exchange_deposit, exchange_withdraw});
  evaluate (__LINE__, __func__, 0, active_count);
  
  // Check the elapsed seconds since order placement.
  // Allow for one second since the current time moves on.
  elapsed_seconds = transfers_get_seconds_since_last_order (sql, coin_abbreviation, {exchange_withdraw, exchange_deposit});
  if ((elapsed_seconds < standard_seconds) || (elapsed_seconds > standard_seconds + 1)) {
    evaluate (__LINE__, __func__, 11, elapsed_seconds);
  }
}


void test_getting_coin_naming ()
{
  vector <string> coin_ids = exchange_get_coin_ids (bittrex_get_exchange ());
  size_t size = coin_ids.size();
  if ((size < 100) || (size > 300)) {
    evaluate (__LINE__, __func__, "Bittrex has about 200 coins", "Real coin count: " + to_string (size));
  }
  
  coin_ids = exchange_get_coin_ids ("");
  size = coin_ids.size();
  if ((size < 600) || (size > 1200)) {
    evaluate (__LINE__, __func__, "All exchange have many distinct coins", "Real coin count: " + to_string (size));
  }
  
  string coin_id = exchange_get_coin_id (bittrex_get_exchange (), "BTC");
  evaluate (__LINE__, __func__, "bitcoin", coin_id);
  
  coin_id = exchange_get_coin_id (bittrex_get_exchange (), "Bitcoin");
  evaluate (__LINE__, __func__, "bitcoin", coin_id);
  
  string coin_name = exchange_get_coin_name (bitfinex_get_exchange (), "BTC");
  evaluate (__LINE__, __func__, "Bitcoin", coin_name);
  
  coin_name = exchange_get_coin_name (bittrex_get_exchange (), "Bitcoin");
  evaluate (__LINE__, __func__, "Bitcoin", coin_name);
  
  coin_name = exchange_get_coin_name (cryptopia_get_exchange (), "bitcoin");
  evaluate (__LINE__, __func__, "Bitcoin", coin_name);
  
  string abbrev = exchange_get_coin_abbrev (bitfinex_get_exchange (), "BTC");
  evaluate (__LINE__, __func__, "BTC", abbrev);
  
  abbrev = exchange_get_coin_abbrev (bittrex_get_exchange (), "Bitcoin");
  evaluate (__LINE__, __func__, "BTC", abbrev);
  
  abbrev = exchange_get_coin_abbrev (cryptopia_get_exchange (), "bitcoin");
  evaluate (__LINE__, __func__, "BTC", abbrev);
  
  coin_id = exchange_get_coin_id ("", "");
  evaluate (__LINE__, __func__, "", coin_id);

  coin_id = exchange_get_coin_id ("", "XZC");
  evaluate (__LINE__, __func__, "zcoin", coin_id);
}


void test_database_wallets ()
{
  SQL sql (front_bot_ip_address ());
  
  // Test updating the amounts to withdraw.
  {
    float amount = 2.55;
    wallets_update_withdrawal_amount (sql, "bittrex", "neo", amount);
    evaluate (__LINE__, __func__, 2.025, amount);
    amount = 3.7;
    wallets_update_withdrawal_amount (sql, "cryptopia", "neo", amount);
    evaluate (__LINE__, __func__, 3.000, amount);
  }
  
  // Test temporarily disabling a wallet then enabling it again after a while.
  {
    // Enter wallet details for testing.
    string exchange = "unit test";
    string coin_id = "testing coin";
    wallets_set_deposit_details (sql, exchange, coin_id, "address", "payment_id");
    
    // Test initial values.
    bool trading = wallets_get_trading (sql, exchange, coin_id);
    evaluate (__LINE__, __func__, false, trading);
    vector <string> exchanges = wallets_get_trading_exchanges (sql, coin_id);
    evaluate (__LINE__, __func__, {}, exchanges);

    // Enable the exchange for trading and check on it.
    wallets_set_trading (sql, exchange, coin_id, true);
    trading = wallets_get_trading (sql, exchange, coin_id);
    evaluate (__LINE__, __func__, true, trading);
    exchanges = wallets_get_trading_exchanges (sql, coin_id);
    evaluate (__LINE__, __func__, {exchange}, exchanges);

    // Test setting and getting values.
    int seconds = seconds_since_epoch ();
    wallets_set_trading (sql, exchange, coin_id, seconds);
    trading = wallets_get_trading (sql, exchange, coin_id);
    evaluate (__LINE__, __func__, true, trading);

    // Let the time progress a bit, it should still be non-trading.
    wallets_set_trading (sql, exchange, coin_id, seconds + 200);
    exchanges = wallets_get_trading_exchanges (sql, coin_id);
    evaluate (__LINE__, __func__, {}, exchanges);

    // Enable the wallet again for trading, simulating the time has expired.
    wallets_set_trading (sql, exchange, coin_id, seconds - 1);
    exchanges = wallets_get_trading_exchanges (sql, coin_id);
    evaluate (__LINE__, __func__, {exchange}, exchanges);

    // Simulate the current time is long past the time the wallet was temporarily disabled.
    // It should still be trading.
    wallets_set_trading (sql, exchange, coin_id, seconds - 1000);
    exchanges = wallets_get_trading_exchanges (sql, coin_id);
    evaluate (__LINE__, __func__, {exchange}, exchanges);

    // Remove this wallet again.
    wallets_delete_deposit_details (sql, exchange, coin_id);
  }
  
  // Test getting or calculating the minimum withdrawal amount.
  {
    string sminimum = wallets_get_minimum_withdrawal_amount (sql, "cryptopia", "ark");
    evaluate (__LINE__, __func__, "", sminimum);
    float fminimum = get_minimum_withdrawal_amount ("ark", "cryptopia");
    evaluate (__LINE__, __func__, 0.0, fminimum);
    fminimum = get_minimum_withdrawal_amount ("bitcoin", "cryptopia");
    evaluate (__LINE__, __func__, 0.050, fminimum);
    sminimum = wallets_get_minimum_withdrawal_amount (sql, "cryptopia", "bitcoin");
    evaluate (__LINE__, __func__, "0.05", sminimum);
    sminimum = wallets_get_minimum_withdrawal_amount (sql, "bitfinex", "ethereum");
    evaluate (__LINE__, __func__, "USD250", sminimum);
    fminimum = get_minimum_withdrawal_amount ("ethereum", "bitfinex");
    float min = 0.15;
    float max = 0.80;
    if ((fminimum < min) || (fminimum > max)) {
      evaluate (__LINE__, __func__, "Between " + to_string (min) + " and " + to_string (max), to_string (fminimum));
    }
    fminimum = get_minimum_withdrawal_amount ("omisego", "bitfinex");
    min = 10;
    max = 40;
    if ((fminimum < 10) || (fminimum > 40)) {
      evaluate (__LINE__, __func__, "Between " + to_string (min) + " and " + to_string (max), to_string (fminimum));
    }
  }
}


void test_arbitrage_processor ()
{
  // The output writer.
  to_output master_output ("");
  to_output * output = nullptr;
  //output = &master_output;

  // Test correct theoretical operation.
  {
    // The market where to trade on.
    string market = bitcoin_id ();
    // The ID of the coin to be tested for trading.
    string coin_id = "verge";
    // The quantity of the coin to be tested for trading.
    float quantity;
    // Minimum price per coin one exchange asks.
    float minimum_ask = 0.00000158;
    // Maximum price per coin another exchange bids.
    float maximum_bid = 0.00000161;
    // The balances on the exchange where to buy.
    float asking_exchange_balance_btc = 0.303585;
    float asking_exchange_balance_ltc = 0.33;
    // The balance on the exchange where to sell.
    float bidding_exchange_balance_verge = 90076.9;
    // The exchange that asks, that's where to buy the coin.
    string asking_exchange = cryptopia_get_exchange ();
    // The exchange that bids, here's where to sell the coin.
    string bidding_exchange = bittrex_get_exchange ();
    // Flags for whether balances are too low to trade.
    bool base_market_coin_balance_too_low;
    bool coin_balance_too_low;
    // The minimum trade sizes.
    map <string, float> minimum_trade_sizes;
    // The order books.
    vector <float> buyer_quantities;
    vector <float> buyer_rates;
    vector <float> seller_quantities;
    vector <float> seller_rates;

    // Standard trade: It takes the lowest commonly available quantity for trading.
    float bid_quantity = 1360.6;
    float ask_quantity = 4000;
    buyer_quantities = { bid_quantity, 0 };
    buyer_rates = { maximum_bid, 0 };
    seller_quantities = { ask_quantity, 0 };
    seller_rates = { minimum_ask, 0 };
    arbitrage_processor (output, market, coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_btc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, bid_quantity, quantity);
    // Same behaviour on another market.
    arbitrage_processor (output, litecoin_id (), coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_ltc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, bid_quantity, quantity);
    
    // A limited order book updates the rates.
    buyer_quantities = { bid_quantity * 0.9f, bid_quantity, 0 };
    buyer_rates = { maximum_bid, maximum_bid * 0.95f, 0 };
    seller_quantities = { bid_quantity * 0.9f, ask_quantity, 0 };
    seller_rates = { minimum_ask, minimum_ask * 1.05f, 0 };
    float holdermax = maximum_bid;
    float holdermin = minimum_ask;
    arbitrage_processor (output, market, coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_btc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, bid_quantity, quantity);
    evaluate (__LINE__, __func__, 0.0015264425, 1000 * maximum_bid);
    evaluate (__LINE__, __func__, 0.0016623196, 1000 * minimum_ask);
    maximum_bid = holdermax;
    minimum_ask = holdermin;
    
    // Dust trade: Cancelled.
    bid_quantity = 56.6;
    ask_quantity = 200;
    buyer_quantities = { bid_quantity, 0 };
    buyer_rates = { maximum_bid, 0 };
    seller_quantities = { ask_quantity, 0 };
    seller_rates = { minimum_ask, 0 };
    arbitrage_processor (output, market, coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_btc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, 0.0, quantity);
    // Same behaviour on another market.
    arbitrage_processor (output, litecoin_id (), coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_ltc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, 0.0, quantity);
    
    // Sufficient available Bitcoins or base market coins to buy the intended amount.
    bid_quantity = 50000.5;
    ask_quantity = 60000.6;
    buyer_quantities = { bid_quantity, 0 };
    buyer_rates = { maximum_bid, 0 };
    seller_quantities = { ask_quantity, 0 };
    seller_rates = { minimum_ask, 0 };
    arbitrage_processor (output, market, coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_btc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, bid_quantity, quantity);
    // Same behaviour on the Litecoin market.
    arbitrage_processor (output, litecoin_id (), coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_ltc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, bid_quantity, quantity);
    
    // Not enough Bitcoins available: Reduce the quantity to trade.
    base_market_coin_balance_too_low = false;
    float old_value = asking_exchange_balance_btc;
    asking_exchange_balance_btc = 0.01;
    arbitrage_processor (output, market, coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_btc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, 5918.3583984375, quantity);
    evaluate (__LINE__, __func__, false, base_market_coin_balance_too_low);
    asking_exchange_balance_btc = old_value;
    
    // Not enough Litecoins available: Reduce the quantity to trade.
    base_market_coin_balance_too_low = false;
    old_value = asking_exchange_balance_ltc;
    asking_exchange_balance_ltc = 0.01;
    arbitrage_processor (output, litecoin_id (), coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_ltc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, 5918.3583984375, quantity);
    evaluate (__LINE__, __func__, false, base_market_coin_balance_too_low);
    asking_exchange_balance_ltc = old_value;
    
    // Too few Bitcoins to be even able to trade: Flag is set for too low balance.
    old_value = asking_exchange_balance_btc;
    asking_exchange_balance_btc = 0.0001;
    base_market_coin_balance_too_low = false;
    arbitrage_processor (output, market, coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_btc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, 0.0, quantity);
    evaluate (__LINE__, __func__, true, base_market_coin_balance_too_low);
    asking_exchange_balance_btc = old_value;
    
    // Too few Litecoins to be even able to trade: Flag is set for too low balance.
    old_value = asking_exchange_balance_ltc;
    asking_exchange_balance_ltc = 0.0001;
    base_market_coin_balance_too_low = false;
    arbitrage_processor (output, litecoin_id (), coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_ltc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, 0.0, quantity);
    evaluate (__LINE__, __func__, true, base_market_coin_balance_too_low);
    asking_exchange_balance_ltc = old_value;
    
    // Not high enough coin balance to sell the desired amount: Reduce quantity to trade.
    bid_quantity = 50000.5;
    ask_quantity = 60000.6;
    buyer_quantities = { bid_quantity, 0 };
    buyer_rates = { maximum_bid, 0 };
    seller_quantities = { ask_quantity, 0 };
    seller_rates = { minimum_ask, 0 };
    coin_balance_too_low = false;
    old_value = bidding_exchange_balance_verge;
    bidding_exchange_balance_verge = 10000.1;
    arbitrage_processor (output, market, coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_btc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, 9500.094727, quantity);
    evaluate (__LINE__, __func__, false, coin_balance_too_low);
    bidding_exchange_balance_verge = old_value;
    
    // Trade too small: Dust trade: Cancel trade.
    bid_quantity = 50.5;
    ask_quantity = 60.6;
    coin_balance_too_low = false;
    arbitrage_processor (output, market, coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_btc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, 0.0, quantity);
    evaluate (__LINE__, __func__, true, coin_balance_too_low);
    
    // Trade would normally go through but because the exchanges have set minimum trade amounts, it is cancelled.
    bid_quantity = 50000;
    ask_quantity = 60000;
    minimum_trade_sizes [asking_exchange + market + coin_id] = 55000;
    minimum_trade_sizes [bidding_exchange + market + coin_id] = 56000;
    coin_balance_too_low = false;
    arbitrage_processor (output, market, coin_id, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_btc, bidding_exchange_balance_verge, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, 0.0, quantity);
    evaluate (__LINE__, __func__, false, coin_balance_too_low);
  }

  // Testing a real-life limit buy failure at Bitfinex.
  {
    // Begin arbitrage pivx @ bitcoin
    // cryptopia bids 0.0008022800 for 1.8696715832 pivx @ bitcoin
    // bittrex asks 0.0007830000 for 44.3278999329 pivx @ bitcoin
    // Correction to reduce the amount of open buy orders bittrex asks 0.0007837830 for 44.3278999329 pivx @ bitcoin
    // Correction to reduce the amount of open sell orders cryptopia bids 0.0008014777 for 1.8696715832 pivx @ bitcoin
    // Arbitrage difference for pivx @ bitcoin is 2.4623260498 %
    // Available for trade is 1.8696715832 pivx @ bitcoin
    // Funds available to buy with is 0.001311 bitcoin
    // The wallet on the exchange where to buy has only 0.0013107918 available bitcoin
    // The available bitcoin can buy only 1.5887715816 pivx
    // The amount to buy was reduced to 1.5887715816 pivx
    // Funds available to sell with is 31.643251 pivx
    // Will buy 1.5887715816 pivx @ bitcoin on bittrex at rate 0.0007837830 and sell it on cryptopia at rate 0.0008014777 gain 0.000022 bitcoin
    // Balances before trade:
    // bittrex bitcoin total 1.0500789881 available 0.0013107918 reserved 0.2307647914 unconfirmed 0.8180033565
    // bittrex pivx total 41.8166923523 available 41.8166923523
    // cryptopia bitcoin total 1.8433905840 available 1.2968125343 reserved 0.5465781093
    // cryptopia pivx total 32.3422088623 available 31.6432514191 reserved 0.5465781093
    // Balances after trade:
    // bittrex bitcoin total 1.0488337278 available 0.0000655396 reserved 0.2307647914 unconfirmed 0.8180033565
    // bittrex pivx total 41.8166923523 available 41.8166923523
    // cryptopia bitcoin total 1.8433905840 available 1.2968125343 reserved 0.5465781093
    // cryptopia pivx total 30.7534370422 available 30.0544795990 reserved 0.5465781093
    // Limit buy JSON: {"success":false,"message":"INSUFFICIENT_FUNDS","result":null}
    // Limit sell JSON: {"Success":true,"Error":null,"Data":{"OrderId":null,"FilledOrders":[29930120,29930121]}}
    // pivx @ bitcoin on bittrex buy order id
    // Failed to buy 1.5887715816 pivx @ bitcoin on bittrex at rate 0.0007837830
    // pivx @ bitcoin at cryptopia sell order id fulfilled
    // End arbitrage pivx @ bitcoin
    
    // The above was due to a logical error in the arbitrage processor.
    // This error was fixed.
    // Regression test is below:
    string market = bitcoin_id ();
    string coin = "pivx";
    float quantity;
    float ask_quantity = 44.3278999329;
    float bid_quantity = 1.8696715832;
    float minimum_ask = 0.0007830000;
    float maximum_bid = 0.0008022800;
    float asking_exchange_balance_bitcoin = 0.0013107918;
    float bidding_exchange_balance_pivx = 41.8166923523;
    string asking_exchange = bittrex_get_exchange ();
    string bidding_exchange = cryptopia_get_exchange ();
    bool base_market_coin_balance_too_low;
    bool coin_balance_too_low;
    map <string, float> minimum_trade_sizes;
    vector <float>  buyer_quantities = { bid_quantity, 0 };
    vector <float> buyer_rates = { maximum_bid, 0 };
    vector <float> seller_quantities = { ask_quantity, 0 };
    vector <float> seller_rates = { minimum_ask, 0 };
    arbitrage_processor (output, market, coin, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_bitcoin, bidding_exchange_balance_pivx, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, 1.5536954403, quantity);
  }
  
  // Regression testing a fix for a real-life limit trade failure at Bitfinex.
  {
    // Begin arbitrage okcash @ bitcoin
    // cryptopia bids 0.0000365300 for 134.7213592529 okcash @ bitcoin
    // bittrex asks 0.0000353200 for 851.5751953125 okcash @ bitcoin
    // Correction to reduce the amount of open buy orders bittrex asks 0.0000353553 for 851.5751953125 okcash @ bitcoin
    // Correction to reduce the amount of open sell orders cryptopia bids 0.0000364935 for 134.7213592529 okcash @ bitcoin
    // Arbitrage difference for okcash @ bitcoin is 3.4258193970 %
    // Available for trade is 134.7213592529 okcash @ bitcoin
    // Funds available to buy with is 0.001602 bitcoin
    // The wallet on the exchange where to buy has only 0.0016020326 available bitcoin
    // The available bitcoin can buy only 41.7041969299 okcash
    // The amount to buy was reduced to 41.7041969299 okcash
    // Funds available to sell with is 383.875183 okcash
    // Will buy 41.7041969299 okcash @ bitcoin on bittrex at rate 0.0000353553 and sell it on cryptopia at rate 0.0000364935 gain 0.000041 bitcoin
    // Balances before trade:
    // bittrex bitcoin total 1.1123087406 available 0.0001305627 reserved 0.0557117425 unconfirmed 1.0564664602
    // bittrex okcash total 284.0751953125 available 284.0751953125
    // cryptopia bitcoin total 1.6731257439 available 1.5451061726 reserved 0.1280195415
    // cryptopia okcash total 383.8751831055 available 383.8751831055
    // Balances after trade:
    // bittrex bitcoin total 1.1108342409 available -0.0013439027 reserved 0.0557117425 unconfirmed 1.0564664602
    // bittrex okcash total 284.0751953125 available 284.0751953125
    // cryptopia bitcoin total 1.6731257439 available 1.5451061726 reserved 0.1280195415
    // cryptopia okcash total 342.1709899902 available 342.1709899902
    // Limit buy JSON: {"success":false,"message":"INSUFFICIENT_FUNDS","result":null}
    // Limit sell JSON: {"Success":true,"Error":null,"Data":{"OrderId":null,"FilledOrders":[31492785,31492786]}}
    // okcash @ bitcoin on bittrex buy order id
    // Failed to buy 41.7041969299 okcash @ bitcoin on bittrex at rate 0.0000353553
    // okcash @ bitcoin at cryptopia sell order id fulfilled
    // End arbitrage okcash @ bitcoin

    string market = bitcoin_id ();
    string coin = "okcash";
    float quantity;
    float ask_quantity = 851.5751953125;
    float bid_quantity = 134.7213592529;
    float minimum_ask = 0.0000353200;
    float maximum_bid = 0.0000365300;
    float asking_exchange_balance_bitcoin = 0.0016020326;
    float bidding_exchange_balance_pivx = 383.8751831055;
    string asking_exchange = bittrex_get_exchange ();
    string bidding_exchange = cryptopia_get_exchange ();
    bool base_market_coin_balance_too_low;
    bool coin_balance_too_low;
    map <string, float> minimum_trade_sizes;
    vector <float>  buyer_quantities = { bid_quantity, 0 };
    vector <float> buyer_rates = { maximum_bid, 0 };
    vector <float> seller_quantities = { ask_quantity, 0 };
    vector <float> seller_rates = { minimum_ask, 0 };
    arbitrage_processor (output, market, coin, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_bitcoin, bidding_exchange_balance_pivx, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, 41.7041969299, quantity);
  }

  // Regression testing a fix for a real-life arbitrage trade failure at the Euro market.
  {
    // Arbitrage bitcoin @ euro @ bl3p and kraken begin
    // kraken bids 6550.0000000000 for 2.0199999809 bitcoin @ euro
    // bl3p asks 6486.0097656250 for 1.4231244326 bitcoin @ euro
    // Difference: 0.986588 %
    // Available for trade is 1.4231244326 bitcoin @ euro
    // Correction to reduce the amount of open buy orders bl3p asks 6492.4956054688 for 1.4231244326 bitcoin @ euro
    // Correction to reduce the amount of open sell orders kraken bids 6543.4501953125 for 2.0199999809 bitcoin @ euro
    // Arbitrage difference is 0.7848205566 %
    // Funds available to buy with is 93.247551 euro
    // The wallet on the exchange where to buy has only 93.2475509644 available euro
    // The available euro can buy only 0.0135379918 bitcoin
    // The amount to buy was reduced to 0.0135379918 bitcoin
    // Funds available to sell with is 0.000630 bitcoin
    // The exchange where to sell has only 0.0006302520 available bitcoin
    // The amount to sell was reduced to 0.0005987394 bitcoin
    // Will buy 0.0005987394 bitcoin @ euro on bl3p at rate 6492.4956054688 and sell it on kraken at rate 6543.4501953125 gain 0.009037 euro
    // Balances before trade:
    // bl3p euro total 93.2475509644 available 93.2475509644
    // bl3p bitcoin total 0.9946509004 available 0.9946509004
    // kraken euro total 2882.8537597656 available 2882.8537597656
    // kraken bitcoin total 0.0006302520 available 0.0006302520
    // Balances after trade:
    // bl3p euro total 89.3602371216 available 89.3602371216
    // bl3p bitcoin total 0.9946509004 available 0.9946509004
    // kraken euro total 2882.8537597656 available 2882.8537597656
    // kraken bitcoin total 0.0000315126 available 0.0000315126
    // Limit buy JSON: { "result": "success", "data": {"order_id": 27490953} }
    // Limit sell JSON: {"error":["EGeneral:Invalid arguments:volume"]}
    // API call error exchange_limit_sell kraken euro bitcoin  {"error":["EGeneral:Invalid arguments:volume"]}
    // {"error":["EGeneral:Invalid arguments:volume"]}
    // Follow-up on sell 0.000599 bitcoin @ euro at rate 6543.4501953125 on kraken error {"error":["EGeneral:Invalid arguments:volume"]} JSON {"error":["EGeneral:Invalid arguments:volume"]} order id
    // Pausing trading this coin at this exchange for an hour
    // Currently trading exchanges
    // bl3p kraken
    // Trading exchanges after removing one
    // bl3p
    // Follow-up on buy 0.000599 bitcoin @ euro at rate 6492.4956054688 on bl3p error  JSON { "result": "success",  "data": {"order_id": 27490953} } order id 27490953
    // Currently trading exchanges
    // bl3p
    // Trading exchanges after removing one
    // Trading bitcoin @ euro @ kraken was disabled for 60 minutes
    // Arbitrage bitcoin @ euro @ bl3p and kraken end
    
    SQL sql (front_bot_ip_address ());
    string market = euro_id ();
    string coin = "bitcoin";
    float quantity;
    float ask_quantity = 1.4231244326;
    float bid_quantity = 2.0199999809;
    float minimum_ask = 6486.0097656250;
    float maximum_bid = 6550.0000000000;
    float asking_exchange_balance_market = 93.247551;
    float bidding_exchange_balance_coin = 0.000630;
    string asking_exchange = kraken_get_exchange ();
    string bidding_exchange = bl3p_get_exchange ();
    bool base_market_coin_balance_too_low;
    bool coin_balance_too_low;
    map <string, float> minimum_trade_sizes = mintrade_get (sql);
    vector <float>  buyer_quantities = { bid_quantity, 0 };
    vector <float> buyer_rates = { maximum_bid, 0 };
    vector <float> seller_quantities = { ask_quantity, 0 };
    vector <float> seller_rates = { minimum_ask, 0 };
    arbitrage_processor (output, market, coin, quantity, bid_quantity, ask_quantity, minimum_ask, maximum_bid, asking_exchange_balance_market, bidding_exchange_balance_coin, asking_exchange, bidding_exchange, base_market_coin_balance_too_low, coin_balance_too_low, minimum_trade_sizes, buyer_quantities, buyer_rates, seller_quantities, seller_rates);
    evaluate (__LINE__, __func__, 0.0000000000, quantity);
  }

}


void test_coin_balancing ()
{
  string bittrex = "bittrex";
  string cryptopia = "cryptopia";
  string bitfinex = "bitfinex";
  string bl3p = "bl3p";
  string tradesatoshi = "tradesatoshi";

  // Test coin balancing over two exchanges.
  {
    // Data to test.
    string coin_id = "verge";
    vector <string> exchanges = { bittrex, cryptopia };

    // Store the balances in the memory cache as total / available / reserved / unconfirmed.
    exchange_set_balance_cache (bittrex, "bitcoin", 0.322178, 0, 0, 0);
    exchange_set_balance_cache (bittrex, "influxcoin", 115.864296, 0, 0, 0);
    exchange_set_balance_cache (bittrex, "nexus", 48.774998, 0, 0, 0);
    exchange_set_balance_cache (bittrex, "verge", 161402.750000, 0, 0, 0);
    exchange_set_balance_cache (cryptopia, "bitcoin", 1.081267, 0, 0, 0);
    exchange_set_balance_cache (cryptopia, "influxcoin", 145.620178, 0, 0, 0);
    exchange_set_balance_cache (cryptopia, "nexus", 48.575001, 0, 0, 48.575001);
    exchange_set_balance_cache (cryptopia, "verge", 161402.562500, 0, 0, 0);

    float cryptopia_total_verge_balance, cryptopia_unconfirmed_verge_balance;
    exchange_get_balance_cached (cryptopia, "verge", &cryptopia_total_verge_balance, nullptr, nullptr, &cryptopia_unconfirmed_verge_balance);
    
    int active_transfer_count = 0;
    int last_transfer_elapsed_seconds = 1000;
    // Processor output storage.
    vector <string> exchanges_withdraw;
    vector <string> exchanges_deposit;
    vector <float> transfer_quantities;

    // Everything is in balance: No transfer orders.
    redistribute_coin_processor (NULL, coin_id, false, exchanges,
                                 active_transfer_count, last_transfer_elapsed_seconds,
                                 exchanges_withdraw, exchanges_deposit, transfer_quantities);
    evaluate (__LINE__, __func__, 0, exchanges_withdraw.size());
    
    // Check it transfers verge from bittrex to cryptopia
    // since the balances on both exchanges differ more than the set threshold.
    exchange_set_balance_cache (cryptopia, "verge", 100.1, 0, 0, 0);
    redistribute_coin_processor (NULL, coin_id, false, exchanges,
                            active_transfer_count, last_transfer_elapsed_seconds,
                            exchanges_withdraw, exchanges_deposit, transfer_quantities);
    evaluate (__LINE__, __func__, 1, exchanges_withdraw.size());
    if (exchanges_withdraw.size() == 1) {
      evaluate (__LINE__, __func__, "bittrex", exchanges_withdraw[0]);
      evaluate (__LINE__, __func__, "cryptopia", exchanges_deposit[0]);
      evaluate (__LINE__, __func__, 80651.320312, transfer_quantities[0]);
    }

    // No transfer of anything because there's only one exchange.
    redistribute_coin_processor (NULL, coin_id, false, {"bittrex"},
                            active_transfer_count, last_transfer_elapsed_seconds,
                            exchanges_withdraw, exchanges_deposit, transfer_quantities);
    evaluate (__LINE__, __func__, 0, exchanges_withdraw.size());
    
    // No transfer because there's an active transfer on the coin already.
    redistribute_coin_processor (NULL, coin_id, false, exchanges,
                            1, last_transfer_elapsed_seconds,
                            exchanges_withdraw, exchanges_deposit, transfer_quantities);
    evaluate (__LINE__, __func__, 0, exchanges_withdraw.size());
    
    // No transfer because the previous transfer is too recent.
    redistribute_coin_processor (NULL, coin_id, false, exchanges,
                            active_transfer_count, 500,
                            exchanges_withdraw, exchanges_deposit, transfer_quantities);
    evaluate (__LINE__, __func__, 0, exchanges_withdraw.size());
    
    // No transfer because there's an unconfirmed balance on the coin.
    exchange_set_balance_cache (cryptopia, "verge", 0, 0, 0, 6000.6);
    redistribute_coin_processor (NULL, coin_id, false, exchanges,
                            active_transfer_count, last_transfer_elapsed_seconds,
                            exchanges_withdraw, exchanges_deposit, transfer_quantities);
    evaluate (__LINE__, __func__, 0, exchanges_withdraw.size());
    
    // Restore old values.
    exchange_set_balance_cache (cryptopia, "verge", cryptopia_total_verge_balance, 0, 0, 0);
    exchange_set_balance_cache (cryptopia, "verge", 0, 0, 0, cryptopia_unconfirmed_verge_balance);
  }

  // Test coin balancing over three exchanges.
  {
    // Data to test.
    string coin_id = "omisego";
    vector <string> exchanges = { bittrex, cryptopia, bitfinex };
    
    // Store the balances in the memory cache as total / available / reserved / unconfirmed.
    exchange_set_balance_cache (bittrex, "bitcoin", 0.362619, 0, 0, 0);
    exchange_set_balance_cache (bittrex, "omisego", 29.303488, 0, 0, 0);
    exchange_set_balance_cache (cryptopia, "bitcoin", 0.440877, 0, 0, 0);
    exchange_set_balance_cache (cryptopia, "omisego", 45.016350, 0, 0, 0);
    exchange_set_balance_cache (bitfinex, "bitcoin", 0.440877, 0, 0, 0);
    exchange_set_balance_cache (bitfinex, "omisego", 0.600000, 0, 0, 0);
    
    int active_transfer_count = 0;
    int last_transfer_elapsed_seconds = 1000;
    // Processor output storage.
    vector <string> exchanges_withdraw;
    vector <string> exchanges_deposit;
    vector <float> transfer_quantities;
    
    // Transfer from the richest to the poorest exchange.
    redistribute_coin_processor (NULL, coin_id, false, exchanges,
                                 active_transfer_count, last_transfer_elapsed_seconds,
                                 exchanges_withdraw, exchanges_deposit, transfer_quantities);
    evaluate (__LINE__, __func__, 1, exchanges_withdraw.size());
    if (!exchanges_withdraw.empty ()) {
      evaluate (__LINE__, __func__, cryptopia, exchanges_withdraw[0]);
      evaluate (__LINE__, __func__, bitfinex, exchanges_deposit[0]);
      evaluate (__LINE__, __func__, 24.373280, transfer_quantities[0]);
    }
  }

  // Test coin balancing over multiple exchanges.
  {
    // Data to test.
    string coin = "token3";
    vector <string> exchanges = { bittrex, cryptopia, bitfinex, bl3p, tradesatoshi };
    exchange_set_balance_cache (bittrex, coin, 1000, 0, 0, 0);
    exchange_set_balance_cache (cryptopia, coin, 500, 0, 0, 0);
    exchange_set_balance_cache (bitfinex, coin, 2000, 0, 0, 0);
    exchange_set_balance_cache (bl3p, coin, 3000, 0, 0, 0);
    exchange_set_balance_cache (tradesatoshi, coin, 2, 0, 0, 0);
    
    // Processor output storage.
    vector <string> exchanges_withdraw;
    vector <string> exchanges_deposit;
    vector <float> transfer_quantities;
    
    // Transfer an amount from the richest to the poorest exchange.
    to_output output ("");
    redistribute_coin_processor (&output, coin, false, exchanges,
                                 0, 3600,
                                 exchanges_withdraw, exchanges_deposit, transfer_quantities);
    evaluate (__LINE__, __func__, 1, exchanges_withdraw.size());
    if (!exchanges_withdraw.empty()) {
      evaluate (__LINE__, __func__, bl3p, exchanges_withdraw[0]);
      evaluate (__LINE__, __func__, tradesatoshi, exchanges_deposit[0]);
      evaluate (__LINE__, __func__, 1298.400024, transfer_quantities[0]);
    }
    output.clear ();
  }

  // Test coin balancing with communication error.
  {
    // Data to test.
    string coin = "token4";
    vector <string> exchanges = { bittrex, cryptopia, bitfinex, bl3p, tradesatoshi };
    exchange_set_balance_cache (bittrex, coin, 1000, 0, 0, 0);
    exchange_set_balance_cache (cryptopia, coin, 50, 0, 0, 0);
    exchange_set_balance_cache (bitfinex, coin, 2000, 0, 0, 0);
    exchange_set_balance_cache (bl3p, coin, 3000, 0, 0, 0);
    
    // Processor output storage.
    vector <string> exchanges_withdraw;
    vector <string> exchanges_deposit;
    vector <float> transfer_quantities;
    
    // Transfer an amount from the richest to the poorest exchange.
    redistribute_coin_processor (NULL, coin, false, exchanges,
                                 0, 3600,
                                 exchanges_withdraw, exchanges_deposit, transfer_quantities);
    evaluate (__LINE__, __func__, 1, exchanges_withdraw.size());
    if (!exchanges_withdraw.empty()) {
      evaluate (__LINE__, __func__, bl3p, exchanges_withdraw[0]);
      evaluate (__LINE__, __func__, cryptopia, exchanges_deposit[0]);
      evaluate (__LINE__, __func__, 1462.500000, transfer_quantities[0]);
    }
  }

  // Test coin balancing where an exchange has a reserved balance.
  {
    // Data to test.
    string coin = "token5";
    vector <string> exchanges = { bittrex, cryptopia };
    exchange_set_balance_cache (bittrex, coin, 1000, 0, 500, 0);
    exchange_set_balance_cache (cryptopia, coin, 50, 0, 30, 0);
    
    // Processor output storage.
    vector <string> exchanges_withdraw;
    vector <string> exchanges_deposit;
    vector <float> transfer_quantities;
    
    // Transfer an amount from the richest to the poorest exchange.
    redistribute_coin_processor (NULL, coin, false, exchanges,
                                 0, 3600,
                                 exchanges_withdraw, exchanges_deposit, transfer_quantities);
    evaluate (__LINE__, __func__, 1, exchanges_withdraw.size());
    if (exchanges_withdraw.size() == 1) {
      evaluate (__LINE__, __func__, bittrex, exchanges_withdraw[0]);
      evaluate (__LINE__, __func__, cryptopia, exchanges_deposit[0]);
      evaluate (__LINE__, __func__, 240.000000, transfer_quantities[0]);
    }
  }

  // Test coin balancing where the exchange provides zero balances in error.
  {
    // Data to test.
    // One exchange has a balance of zero, rather a negative balance.
    string coin = "euro";
    vector <string> exchanges = { bittrex, cryptopia, bitfinex, bl3p, tradesatoshi };
    exchange_set_balance_cache (bittrex, coin, 1000, 0, 0, 0);
    exchange_set_balance_cache (cryptopia, coin, -50, 0, 0, 0);
    exchange_set_balance_cache (bitfinex, coin, 2000, 0, 0, 0);
    exchange_set_balance_cache (bl3p, coin, 3000, 0, 0, 0);
    exchange_set_balance_cache (tradesatoshi, coin, 100, 0, 0, 0);
    
    // Processor output storage.
    vector <string> exchanges_withdraw;
    vector <string> exchanges_deposit;
    vector <float> transfer_quantities;
    
    // Transfer an amount from the richest to the poorest exchange.
    redistribute_coin_processor (NULL, coin, false, exchanges,
                                 0, 3600,
                                 exchanges_withdraw, exchanges_deposit, transfer_quantities);
    evaluate (__LINE__, __func__, 1, exchanges_withdraw.size());
    if (exchanges_withdraw.size() == 1) {
      evaluate (__LINE__, __func__, bl3p, exchanges_withdraw[0]);
      evaluate (__LINE__, __func__, tradesatoshi, exchanges_deposit[0]);
      evaluate (__LINE__, __func__, 1425.000000, transfer_quantities[0]);
    }
  }
}


void test_crypto_logic ()
{
  {
    // Test HmacSha384 hexits.
    string key = "secretkey";
    string data = "This is the data";
    string hexits = hmac_sha384_hexits (key, data);
    string standard = "46c41c1feb832e8a3b077643dd8e1fd81d0d0e6278a92e3f738cdb44f6d141fe45bce8398f9faa0a60329b5a081cc3c7";
    evaluate (__LINE__, __func__, standard, hexits);
  }
  {
    // Test the HMAC-SHA512 method as used, among others, by Poloniex.
    string key = "secretkey";
    string data = "This is the data";
    string hexits = hmac_sha512_hexits (key, data);
    string standard = "f878532938ea8b14d5a79d6772835dc6fc20b59a127fa2b58aa97ea07afe30a23cb59411f023427bbadd32c1e5d1e9489c770697813c015c9456229ab04ab075";
    evaluate (__LINE__, __func__, standard, hexits);
  }
  {
    string data = "This is the data";
    string hash = md5_raw (data);
    string base64 = base64_encode (hash);
    string standard = "vyfQMf/FQQBxxVO+PZMCCw==";
    evaluate (__LINE__, __func__, standard, base64);
  }
  {
    string data = "This is the data";
    string hash = sha256_raw (data);
    string base64 = base64_encode (hash);
    string standard = "mbrQuNTpEOLmpe+bs/2EPj1Y70NDSP2YSylM3A1iGSU=";
    evaluate (__LINE__, __func__, standard, base64);
  }
  {
    string data = "compute sha256";
    string hexits = sha256_hexits (data);
    // Via command line:
    // $ printf "compute sha256" | openssl sha256
    string standard = "1954f96b7346590ef896849a8bf2050c05bd0626f616469007f302703c7ea055";
    evaluate (__LINE__, __func__, standard, hexits);
  }
}


void test_basic_functions ()
{
  {
    vector <int> values = { 1, 2, 3, 4, 5 };
    array_remove (1, values);
    array_remove (111, values);
    array_remove (4, values);
    evaluate (__LINE__, __func__, { 2, 3, 5 }, values);
  }
  {
    string datestamp;
    int seconds;

    datestamp = "2017-12-09T18:01:29.68";
    seconds = exchange_parse_date (bittrex_get_exchange (), datestamp);
    evaluate (__LINE__, __func__, 1512842400, seconds);
    
    datestamp = "1512579268.1";
    seconds = exchange_parse_date (yobit_get_exchange (), datestamp);
    evaluate (__LINE__, __func__, 1512579268, seconds);

    datestamp = "2018-01-09 19:05:43";
    seconds = exchange_parse_date (poloniex_get_exchange (), datestamp);
    evaluate (__LINE__, __func__, 1515524400, seconds);
  }
  {
    string output;
    output = float2visual (0.01);
    evaluate (__LINE__, __func__, "0.01", output);
    output = float2visual (10.01);
    evaluate (__LINE__, __func__, "10.0", output);
    output = float2visual (100.01);
    evaluate (__LINE__, __func__, "100", output);
    output = float2visual (1000.01);
    evaluate (__LINE__, __func__, "1000", output);
  }
  {
    evaluate (__LINE__, __func__, 1.33, str2float ("1.33"));
    evaluate (__LINE__, __func__, 1.3333333333, str2float ("1.3333333333"));
    evaluate (__LINE__, __func__, 1.333333333333333, str2float ("1.333333333333333"));
  }
}


// Test routines for getting trading exchanges, markets, and coins, version 2.
void test_trading_assets_v2 ()
{
  vector <pair <string, string> > exchange_coin_wallets = {
    { "bittrex", "verge" },
    { "cryptopia", "verge" },
    { "bittrex", "ripple" },
    { "cryptopia", "ripple" },
    { "yobit", "ripple" },
    { "yobit", "zcoin" },
    { "yobit", "supercoin" },
  };
  vector <tuple <string, string, string> > exchange_market_coin_triplets = {
    make_tuple ("bittrex", "bitcoin", "verge"),
    make_tuple ("bittrex", "bitcoin", "ripple"),
    make_tuple ("bittrex", "ethereum", "verge"),
    make_tuple ("bittrex", "ethereum", "ripple"),
    make_tuple ("bittrex", "litecoin", "verge"),
    make_tuple ("cryptopia", "bitcoin", "verge"),
    make_tuple ("cryptopia", "bitcoin", "ripple"),
    make_tuple ("cryptopia", "ethereum", "verge"),
    make_tuple ("cryptopia", "ethereum", "ripple"),
    make_tuple ("cryptopia", "litecoin", "verge"),
    make_tuple ("yobit", "bitcoin", "ripple"),
    make_tuple ("yobit", "bitcoin", "zcoin"),
    make_tuple ("yobit", "ethereum", "ripple"),
    make_tuple ("yobit", "ethereum", "zcoin"),
    make_tuple ("yobit", "dogecoin", "ripple"),
    make_tuple ("yobit", "dogecoin", "zcoin"),
    make_tuple ("yobit", "dogecoin", "bitcoingold"),
  };

  vector <string> exchanges;
  vector <string> markets;
  vector <string> coins;

  coins = get_trading_coins (exchange_coin_wallets,exchange_market_coin_triplets);
  evaluate (__LINE__, __func__, { "ripple", "verge", "zcoin" }, coins);

  markets = get_trading_markets ("verge", exchange_coin_wallets,exchange_market_coin_triplets);
  evaluate (__LINE__, __func__, { "bitcoin", "ethereum", "litecoin" }, markets);

  markets = get_trading_markets ("ripple", exchange_coin_wallets,exchange_market_coin_triplets);
  evaluate (__LINE__, __func__, { "bitcoin", "dogecoin", "ethereum" }, markets);

  markets = get_trading_markets ("zcoin", exchange_coin_wallets,exchange_market_coin_triplets);
  evaluate (__LINE__, __func__, { "bitcoin", "dogecoin", "ethereum" }, markets);

  markets = get_trading_markets ("bitcoingold", exchange_coin_wallets,exchange_market_coin_triplets);
  evaluate (__LINE__, __func__, { }, markets);

  markets = get_trading_markets ("supercoin", exchange_coin_wallets,exchange_market_coin_triplets);
  evaluate (__LINE__, __func__, { }, markets);

  exchanges = get_trading_exchanges ("bitcoin", "verge", exchange_coin_wallets,exchange_market_coin_triplets);
  evaluate (__LINE__, __func__, { "bittrex", "cryptopia" }, exchanges);

  exchanges = get_trading_exchanges ("litecoin", "verge", exchange_coin_wallets,exchange_market_coin_triplets);
  evaluate (__LINE__, __func__, { "bittrex", "cryptopia" }, exchanges);

  exchanges = get_trading_exchanges ("ethereum", "ripple", exchange_coin_wallets,exchange_market_coin_triplets);
  evaluate (__LINE__, __func__, { "bittrex", "cryptopia", "yobit" }, exchanges);

  exchanges = get_trading_exchanges ("dogecoin", "zcoin", exchange_coin_wallets,exchange_market_coin_triplets);
  evaluate (__LINE__, __func__, { "yobit" }, exchanges);

  exchanges = get_trading_exchanges ("dogecoin", "unitedcoin", exchange_coin_wallets,exchange_market_coin_triplets);
  evaluate (__LINE__, __func__, { }, exchanges);
}


void test_api_json_parsing ()
{
  string json;
  string result;
  
  json = R"({"Success":true,"Error":null,"Data":{"OrderId":62049648,"FilledOrders":[]}})";
  result = cryptopia_limit_trade_json_parser (json);
  evaluate (__LINE__, __func__, "62049648", result);

  json = R"({"Success":true,"Error":null,"Data":{"OrderId":null,"FilledOrders":[15096400]}})";
  result = cryptopia_limit_trade_json_parser (json);
  evaluate (__LINE__, __func__, trade_order_fulfilled (), result);
}


void test_database_pausetrade ()
{
  SQL sql (front_bot_ip_address ());

  string exchange = "exchange";
  string market = "market";
  string coin = "coin";
  int seconds = seconds_since_epoch ();
  string reason = "reason";
  vector <string> pairs;
  
  // If trading is zero, it's not paused.
  pausetrade_set (sql, exchange, market, coin, 0, reason);
  pairs = pausetrade_get (sql);
  evaluate (__LINE__, __func__, {}, pairs);

  // If trading is beyond the current second, it's paused.
  pausetrade_set (sql, exchange, market, coin, seconds + 2, reason);
  pairs = pausetrade_get (sql);
  evaluate (__LINE__, __func__, true, in_array (exchange + market + coin, pairs));

  // If trading is less than the current second, it's not paused.
  pausetrade_set (sql, exchange, market, coin, seconds - 2, reason);
  pairs = pausetrade_get (sql);
  evaluate (__LINE__, __func__, false, in_array (exchange + coin, pairs));
  
  // Cleanup.
  sql.clear ();
  sql.add ("DELETE FROM pausetrade WHERE exchange =");
  sql.add (exchange);
  sql.add ("AND market =");
  sql.add (market);
  sql.add ("AND coin =");
  sql.add (coin);
  sql.add (";");
  sql.execute ();
}


void test_multipath ()
{
  string exchange = "testexchange";
  SQL sql (front_bot_ip_address ());

  // Create the sample multipath to work with.
  multipath samplepath;
  samplepath.exchange = exchange;
  samplepath.market1name = "bitcoin";
  samplepath.market1quantity = 0.01;
  samplepath.ask1 = 0.0000221500;
  samplepath.coin1name = "ixcoin";
  samplepath.coin1quantity = 451.4672851562;
  samplepath.coin2name = "ixcoin";
  samplepath.coin2quantity = 451.4672851562;
  samplepath.bid2 = 44.0000000000;
  samplepath.market2name = "dogecoin";
  samplepath.market2quantity = 19864.5605468750;
  samplepath.market3name = "dogecoin";
  samplepath.market3quantity = 19864.5605468750;
  samplepath.ask3 = 68.1748733521;
  samplepath.coin3name = "magi";
  samplepath.coin3quantity = 291.3764953613;
  samplepath.coin4name = "magi";
  samplepath.coin4quantity = 291.3764953613;
  samplepath.bid4 = 0.0000430000;
  samplepath.market4name = "bitcoin";
  samplepath.market4quantity = 0.0125291897;
  samplepath.gain = 25.3;

  // The order books.
  vector <float> quantities1 = {
    4666.0795898438,
    100.0000000000,
    31.6176071167,
    100.0000000000,
    100.0000000000,
    100.0000000000,
    100.0000000000,
    100.0000000000,
    144.7259521484,
    100.0000000000,
    100.0000000000,
    100.0000000000,
    100.0000000000,
    100.0000000000,
    51.9785194397,
    100.0000000000
  };
  vector <float> asks1 = {
    0.0000221500,
    0.0000225000,
    0.0000225800,
    0.0000226000,
    0.0000227000,
    0.0000228000,
    0.0000229000,
    0.0000230000,
    0.0000231000,
    0.0000232000,
    0.0000233000,
    0.0000234000,
    0.0000235000,
    0.0000236000,
    0.0000236300,
    0.0000237000,
  };
  vector <float> quantities2 = {
    0.2199161798,
    4.8148145676,
    15.5403118134,
    0.4074074030,
    47.9846458435,
    0.4223354459,
    11.5182399750,
    11.5384454727,
    43.4347801208,
    0.5497251153,
    0.5000000000,
    0.5500000119,
    801.2201538086,
    8.8144016266,
    8.0932235718,
    240.3929443359,
  };
  vector <float> bids2 = {
    44.0000000000,
    27.0000000000,
    27.0000000000,
    27.0000000000,
    26.0499992371,
    26.0456466675,
    26.0456447601,
    26.0000362396,
    23.0000019073,
    20.0100002289,
    20.0100002289,
    20.0000000000,
    1.2479577065,
    1.2479577065,
    1.2479577065,
    1.2479567528,
  };
  vector <float> quantities3 = {
    1.0054266453,
    3.1068456173,
    1.0000000000,
    0.5000000000,
    2.7023959160,
    0.7043581605,
    0.1601457745,
    0.2811245024,
    0.1445466429,
    1.0000000000,
    1.8443803787,
    0.2000000030,
    1.0000000000,
    0.1503006071,
    0.2500000000,
    0.1000000015,
  };
  vector <float> asks3 = {
    68.1748657227,
    230.0000000000,
    230.0000000000,
    239.9999847412,
    240.0000000000,
    248.9999847412,
    248.9999847412,
    248.9999847412,
    248.9999847412,
    450.0000000000,
    500.0000000000,
    996.9998779297,
    997.0000000000,
    998.0000000000,
    998.0000000000,
    1000.0000000000,
  };
  vector <float> quantities4 = {
    3.3699026108,
    80.0821456909,
    19.3230762482,
    33.6258354187,
    35.5944480896,
    31.5809612274,
    22.9870834351,
    10.2943696976,
    5.9598383904,
    33.9694099426,
    3.3323235512,
    38.2430305481,
    39.3543548584,
    43.4000015259,
    20.0000000000,
    126.3524169922,
  };
  vector <float> bids4 = {
    0.0000430000,
    0.0000420000,
    0.0000390000,
    0.0000388600,
    0.0000367400,
    0.0000361900,
    0.0000360000,
    0.0000349900,
    0.0000340200,
    0.0000340000,
    0.0000330100,
    0.0000330000,
    0.0000320000,
    0.0000310200,
    0.0000310100,
    0.0000310000,
  };

  // Test processing and storing the multipath.
  {
    // Create the multipath to work with.
    multipath path = samplepath;
    
    // Store the path in the database.
    multipath_store (sql, { path });
    
    // Get and check the identifier of this path as stored in the database.
    vector <multipath> multipaths = multipath_get (sql);
    for (auto & path2 : multipaths) {
      if (multipath_equal (path, path2)) {
        path.id = path2.id;
      }
    }
    evaluate (__LINE__, __func__, true, path.id > 100);

    evaluate (__LINE__, __func__, multipath_status::bare, path.status);

    map <string, float> minimum_trade_sizes = {
      // The following should trigger a too-low trade event.
      make_pair (path.exchange + path.market1name + path.coin1name, 50)
    };
    
    to_output output ("");
    multipath_output (&output, &path);
    multipath_processor (&output, &path, minimum_trade_sizes,
                         quantities1, asks1, quantities2, bids2,
                         quantities3, asks3, quantities4, bids4,
                         1);
    multipath_output (&output, &path);
    output.clear ();
    
    // Check the path is unprofitable.
    evaluate (__LINE__, __func__, multipath_status::unprofitable, path.status);

    // Update the path in the database.
    multipath_update (sql, path);

    // Get and check the updated status of this path as stored in the database.
    multipaths = multipath_get (sql);
    for (auto & path2 : multipaths) {
      if (multipath_equal (path, path2)) {
        evaluate (__LINE__, __func__, multipath_status::unprofitable, path2.status);
      }
    }
    multipath path2 = multipath_get (sql, path.id);
    evaluate (__LINE__, __func__, multipath_status::unprofitable, path2.status);
    
    // Test the "executing" flag.
    path2 = multipath_get (sql, path.id);
    evaluate (__LINE__, __func__, false, path2.executing);
    path.executing = true;
    multipath_update (sql, path);
    path2 = multipath_get (sql, path.id);
    evaluate (__LINE__, __func__, true, path2.executing);
    path.executing = false;
    multipath_update (sql, path);
    path2 = multipath_get (sql, path.id);
    evaluate (__LINE__, __func__, false, path2.executing);

    // Delete this path.
    multipath_delete (sql, path);
  }
  
  // Test clash in multiple multipaths.
  {
    // Clash container.
    vector <string> exchanges_markets_coins;
    
    // Create the multipaths to work with.
    multipath path1;
    path1.exchange = exchange;
    path1.market1name = "bitcoin";
    path1.coin1name = "ixcoin";
    path1.coin2name = "ixcoin";
    path1.market2name = "dogecoin";
    path1.market3name = "dogecoin";
    path1.coin3name = "magi";
    path1.coin4name = "magi";
    path1.market4name = "bitcoin";
    multipath path2;

    // No clash the first time.
    exchanges_markets_coins.clear ();
    evaluate (__LINE__, __func__, false, multipath_clash  (&path1, exchanges_markets_coins));
    // Clash the second time.
    evaluate (__LINE__, __func__, true, multipath_clash  (&path1, exchanges_markets_coins));

    // No clash if the exchange is different.
    exchanges_markets_coins.clear ();
    evaluate (__LINE__, __func__, false, multipath_clash  (&path1, exchanges_markets_coins));
    path2 = path1;
    evaluate (__LINE__, __func__, true, multipath_clash  (&path2, exchanges_markets_coins));
    path2.exchange += "xxx";
    evaluate (__LINE__, __func__, false, multipath_clash  (&path2, exchanges_markets_coins));

    // No clash is all coins are different.
    exchanges_markets_coins.clear ();
    evaluate (__LINE__, __func__, false, multipath_clash  (&path1, exchanges_markets_coins));
    path2 = path1;
    evaluate (__LINE__, __func__, true, multipath_clash  (&path2, exchanges_markets_coins));
    path2.coin1name += "xxx";
    path2.coin2name += "xxx";
    path2.coin3name += "xxx";
    path2.coin4name += "xxx";
    evaluate (__LINE__, __func__, false, multipath_clash  (&path2, exchanges_markets_coins));

    // Clash if something is the same.
    exchanges_markets_coins.clear ();
    evaluate (__LINE__, __func__, false, multipath_clash  (&path1, exchanges_markets_coins));
    path2 = path1;
    evaluate (__LINE__, __func__, true, multipath_clash  (&path2, exchanges_markets_coins));
    path2.coin2name += "xxx";
    path2.coin3name += "xxx";
    path2.coin4name += "xxx";
    evaluate (__LINE__, __func__, true, multipath_clash  (&path2, exchanges_markets_coins));
  }
  
  // Test whether a multipath is partially paused.
  {
    // Create the multipaths to work with.
    multipath path;
    path.exchange = exchange;
    path.market1name = "bitcoin";
    path.coin1name = "ixcoin";
    path.coin2name = "ixcoin";
    path.market2name = "dogecoin";
    path.market3name = "dogecoin";
    path.coin3name = "magi";
    path.coin4name = "magi";
    path.market4name = "bitcoin";

    vector <string> exchanges_markets_coins;

    // Check on clashes.
    exchanges_markets_coins = { path.exchange + path.market1name + path.coin1name };
    evaluate (__LINE__, __func__, true, multipath_paused  (&path, exchanges_markets_coins, false));

    exchanges_markets_coins = { path.exchange + path.market1name + path.coin1name + "xxx" };
    evaluate (__LINE__, __func__, false, multipath_paused  (&path, exchanges_markets_coins, false));

    exchanges_markets_coins = { path.exchange + path.market2name + path.coin2name };
    evaluate (__LINE__, __func__, true, multipath_paused  (&path, exchanges_markets_coins, false));

    exchanges_markets_coins = {
      path.exchange + path.market1name + path.coin1name,
      path.exchange + path.market3name + path.coin3name
    };
    evaluate (__LINE__, __func__, true, multipath_paused  (&path, exchanges_markets_coins, false));
  }

  // Test the multiplication factor to increase the order size. // Do in the ./lab first.
  {
    multipath path = samplepath;
    map <string, float> minimum_trade_sizes;
    vector <float> bids44;
    for (auto b : bids4) bids44.push_back (b * 2);
    to_output output ("");
    multipath_output (&output, &path);
    multipath_processor (&output, &path, minimum_trade_sizes,
                         quantities1, asks1, quantities2, bids2,
                         quantities3, asks3, quantities4, bids4,
                         1);
    multipath_output (&output, &path);
    output.clear ();
  }
}


void test_allrates ()
{
  SQL sql (front_bot_ip_address ());
  // Test pattern storage, preparation for analysis, and deletion.
  {
    int current_second = seconds_since_epoch ();
    string exchange = "unittestexchange";
    string market = "unittestmarket";
    string coin = "unittestcoin";
    unordered_map <int, float> minute_asks;
    unordered_map <int, float> minute_bids;
    unordered_map <int, float> hour_asks;
    unordered_map <int, float> hour_bids;

    // Create pattern with increasing rates.
    float bid = 0.0010;
    float ask = 0.0011;
    for (int second = current_second - (24 * 3600); second <= current_second; second += 60) {
      bid += 0.00001;
      ask += 0.00001;
      // Omit one hour.
      if (second > current_second - (20 * 3600)) {
        if (second < current_second - (19 * 3600)) {
          continue;
        }
      }
      // Store.
      allrates_add (exchange, market, coin, second, bid, ask);
    }

    // Prepare the pattern for analysis.
    pattern_prepare (nullptr, exchange, market, coin, minute_asks, minute_bids, hour_asks, hour_bids);

    /*
    vector <pair <string, string> > plotdata;
    string picturepath;
    for (int minute = -2000; minute <= 0; minute++) {
      float rate = (minute_asks [minute] + minute_bids [minute]) / 2;
      plotdata.push_back (make_pair (to_string (minute), float2string (rate)));
    }
    plot (plotdata, "unittest", picturepath);
    */
    
    // Spot-check the pattern and its interpolated values.
    evaluate (__LINE__, __func__, 0.0155097414, minute_asks[0]);
    evaluate (__LINE__, __func__, 0.0154797425, minute_asks[-3]);
    evaluate (__LINE__, __func__, 0.0041100127, minute_asks[-1140]);
    evaluate (__LINE__, __func__, 0.0040100119, minute_asks[-1150]);
    evaluate (__LINE__, __func__, 0.0154097453, minute_bids[0]);
    evaluate (__LINE__, __func__, 0.0153797464, minute_bids[-3]);
    evaluate (__LINE__, __func__, 0.0152147524, hour_asks[0]);
    evaluate (__LINE__, __func__, 0.0134148216, hour_asks[-3]);
    evaluate (__LINE__, __func__, 0.0151147572, hour_bids[0]);
    evaluate (__LINE__, __func__, 0.0133148264, hour_bids[-3]);

    // Delete the data again.
    allrates_delete (exchange, market, coin);
    //allrates2_delete (sql, exchange, market, coin);

    // Prepare the pattern for analysis but there should be nothing after deleting it.
    pattern_prepare (nullptr, exchange, market, coin, minute_asks, minute_bids, hour_asks, hour_bids);
    evaluate (__LINE__, __func__, 0, hour_bids.size());
  }

}


void test_patterns ()
{
  {
    int start = -10;
    int window = 15;
    unordered_map <int, float> bids, asks;
    float bid = 0.00021;
    float ask = 0.00022;
    for (int i = start - window; i <= start; i++) {
      bid += 0.0000021;
      ask += 0.0000021;
      bids[i] = bid;
      asks[i] = ask;
    }
    float change = pattern_change (bids, asks, start, window);
    evaluate (__LINE__, __func__, 0.9073446393, change);
  }
  {
    int start = -10;
    int window = 15;
    unordered_map <int, float> bids, asks;
    float bid = 0.00021;
    float ask = 0.00022;
    for (int i = start - window; i <= start; i++) {
      bid -= 0.0000021;
      ask -= 0.0000021;
      bids[i] = bid;
      asks[i] = ask;
    }
    float change = pattern_change (bids, asks, start, window);
    evaluate (__LINE__, __func__, -1.0617648363, change);
  }
}


void test_sqlite ()
{
  SqliteDatabase sqlite ("yobit_bitcoin_</div><!--");

  string exchange = "yobit";
  string market = "bitcoin";
  string coin = "</div><!--";
  allrates_add (exchange, market, coin, seconds_since_epoch (), 0.2f, 0.21f);

}


void test_approximate_order_books ()
{
  // Testing data.
  string exchange = "exchange";
  string market = "market";
  string coin = "coin";

  float price = 0;
  float standard_price = 0;
  vector <float> quantities, rates, standard_quantities, standard_rates;

  // Initially there should be no buyers in the database.
  buyers_get (exchange, market, coin, price, quantities, rates);
  evaluate (__LINE__, __func__, 0.0, price);
  evaluate (__LINE__, __func__, 0, quantities.size());

  // There should be no sellers either, initially.
  sellers_get (exchange, market, coin, price, quantities, rates);
  evaluate (__LINE__, __func__, 0.0, price);
  evaluate (__LINE__, __func__, 0, quantities.size());

  // Store the buyers in the database.
  standard_price = 0.001;
  standard_quantities = { 10, 20 };
  standard_rates = { 0.001, 0.002 };
  buyers_set (exchange, market, coin, standard_price, standard_quantities, standard_rates);

  // Check on the buyers.
  buyers_get (exchange, market, coin, price, quantities, rates);
  evaluate (__LINE__, __func__, standard_price, price);
  evaluate (__LINE__, __func__, standard_quantities, quantities);
  evaluate (__LINE__, __func__, standard_rates, rates);

  // Store the sellers in the database.
  standard_price = 0.002;
  standard_quantities = { 30, 40 };
  standard_rates = { 0.002, 0.003 };
  sellers_set (exchange, market, coin, standard_price, standard_quantities, standard_rates);

  // Check on the sellers.
  sellers_get (exchange, market, coin, price, quantities, rates);
  evaluate (__LINE__, __func__, standard_price, price);
  evaluate (__LINE__, __func__, standard_quantities, quantities);
  evaluate (__LINE__, __func__, standard_rates, rates);

  // Delete all data.
  buyers_sellers_delete (exchange, market, coin);
}


void test_balances ()
{
  map <string, map <string, float> > totals, availables, reserveds, unconfirmeds;
  float total = 1, available = 2, reserved = 3, unconfirmed = 4;
  vector <string> exchanges = exchange_get_names ();
  vector <string> coins = {"aaa", "bbb", "ccc"};
  for (auto exchange : exchanges) {
    for (auto coin : coins) {
      total *= 1.05;
      available *= 1.06;
      reserved *= 1.07;
      unconfirmed *= 1.08;
      totals [exchange][coin] = total;
      availables [exchange][coin] = available;
      reserveds [exchange][coin] = reserved;
      unconfirmeds [exchange][coin] = unconfirmed;
      exchange_set_balance_cache (exchange, coin, total, available, reserved, unconfirmed);
    }
  }
  for (auto exchange : exchanges) {
    for (auto coin : coins) {
      float total, available, reserved, unconfirmed;
      exchange_get_balance_cached (exchange, coin, &total, &available, &reserved, &unconfirmed);
      evaluate (__LINE__, __func__, totals [exchange][coin], total);
      evaluate (__LINE__, __func__, availables [exchange][coin], available);
      evaluate (__LINE__, __func__, reserveds [exchange][coin], reserved);
      evaluate (__LINE__, __func__, unconfirmeds [exchange][coin], unconfirmed);
    }
  }
  for (auto exchange : exchanges) {
    vector <string> balances = exchange_get_coins_with_balances (exchange);
    evaluate (__LINE__, __func__, coins, balances);
  }
}


void test_coins ()
{
  SQL sql (front_bot_ip_address ());
  string exchange = "exchange";
  string market = "market";
  string coin = "coin1 coin2 coin3";
  
  vector <string> exchanges, markets, coins;

  // Get the data, whatever it contains, including live data.
  coins_market_get (sql, exchanges, markets, coins);
  
  // The unit testing data, none of it should be in the database now.
  int record_count = 0;
  for (unsigned int i = 0; i < exchanges.size(); i++) {
    if (exchange != exchanges[i]) continue;
    if (market != markets[i]) continue;
    record_count++;
  }
  evaluate (__LINE__, __func__, 0, record_count);

  // Enter the unit testing data in the database.
  coins_market_set (sql, exchange, market, coin);

  // The unit testing data, it should be in the database now.
  record_count = 0;
  coins_market_get (sql, exchanges, markets, coins);
  for (unsigned int i = 0; i < exchanges.size(); i++) {
    if (exchange != exchanges[i]) continue;
    if (market != markets[i]) continue;
    record_count++;
    evaluate (__LINE__, __func__, coin, coins[i]);
  }
  evaluate (__LINE__, __func__, 1, record_count);

  // Clear the unit testing data off the database.
  coins_market_set (sql, exchange, market, "");

  // The unit testing data, none of it should be in the database now.
  record_count = 0;
  coins_market_get (sql, exchanges, markets, coins);
  for (unsigned int i = 0; i < exchanges.size(); i++) {
    if (exchange != exchanges[i]) continue;
    if (market != markets[i]) continue;
    record_count++;
  }
  evaluate (__LINE__, __func__, 0, record_count);
}


void test_optimizer ()
{
  SQL sql (front_bot_ip_address ());
  string exchange = "exchange";
  string coin = "coin";
  float rate = 1.1;
  float projected = 2.2;
  float realized = 1.9;
  
  vector <int> ids;
  vector <string> exchanges, coins;
  vector <float> starts, forecasts, realizeds;
  
  // None of the values of the unit test should be in the database.
  optimizer_get (sql, ids, exchanges, coins, starts, forecasts, realizeds);
  evaluate (__LINE__, __func__, false, in_array (exchange, exchanges));
  evaluate (__LINE__, __func__, false, in_array (coin, coins));

  // Add the values for the unit test.
  optimizer_add (sql, exchange, coin, rate, projected);
  
  // Check the added values are there now in the database.
  optimizer_get (sql, ids, exchanges, coins, starts, forecasts, realizeds);
  evaluate (__LINE__, __func__, true, in_array (exchange, exchanges));
  evaluate (__LINE__, __func__, true, in_array (coin, coins));
  evaluate (__LINE__, __func__, true, in_array (rate, starts));
  evaluate (__LINE__, __func__, true, in_array (projected, forecasts));
  
  int id = 0;
  for (unsigned int i = 0; i < exchanges.size(); i++) {
    if (exchanges[i] == exchange) {
      if (coins[i] == coin) {
        id = ids[i];
      }
    }
  }

  // Update the realized rate.
  optimizer_update (sql, id, realized);
  
  // Check that the realized rate is now in the database.
  optimizer_get (sql, ids, exchanges, coins, starts, forecasts, realizeds);
  evaluate (__LINE__, __func__, true, in_array (realized, realizeds));

  // Delete the entry again.
  optimizer_delete (sql, id);

  // The entry should no longer be in the database now.
  optimizer_get (sql, ids, exchanges, coins, starts, forecasts, realizeds);
  evaluate (__LINE__, __func__, false, in_array (exchange, exchanges));
  evaluate (__LINE__, __func__, false, in_array (coin, coins));
}


void test_plotter ()
{
  vector <pair <float, float> > plotdata1 = {
    make_pair (1, 1.1),
    make_pair (2, 1.5),
    make_pair (3, 1.7),
  };
  vector <pair <float, float> > plotdata2 = {
    make_pair (1, 1.2),
    make_pair (2, 1.4),
    make_pair (3, 1.6),
  };
  string title1 = "test1";
  string title2 = "test2";
  string picturepath;
  plot (plotdata1, plotdata2, title1, title2, picturepath);
}


void test_weights ()
{
  SQL sql (front_bot_ip_address ());
  string exchange = "exchange";
  string market = "market";
  string coin = "coin";
  string weight = "weight";
  string value;
  float hourly = 1.1111111;
  float daily = 2.2222222;
  float weekly = 3.3333333;
  unordered_map <int, float> curves = {
    make_pair (1, 11.11),
    make_pair (2, 22.22),
  };
  string json;
  float sum_absolute_residuals = 5.55555;
  float maximum_residual_percentage = 10.101;
  float average_residual_percentage = 2.2222;
  float sum_absolute_residuals2, maximum_residual_percentage2, average_residual_percentage2;

  // None of the values of the unit test should be in the database.
  weights_get (sql, exchange, market, coin, value,
               sum_absolute_residuals2, maximum_residual_percentage2, average_residual_percentage2);
  evaluate (__LINE__, __func__, "", value);
  evaluate (__LINE__, __func__, numeric_limits<float>::max(), sum_absolute_residuals2);
  evaluate (__LINE__, __func__, numeric_limits<float>::max(), maximum_residual_percentage2);
  evaluate (__LINE__, __func__, numeric_limits<float>::max(), average_residual_percentage2);

  // Add the values for the unit test.
  weights_save (sql, exchange, market, coin, weight,
                sum_absolute_residuals, maximum_residual_percentage, average_residual_percentage);
  
  // Check the added weight is there now in the database.
  weights_get (sql, exchange, market, coin, value,
               sum_absolute_residuals2, maximum_residual_percentage2, average_residual_percentage2);
  evaluate (__LINE__, __func__, weight, value);
  evaluate (__LINE__, __func__, sum_absolute_residuals, sum_absolute_residuals2);
  evaluate (__LINE__, __func__, maximum_residual_percentage, maximum_residual_percentage2);
  evaluate (__LINE__, __func__, average_residual_percentage, average_residual_percentage2);

  // Delete the entry again.
  weights_delete (sql, exchange, market, coin);
  
  // Check that the deleted weights and associated values are no longer in the database.
  weights_get (sql, exchange, market, coin, value,
               sum_absolute_residuals2, maximum_residual_percentage2, average_residual_percentage2);
  evaluate (__LINE__, __func__, "", value);
  evaluate (__LINE__, __func__, numeric_limits<float>::max(), sum_absolute_residuals2);
  evaluate (__LINE__, __func__, numeric_limits<float>::max(), maximum_residual_percentage2);
  evaluate (__LINE__, __func__, numeric_limits<float>::max(), average_residual_percentage2);

  // Encoder and decoder.
  json = weights_encode (hourly, daily, weekly, curves);
  float hourly1, daily1, weekly1;
  unordered_map <int, float> curves1;
  weights_decode (json, hourly1, daily1, weekly1, curves1);
  evaluate (__LINE__, __func__, hourly, hourly1);
  evaluate (__LINE__, __func__, daily, daily1);
  evaluate (__LINE__, __func__, weekly, weekly1);
  evaluate (__LINE__, __func__, curves[1], curves1[1]);
  evaluate (__LINE__, __func__, curves[2], curves1[2]);
}


void test_surfer ()
{
  SQL sql (front_bot_ip_address ());
  string coin = "coin";
  string trade = "trade";
  string status = "status";
  float rate = 1.1111111;
  float amount = 2.2222222;
  string remark = "remark";

  vector <int> ids, stamps;
  vector <string> coins, trades, statuses, remarks;
  vector <float> rates, amounts;

  // The values of the unit test should not be in the database, at this stage.
  surfer_get (sql, ids, stamps, coins, trades, statuses, rates, amounts, remarks);
  evaluate (__LINE__, __func__, false, in_array (coin, coins));
  evaluate (__LINE__, __func__, false, in_array (status, statuses));

  // Add the values for the unit test.
  surfer_add (sql, coin, trade, status);
  
  // Check the added record is there now in the database.
  surfer_get (sql, ids, stamps, coins, trades, statuses, rates, amounts, remarks);
  evaluate (__LINE__, __func__, true, in_array (coin, coins));
  evaluate (__LINE__, __func__, true, in_array (trade, trades));
  evaluate (__LINE__, __func__, true, in_array (status, statuses));

  // Get the identifier of this record.
  int identifier = 0;
  if (!ids.empty ()) identifier = ids.front ();

  // Update this record.
  surfer_update (sql, identifier, "", rate, amount);
  surfer_update (sql, identifier, "", remark);

  // Check the updated record is in the database.
  surfer_get (sql, ids, stamps, coins, trades, statuses, rates, amounts, remarks);
  evaluate (__LINE__, __func__, true, in_array (rate, rates));
  evaluate (__LINE__, __func__, true, in_array (amount, amounts));
  evaluate (__LINE__, __func__, true, in_array (remark, remarks));

  // Delete the entry again.
  surfer_delete (sql, identifier);
  
  // Check that the deleted record is no longer in the database.
  surfer_get (sql, ids, stamps, coins, trades, statuses, rates, amounts, remarks);
  evaluate (__LINE__, __func__, false, in_array (coin, coins));
  evaluate (__LINE__, __func__, false, in_array (trade, trades));
  evaluate (__LINE__, __func__, false, in_array (status, statuses));
  evaluate (__LINE__, __func__, false, in_array (rate, rates));
  evaluate (__LINE__, __func__, false, in_array (amount, amounts));
  evaluate (__LINE__, __func__, false, in_array (remark, remarks));
}


void test_bought ()
{
  SQL sql (front_bot_ip_address ());
  string exchange = "exchange";
  string exchange2 = "exchange2";
  string market = "market";
  string coin = "coin";
  float rate = 1.1111111;

  vector <int> ids, hours;
  vector <string> exchanges, markets, coins;
  vector <float> rates;

  {
    // The values of the unit test should not be in the database, at this stage.
    bought_get (sql, ids, hours, exchanges, markets, coins, rates);
    evaluate (__LINE__, __func__, false, in_array (coin, coins));
    evaluate (__LINE__, __func__, false, in_array (exchange, exchanges));
    
    // Add the values for the unit test.
    bought_add (sql, exchange, market, coin, rate);
    
    // Check the added record is there now in the database.
    bought_get (sql, ids, hours, exchanges, markets, coins, rates);
    evaluate (__LINE__, __func__, true, in_array (coin, coins));
    evaluate (__LINE__, __func__, true, in_array (exchange, exchanges));
    evaluate (__LINE__, __func__, true, in_array (market, markets));
    evaluate (__LINE__, __func__, true, in_array (rate, rates));
    
    // Get the identifier(s) of the record(s).
    vector <int> identifiers;
    for (unsigned int i = 0; i < ids.size(); i++) {
      if (exchange != exchanges[i]) continue;
      if (market != markets[i]) continue;
      if (coin != coins[i]) continue;
      identifiers.push_back (ids[i]);
    }
    evaluate (__LINE__, __func__, true, identifiers.size() >= 1);
    
    // Delete the entry or entries again.
    for (auto id : identifiers) bought_delete (sql, id);
    
    // Check the added record is there now in the database.
    bought_get (sql, ids, hours, exchanges, markets, coins, rates);
    evaluate (__LINE__, __func__, false, in_array (coin, coins));
    evaluate (__LINE__, __func__, false, in_array (exchange, exchanges));
    evaluate (__LINE__, __func__, false, in_array (market, markets));
    evaluate (__LINE__, __func__, false, in_array (rate, rates));
  }
  
  // Testing redistributing the coin prices.
  {
    // There's no data yet, and the price is not yet given,
    // so calling the function should have no effect in the database.
    float price = 0;
    redistribute_coin_bought_price (exchange, exchange2, market, coin, price);
    bought_get (sql, ids, hours, exchanges, markets, coins, rates);
    evaluate (__LINE__, __func__, false, in_array (coin, coins));
    evaluate (__LINE__, __func__, false, in_array (exchange, exchanges));
    evaluate (__LINE__, __func__, false, in_array (exchange2, exchanges));

    // Set the price of the original exchange.
    // It should be copied to the destination exchange.
    bought_add (sql, exchange, market, coin, rate);
    redistribute_coin_bought_price (exchange, exchange2, market, coin, price);
    bought_get (sql, ids, hours, exchanges, markets, coins, rates);
    evaluate (__LINE__, __func__, true, in_array (coin, coins));
    evaluate (__LINE__, __func__, true, in_array (exchange, exchanges));
    evaluate (__LINE__, __func__, true, in_array (exchange2, exchanges));
    evaluate (__LINE__, __func__, true, in_array (rate, rates));

    // Check the price on both exchanges.
    float original_price = 0;
    float target_price = 0;
    for (unsigned int i = 0; i < ids.size(); i++) {
      if (market != markets[i]) continue;
      if (coin != coins[i]) continue;
      float price = rates[i];
      if (exchange == exchanges[i]) original_price = price;
      if (exchange2 == exchanges[i]) target_price = price;
    }
    evaluate (__LINE__, __func__, rate, original_price);
    evaluate (__LINE__, __func__, rate, target_price);

    // Set a lower price: It should not be recorded.
    price = rate * 0.9;
    redistribute_coin_bought_price (exchange, exchange2, market, coin, price);
    bought_get (sql, ids, hours, exchanges, markets, coins, rates);
    original_price = 0;
    target_price = 0;
    for (unsigned int i = 0; i < ids.size(); i++) {
      if (market != markets[i]) continue;
      if (coin != coins[i]) continue;
      float price = rates[i];
      if (exchange == exchanges[i]) original_price = price;
      if (exchange2 == exchanges[i]) target_price = price;
    }
    evaluate (__LINE__, __func__, rate, original_price);
    evaluate (__LINE__, __func__, rate, target_price);

    // Set a higher price: It should be recorded.
    price = rate * 1.1;
    redistribute_coin_bought_price (exchange, exchange2, market, coin, price);
    bought_get (sql, ids, hours, exchanges, markets, coins, rates);
    original_price = 0;
    target_price = 0;
    for (unsigned int i = 0; i < ids.size(); i++) {
      if (market != markets[i]) continue;
      if (coin != coins[i]) continue;
      float price = rates[i];
      if (exchange == exchanges[i]) original_price = price;
      if (exchange2 == exchanges[i]) target_price = price;
    }
    evaluate (__LINE__, __func__, price, original_price);
    evaluate (__LINE__, __func__, price, target_price);

    // Delete all test data.
    bought_get (sql, ids, hours, exchanges, markets, coins, rates);
    for (unsigned int i = 0; i < ids.size(); i++) {
      if (market == markets[i]) {
        if (coin == coins[i]) {
          bought_delete (sql, ids[i]);
        }
      }
    }
  }
  
}


void test_sells ()
{
  SQL sql (front_bot_ip_address ());
  string exchange = "exchange";
  string market = "market";
  string coin = "coin";
  float quantity = 3.33333;
  float buyrate = 1.1111111;
  float sellrate = 2.222222;
  
  vector <int> ids, hours;
  vector <string> exchanges, markets, coins;
  vector <float> quantities, buyrates, sellrates;
  
  // The values of the unit test should not be in the database, at this stage.
  sold_get (sql, ids, hours, exchanges, markets, coins, quantities, buyrates, sellrates);
  evaluate (__LINE__, __func__, false, in_array (coin, coins));
  evaluate (__LINE__, __func__, false, in_array (exchange, exchanges));
  
  // Add the values for the unit test.
  sold_store (sql, exchange, market, coin, quantity, buyrate, sellrate);
  
  // Check the added record is there now in the database.
  sold_get (sql, ids, hours, exchanges, markets, coins, quantities, buyrates, sellrates);
  evaluate (__LINE__, __func__, true, in_array (coin, coins));
  evaluate (__LINE__, __func__, true, in_array (exchange, exchanges));
  evaluate (__LINE__, __func__, true, in_array (market, markets));
  evaluate (__LINE__, __func__, true, in_array (quantity, quantities));
  evaluate (__LINE__, __func__, true, in_array (buyrate, buyrates));
  evaluate (__LINE__, __func__, true, in_array (sellrate, sellrates));
  
  // Get the identifier(s) of the record(s).
  vector <int> identifiers;
  for (unsigned int i = 0; i < ids.size(); i++) {
    if (exchange != exchanges[i]) continue;
    if (market != markets[i]) continue;
    if (coin != coins[i]) continue;
    identifiers.push_back (ids[i]);
  }
  evaluate (__LINE__, __func__, true, identifiers.size() >= 1);
  
  // Delete the entry or entries again.
  for (auto id : identifiers) sold_delete (sql, id);
  
  // Check the added record is there now in the database.
  sold_get (sql, ids, hours, exchanges, markets, coins, quantities, buyrates, sellrates);
  evaluate (__LINE__, __func__, false, in_array (coin, coins));
  evaluate (__LINE__, __func__, false, in_array (exchange, exchanges));
  evaluate (__LINE__, __func__, false, in_array (market, markets));
  evaluate (__LINE__, __func__, false, in_array (quantity, quantities));
  evaluate (__LINE__, __func__, false, in_array (buyrate, buyrates));
  evaluate (__LINE__, __func__, false, in_array (sellrate, sellrates));
}


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);

  //test_trade_quantities_rates ();
  //test_getting_coin_naming ();
  //test_arbitrage_processor ();
  //test_crypto_logic ();
  //test_coin_balancing ();
  //test_exchange_deposit_history ();
  //test_transfers_database ();
  //transfers_clean_tests (sql);
  //test_database_wallets ();
  //test_basic_functions ();
  //test_trading_assets_v2 ();
  //test_api_json_parsing ();
  //test_database_pausetrade ();
  //test_multipath ();
  //test_allrates ();
  //test_patterns ();
  //test_sqlite ();
  //test_approximate_order_books ();
  //test_balances ();
  //test_coins ();
  //test_optimizer ();
  //test_plotter ();
  //test_weights ();
  //test_surfer ();
  //test_bought ();
  //test_sells ();

  finalize_program ();
  return EXIT_SUCCESS;
}
