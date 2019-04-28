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


#include "controllers.h"
#include "sql.h"
#include "shared.h"
#include "models.h"
#include "exchanges.h"
#include "cryptopia.h"
#include "io.h"
#include "proxy.h"
#include "bitfinex.h"
#include "kraken.h"
#include "bl3p.h"
#include "poloniex.h"
#include "traders.h"


mutex get_all_balances_mutex;


void order_transfer (void * output,
                     string exchange_withdraw, string exchange_deposit,
                     string coin, float quantity, bool manual)
{
  // The object to write the output to in bundled format.
  to_output * output_block = (to_output *) output;

  output_block->add ({"Order a transfer of", float2string (quantity), coin, "from", exchange_withdraw, "to", exchange_deposit});
  
  
  SQL sql (front_bot_ip_address ());
  
  
  // Check that the coin identifier is enabled for balancing in the database for both of the exchanges.
  // Don't check this for manual transfers.
  bool active = wallets_get_balancing (sql, exchange_withdraw, coin);
  if (!active) {
    output_block->add ({exchange_withdraw, ":", "coin not balancing in the database"});
    output_block->to_stderr (true);
    if (!manual) return;
  }
  active = wallets_get_balancing (sql, exchange_deposit, coin);
  if (!active) {
    output_block->add ({exchange_deposit, ":", "coin not balancing in the database"});
    output_block->to_stderr (true);
    if (!manual) return;
  }
  
  
  string coin_abbreviation_withdraw = exchange_get_coin_abbrev (exchange_withdraw, coin);
  string coin_abbreviation_deposit = exchange_get_coin_abbrev (exchange_deposit, coin);

  
  // Check that the address in the database is the same as on the exchange.
  // Do that for both of the exchanges.
  // This will also automatically check whether both wallets are enabled on the exchanges.
  // Because if a wallet is not enabled, requesting the wallet address will give an error.
  // For example, on Cryptopia, asking the wallet address for e.g. boolberry, it once gave an error.
  // Indeed, it was then under investigation for a possible fork.
  string database_address_withdraw = wallets_get_address (sql, exchange_withdraw, coin);
  if (database_address_withdraw.empty ()) {
    output_block->add ({exchange_withdraw, ":", "address not available in the database"});
    output_block->to_stderr (true);
    return;
  }
  string exchange_address_withdraw;
  string exchange_payment_id_withdraw;
  exchange_get_wallet_details (exchange_withdraw, coin, exchange_address_withdraw, exchange_payment_id_withdraw);
  if (exchange_address_withdraw.empty ()) {
    output_block->add ({exchange_withdraw, coin_abbreviation_withdraw, ":", "address not available from the exchange"});
    return;
  }
  if (database_address_withdraw != exchange_address_withdraw) {
    output_block->add ({exchange_withdraw, "address from exchange is ", exchange_address_withdraw, "differs from address stored in the database", database_address_withdraw});
    output_block->to_stderr (true);
    return;
  }

  
  // Check consistency of deposit details.
  string database_address_deposit = wallets_get_address (sql, exchange_deposit, coin);
  if (database_address_deposit.empty ()) {
    output_block->add ({exchange_deposit, ":", "address not available in the database"});
    return;
  }
  string database_payment_id_deposit = wallets_get_payment_id (sql, exchange_deposit, coin);
  string exchange_address_deposit;
  string exchange_payment_id_deposit;
  exchange_get_wallet_details (exchange_deposit, coin, exchange_address_deposit, exchange_payment_id_deposit);
  if (exchange_address_deposit.empty ()) {
    output_block->add ({exchange_deposit, coin_abbreviation_deposit, ":", "address not available from the exchange"});
    return;
  }
  if (database_address_deposit != exchange_address_deposit) {
    output_block->add ({exchange_deposit, "address from exchange is ", exchange_address_deposit, "differs from address stored in the database", database_address_deposit});
    output_block->to_stderr (true);
    return;
  }
  if (database_payment_id_deposit != exchange_payment_id_deposit) {
    output_block->add ({exchange_deposit, "payment ID from exchange is ", exchange_payment_id_deposit, "differs from payment ID stored in the database", database_payment_id_deposit});
    output_block->to_stderr (true);
    return;
  }
  
  
  // Get the address where to desposit to, and if available, the payment ID.
  string address_deposit (database_address_deposit);
  output_block->add ({"Address to deposit to:", address_deposit});
  string paymentid_deposit (database_payment_id_deposit);
  if (!paymentid_deposit.empty ()) {
    output_block->add ({exchange_deposit, "payment ID to use:", paymentid_deposit});
  }
  
  
  // Optional conversions of address formats.
  // As of August 2018 this was in use for converting old and new formats for Bitcoin Cash addresses.
  string converted_deposit_address = conversions_run (sql, address_deposit);
  if (converted_deposit_address != address_deposit) {
    output_block->to_stderr (true);
    output_block->add ({"Converting deposit address", address_deposit, "to", converted_deposit_address});
    address_deposit = converted_deposit_address;
  }

  
  // Place the withdrawal order on the exchange.
  string json;
  string order_id = exchange_withdrawal_order (exchange_withdraw, coin_abbreviation_withdraw, quantity, address_deposit, paymentid_deposit, json, output);
  if (!order_id.empty ()) {
    output_block->add ({"Withdrawal order placed. Order id:", order_id});
  } else {
    // If the order could not be placed, record it as a bot error.
    to_failures (failures_type_bot_error (), exchange_withdraw, coin_abbreviation_withdraw, quantity, {"The order to withdraw", to_string (quantity), coin_abbreviation_withdraw, "from", exchange_withdraw, "could not be placed", "-", "JSON", json});
  }
  
  
  // If the withdrawal order was placed on the exchange successfully, add it to the database.
  // This is for following up on whether it gets through, and when.
  if (!order_id.empty ()) {
    transfers_enter_order (sql, order_id, exchange_withdraw, coin_abbreviation_withdraw, quantity, exchange_deposit, coin_abbreviation_deposit, address_deposit, paymentid_deposit);
    // Add it to the security breakdown check database also.
    withdrawals_enter (sql, exchange_withdraw, order_id);
  }
}


// Accurate monitoring of funds withdrawals is essential to the bot's operations.
// The following function monitors whether withdrawals, ordered on the exchanges,
// have actually been executed by the exchanges.
// There have been cases with Yobit, where withdrawals could not be monitored automatically,
// that funds evapored into thin air, and that the operator didn't know at all,
// why the bot no longer made a gain, but ran into a loss instead.
// The operator prayed to God for guidance and then it occurred to him that perhaps a loss
// was made due to Yobit.
// After no longer using Yobit, the gains started to be made again.
// Upon reflection on this, later, likely the transfer losses were more than the arbitrage gains.
void monitor_withdrawals ()
{
  to_output output ("Monitoring withdrawals");
  
  SQL sql (front_bot_ip_address ());
  
  // Query the database for orders that were placed but have not yet been executed by the exchange.
  vector <string> pending_order_ids;
  vector <string> pending_withdraw_exchanges;
  vector <string> pending_coin_abbreviations;
  vector <string> pending_deposit_exchanges;
  vector <float> pending_quantities;
  transfers_get_non_executed_orders (sql, pending_order_ids, pending_withdraw_exchanges, pending_coin_abbreviations, pending_deposit_exchanges, pending_quantities);
  
  // Iterate over the pending transfer orders read from the database.
  // If there's no such order in the database, then:
  // * No need to call the exchange's API's for further information.
  // * Skipping the call makes things work faster.
  // If there is any such orders: Query the exchanges to find out about it.
  for (unsigned int i = 0; i < pending_coin_abbreviations.size(); i++) {
    
    // Get the pending order details.
    string pending_order_id = pending_order_ids[i];
    string pending_withdraw_exchange = pending_withdraw_exchanges[i];
    string pending_coin_abbreviation = pending_coin_abbreviations[i];
    float pending_quantity = pending_quantities[i];
    
    // Get the withdrawal history of the coin on the relevant exchange.
    vector <string> exchange_order_ids;
    vector <string> exchange_coin_abbreviations;
    vector <string> exchange_coin_ids;
    vector <float> exchange_quantities;
    vector <string> exchange_addresses;
    vector <string> exchange_transaction_ids;
    exchange_withdrawalhistory (pending_withdraw_exchange,
                                pending_coin_abbreviation,
                                exchange_order_ids,
                                exchange_coin_abbreviations,
                                exchange_coin_ids,
                                exchange_quantities,
                                exchange_addresses,
                                exchange_transaction_ids);
    
    // Consider marking the order as executed when the ID of the order placed is found in the withdrawal history.
    // Because if the ID is there, it is in the process of being executed.
    // But when the transaction identifier is not yet given by the exchange, the order has not yet been executed.
    // In that case the order should not yet be marked as executed.
    // There have been a number of cases that the exchange was stuck in the "Processing" state.
    // Such an order should not be marked yet as executed.
    // Such an order can be identified by the absense of a transation ID.
    // Marking the order ID as executed records how many minutes it took to first arrive in the withdrawal history.
    for (unsigned int xs = 0; xs < exchange_order_ids.size(); xs++) {
      if (pending_order_id == exchange_order_ids[xs]) {
        if (!exchange_transaction_ids[xs].empty ()) {
          transfers_set_order_executed (sql, pending_withdraw_exchange, pending_order_id, exchange_transaction_ids[xs]);
          output.add ({"Exchange", pending_withdraw_exchange, "has executed the withdrawal of", float2string (pending_quantity), pending_coin_abbreviation, "order ID", pending_order_id, "txid", exchange_transaction_ids[xs]});
        }
      }
    }
  }
}


void monitor_poloniex_withdrawals ()
{
  to_output output ("Monitoring Poloniex withdrawals");
  
  SQL sql (front_bot_ip_address ());
  
  // Query the database for the most recent transfers.
  vector <string> database_order_ids, database_coin_abbrevs, database_txexchanges, database_txids, database_executeds, database_rxexchanges, database_addresses, database_visibles, database_arriveds;
  vector <int> database_row_ids, database_seconds;
  vector <float> database_txquantities, database_rxquantities;
  transfers_get_month (sql, database_row_ids, database_seconds, database_order_ids, database_coin_abbrevs, database_txexchanges, database_txquantities, database_txids, database_executeds, database_rxexchanges, database_addresses, database_visibles, database_rxquantities, database_arriveds);
  
  // Iterate over the most recent transfer orders read from the database.
  for (unsigned int db = 0; db < database_order_ids.size(); db++) {
    
    // Deal with the special case of withdrawals made from Poloniex.
    // The API call for making a withdrawal at Poloniex does not return the order ID.
    // The bot instead writes "poloniex" as the order ID to the database.
    // If this order ID is found, then proceed with it in order to find the real order ID.
    if (database_order_ids[db] != poloniex_get_exchange ()) continue;
    
    // Here is a withdrawal from Poloniex with an unknown order ID.
    // Get more details about this withdrawal.
    string exchange = database_txexchanges[db];
    string coin = exchange_get_coin_id (exchange, database_coin_abbrevs[db]);
    float database_txquantity = database_txquantities[db];
    string database_address = database_addresses [db];
    
    to_tty ({database_order_ids[db], float2string (database_txquantity)});
    
    // Get the withdrawal history of the matching coin on the relevant exchange.
    vector <string> exchange_order_ids;
    vector <string> exchange_coin_abbreviations;
    vector <string> exchange_coin_ids;
    vector <float> exchange_quantities;
    vector <string> exchange_addresses;
    vector <string> exchange_transaction_ids;
    exchange_withdrawalhistory (exchange, coin,
                                exchange_order_ids,
                                exchange_coin_abbreviations,
                                exchange_coin_ids,
                                exchange_quantities,
                                exchange_addresses,
                                exchange_transaction_ids);
    
    // Iterate over the withdrawal history obtained from the exchanges.
    for (size_t x = 0; x < exchange_order_ids.size(); x++) {
      
      // Match the withdrawn amount in the database
      // with the withdrawn amount in the exchange's withdrawal history.
      // Apply a small margin to make the match more robust.
      float exchange_quantity = exchange_quantities [x];
      if (database_txquantity < (0.999 * exchange_quantity)) continue;
      if (database_txquantity > (1.001 * exchange_quantity)) continue;
      
      // Match the deposit addresses as read from the database with the ones as given by the exchange withdrawal history.
      if (database_address != exchange_addresses[x]) continue;
      
      // Check that the transaction ID given by the exchange is not yet found in the database.
      string exchange_txid = exchange_transaction_ids [x];
      if (in_array (exchange_txid, database_txids)) continue;
      
      // Since all he following conditions apply, update the order ID in the database.
      int rowid = database_row_ids [db];
      string exchange_order_id = exchange_order_ids[x];
      transfers_set_order_id (sql, rowid, exchange_order_id);
      to_stderr ({"Updating Poloniex withdrawal order id to", exchange_order_id});
    }
  }
}


// Accurate monitoring of funds deposits is essential to the bot's operations.
// The following function monitors whether deposits, ordered on an exchange,
// have actually been arrived at the destination exchange.
// There have been cases with Yobit, where deposits could not be monitored automatically,
// that funds evapored into thin air, and that the operator didn't know at all,
// why the bot no longer made a gain, but ran into a loss instead.
// The operator prayed to God for guidance and then it occurred to him that perhaps a loss
// was made due to Yobit.
// After no longer using Yobit, the gains started to be made again.
void monitor_deposits_history ()
{
  to_output output ("Monitoring deposits via history");
  
  SQL sql (front_bot_ip_address ());

  // Query the database for the most recent transfers.
  vector <string> database_coin_abbrevs, database_txexchanges, database_txids, database_executeds, database_rxexchanges, database_addresses, database_visibles, database_arriveds;
  vector <int> database_ids;
  {
    vector <float> txquantities, rxquantities;
    vector <string> order_ids;
    vector <int> seconds;
    transfers_get_month (sql, database_ids, seconds, order_ids, database_coin_abbrevs, database_txexchanges, txquantities, database_txids, database_executeds, database_rxexchanges, database_addresses, database_visibles, rxquantities, database_arriveds);
  }

  // Iterate over the most recent transfers.
  for (unsigned int db = 0; db < database_coin_abbrevs.size(); db++) {

    // Skip orders that have not executed yet, as they cannot have arrived yet.
    if (database_executeds [db].empty ()) continue;

    // Skip orders that have arrived.
    if (!database_arriveds [db].empty ()) continue;

    // Skip orders that have their unconfirmed balances visible.
    if (!database_visibles [db].empty ()) continue;

    // Get the exchange where the deposit was made.
    string exchange = database_rxexchanges [db];
    
    // Get the coin identifier.
    // Currently the database contains two coin abbreviations. Usually these two are the same.
    // But there's cases that the coin abbreviations are different.
    // An example of this is BCC or BCH. Both are used for Bitcoin Cash.
    string coin = exchange_get_coin_id (exchange, database_coin_abbrevs[db]);
    
    // Get the transfer ID.
    string txid = database_txids [db];
    
    // The row ID in the database.
    int rowid = database_ids[db];

    // Get the whole deposit history from the exchange where the deposit was made.
    vector <string> exchange_coin_abbreviations;
    vector <string> exchange_coin_ids;
    vector <float> exchange_quantities;
    vector <string> exchange_order_ids;
    vector <string> exchange_addresses;
    vector <string> exchange_transaction_ids;
    exchange_deposithistory (exchange, coin,
                             exchange_order_ids,
                             exchange_coin_abbreviations,
                             exchange_coin_ids,
                             exchange_quantities,
                             exchange_addresses,
                             exchange_transaction_ids);

    // Iterate over de deposited amounts and other data, to find a match.
    for (unsigned int ex = 0; ex < exchange_coin_abbreviations.size (); ex++) {

      // Check on matching coin.
      if (coin == exchange_coin_ids[ex]) {

        // Take the transfer costs in account.
        // As a result of this, the quantity that arrived will be slightly less than what was ordered.
        // This is no longer done here, since normally it detects deposite through the unconfirmed balance.
        //float minimum_quantity = 0.80 * database_quantity;
        //float maximum_quantity = database_quantity * 1.01;
        //if ((exchange_quantities [ex] >= minimum_quantity) && (exchange_quantities [ex] <= maximum_quantity)) {
        //}
        
        // Check on matching transaction ID.
        if (txid == exchange_transaction_ids[ex]) {

          // Feedback.
          float rxquantity = exchange_quantities [ex];
          output.add ({"The deposit of", coin, "to", exchange, "is being confirmed with a balance of", float2string (rxquantity)});
          
          // Mark the order as visible and arrived when the deposit history of the exchange indicates it is there.
          // It will indicate the elapsed minutes since placing the order and when it arrived.
          transfers_set_visible (sql, rowid, rxquantity);
          transfers_set_arrived (sql, rowid);
        }
      }
    }
  }
}


// Accurate monitoring of funds deposits is essential to the bot's operations.
// The following function monitors whether deposits, ordered on an exchange,
// have actually been arrived at the destination exchange.
// There have been cases with Yobit, where deposits could not be monitored automatically,
// that funds evapored into thin air, and that the operator didn't know at all,
// why the bot no longer made a gain, but ran into a loss instead.
// The operator prayed to God for guidance and then it occurred to him that perhaps a loss
// was made due to Yobit.
// After no longer using Yobit, the gains started to be made again.
void monitor_deposits_balances ()
{
  to_output output ("Monitoring deposits via unconfirmed balances");
  
  SQL sql (front_bot_ip_address ());
  
  // Query the database for the most recent transfers.
  vector <string> order_ids, coin_abbrevs, txexchanges, txids, executeds, rxexchanges, addresses, visibles, arriveds;
  vector <int> database_ids, seconds;
  vector <float> txquantities, rxquantities;
  transfers_get_month (sql, database_ids, seconds, order_ids, coin_abbrevs, txexchanges, txquantities, txids, executeds, rxexchanges, addresses, visibles, rxquantities, arriveds);
  for (unsigned int i = 0; i < seconds.size(); i++) {

    // Skip orders that have arrived.
    if (!arriveds [i].empty ()) continue;

    // The order is visible if the balance is found unconfirmed.
    string visible = visibles [i];

    // Get the two exchanges.
    string txexchange = txexchanges [i];
    string rxexchange = rxexchanges [i];
    
    // Get the coin identifier.
    // Currently the database contains two coin abbreviations. Usually these two are the same.
    // But there's cases that the coin abbreviations are different.
    // An example of this is BCC or BCH. Both are used for Bitcoin Cash.
    string coin = exchange_get_coin_id (txexchange, coin_abbrevs[i]);

    // The quantity withdrawn.
    float txquantity = txquantities[i];

    // The row ID in the database.
    int rowid = database_ids[i];

    // Get the total and unconfirmed balances at the exchange for this coin.
    float total = 0, unconfirmed = 0;
    exchange_get_balance_cached (rxexchange, coin, &total, nullptr, nullptr, &unconfirmed);
    
    // It used to look at the exchange deposit history to find out whether the deposit had arrived.
    // But with Cryptopia, and probably other exchanges too, it had been noticed,
    // that the amount deposited in the history was lower than the real amount credited to the wallet.
    // This didn't happen always, but sometimes, and not with all coins.
    // This led to inaccurate transfer losses.
    // So here's a new way of detecting a deposit:
    // 1. Check for the unconfirmed balance.
    // 2. Write the unconfirmed quantity to the database.
    // 3. Wait till the unconfirmed balances goes back to zero.
    // 4. Record the transfer as complete.

    // The first arrival detection step is to notice when the deposited amount is first visible as an unconfirmed amount.
    // Conditions:
    // 1. It was not yet visible.
    // 2. An unconfirmed balance is visible now.
    if (visible.empty()) {
      if (unconfirmed > 0) {
        output.add ({"The deposit of", float2string (txquantity), coin, "is visible as an unconfirmed balance of", float2string (unconfirmed)});
        // Update the database with the amount that is now visible at the exchange.
        transfers_set_visible (sql, rowid, unconfirmed);
      }
    }
    
    // The second and last arrival detection step is to notice that the unconfirmed amount drops to zero now again.
    // Conditions:
    // 1. It was visible already.
    // 2. A total higher than zero is given (this handles API call failures too).
    // 3. The unconfirme balance is zero now again.
    //to_stderr ({rxexchange, coin, "visible", visible, "total", float2string (total), "unconfirmed", float2string (unconfirmed)});
    if (!visible.empty ()) {
      if (unconfirmed == 0) {
        if (total > 0) {
          output.add ({"The deposit of", float2string (txquantity), coin, "has arrived and is available for use"});
          transfers_set_arrived (sql, rowid);
        }
      }
    }
  }
}


void get_all_balances_internal (string exchange)
{
  // Get all balances in the wallets on the exchange.
  vector <string> coin_abbrev;
  vector <string> coin_ids;
  vector <float> total;
  vector <float> reserved;
  vector <float> unconfirmed;
  exchange_get_balances (exchange, coin_abbrev, coin_ids, total, reserved, unconfirmed);

  // This function is called in parallel threads.
  // It used to require the database server for querying data.
  // It no longer needs that database.
  // The following comment is left here for reference.
  // Passing a reference to SQL to this thread works fine most of the time.
  // But there's times that it says that the SQL server went away.
  // Obviously there's a problem here.
  // The solution is to use a new SQL server within each thread.

  // Store all the balances in memory.
  // In case of errors in the API calls, nothing will be stored, since any data would have been cleared.
  for (unsigned int i = 0; i < coin_ids.size (); i++) {
  
    string coin_identifier = coin_ids[i];
    
    // Skip and report empty coin identifier.
    if (coin_identifier.empty ()) {
      // Dont' report a zero balance as that makes no sense.
      // Yobit has been seen giving coins with zero balance.
      // Skip those.
      if (total[i] > 0) {
        SQL sql (front_bot_ip_address ());
        failures_store (sql, failures_type_bot_error (), exchange, coin_abbrev[i], total[i], "This coin has a balance but the bot cannot report it because it does not know the coin's identifier");
      }
      continue;
    }

    // Store the balances in the memory caches.
    exchange_set_balance_cache (exchange, coin_identifier, total[i], total [i] - reserved [i] - unconfirmed [i], reserved[i], unconfirmed[i]);
  }
}


// Gets all balances for all coins identifiers in all wallets on all exchanges.
void get_all_balances ()
{
  // Storage for parallel jobs.
  vector <thread *> jobs;
  // Start threads for all exchanges.
  vector <string> exchanges = exchange_get_names ();
  for (auto exchange : exchanges) {
    thread * job = new thread (get_all_balances_internal, exchange);
    jobs.push_back (job);
  }
  // Wait till all threads have completed.
  for (auto job : jobs) {
    job->join ();
  }
}


// This records the balances in the database.
void record_all_balances ()
{
  // The current number of seconds since the Unix Epoch.
  // Should be the same throughout this recording session.
  // This enables SQL grouping of values per session.
  int seconds = seconds_since_epoch ();
  
  
  // Use one connection to the database for all operations.
  SQL sql (front_bot_ip_address ());
 

  // Store most recently recorded Bitcoin balances for all exchanges.
  map <string, float> most_recent_btc_balances;
  {
    vector <string> exchanges, coins;
    vector <float> totals;
    balances_get_current (sql, exchanges, coins, totals);
    for (size_t i = 0; i < exchanges.size(); i++) {
      if (coins[i] == bitcoin_id ()) {
        string exchange = exchanges[i];
        float total = totals[i];
        most_recent_btc_balances [exchange] = total;
      }
    }
  }

  
  // Get the most recent second currently recorded in the balances database.
  int most_recent_recorded_second = 0;
  {
    sql.clear ();
    sql.add ("SELECT MAX(seconds) FROM balances;");
    map <string, vector <string> > result = sql.query ();
    vector <string> seconds = result ["MAX(seconds)"];
    if (!seconds.empty ()) {
      most_recent_recorded_second = str2int (seconds[0]);
    }
  }
  
  
  vector <string> exchanges = exchange_get_names ();
  
  
  // Do one large mass reading of all rates and keep them in memory for super fast access.
  // This avoids deadlocks.
  // Deadlocks have been seen when querying the rates while inserting at the same time.
  unordered_map <string, float> allrates;
  {
    vector <int> seconds;
    vector <string> exchanges, markets, coins;
    vector <float> bids, asks, rates;
    rates_get (sql, seconds, exchanges, markets, coins, bids, asks, rates, true);
    for (unsigned int i = 0; i < rates.size (); i++) {
      string exchange = exchanges[i];
      string market = markets[i];
      string coin = coins [i];
      float rate = rates[i];
      allrates [exchange + market + coin] = rate;
    }
  }
  
  
  // Iterate over all the exchanges.
  for (auto exchange : exchanges) {
    
    
    // Iterate over all the coins at the exchange that have a certain balance.
    vector <string> coins = exchange_get_coins_with_balances (exchange);
    for (auto coin : coins) {

      
      // Get the balances from the memory cache.
      float total, available, reserved, unconfirmed;
      exchange_get_balance_cached (exchange, coin, &total, &available, &reserved, &unconfirmed);

      
      // Email on sudden relatively large drop in Bitcoin balance.
      if (coin == bitcoin_id ()) {
        float previous_btc_total = most_recent_btc_balances [exchange];
        float current_btc_total = total;
        float percentage = get_percentage_change (previous_btc_total, current_btc_total);
        if (percentage < -30) {
          to_stderr ({"The Bitcoin balance at exchange", exchange, "changed from", float2string (previous_btc_total), "to", float2string (current_btc_total), "which is a change of", float2visual (percentage), "% which may be an indicator of a security breach"});
        }
      }
      
      // Record the balances into the database.
      sql.clear ();
      sql.add ("INSERT INTO balances (seconds, exchange, coin, total, btc, euro) VALUES (");
      sql.add (seconds);
      sql.add (",");
      sql.add (exchange);
      sql.add (",");
      sql.add (coin);
      sql.add (",");
      // The total balance for this coin.
      // This includes also the reserved and unconfirmed amounts.
      sql.add (total);
      sql.add (",");
      // Calculate the value of this expressed in Bitcoins with the current rate.
      float btc = total * allrates [exchange + bitcoin_id () + coin];
      sql.add (btc);
      sql.add (",");
      // Calculate the value of this expressed in Euros with the current rate.
      float euro = bitcoin2euro (btc);
      sql.add (euro);
      sql.add (");");
      sql.execute ();

      
      // Output the balances visibly.
      string reserved_fragment;
      if (reserved != 0) {
        reserved_fragment = "(reserved: " + float2string (reserved) + ")";
      }
      string unconfirmed_fragment;
      if (unconfirmed) {
        unconfirmed_fragment = "(unconfirmed: " + float2string (unconfirmed) + ")";
      }
      to_stdout ({exchange, "has", float2string (total), coin, reserved_fragment, unconfirmed_fragment});
    }


    // In case of errors while communicating with the exchange, there won't be any coins.
    // Handle this situation as follows:
    // Copy the values from the most recent recorded second to the current second.
    // The result of this will be this:
    // The balances graphs will not fluctuate due to API call errors.
    if (coins.empty ()) {
      to_stdout ({"Cannot obtain current balances from", exchange, "so recording the most recent balances"});
      sql.clear ();
      sql.add ("INSERT INTO balances (seconds, exchange, coin, total, btc, euro)");
      sql.add ("SELECT");
      sql.add (seconds);
      sql.add (", exchange, coin, total, btc, euro FROM balances WHERE seconds =");
      sql.add (most_recent_recorded_second);
      sql.add ("AND exchange =");
      sql.add (exchange);
      sql.add (";");
      sql.execute ();
    }
  }
}


// This calls the exchange coin redistributors.
void redistribute_all_coins (vector <tuple <string, string, float> > & withdrawals)
{
  // In general, balancing the coins over the exchanges leads to improved trading.
  // If doing balancing, the bot cannot be easily left unattended.
  // At times balances don't get withdrawn, or don't get deposited.
  // This requires follow-up with the exchanges.
  // Near the end of 2017, the help desk of the exchanges were so busy
  // that they have not attended to stuck withdrawals for months now.
  // This means that currently it's not possible to do coin balancing at a large scale.
  // Another issue is that the cost of withdrawing and depositing at times
  // may be more than the income gained by trading.

  // Update 1: Checking on July 2018, the prices of withdrawals have dropped dramatically.
  // And the helpdesks of the exchanges are back at capacity again.
  // So it's worth trying to do the coin balancing again.

  // Iterate over the active coins in the wallets in preparation of redistributing them.
  SQL sql (front_bot_ip_address ());
  vector <string> active_coins = wallets_get_balancing_coins (sql);
  for (auto coin : active_coins) {
    redistribute_coin (coin, withdrawals);
  }
}


// This is some core logic that processes and decides whether to transfer a coin between exchanges.
// The core logic is separated to enable regression testing without doing real transfers.
void redistribute_coin_processor (void * output,
                                  string coin,
                                  bool carrier,
                                  vector <string> exchanges,
                                  int active_transfer_count, int last_transfer_elapsed_seconds,
                                  vector <string> & exchanges_withdraw,
                                  vector <string> & exchanges_deposit,
                                  vector <float> & transfer_quantities)
{
  // The possible object to write the output to in bundled format.
  to_output * output_block = NULL;
  if (output) output_block = (to_output *) output;
  
  // Clear the result containers.
  exchanges_withdraw.clear ();
  exchanges_deposit.clear ();
  transfer_quantities.clear ();
  
  // Check that all exchanges have provided their balances.
  // In case of communications errors with a certain exchange,
  // there would not be balances for this exchange.
  // This exchange should then be removed from the exchanges to be considered for balancing.
  // If it were not removed, then the balance on that exchange would be zero.
  // And a certain amount would be deposited to that exchange.
  // This deposit would be in error, and could lead to possible lost funds.
  // Also remove exchanges that provide a zero-balance, in error, for the same reasons.
  // It has been noticed even, that an exchanges provided negative balances: Remove those too.
  {
    vector <string> live_exchanges;
    for (auto exchange : exchanges) {
      float amount = 0;
      exchange_get_balance_cached (exchange, coin, &amount, nullptr, nullptr, nullptr);
      if (amount > 0) {
        live_exchanges.push_back (exchange);
      }
      else {
        if (output) output_block->add ({"Omitting", exchange, "because it has a zero", coin, "balance"});
      }
    }
    exchanges = live_exchanges;
  }
  
  // Needs at least two exchanges.
  if (exchanges.size () < 2) {
    // Error messages about less than two exchanges.
    if (output) {
      output_block->add ({"Needs at least two active exchanges for", coin, "balancing"});
      // If there are fewer than two exchanges for this coin identifier, list the exchanges it has instead.
      // This may be useful for debugging purposes.
      vector <string> messages = exchanges;
      messages.insert (messages.begin (), "Active exchanges:");
      output_block->add (messages);
    }
    return;
  }

  // Calculate the sum of the balances on all of the exchanges.
  float summed_balance = 0;
  for (auto exchange : exchanges) {
    float total = 0, reserved = 0, unconfirmed = 0;
    exchange_get_balance_cached (exchange, coin, &total, nullptr, &reserved, &unconfirmed);
    summed_balance += total;
    summed_balance -= reserved;
    summed_balance -= unconfirmed;
    if (output) output_block->add ({"Exchange", exchange, "has", float2string (total - reserved - unconfirmed), coin});
  }
  if (output) output_block->add ({"Total available balance on all", to_string (exchanges.size()), "exchanges is", float2string (summed_balance), coin});
  float optimal_balance = summed_balance / exchanges.size();
  if (output) output_block->add ({"Optimal balance on each exchange is", float2string (optimal_balance), coin});
  
  // Low balance threshold.
  // It used to be a factor 0.4.
  // But that led to regular Bitcoin (and other coin) redistributions.
  // Since the Bitcoin miners' fees are high, it is better to keep the balancing operations as few as possible.
  // There are more coins now that have high transfer fees.
  // So the factor is much lower now.
  // It was updated to 0.1.
  // But when trading with very small coins, it would not balance
  // because dust trade prevented this balance from getting low enough to start a balancing operation.
  // The factor was updated to 0.2.
  float minimum_balance = optimal_balance * 0.2;
  // In case of carrier coins,
  // the low balance threshold is such
  // that there's always enough of the carrier coins available
  // to carry other coins to other exchanges.
  if (carrier) {
    minimum_balance = optimal_balance * 0.25;
    if (output) output_block->add ({"The", coin, "is a carrier"});
  }

  // Look for the exchange with the highest balance, and the one with the lowest balance.
  float highest_balance = numeric_limits<float>::min();
  string richest_exchange;
  float lowest_balance = numeric_limits<float>::max();
  string poorest_exchange;
  for (auto exchange : exchanges) {
    // Use available balance.
    float total = 0, reserved = 0, unconfirmed = 0;
    exchange_get_balance_cached (exchange, coin, &total, nullptr, &reserved, &unconfirmed);
    float balance = total - reserved - unconfirmed;
    if (balance < lowest_balance) {
      lowest_balance = balance;
      poorest_exchange = exchange;
    }
    if (balance > highest_balance) {
      highest_balance = balance;
      richest_exchange = exchange;
    }
  }
  
  // If the balance on the poorest exchange is too low, it should balance.
  // If that balance is not too low, everything can be left as it is.
  string exchange_withdraw = richest_exchange;
  string exchange_deposit = poorest_exchange;
  if (lowest_balance >= minimum_balance) {
    return;
  }
  
  // Handle cases where there's active transfers for this coin.
  if (active_transfer_count > 0) {
    if (output) output_block->add ({"Cannot belance because there is an active transfer order for", coin});
    return;
  }
  
  // Handle cases where the previous transfer for this coin is too recent.
  if (last_transfer_elapsed_seconds < 600) {
    if (output) output_block->add ({"Cannot balance because the previous transfer for", coin, "was done only", to_string (last_transfer_elapsed_seconds / 60), "minutes ago"});
    return;
  }
  
  // Check if there's unconfirmed balances on any of the exchanges for this coin.
  for (auto exchange : exchanges) {
    float unconfirmed = 0;
    exchange_get_balance_cached (exchange, coin, nullptr, nullptr, nullptr, &unconfirmed);
    float quantity = unconfirmed;
    if (quantity > 0) {
      if (output) output_block->add ({"Cannot balance because the unconfirmed balance on", exchange, "is", float2string (quantity), coin});
      return;
    }
  }
  
  // Ensure that a Bitcoin transfer is not dore more often than once a day.
  // This is for security reasons.
  // Because if a foregin entity steals some Bitcoins,
  // then the bot will try to balance the funds again over the exchanges.
  // So it's important to keep those balancing operations low.
  // This prevents all Bitcoins from leaking away through a security hole on one of the exchanges.
  if (coin == bitcoin_id ()) {
    if (last_transfer_elapsed_seconds < 86400) {
      if (output) output_block->add ({"Cannot balance Bitcoins because the previous operations was done only", to_string (last_transfer_elapsed_seconds / 3600), "hours ago"});
      if (output) output_block->to_stderr (true);
      return;
    }
  }
  
  // All checks have passed: It is okay to balance now.
  float quantity = optimal_balance - lowest_balance;
  if (output) output_block->add ({"Balancing: Transfer", float2string (quantity), coin, "from", exchange_withdraw, "to", exchange_deposit});

  // Store the details for the transfer order.
  exchanges_withdraw.push_back (exchange_withdraw);
  exchanges_deposit.push_back (exchange_deposit);
  transfer_quantities.push_back (quantity);
}


void redistribute_coin (string coin, vector <tuple <string, string, float> > & withdrawals)
{
  // Use one connection to the database for all operations.
  SQL sql (front_bot_ip_address ());

  // Get the balancing exchanges that have this coin in preparation of balancing funds on them.
  vector <string> exchanges = wallets_get_balancing_exchanges (sql, coin);
  
  // Get and store the coin abbreviations for the active exchanges as container [exchange] = coin abbreviation.
  map <string, string> coin_abbreviations;
  for (auto exchange : exchanges) {
    string coin_abbreviation = exchange_get_coin_abbrev (exchange, coin);
    coin_abbreviations [exchange] = coin_abbreviation;
  }

  // Check if there's active transfers recorded in the database.
  // Check how long ago the previous transfer was done.
  int active_transfer_count;
  int last_transfer_elapsed_seconds;
  {
    string coin_abbreviation = coin_abbreviations[exchanges[0]];
    active_transfer_count = transfers_get_active_orders (sql, coin_abbreviation, exchanges);
    last_transfer_elapsed_seconds = transfers_get_seconds_since_last_order (sql, coin_abbreviation, exchanges);
  }

  // Bundled output.
  to_output output ("");
  
  // Storage for the transfers to make.
  vector <string> exchanges_withdraw;
  vector <string> exchanges_deposit;
  vector <float> transfer_quantities;
  
  // Run the core coin redistribution processor.
  redistribute_coin_processor (&output, coin, false, exchanges,
                               active_transfer_count, last_transfer_elapsed_seconds,
                               exchanges_withdraw, exchanges_deposit, transfer_quantities);
  
  // Iterate over de data to place the transfer orders, if any.
  for (size_t i = 0; i < exchanges_withdraw.size (); i++) {
    // Obtain the values from the containers.
    float quantity = transfer_quantities[i];
    string exchange_withdraw = exchanges_withdraw[i];
    string exchange_deposit = exchanges_deposit[i];
    // Handle a green lane for the exchange where to withdraw.
    // An exchange may have a green lane.
    // In that case, a withdrawal does not need additional approval from the user.
    string greenlane = wallets_get_greenlane (sql, exchange_withdraw, coin);
    if (!greenlane.empty ()) {
      // This green lane updates the exchange where to deposit.
      vector <string> exchanges = exchange_get_names ();
      output.add ({"Green lane for", coin, "at", exchange_withdraw, "is", greenlane});
      for (auto exchange : exchanges) {
        string address = wallets_get_address (sql, exchange, coin);
        if (address == greenlane) {
          output.add ({"Green lane to", exchange, "instead of to", exchange_deposit});
          exchange_deposit = exchange;
        }
      }
    }
    // If the exchange or coin requires, slightly update the amount to transfer.
    float old_quantity = quantity;
    wallets_update_withdrawal_amount (sql, exchange_withdraw, coin, quantity);
    if (quantity != old_quantity) {
      // Normally it uses "float2string", but this upsets Bittrex's Neo withdrawals.
      // So for consistency, it uses "to_string" here and when withdrawinng from Bittrex.
      output.add ({"The quantity was updated to", to_string (quantity), coin});
    }
    // Check that the amount to withdraw is not too low.
    float minimum_withdrawal_amount = get_minimum_withdrawal_amount (coin, exchange_withdraw);
    if (minimum_withdrawal_amount > 0) {
      output.add ({"Minimum withdrawal amount:", to_string (minimum_withdrawal_amount)});
      // Use a slight margin due to rounding errors.
      if (quantity < (minimum_withdrawal_amount * 1.001)) {
        output.to_stderr (true);
        output.add ({"This is below the minimum amount so the withdrawal was cancelled"});
        output.add ({"This may indicate too low a total balance of", coin, "to trade with"});
        // Break here.
        continue;
      }
    }
    // Place the withdrawal order.
    string coin_abbreviation_withdraw = coin_abbreviations [exchange_withdraw];
    string coin_abbreviation_deposit = coin_abbreviations [exchange_deposit];
    order_transfer (&output, exchange_withdraw, exchange_deposit, coin, quantity, false);
    // Store the withdrawal: 0: exchange | 1: coin_id | 2: withdrawn amount.
    withdrawals.push_back (make_tuple (exchange_withdraw, coin, quantity));
    // Conditionally update the database with prices coins were bought for.
    redistribute_coin_bought_price (exchange_withdraw, exchange_deposit, bitcoin_id (), coin, 0);
    // For security reasons, if a Bitcoin transfer operation is done, email the operator.
    // The reason is that if a foreign entity withdraws Bitcoins to a foreign address,
    // then the bot will start to transfer funds to get the Bitcoins in balances again.
    // So it's good to be informed of a Bitcoin transfer.
    if (coin == bitcoin_id ()) {
      output.to_stderr (true);
    }
  }
}


// Show the balances for the $coin_ids at the $exchanges.
void show_balances (void * output, vector <string> exchanges, vector <string> coins)
{
  to_output * output_block = (to_output *) output;
  for (auto exchange : exchanges) {
    for (auto coin : coins) {
      // Text fragments for output.
      vector <string> fragments;
      // Display the total balances.
      float total, available, reserved, unconfirmed;
      exchange_get_balance_cached (exchange, coin, &total, &available, &reserved, &unconfirmed);
      fragments = { exchange, coin, "total", float2string (total), "available", float2string (available) };
      if (reserved != 0) {
        fragments.push_back ("reserved");
        fragments.push_back (float2string (reserved));
      }
      if (unconfirmed != 0) {
        fragments.push_back ("unconfirmed");
        fragments.push_back (float2string (unconfirmed));
      }
      output_block->add (fragments);
    }
  }
}


// Temporarily set the exchange/market/coin non-trading.
void temporarily_pause_trading (string exchange, string market, string coin, int minutes, string reason)
{
  // Upon a trade failure, it used to disable balancing for this coin at this exchange.
  // This led to the following situation, at times:
  // * Trade failure -> disable balancing.
  // * User enters trade manually -> fails due to shortage of available funds in the wallet.
  // So to fix this, it now no longer disables balancing this coin.
  // This allows the bot to fill up a low balance.
  // Then when the user wants to enter the trade manually, there's more of a change that funds are now available.
  // It still, and of course, continues to disable trading this coin at this exchange.
  // Secondly, upon trade failure, it used to disable trading this coin at this exchange.
  // But then the wallet would remain disabled till the user would intervene.
  // And that can take a long time, depending on the availability of the operator.
  // So now it temporarily disables trading, just for a short time.
  // Then the monitor will enable it again after that time has expired.
  // It has been seen at Cryptopia that this error comes:
  // cryptopia_limit_trade ﻿{"Success":false,"Error":"Nonce has already been used for this request."}
  // After checking, in most cases, the trade order has been placed, despite the error message.
  SQL sql (front_bot_ip_address ());
  // Disable trading the exchange/market/coin for a number of minutes.
  int seconds = seconds_since_epoch () + (minutes * 60);
  pausetrade_set (sql, exchange, market, coin, seconds, reason);
}


void monitor_foreign_withdrawals ()
{
  to_tty ({"Monitoring withdrawals"});
  
  // Database access.
  SQL sql (front_bot_ip_address ());
  
  // Iterate over all known exchanges.
  vector <string> exchanges = exchange_get_names ();
  for (auto exchange : exchanges) {
    
    // Iterate over the coin identifiers of the relevant wallets on that exchange.
    vector <string> coin_ids = wallets_get_coin_ids (sql, exchange);
    for (auto coin_id : coin_ids) {
      
      // Get the coin abbreviation.
      string coin_abbrev = exchange_get_coin_abbrev (exchange, coin_id);
      
      // Get the withdrawal history plus their details for this coin at this exchange.
      // Plus other relevant details
      vector <string> order_ids;
      vector <string> coin_abbreviations;
      vector <string> coin_ids2;
      vector <float> quantities;
      vector <string> addresses;
      vector <string> transaction_ids;
      exchange_withdrawalhistory (exchange, coin_abbrev,
                                  order_ids, coin_abbreviations, coin_ids2, quantities, addresses, transaction_ids);
      
      // Iterate over all orders.
      for (unsigned int history = 0; history < order_ids.size(); history++) {
        
        // Check whether the order is present in the database with all recognized withdrawals.
        bool present = withdrawals_present (sql, exchange, order_ids[history]);
        if (!present) {

          // It is not present.
          // Get more details about this suspect withdrawal.
          string suspect_exchange (exchange);
          string suspect_order_id (order_ids[history]);
          string suspect_address = addresses[history];
          float suspect_quantity = quantities[history];
          string suspect_coin_abbrev (coin_abbreviations[history]);
          
          // Record the order id, so it reports only once.
          withdrawals_enter (sql, suspect_exchange, suspect_order_id);

          // Report it to the operator.
          to_output output ("");
          output.to_stderr (true);
          output.add ({"Unrecognized withdrawal of", float2string (suspect_quantity), suspect_coin_abbrev, "from", suspect_exchange, "to address", suspect_address, "order id", suspect_order_id});

          // Check whether the withdrawal was made to one of the bot's known addresses.
          vector <string> known_exchanges;
          vector <string> known_coin_ids;
          vector <string> known_addresses;
          wallet_get_details (sql, known_exchanges, known_coin_ids, known_addresses);
          bool withdrawal_recognized = false;
          for (size_t known = 0; known < known_addresses.size(); known++) {
            string known_address = known_addresses [known];
            if (suspect_address == known_address) {
              output.add ({"It appears the withdrawal was made to a known address at", known_exchanges[known], "so it is okay"});
              output.to_stderr (false);
              // Set a flag.
              withdrawal_recognized = true;
              // Enter this order for following up on this transfer.
              output.add ({"Recording this transfer for automatic follow-up"});
              {
                // While getting the turnover amounts for both of the exchanges,
                // there is a potential problem:
                // When an exchange has already processed the withdrawal, or the deposit,
                // it means that their turnover values have increased already with the withdrawn or deposited amount.
                // Thus the detector whether a withdrawal has been executed,
                // and the detector whether a deposit has arrived,
                // if they base the detection algorithm on the turnover values, would fail.
                string dummy;
                exchange_get_wallet_details (suspect_exchange, suspect_coin_abbrev, dummy, dummy);
                exchange_get_wallet_details (known_exchanges[known], exchange_get_coin_abbrev (known_exchanges[known], known_coin_ids[known]), dummy, dummy);
              }
              transfers_enter_order (sql, suspect_order_id, suspect_exchange, suspect_coin_abbrev, suspect_quantity, known_exchanges[known], exchange_get_coin_abbrev (known_exchanges[known], known_coin_ids[known]), known_address, "");
            }
          }

          if (!withdrawal_recognized) {
            // Unrecognized withdrawal.
            // Disable all coins from balancing on this exchange.
            vector <string> balancing_coins = wallets_get_balancing_coins (sql);
            for (auto coin_id : balancing_coins) {
              wallets_set_balancing (sql, suspect_exchange, coin_id, false);
            }

            // Give extra information.
            to_stderr ({"This may indicate a security breakdown, so to prevent the other exchanges from filling up the wallet again through their balancing operations, balancing of all coins at", exchange, "has been disabled"});
          }
        }
      }
      
      // Some exchanges return only the orders for the coin abbreviations passed in the call.
      // Other exchanges just return all the orders, regardless of the coin abbreviations passed to the call.
      // So here check whether there's multiple coin abbreviations, if so, we've got all orders now.
      set <string> distinct_coins (coin_abbreviations.begin(), coin_abbreviations.end());
      if (distinct_coins.size() != 1) break;
    }
  }
  to_tty ({"Ready checking the withdrawals"});
}


// The daily bookkeeping record.
void daily_book_keeper (bool mail)
{
  string title = "Daily bookkeeper";

  // Storage for the output to be mailed or sent to the terminal.
  vector <vector <string> > output;
  
  SQL sql_front (front_bot_ip_address ());
  SQL sql_back (back_bot_ip_address ());

  // Get the markets that had trades during the past day.
  vector <string> markets = trades_get_coins (sql_front);

  // The gains per coin per market for the past 24 hours.
  {
    // Get all tradable coins.
    // This makes sure that the output will also include coins that didn't trade past 24 hours.
    vector <string> trading_coins = wallets_get_trading_coins (sql_front);

    // Iterate over the distinct traded markets for the past day.
    for (auto market : markets) {

      // New header.
      output.push_back ({""});
      output.push_back ({"Gains per coin", "@", market});
      output.push_back ({""});

      map <string, float> coins_gains;
      trades_get_daily_coin_statistics (sql_front, market, coins_gains);
      float total_gain = 0;
      
      vector <string> lines;
      for (auto coin : trading_coins) {
        total_gain += coins_gains[coin];
        lines.push_back (to_string (coins_gains[coin]) + " " + coin);
      }
      // Display most profitable coin first.
      sort (lines.begin(), lines.end());
      reverse (lines.begin(), lines.end());
      for (auto line : lines) {
        output.push_back ({line});
      }
      output.push_back ({""});
      output.push_back ({to_string (total_gain), "total", market});
    }
  }
  
  // The gains per market @ exchange over the last 24 hours.
  {
    output.push_back ({""});
    output.push_back ({"Gains per exchange"});
    output.push_back ({""});
    for (auto market : markets) {
      map <string, float> exchange_gain;
      trades_get_daily_exchange_statistics (sql_front, market, exchange_gain);
      // Get all possible exchanges.
      // This makes sure that the output will also include exchanges that didn't trade past 24 hours.
      vector <string> exchanges = exchange_get_names ();
      vector <string> output;
      for (auto exchange : exchanges) {
        output.push_back (to_string (exchange_gain[exchange]) + " " + market + " @ " + exchange);
      }
      // Display most profitable exchange first.
      sort (output.begin(), output.end());
      reverse (output.begin(), output.end());
      for (auto line : output) {
        output.push_back ({line});
      }
    }
  }
  
  // The realistic gains based on the actual balances, over the last 24 hours.
  {
    output.push_back ({""});
    output.push_back ({"Gains at balance sheet of two days ago, yesterday, today"});
    output.push_back ({""});
    vector <string> trading_exchange1s, trading_exchange2s, trading_markets, trading_coins;
    vector <int> trading_days;
    pairs_get (sql_front, trading_exchange1s, trading_exchange2s, trading_markets, trading_coins, trading_days);
    for (unsigned int i = 0; i < trading_exchange1s.size (); i++) {
      vector <string> exchanges = {trading_exchange1s[i], trading_exchange2s[i]};
      for (auto exchange : exchanges) {
        output.push_back ({"Average balance of the coins converted to Bitcoins:"});
        float average1 = balances_get_average_hourly_btc_equivalent (sql_back, exchange, "", 72, 48);
        float average2 = balances_get_average_hourly_btc_equivalent (sql_back, exchange, "", 48, 24);
        float average3 = balances_get_average_hourly_btc_equivalent (sql_back, exchange, "", 24, 0);
        output.push_back ({exchange, to_string (average1), to_string (average2), to_string (average3)});
        output.push_back ({"Average balance of the Bitcoins:"});
        average1 = balances_get_average_hourly_btc_equivalent (sql_back, exchange, bitcoin_id (), 72, 48);
        average2 = balances_get_average_hourly_btc_equivalent (sql_back, exchange, bitcoin_id (), 48, 24);
        average3 = balances_get_average_hourly_btc_equivalent (sql_back, exchange, bitcoin_id (), 24, 0);
        output.push_back ({exchange, to_string (average1), to_string (average2), to_string (average3)});
      }
    }
    output.push_back ({"Total average balance of all coins converted to Bitcoins:"});
    float average1 = balances_get_average_hourly_btc_equivalent (sql_back, "", "", 72, 48);
    float average2 = balances_get_average_hourly_btc_equivalent (sql_back, "", "", 48, 24);
    float average3 = balances_get_average_hourly_btc_equivalent (sql_back, "", "", 24, 0);
    output.push_back ({to_string (average1), to_string (average2), to_string (average3)});
    output.push_back ({"Total average balance of all Bitcoins:"});
    average1 = balances_get_average_hourly_btc_equivalent (sql_back, "", bitcoin_id (), 72, 48);
    average2 = balances_get_average_hourly_btc_equivalent (sql_back, "", bitcoin_id (), 48, 24);
    average3 = balances_get_average_hourly_btc_equivalent (sql_back, "", bitcoin_id (), 24, 0);
    output.push_back ({to_string (average1), to_string (average2), to_string (average3)});
  }

  // Output.
  if (mail) {
    to_email to_mail (title);
    for (auto line : output) {
      to_mail.add (line);
    }
  } else {
    to_tty ({title});
    for (auto line : output) {
      to_tty (line);
    }
  }
}


void failures_reporter (bool mail)
{
  SQL sql (front_bot_ip_address ());

  string title = "Failures";
  
  // Storage for the output to be mailed or else to be sent to the terminal.
  vector <vector <string> > output;

  // The failed important API calls, grouped by failure.
  {
    vector <string> timestamps;
    vector <string> exchanges;
    vector <string> coins;
    vector <float> quantities;
    vector <string> messages;
    failures_retrieve (sql, failures_type_api_error (), timestamps, exchanges, coins, quantities, messages);
    if (!messages.empty ()) {
      output.push_back ({" "});
      output.push_back ({"Failed important API calls"});
      map <string, int> failures;
      for (unsigned int i = 0; i < messages.size(); i++) {
        failures [messages[i]]++;
      }
      for (auto element : failures) {
        output.push_back ({element.first, "-", to_string (element.second)});
      }
    }
  }
  
  // The failed important bot errors, grouped by failure.
  {
    vector <string> timestamps;
    vector <string> exchanges;
    vector <string> coins;
    vector <float> quantities;
    vector <string> messages;
    failures_retrieve (sql, failures_type_bot_error (), timestamps, exchanges, coins, quantities, messages);
    if (!messages.empty ()) {
      output.push_back ({" "});
      output.push_back ({"Failed important bot operations"});
      map <string, int> failures;
      for (unsigned int i = 0; i < messages.size(); i++) {
        string message = exchanges [i] + " " + coins[i] + " " + to_string (quantities[i]) + " " + messages[i];
        failures [message]++;
      }
      for (auto element : failures) {
        output.push_back ({element.first, "-", to_string (element.second)});
      }
    }
  }
  
  // The time-sensitive timeouts.
  {
    map <string, int> exchanges_success_count;
    {
      vector <string> timestamps;
      vector <string> exchanges;
      vector <string> coins;
      vector <float> quantities;
      vector <string> messages;
      failures_retrieve (sql, failures_hurried_call_succeeded (), timestamps, exchanges, coins, quantities, messages);
      for (unsigned int i = 0; i < exchanges.size(); i++) {
        string exchange = exchanges [i];
        int count = round (quantities[i]);
        exchanges_success_count [exchange] += count;
      }
    }
    map <string, int> exchanges_fail_count;
    {
      vector <string> timestamps;
      vector <string> exchanges;
      vector <string> coins;
      vector <float> quantities;
      vector <string> messages;
      failures_retrieve (sql, failures_hurried_call_failed (), timestamps, exchanges, coins, quantities, messages);
      for (unsigned int i = 0; i < exchanges.size(); i++) {
        string exchange = exchanges [i];
        int count = round (quantities[i]);
        exchanges_fail_count [exchange] += count;
      }
    }
    if (!exchanges_fail_count.empty ()) {
      output.push_back ({" "});
      output.push_back ({"Timed out time-sensitive calls"});
      for (auto element : exchanges_fail_count) {
        string exchange = element.first;
        int fails = element.second;
        int passes = exchanges_success_count [exchange];
        float percentage = 100 * float (fails) / (fails + passes);
        output.push_back ({exchange, "failed", to_string (fails), "passed", to_string (passes), "failure rate",         float2visual (percentage), "%"});
      }
    }
  }
  
  
  // Clear expired entries on both bots.
  failures_expire (sql);
  
  // Output.
  if (mail) {
    to_email to_mail (title);
    for (auto line : output) {
      to_mail.add (line);
    }
  } else {
    to_tty ({title});
    for (auto line : output) {
      to_tty (line);
    }
  }
}


// The open orders statistics.
void orders_reporter ()
{
  to_email mail ("Open orders");

  map <string, int> total_orders;
  map <string, float> total_coins;

  vector <string> exchanges = exchange_get_names ();
  for (auto exchange : exchanges) {

    map <string, float> market_coins;
    map <string, int> market_orders;

    vector <string> identifier;
    vector <string> market;
    vector <string> dummy;
    vector <string> coin_ids;
    vector <string> buy_sell;
    vector <float> quantity;
    vector <float> rate;
    vector <string> date;
    string error;
    exchange_get_open_orders (exchange, identifier, market, dummy, coin_ids, buy_sell, quantity, rate, date, error);
    if (identifier.empty ()) continue;
    for (unsigned int i = 0; i < identifier.size(); i++) {
      string market_id = market[i];
      string coin_identifier = coin_ids[i];
      // Find out how may days this order has been active.
      int order_seconds = exchange_parse_date (exchange, date[i]);
      int order_age = seconds_since_epoch () - order_seconds;
      order_age /= 86400;
      // Feedback.
      vector <string> order_description = {exchange, buy_sell[i], float2string (quantity[i]), coin_identifier, "at rate", float2string (rate[i]), "total", float2string (quantity[i] * rate[i]), market_id, to_string (order_age), "days old - id", identifier [i]};
      mail.add (order_description);
      // Cancel stale order.
      if (order_age > 7) {
        bool cancelled = exchange_cancel_order (exchange, identifier[i]);
        if (cancelled) {
          mail.add ({"This order has now been cancelled"});
        } else {
          vector <string> message = {"This order was supposed to be cancelled but it failed to"};
          mail.add (message);
          // Since this order failed to be cancelled, this could have impact on trading, so send it to the user.
          to_stderr (order_description);
          to_stderr (message);
        }
      }
      // Add to the exchange's total Bitcoins or market coins.
      market_coins [market_id] += (quantity[i] * rate[i]);
      market_orders [market_id]++;
    }
    // Spacing.
    mail.add ({" "});
    if (!identifier.empty()) {
      for (auto element : market_coins) {
        string market = element.first;
        float coins = element.second;
        mail.add ({exchange, "has", to_string (market_orders[market]), "orders", float2string (coins), market});
        total_orders [market] += market_orders[market];
        total_coins [market] += coins;
      }
    }
    // Spacing.
    mail.add ({" "});
  }
  
  // Totals.
  for (auto element : total_orders) {
    string market = element.first;
    mail.add ({"Total", to_string (element.second), "orders", float2string (total_coins[market]), market});
  }
  if (!total_orders.empty ()) {
    mail.add ({" "});
    mail.add ({"Orders for coins that remain open for too long are not good for arbitrage trading"});
  }
}


vector <string> get_trading_coins (const vector <pair <string, string> > & exchange_coin_wallets,
                                   const vector <tuple <string, string, string> > & exchange_market_coin_triplets)
{
  // Get the distinct trading coins from the wallets.
  vector <string> distinct_wallet_coins;
  {
    set <string> distinct_coins;
    for (auto & element : exchange_coin_wallets) {
      distinct_coins.insert (element.second);
    }
    distinct_wallet_coins.assign (distinct_coins.begin (), distinct_coins.end());
  }
  
  // Get the distinct trading coins from the triplets.
  vector <string> distinct_triplet_coins;
  {
    set <string> distinct_coins;
    for (auto & element : exchange_market_coin_triplets) {
      distinct_coins.insert (get<2>(element));
    }
    distinct_triplet_coins.assign (distinct_coins.begin (), distinct_coins.end());
  }
  
  // Get the intersection of both the above lots of trading coins.
  // This means that a coin is trading if it is live in both sets.
  return array_intersect (distinct_wallet_coins, distinct_triplet_coins);
}


vector <string> get_trading_markets (const string & coin,
                                     const vector <pair <string, string> > & exchange_coin_wallets,
                                     const vector <tuple <string, string, string> > & exchange_market_coin_triplets)
{
  // Get the trading exchanges for this coin from the wallets.
  vector <string> wallet_exchanges;
  for (auto & element : exchange_coin_wallets) {
    if (element.second == coin) {
      wallet_exchanges.push_back (element.first);
    }
  }
  
  // Get the enabled markets for this exchange/coin combination from the triplets.
  vector <string> trading_markets;
  {
    set <string> distinct_markets;
    for (auto & element : exchange_market_coin_triplets) {
      string triplet_coin = get<2>(element);
      if (coin != triplet_coin) continue;
      string triplet_exchange = get<0>(element);
      if (!in_array (triplet_exchange, wallet_exchanges)) continue;
      string triplet_market = get<1>(element);
      distinct_markets.insert (triplet_market);
    }
    trading_markets.assign (distinct_markets.begin (), distinct_markets.end());
  }
  
  // Done.
  return trading_markets;
}


vector <string> get_trading_exchanges (const string & market, const string & coin,
                                       const vector <pair <string, string> > & exchange_coin_wallets,
                                       const vector <tuple <string, string, string> > & exchange_market_coin_triplets)
{
  // Get the trading exchanges for this coin from the wallets.
  vector <string> wallet_exchanges;
  for (auto & element : exchange_coin_wallets) {
    if (element.second == coin) {
      wallet_exchanges.push_back (element.first);
    }
  }
  
  // Get the enabled exchanges for this market/coin combination from the triplets.
  vector <string> triplet_exchanges;
  {
    set <string> distinct_exchanges;
    for (auto & element : exchange_market_coin_triplets) {
      string triplet_market = get<1>(element);
      if (market != triplet_market) continue;
      string triplet_coin = get<2>(element);
      if (coin != triplet_coin) continue;
      string triplet_exchange = get<0>(element);
      distinct_exchanges.insert (triplet_exchange);
    }
    triplet_exchanges.assign (distinct_exchanges.begin (), distinct_exchanges.end());
  }
  
  // Get the intersection of both the above lots of trading coins.
  // This means that a coin is trading if it is live in both sets.
  return array_intersect (wallet_exchanges, triplet_exchanges);
}


float get_minimum_withdrawal_amount (string coin, string exchange)
{
  SQL sql (front_bot_ip_address ());

  // Get the amount as a string. It may be empty also.
  string minwithdraw = wallets_get_minimum_withdrawal_amount (sql, exchange, coin);
  
  if (!minwithdraw.empty ()) {
    
    // Stream it into a float.
    istringstream iss (minwithdraw);
    float f;
    iss >> f;
    // If the value is larger than 0, it was a float.
    if (f > 0) {
      return f;
    }

    // Handle the Bitfinex USD250 limit.
    if (minwithdraw.find ("USD") == 0) {
      string value (minwithdraw);
      value.erase (0, 3);
      if (!value.empty ()) {
        float amount = str2float (value);
        float rate = rate_get_cached (exchange, usdollar_id (), coin);
        if (rate > 0) {
          amount /= rate;
          // Margin of 10% to account for rate fluctuation.
          amount *= 1.1;
          // Done.
          return amount;
        }
      }
    }
    
    // Unknown value.
    to_stderr ({__FUNCTION__, coin, exchange, minwithdraw});
    
  }
  
  // Default is: No minimum value.
  return 0.0;
}


// Returns true if the $quanty traded at $rate would be considered dust trade.
bool is_dust_trade (string market, float quantity, float rate)
{
  // Default limit.
  float limit = 0.001;
  
  // Bitcoin market:
  // After placing an order too small, Bittrex would say:
  // {"success":false,"message":"MIN_TRADE_REQUIREMENT_NOT_MET","result":null}
  // And Cryptopia would say, in that case:
  // {"Success":false,"Error":"Invalid trade amount, Minimum total trade is 0.00005000 BTC","Data":null}
  // The values differ per exchange.
  // Here it takes the highest values over all exchanges.
  // Taking higher values has the advantage that higher trades will be preferred,
  // leading to larger gains.
  if (market == bitcoin_id ()) {
    limit = 0.001;
  }
  
  // Litecoin market:
  // Cryptopia says: Invalid trade amount, Minimum total trade is 0.01000000 LTC
  if (market == litecoin_id ()) {
    limit = 0.01;
  }
  
  // Ethereum market:
  // Bittrex says: The minimum order size is .00050000
  if (market == ethereum_id ()) {
    limit = 0.0005;
  }
  
  // Dogecoin market:
  // Cryptopia says: Minimum trade: 100.00000000 DOGE
  // TradeSatoshi says: Minimum trade: 1.00000000 DOGE.
  if (market == dogecoin_id ()) {
    limit = 100;
  }
  
  // USD Tether market.
  // Cryptopia says: Minimum trade: 1.00000000 USDT.
  if (market == usdtether_id ()) {
    limit = 1;
  }
  
  // US dollar market.
  // Yobit says: Total transaction amount is less than minimal total: 0.10000000
  if (market == usdollar_id ()) {
    limit = 0.1;
  }
  
  float value = quantity * rate;
  return (value < limit);
}


void fix_dust_trades (string market, vector <float> & quantities, vector <float> & rates)
{
  // If trading $quantities at $rates
  // would be considered dust trade by the currency exchanges,
  // combine that quantity with the next quantity.
  while (order_book_is_good (quantities, rates, false) && (is_dust_trade (market, quantities[0], rates[0]))) {
    float quantity = quantities.front ();
    quantities.erase (quantities.begin());
    rates.erase (rates.begin ());
    quantities.front () += quantity;
  }
}


// This returns true if the trade $size is too low.
bool trade_size_too_low (const map <string, float> & minimum_trade_sizes,
                         const string & exchange, const string & market, const string & coin,
                         float quantity)
{
  // The minimum trade sizes passed to this function contains the values as follows:
  // container [exchange+market+coin] = minimum trade size
  string key = exchange + market + coin;
  float minimum_size = 0;
  // Check that the value exists so as not to modify the container.
  if (minimum_trade_sizes.find (key) == minimum_trade_sizes.end ()) {
    // Does not exist: Everything is OK.
    return false;
  }
  // Get the value, without modifying the container, for thread safety.
  minimum_size = minimum_trade_sizes.at (key);
  // Check the trade quantity whether it's large enough.
  if (quantity < minimum_size) {
    return true;
  }
  // Everything is OK.
  return false;
}


// This fixes the order book if any of the quantities in the order book implies the trade would be too low.
void fix_too_low_trades (const map <string, float> & minimum_trade_sizes,
                         const string & exchange, const string & market, const string & coin,
                         vector <float> & quantities, vector <float> & rates)
{
  // If trading $quantities at $rates
  // would be considered too low a trade by the exchange at the market,
  // combine that quantity with the next quantity.
  while (order_book_is_good (quantities, rates, false) && (trade_size_too_low (minimum_trade_sizes,
                                                           exchange, market, coin,
                                                           quantities[0]))) {
    float quantity = quantities.front ();
    quantities.erase (quantities.begin());
    rates.erase (rates.begin ());
    quantities.front () += quantity;
  }
}


// This sets an appropriate rate for trading at least $quantity of the implied coin.
void fix_rate_for_quantity (float quantity, vector <float> & quantities, vector <float> & rates)
{
  // If the given $quantity is not entirely available for the rate,
  // take the next rate in the array,
  // till the given $quantity is available.
  bool order_book_updated;
  do {
    order_book_updated = false;
    if (order_book_is_good (quantities, rates, false)) {
      float trade_quantity = quantities.front ();
      if (trade_quantity < quantity) {
        quantities.erase (quantities.begin());
        rates.erase (rates.begin ());
        quantities.front () += trade_quantity;
        order_book_updated = true;
      }
    }
  } while (order_book_updated);
}


// Returns true if the order book is good.
// If the order book is to be checked on whether it is $nearly good,
// then the order book is required to be one item larger.
bool order_book_is_good (const vector <float> & quantities, const vector <float> & rates, bool nearly)
{
  size_t size = 2;
  if (nearly) size = 3;
  return (quantities.size () >= size) && (rates.size () >= size);
}


// Gets the market summaries for arbitrage in the shortest possible time.
void get_market_summaries_hurry (const vector <string> exchanges,
                                 const string & market,
                                 const string & coin,
                                 map <string, vector <float> > & buyer_quantities,
                                 map <string, vector <float> > & buyer_rates,
                                 map <string, vector <float> > & seller_quantities,
                                 map <string, vector <float> > & seller_rates,
                                 void * output)
{
  // Clear whatever data might be there.
  buyer_quantities.clear ();
  buyer_rates.clear ();
  seller_quantities.clear ();
  seller_rates.clear ();
  
  // Storage for parallel jobs.
  vector <thread *> jobs;

  // Start threads for all exchanges.
  // The purpose of using threads is to get the market summaries sooner.
  // When doing arbitrage, the timing is very important.
  // Bad timing leads to losses.
  // Good timing leads to gains.
  for (auto exchange : exchanges) {
    thread * job;
    job = new thread (exchange_get_buyers_via_satellite, exchange, market, coin,
                      ref (buyer_quantities[exchange]),
                      ref (buyer_rates[exchange]),
                      true, output);
    jobs.push_back (job);
    job = new thread (exchange_get_sellers_via_satellite, exchange, market, coin,
                      ref (seller_quantities[exchange]),
                      ref (seller_rates[exchange]),
                      true, output);
    jobs.push_back (job);
  }

  // Wait till all threads have completed.
  for (auto job : jobs) {
    job->join ();
  }
}


// Looks into the given multipath trade whether it is feasable with the current rates and limits.
// Returns true if the path was investigated properly.
bool multipath_investigate (void * output,
                            const map <string, float> & minimum_trade_sizes,
                            void * path)
{
  // Local object.
  multipath * mp = (multipath *) path;
  
  
  // Storage for the raw order books.
  vector <float> quantities1;
  vector <float> asks1;
  vector <float> quantities2;
  vector <float> bids2;
  vector <float> quantities3;
  vector <float> asks3;
  vector <float> quantities4;
  vector <float> bids4;
  
  
  // Obtain prices in a hurry for a more realistic picture.
  bool hurry = true;
  
  // Fetch the relevant order books in parallel jobs for better speed and more actual prices.
  vector <thread *> jobs;
  
  // Step 1: Get prices for buying coin 1 at market 1, if the market differs from the coin.
  if (mp->market1name != mp->coin1name) {
    thread * job;
    job = new thread (exchange_get_sellers_via_satellite, mp->exchange, mp->market1name, mp->coin1name, ref (quantities1), ref (asks1), hurry, output);
    jobs.push_back (job);
  }
  
  // Step 2: Get prices for selling coin 2 at market 2, if the market differs from the coin..
  if (mp->market2name != mp->coin2name) {
    thread * job;
    // Obtain it in a hurry for a more realistic picture.
    job = new thread (exchange_get_buyers_via_satellite, mp->exchange, mp->market2name, mp->coin2name, ref (quantities2), ref (bids2), hurry, output);
    jobs.push_back (job);
  }
  
  // Step 3: Get prices for buying coin 3 at market 3, if the market differs from the coin..
  if (mp->market3name != mp->coin3name) {
    thread * job;
    // Obtain it in a hurry for a more realistic picture.
    job = new thread (exchange_get_sellers_via_satellite, mp->exchange, mp->market3name, mp->coin3name, ref (quantities3), ref (asks3), hurry, output);
    jobs.push_back (job);
  }
  
  // Step 4: Get prices for selling coin 4 at market 4, if the market differs from the coin..
  if (mp->market4name != mp->coin4name) {
    thread * job;
    // Obtain it in a hurry for a more realistic picture.
    job = new thread (exchange_get_buyers_via_satellite, mp->exchange, mp->market4name, mp->coin4name, ref (quantities4), ref (bids4), hurry, output);
    jobs.push_back (job);
  }
  
  // Wait till all order books have been received.
  for (auto job : jobs) {
    job->join ();
  }
  
  // Error handling of the exchange calls.
  // Cater for situations that the coin is the same as the market: Skip those.
  if (mp->market1name != mp->coin1name) {
    if (quantities1.empty ()) return false;
  }
  if (mp->market2name != mp->coin2name) {
    if (quantities2.empty ()) return false;
  }
  if (mp->market3name != mp->coin3name) {
    if (quantities3.empty ()) return false;
  }
  if (mp->market4name != mp->coin4name) {
    if (quantities4.empty ()) return false;
  }

  // Process the multipath.
  multipath_processor (output, path, minimum_trade_sizes,
                       quantities1, asks1, quantities2, bids2, quantities3, asks3, quantities4, bids4,
                       1);
  
  // Path as investigated properly.
  return true;
}


void multipath_dust_trade_feedback (void * output, float coinquantity, string coinname, string marketname, float rate)
{
  to_output * out = (to_output *) output;
  if (out) {
    out->add ({"Trading", float2string (coinquantity), coinname, "@", marketname, "at rate", float2string (rate), "is considered dust trade" });
  }
}


void multipath_too_low_trade_feedback (void * output, float coinquantity, string coinname, string marketname, string exchange)
{
  to_output * out = (to_output *) output;
  if (out) {
    out->add ({"Trading", float2string (coinquantity), coinname, "@", marketname, "@", exchange, "is considered too low a trade" });
  }
}


bool multipath_update_rate_feedback (void * output, string coin, string market,
                                     float & multipathrate, const vector <float> rates,
                                     int change_only)
{
  bool update = false;
  if (change_only > 0) {
    // The rate can increase only.
    if (rates [0] > multipathrate) update = true;
  }
  if (change_only < 0) {
    // The rate can decrease only.
    if (rates [0] < multipathrate) update = true;
  }
  if (update) {
    to_output * out = (to_output *) output;
    if (out) {
      out->add ({"Trading", coin, "@", market, "rate updated from", float2string (multipathrate), "to", float2string (rates[0]), "change", float2visual ((rates[0] - multipathrate) / multipathrate * 100), "%" });
    }
    multipathrate = rates [0];
    return true;
  }
  return false;
}


// Processes a multipath.
// It checks if the quantities and rates are realistic as compared to the order books,
// and as compared to the limits the exchanges set on quantities to trade.
void multipath_processor (void * output,
                          void * path,
                          const map <string, float> & minimum_trade_sizes,
                          const vector <float> & quantities1, const vector <float> & asks1,
                          const vector <float> & quantities2, const vector <float> & bids2,
                          const vector <float> & quantities3, const vector <float> & asks3,
                          const vector <float> & quantities4, const vector <float> & bids4,
                          float multiplier)
{
  // Local objects.
  to_output * out = (to_output *) output;
  multipath * mp = (multipath *) path;

  
  // The number of trading steps.
  int total_trading_steps = 0;
  
  
  // The arbitrage investigator started the whole chain of multipath trading
  // with an amount of 0.01 Bitcoin.
  // This amount usually causes increased rates and thus decreased gains,
  // in case there's only a few coins available on a certain market,
  // because to trade a value equivalent to 0.01 BTC on such a market, that pushes the rates up a lot,
  // in order to buy sufficient coins on that market.
  // So here now it is going to start with a very small amount of Bitcoins.
  // It then runs the calculations over the whole multipath chain to see if this low amount is feasable.
  // If it triggers dust trades or minimum trade sizes, it will increase the rates enough
  // to no longer trigger those.

  
  // Start at a very low quantity for the first market coin.
  mp->market1quantity = 0.00001;
  // Update the entire path with this new quantity.
  multipath_calculate (path);
  // Iterate over the starting quantity and keep increasing it till it is no longer considered dust trade.
  {
    int iterations = 0;
    while (is_dust_trade (mp->market1name, mp->coin1quantity, mp->ask1) && (iterations < 1000)) {
      iterations++;
      mp->market1quantity *= 1.05;
      multipath_calculate (path);
    }
  }
  
  
  // Keep running cycles to update quantities and rates based on trade size limits,
  // till there's no more updates made,
  // or till there's too many iterations.
  bool keep_going = false;
  int multipath_iterations = 0;
  bool multipath_okay = true;
  do {
    multipath_iterations++;
    bool multipath_update = false;
    bool increase_quantity = false;

    // Update the quantities in this multipath.
    multipath_calculate (path);
    
    
    // Step 1: Buy coin 1 on market 1.
    if (mp->coin1name == mp->market1name) {
      // Coin and market the same.
      if (out) out->add ({"Skip trading", mp->coin1name, "at the", mp->market1name, "market"});
    } else {

      total_trading_steps++;
      
      // Check on dust trade, if there was no too low a trade in a previous step.
      if (!multipath_update) {
        if (is_dust_trade (mp->market1name, mp->coin1quantity, mp->ask1)) {
          multipath_dust_trade_feedback (output, mp->coin1quantity, mp->coin1name, mp->market1name, mp->ask1);
          increase_quantity = true;
          multipath_update = true;
        }
      }

      // Check on too low a trade, if there was no too low a trade in a previous step.
      // It checks on the absolute quantity of the coin that can be traded.
      // It omits the order book, so as not to inflate prices.
      if (!multipath_update) {
        if (trade_size_too_low (minimum_trade_sizes, mp->exchange, mp->market1name, mp->coin1name, mp->coin1quantity)) {
          multipath_too_low_trade_feedback (output, mp->coin1quantity, mp->coin1name, mp->market1name, mp->exchange);
          increase_quantity = true;
          multipath_update = true;
        }
      }
      
      // Check on whether the required amount of coins can be traded at the desired rate.
      if (!multipath_update) {
        // Look at the sellers of coin 1 at market 1 to see which quantity can be bought there at which rate.
        // Copy the order book, so the original one won't be modified.
        vector <float> quantities = quantities1;
        vector <float> asks = asks1;
        // Fix the rates so the required amount of coins is available.
        fix_rate_for_quantity (mp->coin1quantity, quantities, asks);
        // Check on the updated rates.
        if (order_book_is_good (quantities, asks, true)) {
          if (multipath_update_rate_feedback (output, mp->coin1name, mp->market1name, mp->ask1, asks, 1)) {
            multipath_update = true;
          }
        } else {
          multipath_okay = false;
        }
      }
      
      // Reading checking step 1.
    }
    
    
    // Step 2: Sell coin 2, bought in the previous step, on market 2.
    if (mp->coin2name == mp->market2name) {
      // Coin and market the same.
      if (out) out->add ({"Skip trading", mp->coin2name, "at the", mp->market2name, "market"});
    } else {

      total_trading_steps++;

      // Check on dust trade, if there was no too low a trade in a previous step.
      if (!multipath_update) {
        if (is_dust_trade (mp->market2name, mp->coin2quantity, mp->bid2)) {
          multipath_dust_trade_feedback (output, mp->coin2quantity, mp->coin2name, mp->market2name, mp->bid2);
          increase_quantity = true;
          multipath_update = true;
        }
      }

      // Check on too low a trade, if there was no too low a trade in a previous step.
      // It checks on the absolute quantity of the coin that can be traded.
      // It omits the order book, so as not to inflate prices.
      if (!multipath_update) {
        if (trade_size_too_low (minimum_trade_sizes, mp->exchange, mp->market2name, mp->coin2name, mp->coin2quantity)) {
          multipath_too_low_trade_feedback (output, mp->coin2quantity, mp->coin2name, mp->market2name, mp->exchange);
          increase_quantity = true;
          multipath_update = true;
        }
      }

      // Check on whether the required amount of coins can be traded at the desired rate.
      if (!multipath_update) {
        // Look at the buyers of coin 2 at market 2 to see which quantity can be sold there at which rate.
        // Copy the order book, so the original one won't be modified.
        vector <float> quantities = quantities2;
        vector <float> bids = bids2;
        // Fix the rates so the required amount of coins is available.
        fix_rate_for_quantity (mp->coin2quantity, quantities, bids);
        // Check on the updated rates.
        if (order_book_is_good (quantities, bids, true)) {
          if (multipath_update_rate_feedback (output, mp->coin2name, mp->market2name, mp->bid2, bids, -1)) {
            multipath_update = true;
          }
        } else {
          multipath_okay = false;
        }
      }
      
      // Reading checking step 2.
    }

    
    // Step 3: Buy coin 3 on market 3.
    if (mp->coin3name == mp->market3name) {
      // Coin and market the same.
      if (out) out->add ({"Skip trading", mp->coin3name, "at the", mp->market3name, "market"});
    } else {

      total_trading_steps++;

      // Check on dust trade, if there was no too low a trade in a previous step.
      if (!multipath_update) {
        if (is_dust_trade (mp->market3name, mp->coin3quantity, mp->ask3)) {
          multipath_dust_trade_feedback (output, mp->coin3quantity, mp->coin3name, mp->market3name, mp->ask3);
          increase_quantity = true;
          multipath_update = true;
        }
      }

      // Check on too low a trade, if there was no too low a trade in a previous step.
      // It checks on the absolute quantity of the coin that can be traded.
      // It omits the order book, so as not to inflate prices.
      if (!multipath_update) {
        if (trade_size_too_low (minimum_trade_sizes, mp->exchange, mp->market3name, mp->coin3name, mp->coin3quantity)) {
          multipath_too_low_trade_feedback (output, mp->coin3quantity, mp->coin3name, mp->market3name, mp->exchange);
          increase_quantity = true;
          multipath_update = true;
        }
      }

      // Check on whether the required amount of coins can be traded at the desired rate.
      if (!multipath_update) {
        // Look at the sellers of coin 3 at market 3 to see which quantity can be bought there at which rate.
        // Copy the order book, so the original one won't be modified.
        vector <float> quantities = quantities3;
        vector <float> asks = asks3;
        // Fix the rates so the required amount of coins is available.
        fix_rate_for_quantity (mp->coin3quantity, quantities, asks);
        // Check on the updated rates.
        if (order_book_is_good (quantities, asks, true)) {
          if (multipath_update_rate_feedback (output, mp->coin3name, mp->market3name, mp->ask3, asks, 1)) {
            multipath_update = true;
          }
        } else {
          multipath_okay = false;
        }
      }
      
      // Reading checking step 3.
    }

    
    // Step 4: Sell coin 4, bought in the previous step, on the initial market, that's market 4.
    if (mp->coin4name == mp->market4name) {
      // Coin and market the same.
      if (out) out->add ({"Skip trading", mp->coin4name, "at the", mp->market4name, "market"});
    } else {

      total_trading_steps++;

      // Check on dust trade, if there was no too low a trade in a previous step.
      if (!multipath_update) {
        if (is_dust_trade (mp->market4name, mp->coin4quantity, mp->bid4)) {
          multipath_dust_trade_feedback (output, mp->coin4quantity, mp->coin4name, mp->market4name, mp->bid4);
          increase_quantity = true;
          multipath_update = true;
        }
      }

      // Check on too low a trade, if there was no too low a trade in a previous step.
      // It checks on the absolute quantity of the coin that can be traded.
      // It omits the order book, so as not to inflate prices.
      if (!multipath_update) {
        if (trade_size_too_low (minimum_trade_sizes, mp->exchange, mp->market4name, mp->coin4name, mp->coin4quantity)) {
          multipath_too_low_trade_feedback (output, mp->coin4quantity, mp->coin4name, mp->market4name, mp->exchange);
          increase_quantity = true;
          multipath_update = true;
        }
      }

      // Check on whether the required amount of coins can be traded at the desired rate.
      if (!multipath_update) {
        // Look at the buyers of coin 4 at market 4 to see which quantity can be sold there at which rate.
        // Copy the order book, so the original one won't be modified.
        vector <float> quantities = quantities4;
        vector <float> bids = bids4;
        // Fix the rates so the required amount of coins is available.
        fix_rate_for_quantity (mp->coin4quantity, quantities, bids);
        // Check on the updated rates.
        if (order_book_is_good (quantities, bids, true)) {
          if (multipath_update_rate_feedback (output, mp->coin4name, mp->market4name, mp->bid4, bids, -1)) {
            multipath_update = true;
          }
        } else {
          multipath_okay = false;
        }
      }
      
      // Reading checking step 2.
    }


    // If there was dust trade, or too low a trade, increase the quantities slightly.
    if (increase_quantity) {
      mp->market1quantity *= 1.1;
      if (out) out->add ({"Increasing the initial market quantity to", float2string (mp->market1quantity), mp->market1name});
    }

    // Do an entire multipath calculation after this iteration.
    multipath_calculate (path);

    // Keep going if certain conditions apply.
    keep_going = false;
    if (multipath_update) keep_going = true;
    if (multipath_iterations >= 50) {
      multipath_okay = false;
      keep_going = false;
    }
    if (mp->gain < 0) {
      keep_going = false;
    }
  } while (keep_going);

  
  // Check if the path is okay.
  if (!multipath_okay) mp->gain = 0;
  
  // Check if the path is profitable.
  // More step would require a higher gain than fewer steps.
  float minimum_required_gain = multipath_minimum_required_gain (total_trading_steps);
  if (mp->gain >= minimum_required_gain) mp->status = multipath_status::profitable;
  else mp->status = multipath_status::unprofitable;
  
  (void) multiplier;
}


// Outputs human-readable information about the multipath trade path.
void multipath_output (void * output, void * path)
{
  to_output * to = (to_output *) output;
  multipath * mp = (multipath *) path;
  if (to) to->add ({"Exchange", mp->exchange});
  if (to) to->add ({"1", "Spend", float2string (mp->market1quantity), mp->market1name, "at this market", "and buy", float2string (mp->coin1quantity), mp->coin1name, "at rate", float2string (mp->ask1) });
  if (to) to->add ({"2", "Sell", float2string (mp->coin2quantity), mp->coin2name, "and gain", float2string (mp->market2quantity), mp->market2name, "at this market", "at rate", float2string (mp->bid2) });
  if (to) to->add ({"3", "Spend", float2string (mp->market3quantity), mp->market3name, "at this market", "and buy", float2string (mp->coin3quantity), mp->coin3name, "at rate", float2string (mp->ask3) });
  if (to) to->add ({"4", "Sell", float2string (mp->coin4quantity), mp->coin4name, "and gain", float2string (mp->market4quantity), mp->market4name, "at this market", "at rate", float2string (mp->bid4) });
  if (to) to->add ({"Gain", float2visual (mp->gain), "%"});
}


// Calculates the quantities for the multipath.
void multipath_calculate (void * path)
{
  // The multipath object to operate on.
  multipath * mp = (multipath *) path;

  // Apply the trading fees.
  // An example shows how the trading fees work:
  // Buy an amount of 32.28700393 FCT.
  // At a price of 0.00333333 BTC.
  // The sum is 0.10762324 BTC.
  // Add a trading fee of 0.00021525 BTC, which is 0.20%.
  // The total cost is 0.10783849 BTC.
  // Another example:
  // Sell an amount of 32.28700393 FCT.
  // At a price of 0.00333333 BTC.
  // The sum is 0.10762324 BTC.
  // Subtract the trading fee of 0.00021525 BTC, this is 0.20%.
  // The total proceeds will be 0.10740800 BTC.
  // At the same time simulate applying margins on the rates
  // to increase limit trade fulfillment chances and speed.

  // Get the fee this exchange charges on trading.
  float fee = exchange_get_trade_fee (mp->exchange);
  // For increased robustness, apply this fee twice.
  float feefactor = 1 - (2 * fee);

  // Step 1: Buying coin 1 at market 1.
  // The rate set in the multipath, take it in account.
  // Calculate the quantity of coin 1 to be bought on market 1.
  mp->coin1quantity = mp->market1quantity / mp->ask1;
  // After applying the trading fee, it buys less of coin 1.
  mp->coin1quantity *= feefactor;

  // Transfer coin 1 to coin 2.
  mp->coin2quantity = mp->coin1quantity;
  
  // Step 2: Selling coin 2 at market 2.
  // The rate set in the multipath, take it in account.
  // Convert the second coin into value at the second market.
  // The proceeds will be a certain amount of that second market coin.
  mp->market2quantity = mp->bid2 * mp->coin2quantity;
  // After applying the trading fee, the proceeds is less of the market 2 coin.
  mp->market2quantity *= feefactor;

  // Transfer the market 2 quantity to a market 3 quantity.
  mp->market3quantity = mp->market2quantity;

  // Step 3: Buying coin 3 at market 3.
  // The rate set in the multipath, take it in account.
  // Buy that coin on that market.
  mp->coin3quantity = mp->market3quantity / mp->ask3;
  // After applying the fee, it buys less of coin 3.
  mp->coin3quantity *= feefactor;
  
  // Transfer coin 3 to coin 4.
  mp->coin4quantity = mp->coin3quantity;

  // Step 4: Selling coin 4 at market 4.
  // Use the rate for selling the fourth coin back on the primary market, called market 4 here.
  mp->market4quantity = mp->bid4 * mp->coin4quantity;

  // After applying the fee, the proceeds is less of market 4 coin.
  mp->market4quantity *= feefactor;
  
  // Do the final gains calculations in percentages.
  mp->gain = (mp->market4quantity - mp->market1quantity) / mp->market1quantity * 100;
  if (isnan (mp->gain)) mp->gain = 0;
  if (isinf (mp->gain)) mp->gain = 0;
  
  // Do further checks on whether the calculation produces real figures.
  if (isnan (mp->coin1quantity)) mp->gain = 0;
  if (isinf (mp->coin1quantity)) mp->gain = 0;
  if (isnan (mp->market2quantity)) mp->gain = 0;
  if (isinf (mp->market2quantity)) mp->gain = 0;
  if (isnan (mp->coin3quantity)) mp->gain = 0;
  if (isinf (mp->coin3quantity)) mp->gain = 0;
  if (isnan (mp->market4quantity)) mp->gain = 0;
  if (isinf (mp->market4quantity)) mp->gain = 0;
}


// This returns true if there's a clash between the multi$path and the $exchanges_markets_coins.
// It updates the $exchanges_markets_coins container.
bool multipath_clash  (void * path, vector <string> & exchanges_markets_coins)
{
  // The multipath object to operate on.
  multipath * mp = (multipath *) path;

  // Flag whether there is a clash.
  bool clash = false;
  
  string exchange_market_coin;
  
  // Check clash in step 1.
  exchange_market_coin = mp->exchange + mp->market1name + mp->coin1name;
  if (in_array (exchange_market_coin, exchanges_markets_coins)) clash = true;
  // Update container.
  else exchanges_markets_coins.push_back (exchange_market_coin);

  // Check clash in step 2.
  exchange_market_coin = mp->exchange + mp->market2name + mp->coin2name;
  if (in_array (exchange_market_coin, exchanges_markets_coins)) clash = true;
  // Update container.
  else exchanges_markets_coins.push_back (exchange_market_coin);

  // Check clash in step 3.
  exchange_market_coin = mp->exchange + mp->market3name + mp->coin3name;
  if (in_array (exchange_market_coin, exchanges_markets_coins)) clash = true;
  // Update container.
  else exchanges_markets_coins.push_back (exchange_market_coin);

  // Check clash in step 4.
  exchange_market_coin = mp->exchange + mp->market4name + mp->coin4name;
  if (in_array (exchange_market_coin, exchanges_markets_coins)) clash = true;
  // Update container.
  else exchanges_markets_coins.push_back (exchange_market_coin);
  
  // Done.
  return clash;
}


bool multipath_paused  (void * path, const vector <string> & exchanges_markets_coins, bool output)
{
  // The multipath object to operate on.
  multipath * mp = (multipath *) path;
  
  string exchange_market_coin;
  bool paused;
  
  // Check whether step 1 is paused.
  exchange_market_coin = mp->exchange + mp->market1name + mp->coin1name;
  paused = in_array (exchange_market_coin, exchanges_markets_coins);
  if (paused) {
    if (output) to_stdout ({"Exchange", mp->exchange, "paused multipath trading", mp->coin1name, "@", mp->market1name});
    return true;
  }
  
  // Check whether step 2 is paused.
  exchange_market_coin = mp->exchange + mp->market2name + mp->coin2name;
  paused = in_array (exchange_market_coin, exchanges_markets_coins);
  if (paused) {
    if (output) to_stdout ({"Exchange", mp->exchange, "paused multipath trading", mp->coin2name, "@", mp->market2name});
    return true;
  }

  // Check whether step 3 is paused.
  exchange_market_coin = mp->exchange + mp->market3name + mp->coin3name;
  paused = in_array (exchange_market_coin, exchanges_markets_coins);
  if (paused) {
    if (output) to_stdout ({"Exchange", mp->exchange, "paused multipath trading", mp->coin3name, "@", mp->market3name});
    return true;
  }

  // Check whether step 4 is paused.
  exchange_market_coin = mp->exchange + mp->market4name + mp->coin4name;
  paused = in_array (exchange_market_coin, exchanges_markets_coins);
  if (paused) {
    if (output) to_stdout ({"Exchange", mp->exchange, "paused multipath trading", mp->coin4name, "@", mp->market4name});
    return true;
  }

  // Not paused.
  return false;
}


string multipath_get_market (multipath * path, int step)
{
  if (step == 1) return path->market1name;
  if (step == 2) return path->market2name;
  if (step == 3) return path->market3name;
  if (step == 4) return path->market4name;
  return "";
}


string multipath_get_coin (multipath * path, int step)
{
  if (step == 1) return path->coin1name;
  if (step == 2) return path->coin2name;
  if (step == 3) return path->coin3name;
  if (step == 4) return path->coin4name;
  return "";
}


float multipath_get_coin_quantity (multipath * path, int step)
{
  if (step == 1) return path->coin1quantity;
  if (step == 2) return path->coin2quantity;
  if (step == 3) return path->coin3quantity;
  if (step == 4) return path->coin4quantity;
  return 0;
}


float multipath_get_rate (multipath * path, int step)
{
  if (step == 1) return path->ask1;
  if (step == 2) return path->bid2;
  if (step == 3) return path->ask3;
  if (step == 4) return path->bid4;
  return 0;
}


float multipath_get_market_quantity (multipath * path, int step)
{
  if (step == 1) return path->market1quantity;
  if (step == 2) return path->market2quantity;
  if (step == 3) return path->market3quantity;
  if (step == 4) return path->market4quantity;
  return 0;
}


void multipath_set_status_order_uncertain (multipath * path, int step)
{
  if (step == 1) path->status = multipath_status::buy1uncertain;
  if (step == 2) path->status = multipath_status::sell2uncertain;
  if (step == 3) path->status = multipath_status::buy3uncertain;
  if (step == 4) path->status = multipath_status::sell4uncertain;
}


void multipath_set_status_order_placed (multipath * path, int step)
{
  if (step == 1) path->status = multipath_status::buy1placed;
  if (step == 2) path->status = multipath_status::sell2placed;
  if (step == 3) path->status = multipath_status::buy3placed;
  if (step == 4) path->status = multipath_status::sell4placed;
}


void multipath_set_status_balance_good (multipath * path, int step)
{
  if (step == 1) path->status = multipath_status::balance1good;
  if (step == 2) path->status = multipath_status::balance2good;
  if (step == 3) path->status = multipath_status::balance3good;
  if (step == 4) path->status = multipath_status::balance4good;
}


void multipath_set_order_id (multipath * path, int step, const string & order_id)
{
  if (step == 1) path->order1id = order_id;
  if (step == 2) path->order2id = order_id;
  if (step == 3) path->order3id = order_id;
  if (step == 4) path->order4id = order_id;
}


void multipath_set_status_next_status (multipath * path, int step)
{
  if (step == 1) path->status = multipath_status::sell2place;
  if (step == 2) path->status = multipath_status::buy3place;
  if (step == 3) path->status = multipath_status::sell4place;
  if (step == 4) path->status = multipath_status::done;
}


void multipath_place_limit_order (void * output,
                                  void * path,
                                  int step,
                                  mutex & update_mutex,
                                  map <string, float> minimum_trade_sizes,
                                  vector <string> & paused_trades)
{
  // The output object.
  to_output * out = (to_output *) output;
  
  // The multipath object.
  multipath * mp = (multipath *) path;
  
  // Get the buy or sell order details for this multipath step.
  string exchange = mp->exchange;
  string market = multipath_get_market (mp, step);
  string coin = multipath_get_coin (mp, step);
  float coin_quantity = multipath_get_coin_quantity (mp, step);
  float expected_rate = multipath_get_rate (mp, step);
  float market_quantity = multipath_get_market_quantity (mp, step);
  
  // Handle the situation that the coin is the same as the market.
  if (coin == market) {
    // Move the status a few steps forward:
    // If the balance is good for this market, then it will also be good for this coin.
    // This is because the coin is the same as the market in this step.
    multipath_set_status_balance_good (mp, step);
    // No trading possible.
    // Done.
    return;
  }
  
  // Information about whether to buy or to sell.
  bool buy = (step % 2) == 1;
  string buy_sell = buy ? "buy" : "sell";

  out->add ({"Step", to_string (step), buy_sell, float2string (coin_quantity), coin, "@", exchange, "at rate", float2string (expected_rate), buy ? "cost" : "gain", float2string (market_quantity), market});

  // Flag for further processing below, if the state is still good to go.
  bool checks_good = true;

  // Variable for how long to pause this trade.
  int pause_trade_minutes = 0;

  // Step 1 and 3: Get the sellers of this coin.
  // Step 2 and 4: Get the buyers of this coin.
  vector <float> quantities;
  vector <float> rates;
  if (checks_good) {
    if (buy) exchange_get_sellers_via_satellite (exchange, market, coin, quantities, rates, true, output);
    else exchange_get_buyers_via_satellite (exchange, market, coin, quantities, rates, true, output);
    if (quantities.empty ()) {
      out->add ({"Cannot get the", buy ? "sellers" : "buyers", "of", coin, ": Cancel"});
      checks_good = false;
      pause_trade_minutes = 5;
      mp->status = multipath_status::error;
    }
    if (checks_good) {
      out->add ({"Getting the", buy ? "sellers" : "buyers", "is OK"});
    }
  }

  // Fix the rates based on the quantity and the order book.
  // The issue here is that some quantities cannot be traded for the desired rate.
  // So to get higher quantities, the rates might become less profitable.
  if (checks_good) {
    fix_rate_for_quantity (coin_quantity, quantities, rates);
    if (!order_book_is_good (quantities, rates, false)) {
      out->add ({"The order book at the exchange is too small for this quantity: Cancel"});
      checks_good = false;
      pause_trade_minutes = 60;
      mp->status = multipath_status::error;
    }
  }
  if (checks_good) {
    out->add ({"Order book size at the exchange is good"});
  }

  // The current rate at the market.
  float current_rate = 0;
  if (checks_good) {
    current_rate = rates[0];
    out->add ({"The rate of", coin, "is", float2string (current_rate), "@", market});
  }

  // When buying: Increase the price offered slightly.
  // When selling: Decrease the price asked slightly.
  // The reason for this is as follows:
  // When buying coins for the ask price, or selling the coins for the bid price,
  // unfulfilled buy orders slowly accumulate on the various exchanges.
  // This is what happens in practise.
  // Increasing the price to buy the coins for is expected to result in fewer open buy orders.
  // Same for decreasing the price to sell the coins for.
  if (checks_good) {
    if (buy) current_rate += (current_rate * exchange_get_trade_order_ease_percentage (exchange) / 100);
    else current_rate -= (current_rate * exchange_get_trade_order_ease_percentage (exchange) / 100);
    out->add ({buy ? "Raise" : "Lower", "the rate of the coin to", float2string (current_rate)});
  }
  
  // Lock the container with balances.
  // This is because there's multiple parallel traders all accessing this container.
  update_mutex.lock ();

  // When buying:
  // The quantity of market balance this requires.
  // When selling: The coin quantity to sell is known in the multipath object.
  market_quantity = 0;
  if (buy && checks_good) {
    market_quantity = coin_quantity * current_rate;
    out->add ({"This needs", float2string (market_quantity), market});
  }
  
  // When buying:
  // Check the market balance is sufficient to cover this purchase.
  // Use a margin of 95%.
  float market_balance = 0;
  if (buy && checks_good) {
    exchange_get_balance_cached (exchange, market, NULL, &market_balance, NULL, NULL);
    bool balance_too_low = (market_balance < market_quantity);
    if (balance_too_low) {
      out->add ({"Required market quantity is", float2string (market_quantity), market, "available market balance is", float2string (market_balance), market, ": Too low: Cancel"});
      checks_good = false;
      pause_trade_minutes = 120;
      mp->status = multipath_status::error;
    }
  }
  if (checks_good) {
    out->add ({"There is enough market balance"});
  }
  
  // When selling:
  // Check that the coin balance is sufficient to do this sale.
  float coin_balance = 0;
  if (!buy && checks_good) {
    exchange_get_balance_cached (exchange, coin, NULL, &coin_balance, NULL, NULL);
    bool balance_too_low = (coin_balance < coin_quantity);
    if (balance_too_low) {
      out->add ({"Required coin quantity is", float2string (coin_quantity), coin, "available coin balance is", float2string (coin_balance), coin, ": Too low: Cancel"});
      checks_good = false;
      pause_trade_minutes = 5;
      mp->status = multipath_status::error;
    }
  }

  // Check that the current rate at the market is about the same as the rate stored in the multipath.
  // Or if there's a larger difference in rates, that the difference is a bit smaller than the path's gain.
  if (checks_good) {
    float percentage = abs (current_rate - expected_rate) / expected_rate * 100;
    if (percentage > 0.1) {
      out->add ({"The expected rate is", float2string (expected_rate), "and the current rate at the market is", float2string (current_rate)});
      out->add ({"Difference in rate is", float2visual (percentage), "and the gain is", float2visual (mp->gain), "%"});
      // If the multipath gain is relatively high,
      // and then it used to cancel out on a few percents difference in rate,
      // that was not a wise decision.
      // The gain was high enough, so now the bot considers the total gain,
      // then decides which different rate it deems acceptable, so as to make some gain.
      if ((percentage + 2) > mp->gain) {
        // The current order books indicate a rate a off from what was calculated.
        // To such a degree that executing this path beyond this point no longer leads to a gain.
        // So stop further trading of this path, because else the bot will at a loss.
        // The bot will eventually sell a possible already bought coin balance back to a base market.
        out->add ({"Cancel"});
        checks_good = false;
        pause_trade_minutes = 5;
        mp->status = multipath_status::error;
      }
    }
  }
  
  // Check on dust trade at the base market.
  if (buy && checks_good) {
    if (is_dust_trade (market, market_quantity * 0.999, 1)) {
      out->add ({"The market quantity leads to dust trade: Cancel"});
      checks_good = false;
      pause_trade_minutes = 15;
      mp->status = multipath_status::error;
    }
  }
  if (!buy && checks_good) {
    if (is_dust_trade (market, coin_quantity, current_rate)) {
      out->add ({"The coin quantity leads to dust trade: Cancel"});
      checks_good = false;
      pause_trade_minutes = 15;
      mp->status = multipath_status::error;
    }
  }
  if (checks_good) {
    out->add ({"Passed the dust trade checks"});
  }
    
  // Check that the trade is not lower than the possible minimum trade size at the exchange.
  if (checks_good) {
    float minimum = minimum_trade_sizes [exchange + market + coin];
    // A value of 0 means: No limits set by the exchange.
    if (minimum > 0) {
      // It used to apply a 2 percent margin for safety.
      // But since the multipath analyzer used a lower margin than this,
      // using a margin of 2 percent would lead to cancellation of the order at times.
      // So for just now, it uses a smaller margin.
      if (coin_quantity < (minimum * 1.005)) {
        out->add ({"Exchange", exchange, "has a minimum order size of", float2string (minimum), coin, "@", market, ": Cancel"});
        checks_good = false;
        pause_trade_minutes = 15;
        mp->status = multipath_status::error;
      } else {
        out->add ({"Passed the minimum order size check"});
      }
    }
  }
  
  // At this point the limit buy order is going to be placed.
  // Buy: Subtract the required market quantity from the available balances.
  // Sell: Subtract the coin quantity to be sold from the available balances.
  if (buy) exchange_set_balance_cache (exchange, market, 0, market_balance - market_quantity, 0, 0);
  else exchange_set_balance_cache (exchange, coin, 0, coin_balance - coin_quantity, 0, 0);
  
  // Make the balances available again to other threads.
  update_mutex.unlock ();
  
  // Bail out if the checks are not good.
  // This happens after the mutex has been unlocked.
  // Update the paused trading coins too.
  if (!checks_good) {
    string reason = "multipath";
    temporarily_pause_trading (exchange, market, coin, pause_trade_minutes, reason);
    paused_trades.push_back (exchange + market + coin);
    return;
  }
  
  // Place the order at the exchange.
  string order_id, error, json;
  if (buy) {
    exchange_limit_buy (exchange, market, coin, coin_quantity, current_rate, error, json, order_id, output);
  } else {
    exchange_limit_sell (exchange, market, coin, coin_quantity, current_rate, error, json, order_id, output);
  }
  out->add ({"Order placed with ID", order_id});
  
  // Follow-up on this trade.
  vector <string> dummy;
  limit_trade_follow_up (output, exchange, market, coin, error, json, order_id, coin_quantity, current_rate, buy_sell, dummy);
 
  // If the trade order was placed successfully, there will be a good order ID.
  // If not, the order might have been place, but the bot cannot yet be sure of that.
  if (order_id.empty ()) {
    multipath_set_status_order_uncertain (mp, step);
  } else {
    multipath_set_status_order_placed (mp, step);
  }
  
  // Store the order ID, if any, in the multipath.
  multipath_set_order_id (mp, step, order_id);
}


void multipath_verify_limit_order (void * output, void * path, int step)
{
  // The output object.
  to_output * out = (to_output *) output;
  
  // The multipath object.
  multipath * mp = (multipath *) path;

  // At this stage, there is a limit order whose order ID is not certain.
  // Get the buy or sell order details for this multipath step.
  string exchange = mp->exchange;
  string market = multipath_get_market (mp, step);
  string coin = multipath_get_coin (mp, step);
  float rate = multipath_get_rate (mp, step);
  
  // Handle the situation that the coin is the same as the market.
  if (coin == market) {
    // No limit order was placed.
    return;
  }
  
  // Flag for whether to buy or to sell.
  bool buy = (step % 2) == 1;
  
  out->add ({"Step", to_string (step), buy ? "buy" : "sell", coin, "@", exchange, "at rate", float2string (rate), "locating order ID"});
  
  // Call function to locate the order ID based on input variables.
  string located_order_id;
  bool located = locate_trade_order_id (output, exchange, market, coin, rate, located_order_id);
  
  if (located) {
    out->add ({"The order has been updated with order ID", located_order_id});
  }
  
  // Store the order ID, if any, in the multipath.
  multipath_set_order_id (mp, step, located_order_id);
}


void multipath_verify_balance (void * output,
                               void * path,
                               int step,
                               mutex & update_mutex,
                               int & timer)
{
  (void) update_mutex;
  
  // The output object.
  to_output * out = (to_output *) output;
  
  // The multipath object.
  multipath * mp = (multipath *) path;

  // At this stage, there is a limit order that has been placed.
  // It may have been fulfilled, or partly filled, or still entirely open.
  string exchange = mp->exchange;
  string coin = multipath_get_coin (mp, step);
  string market =  multipath_get_market (mp, step);
  float coin_quantity = multipath_get_coin_quantity (mp, step);
  float market_quantity = multipath_get_market_quantity (mp, step);
  
  // Flag for whether to buy or to sell.
  bool buy = (step % 2) == 1;
  
  out->add ({"Step", to_string (step), "verify balance of", buy ? "buy" : "sell", float2string (coin_quantity), coin, "for", float2string (market_quantity), market, "at", exchange});

  // Decide on the quantity and unit to look for.
  float quantity = 0;
  string unit;
  if (buy) {
    quantity = coin_quantity;
    unit = coin;
  } else {
    quantity = market_quantity;
    unit = market;
  }
  out->add ({"Looking for a balance of", float2string (quantity), unit});

  // Check the available balance the bot already has.
  float bot_available = 0;
  exchange_get_balance_cached (exchange, unit, NULL, &bot_available, NULL, NULL);
  float bot_factor = bot_available / quantity;
  out->add ({"Available balance at the bot is", float2visual (100 * bot_factor), "%"});
  
  // Check the currrent available balance at the exchange.
  float exchange_available = 0;
  {
    vector <string> coin_abbrevs, coins;
    vector <float> total, reserved, unconfirmed;
    exchange_get_balances (exchange, coin_abbrevs, coins, total, reserved, unconfirmed);
    for (unsigned int i = 0; i < coins.size(); i++) {
      if (unit == coins[i]) {
        exchange_available = total [i] - reserved [i] - unconfirmed [i];
      }
    }
  }
  float exchange_factor = exchange_available / quantity;
  out->add ({"Available balance at the exchange is", float2visual (100 * exchange_factor), "%"});

  // Get the best balance factor of the two.
  float factor = bot_factor;
  if (exchange_factor > factor) factor = exchange_factor;

  // Update the memory container that holds the available balance.
  // This is so that the next step can immediately read that balance again from that same container.
  exchange_set_balance_cache (exchange, unit, 0, factor * quantity, 0, 0);
  
  // Handle the situation that all of the balance is available.
  if (factor >= 1) {
    multipath_set_status_next_status (mp, step);
  }
  
  // Handle the situation that most of the balance is available.
  else if (factor >= 0.95) {
    // Update the quantities still to be processed.
    int localstep = step;
    if (localstep == 1) {
      mp->coin1quantity *= factor;
      mp->coin2quantity *= factor;
      localstep = 2;
    }
    if (localstep == 2) {
      mp->market2quantity *= factor;
      mp->market3quantity *= factor;
      localstep = 3;
    }
    if (localstep == 3) {
      mp->coin3quantity *= factor;
      mp->coin4quantity *= factor;
      localstep = 4;
    }
    if (localstep == 4) {
      mp->market4quantity *= factor;
    }
    // There's nearly enough balance, just move to the next status.
    multipath_set_status_next_status (mp, step);
  }
  
  // Handle the situation that the balance is insufficient yet.
  else {
    // Increase the timer.
    timer++;
    if (timer > 5) {
      // Too many tries: Multipath failure beyond recovery.
      // It is set to this status so a situation like this does not occur more than a few times a day.
      mp->status = multipath_status::unrecoverable;
    } else {
      // Wait shortly.
      this_thread::sleep_for (chrono::seconds (2));
      // The status does not change, so it will retry this step.
    }
  }
}


// This returns the gain required, in percents, for a multipath to be considered profitable.
float multipath_minimum_required_gain (int trading_step_count)
{
  // More step require a higher gain than fewer steps.
  float percents_gain_per_step = 0.75;
  float minimum_required_gain = trading_step_count * percents_gain_per_step;
  return minimum_required_gain;
}


void pattern_prepare (void * output,
                      const vector <int> & seconds,
                      const vector <float> & asks,
                      const vector <float> & bids,
                      unordered_map <int, float> & minute_asks,
                      unordered_map <int, float> & minute_bids,
                      unordered_map <int, float> & hour_asks,
                      unordered_map <int, float> & hour_bids)
{
  // The output object.
  to_output * out = (to_output *) output;
  
  // The unordered maps are for super fast access to the bids and the asks.
  // Clear the containers.
  minute_asks.clear ();
  minute_bids.clear ();
  hour_asks.clear ();
  hour_bids.clear ();
  
  // No rates available: Done.
  if (seconds.empty ()) {
    return;
  }
  
  // The current second.
  int current_second = seconds_since_epoch ();
  
  // The highest second: To be used to see if the rates are current.
  int highest_second = seconds.back ();
  // Check age of the rates.
  int age_second = current_second - highest_second;
  if (age_second > 150) {
    if (out) out->add ({"Rates are not current, the most recent rate is", to_string (age_second / 60), "minutes old"});
    return;
  }
  
  // Store the rates for super fast access.
  // Convert the seconds since the Epoch to minutes relative to now.
  // That's for easier and faster access.
  // Interpolate missing rates.
  int oldest_minute = 0;
  int previous_minute = 0;
  float previous_ask = 0, previous_bid = 0;
  for (size_t i = 0; i < seconds.size(); i++) {
    // Calculate the minute this rate belongs to.
    // Using minutes is easier as the system can iterate through the consequtive minutes.
    // When using seconds, the seconds are not consecutive, but there is around 60 between them,
    // sometimes more, sometimes less.
    // The rates recorder runs right at the start of the minute.
    // Converting the associated seconds to minutes may not give consequtive seconds.
    // But it appears that when adding 30 seconds to the seconds since Epoch,
    // it leads to consecutive minutes.
    // Also subtract the second from the current second, for far lower rounding errors from float to int.
    // A minute of 0 is the most recent minute.
    // A minute of -1 is one minute ago. And so on.
    int minute = float (current_second - seconds[i] + 30) / 60;
    minute = 0 - minute;
    if (previous_minute) {
      if (minute != previous_minute + 1) {
        // Interpolate the missing value(s).
        float next_ask = asks[i];
        float next_bid = bids[i];
        for (int interpolate = previous_minute + 1; interpolate < minute; interpolate++) {
          float ask = previous_ask + (interpolate - previous_minute) * (next_ask - previous_ask) / (float) (minute - previous_minute);
          if (isnan (ask)) ask = 0;
          if (isinf (ask)) ask = 0;
          float bid = previous_bid + (interpolate - previous_minute) * (next_bid - previous_bid) / (float) (minute - previous_minute);
          if (isnan (bid)) bid = 0;
          if (isinf (bid)) bid = 0;
          minute_asks [interpolate] = ask;
          minute_bids [interpolate] = bid;
        }
      }
    }
    minute_asks [minute] = asks[i];
    minute_bids [minute] = bids[i];
    // Record the oldest minute.
    if (minute < oldest_minute) oldest_minute = minute;
    // Record state.
    previous_minute = minute;
    previous_ask = asks[i];
    previous_bid = bids[i];
  }
  
  // Roll-over hour detection.
  int previous_hour = 0;
  // Detect total minutes for rate calculation.
  int minutes_with_rate = 0;
  // Totals for average calculation.
  float total_ask = 0;
  float total_bid = 0;
  for (int minute = 0; minute >= oldest_minute; minute--) {
    // The current hour.
    int hour = minute / 60;
    // New hour detection.
    if (hour != previous_hour) {
      // Calculate the averages.
      float average_ask = total_ask / minutes_with_rate;
      float average_bid = total_bid / minutes_with_rate;
      if (isnan (average_ask)) average_ask = 0;
      if (isinf (average_ask)) average_ask = 0;
      if (isnan (average_bid)) average_bid = 0;
      if (isinf (average_bid)) average_bid = 0;
      // Store the average hourly rates in a format suitable for super fast access.
      // An hour of 0 is the current hour.
      // An hour of -1 is last hour.
      // An hour of -2 is two hours ago.
      // And so on.
      hour_asks [previous_hour] = average_ask;
      hour_bids [previous_hour] = average_bid;
      // Reset values.
      previous_hour = hour;
      minutes_with_rate = 0;
      total_ask = 0;
      total_bid = 0;
    }
    // Store total asks and bids.
    // Record minutes with values.
    float bid = minute_bids [minute];
    if (bid > 0) minutes_with_rate++;
    total_bid += bid;
    total_ask += minute_asks [minute];
  }
}


void pattern_prepare (void * output,
                      const string & exchange, const string & market, const string & coin,
                      unordered_map <int, float> & minute_asks,
                      unordered_map <int, float> & minute_bids,
                      unordered_map <int, float> & hour_asks,
                      unordered_map <int, float> & hour_bids)
{
  vector <int> seconds;
  vector <float> asks, bids;
  allrates_get (exchange, market, coin, seconds, bids, asks);
  pattern_prepare (output, seconds, asks, bids, minute_asks, minute_bids, hour_asks, hour_bids);
}


void pattern_prepare (void * output,
                      const string & exchange, const string & market, const string & coin,
                      int hours,
                      unordered_map <int, float> & minute_asks,
                      unordered_map <int, float> & minute_bids,
                      unordered_map <int, float> & hour_asks,
                      unordered_map <int, float> & hour_bids)
{
  // Clear containers.
  minute_asks.clear ();
  minute_bids.clear ();
  hour_asks.clear ();
  hour_bids.clear ();
  
  // If the requested market exists, take the rates straight from the database.
  if (allrates_exists (exchange, market, coin)) {
    pattern_prepare (output, exchange, market, coin, minute_asks, minute_bids, hour_asks, hour_bids);
  }
  
  // If the requested market does not exist,
  // it takes the rates of the coins on the Bitcoin market,
  // then it takes the rates of the Bitcoin on the desired market,
  // then it converts the rates of the coin to the desired market..
  else {
    
    // Get the rates of the coin on the Bitcoin market.
    unordered_map <int, float> bitcoin_minute_asks, bitcoin_minute_bids, bitcoin_hour_asks, bitcoin_hour_bids;
    pattern_prepare (output, exchange, bitcoin_id (), coin,
                     bitcoin_minute_asks, bitcoin_minute_bids, bitcoin_hour_asks, bitcoin_hour_bids);
    
    // Get the rates of the Bitcoin on the desired market.
    unordered_map <int, float> market_minute_asks, market_minute_bids, market_hour_asks, market_hour_bids;
    vector <string> exchanges = exchange_get_names ();
    for (auto exchange : exchanges) {
      if (!allrates_exists (exchange, market, bitcoin_id ())) continue;
      pattern_prepare (output, exchange, market, bitcoin_id (),
                       market_minute_asks, market_minute_bids, market_hour_asks, market_hour_bids);
      break;
    }
    
    // The negative minute before now, where to start calculating the minutely patterns.
    int starting_minute = 0 - (hours * 60);
    
    // Iterate over the data to generate the data points.
    for (int minute = starting_minute; minute <= 0; minute++) {
      
      // The rate of the coin.
      float coin_btc_bid = bitcoin_minute_bids[minute];
      float coin_btc_ask = bitcoin_minute_asks[minute];
      float btc_market_bid = market_minute_bids[minute];
      float btc_market_ask = market_minute_asks[minute];
      float bid = coin_btc_bid * btc_market_bid;
      float ask = coin_btc_ask * btc_market_ask;
      
      // If the conversion did not work, don't store the data.
      if ((bid == 0) || (ask == 0)) continue;
      
      // Store the ask and bid.
      minute_asks [minute] = ask;
      minute_bids [minute] = bid;
    }
    
    // The negative hour before now, where to start calculating the hourly patterns.
    int starting_hour = 0 - hours;
    
    // Iterate over the data to generate the data points.
    for (int hour = starting_hour; hour <= 0; hour++) {
      
      // The rate of the coin.
      float coin_btc_bid = bitcoin_hour_bids[hour];
      float coin_btc_ask = bitcoin_hour_asks[hour];
      float btc_market_bid = market_hour_bids[hour];
      float btc_market_ask = market_hour_asks[hour];
      float bid = coin_btc_bid * btc_market_bid;
      float ask = coin_btc_ask * btc_market_ask;
      
      // If the conversion did not work, don't store the data.
      if ((bid == 0) || (ask == 0)) continue;
      
      // Store the ask and bid.
      hour_asks [hour] = ask;
      hour_bids [hour] = bid;
    }
  }
}


void pattern_smooth (unordered_map <int, float> & asks, unordered_map <int, float> & bids,
                     unordered_map <int, float> & smoothed_rates, unsigned int window)
{
  // Find the oldest time.
  int oldest_time = 0;
  for (auto & element : asks) {
    int time = element.first;
    if (time < oldest_time) oldest_time = time;
  }

  smoothed_rates.clear ();
  
  // Store values for the moving average.
  queue <double> measurements;
  
  // The sum of all the measurements currently in the queue.
  double measurement_sum = 0;

  float oldrate = 0;
  for (int time = oldest_time; time <= 0; time++) {
    float rate = (asks [time] + bids [time]) / 2;
    if (rate == 0) rate = oldrate;
    oldrate = rate;
    if (rate == 0) continue;
    measurement_sum += rate;
    measurements.push (rate);
    if (measurements.size () > window) {
      measurement_sum -= measurements.front();
      measurements.pop();
    }
    rate = measurement_sum / measurements.size();
    smoothed_rates [time] = rate;
  }
}


// This returns the percentage change in the rates over the given period.
// When the $start is 0, it refers to now.
// The $window indicates how many units of the past time it includes for calculating the outcome.
float pattern_change (unordered_map <int, float> & asks, unordered_map <int, float> & bids, int start, int window)
{
  // Notes:
  // Is this averages calculation correct?
  // The figures of the week, or over the month, looks so high, is the calculation working well?
  // Else it should just look at the start of the period, and at the end,
  // and then calculate the difference from there,
  // or take the average of one day or a few hours, for the calculation of the change percentage.
  // It just looks too high now.
  
  double average_change = 0;
  int total_change_item_count = 0;
  double previous_rate = 0;
  int first = start - window;
  for (int i = first; i <= start; i++) {
    double rate = (asks[i] + bids[i]) / 2;
    if (rate == 0) continue;
    if (previous_rate == 0) {
      previous_rate = rate;
      continue;
    }
    double change = (rate - previous_rate) / previous_rate;
    if (rate == previous_rate) change = 0;
    previous_rate = rate;
    if (isnan (change)) continue;
    if (isinf (change)) continue;
    average_change += change;
    total_change_item_count ++;
  }
  average_change /= total_change_item_count;
  average_change *= 100;
  // It has now calculated the average change per time unit.
  // To get the change percentage over the whole time window, multiply it with that said window.
  average_change *= window;

  // Done.
  return average_change;
}


// This returns true if the price of the coin has moved regularly recently.
bool pattern_coin_has_regular_recent_price_movements (unordered_map <int, float> & hour_asks,
                                                      unordered_map <int, float> & hour_bids)
{
  // Check that the coin has regular price movements, rather than a stable price.
  // For the past month, the average price point for each hour
  // should differ from the average of the previous hour,
  // for most of the time.
  // The reason for this requirement is that the bot can be reasonably sure,
  // that this coins is being actively traded,
  // rather than being a coin without much trading going on.
  float previous_price = 0;
  int different_price_counter = 0;
  int start_hour = -720;
  for (int hour = start_hour; hour <= 0; hour++) {
    float price = (hour_asks [hour] + hour_bids [hour]) / 2;
    if (price != previous_price) different_price_counter++;
    previous_price = price;
  }
  return (different_price_counter > abs (start_hour * 0.75));
}


// This function detects whether a sale can be made in a simulation.
// The $buy_time is the time at which the coin is bought.
// The $max_open_time is the maximum time units the order may remain open in this simulation.
// The $buy_price and $sell_price indicate the prices the coin was bought for and should be sold for.
// The $time_asks and $time_bids are the actual coin prices per time unit.
// The functions returns this:
// $real_open_time: How many time units after coin purchase the coin was sold again.
// $sold_in_time: Whether the coin was sold within the stated hours.
void patterns_simulate_sale (void * output,
                             int buy_time, int max_open_time,
                             float buy_price, float sell_price,
                             unordered_map <int, float> & time_asks,
                             unordered_map <int, float> & time_bids,
                             int & real_open_time, bool & sold_in_time)
{
  // How long the order has been open.
  real_open_time = numeric_limits<int>::max();
  
  // Whether the coin was sold within the specified maximum time.
  sold_in_time = false;
  
  // Iterate over the container with the rates.
  // Take the bid price, since the bot wants to sell the coin.
  for (int time = buy_time; time <= 0; time++) {
    float bid_price = time_bids [time];
    if (bid_price > sell_price) {
      // Sold: Set variables.
      real_open_time = time - buy_time;
      sold_in_time = (time <= buy_time + max_open_time);
      // Logging.
      if (output) {
        to_output * out = (to_output *) output;
        string remark = sold_in_time ? "in time" : "too late";
        out->add ({"bought at", to_string (time), "sold after", to_string (real_open_time), remark});
      }
      // Done.
      break;
    }
  }
  
  // Silence compile warnings.
  (void) time_asks;
  (void) buy_price;
}


bool pattern_increase_day_week_detector (void * output,
                                         int detection_hour,
                                         unordered_map <int, float> & hour_asks,
                                         unordered_map <int, float> & hour_bids)
{
  // The bundled output object.
  to_output * out = nullptr;
  if (output) out = (to_output *) output;
  
  // The percentage increase required for this coin to be considered profitable.
  // The value of the coin should have been increasing for several days in a row.
  float minimum_daily_increase = 2;
  float minimum_weekly_increase = 5;
  
  // The number of hours in a unit of time.
  int day_hours = 24;
  int week_hours = 168;
  
  // Check whether there's an increase in the value of this coin at this very moment.
  // If not, it's done right away, no need to investigate the coin's history.
  bool pattern_detected = true;
  float change_percentage_day = pattern_change (hour_asks, hour_bids, detection_hour, day_hours);
  if (change_percentage_day < minimum_daily_increase) pattern_detected = false;
  float change_percentage_week = pattern_change (hour_asks, hour_bids, detection_hour, week_hours);
  if (change_percentage_week < minimum_weekly_increase) pattern_detected = false;
  
  if (out) {
    out->add ({"daily", float2visual (change_percentage_day), "%", "weekly", float2visual (change_percentage_week), "%"});
  }
  
  // Whether the pattern has been detected.
  return pattern_detected;
}


bool pattern_increase_day_week_simulator (void * output,
                                          unordered_map <int, float> & hour_asks,
                                          unordered_map <int, float> & hour_bids,
                                          const string & reason)
{
  // The bundled output object.
  to_output * out = nullptr;
  if (output) out = (to_output *) output;
  
  // Go over the history to similate trades.
  int bought_counter = 0, sold_counter = 0;
  float buy_price = 0;
  int maximum_open_order_hours = 48;
  for (int hour = -500; hour <= 0 - (maximum_open_order_hours); hour++) {
    bool hit = pattern_increase_day_week_detector (nullptr, hour, hour_asks, hour_bids);
    if (hit) {
      buy_price = hour_asks [hour];
      //if (out) out->add ({"purchased at", to_string (hour), "for", float2string (buy_price)});
      bought_counter++;
      int sold_after_hours = 0;
      bool sold_in_time = false;
      patterns_simulate_sale (nullptr, hour, maximum_open_order_hours, buy_price, buy_price * 1.02, hour_asks, hour_bids, sold_after_hours, sold_in_time);
      if (sold_in_time) {
        sold_counter++;
      }
    }
  }
  
  // Nothing bought.
  if (bought_counter == 0) return false;
  
  // The percentage of successful trades should be good.
  float sold_percentage = (float) sold_counter / (float) bought_counter * 100;
  if (sold_percentage < 75) return false;
  if (out) out->add ({"Pattern", reason, "bought", to_string (bought_counter), "sold", to_string (sold_counter), float2visual (sold_percentage), "%"});
  if (sold_percentage < 80) return false;
  
  // Good coin.
  return true;
}


/*
void pattern_verify_executed_sell_orders (void * output)
{
  // Runs once every five minutes.
  // This is to limit the number of API calls to the exchanges.
  if ((minutes_within_hour () % 5) != 0) return;
  
  // The output object.
  to_output * out = (to_output *) output;
  
  out->add ({"Checking on executed pattern-based sell orders"});
  
  SQL sql (front_bot_ip_address ());
  
  // Get all the pattern-related orders.
  vector <string> exchanges, markets, coins, reasons, statuses, orderids1, types;
  vector <float> quantities, rates1, rates2;
  vector <int> ids, minutes;
  live_get (sql, ids, exchanges, markets, coins, quantities, rates1, reasons, statuses, orderids1, minutes, rates2);
  
  // Iterate over the orders.
  for (unsigned int lo = 0; lo <  orderids1.size(); lo++) {
    
    // The status.
    string status = statuses [lo];

    // Check on a sell order that has been placed.
    if ((status != pattern_status_sell_done ()) & (status != pattern_status_stop_loss ())) {
      continue;
    }
    
    // The order ID of this sell order.
    string order_id = orderids1[lo];
    
    // The exchange of this sell order.
    string exchange = exchanges [lo];
    
    // Get the open orders at the exchange, if there's any at all.
    vector <string> open_identifiers, open_markets, open_coin_abbrevs, open_coin_ids, open_buy_sells, open_dates;
    vector <float> open_quantities, open_rates;
    string error;
    exchange_get_open_orders (exchange, open_identifiers, open_markets, open_coin_abbrevs, open_coin_ids, open_buy_sells, open_quantities, open_rates, open_dates, error);
    
    if (!error.empty ()) continue;
    
    // If the order ID is still open, leave it like that.
    if (in_array (order_id, open_identifiers)) continue;
    
    // The order is no longer open.
    // It was either cancelled, or else it was fulfilled.
    out->add ({"Pattern-based sell order ID", order_id, "at", exchange, "is no longer open: Update database"});
    out->to_stderr (true);

    // Update the status in the database.
    int record_id = ids[lo];
    live_update (sql, record_id, "status", pattern_status_sell_done (), NULL, 0);
  }
}
*/


/*
void pattern_angles (unordered_map <int, float> & rates, unordered_map <int, float> & angles, unsigned int window)
{
  // Find the oldest time.
  int oldest_time = 0;
  for (auto & element : rates) {
    int time = element.first;
    if (time < oldest_time) oldest_time = time;
  }

  angles.clear ();

  // Iterate over the values.
  // Do not begin right at the start, but at an offset equivalent to the window size.
  // This is to get good angles right from the start, rather than wildly fluctuating angles at the start.
  for (int time = oldest_time + window; time <= 0; time++) {
    float angle, deviation;
    pattern_angle_deviation (rates, time, window, angle, deviation);
    angles [time] = angle;
  }
}


void pattern_angle_deviation (unordered_map <int, float> & rates,
                              int start, int amount,
                              float & angle, float & deviation)
{
  // Calculate the average rate over the time span.
  vector <float> values;
  float previousvalue = 0;
  for (int i = 0 - amount; i <= 0; i++) {
    int time = start + i;
    float value = rates [time];
    if (value == 0) value = previousvalue;
    previousvalue = value;
    values.push_back (value);
  }
  float meanvalue = accumulate (values.begin(), values.end(), 0.0) / values.size();
  
  // Calculate the average angle of the changes in the rates.
  vector <float> angles;
  previousvalue = 0;
  for (auto & value : values) {
    if (previousvalue > 0) {
      float tangent = atan2 (value - previousvalue, meanvalue);
      float angle = tangent * 180 / M_PI;
      angles.push_back (angle);
    }
    previousvalue = value;
  }
  angle = accumulate (angles.begin(), angles.end(), 0.0) / angles.size();
  
  // Calculate the standard deviation of the rates.
  float variance = 0;
  for (auto a : angles) {
    variance += (a - angle) * (a - angle);
  }
  variance /= angles.size();
  deviation = sqrt (variance);
}


*/


// Collects data for creating a balance chart.
// $exchanges: One or more exchange where the balances are.
// $coin: The coin to get the balances for.
// $days: The length of the period to gather data for.
// $btc: Convert the balances to an equivalent value in Bitcoins.
// $eur: Convert the balances to an equivalent value in Euros.
vector <pair <float, float> > chart_balances (vector <string> exchanges,
                                              string coin,
                                              int days,
                                              bool btc,
                                              bool eur)
{
  vector <pair <float, float> > plotdata;
  SQL sql (back_bot_ip_address ());
  sql.clear ();
  sql.add ("SELECT seconds,");
  string sum = "SUM(total)";
  if (btc) sum = "SUM(btc)";
  if (eur) sum = "SUM(euro)";
  sql.add (sum.c_str());
  sql.add ("FROM balances");
  sql.add ("WHERE TRUE");
  if (!exchanges.empty ()) {
    sql.add ("AND (");
  }
  for (unsigned int i = 0; i < exchanges.size(); i++) {
    if (i) sql.add ("OR");
    sql.add ("exchange =");
    sql.add (exchanges[i]);
  }
  if (!exchanges.empty ()) {
    sql.add (")");
  }
  if (!coin.empty ()) {
    sql.add ("AND coin =");
    sql.add (coin);
  }
  sql.add ("AND stamp >= NOW() - INTERVAL");
  sql.add (days);
  sql.add ("DAY");
  sql.add ("GROUP BY seconds ORDER BY seconds;");
  to_tty ({sql.sql});
  map <string, vector <string> > result = sql.query ();
  vector <string> seconds = result ["seconds"];
  vector <string> total = result [sum];
  plotdata.clear ();
  int lowest_second = 0;
  if (!seconds.empty ()) lowest_second = stoi (seconds[0]);
  for (unsigned int i = 0; i < total.size (); i++) {
    float day = (stoi (seconds[i]) - lowest_second);
    day = day / 3600 / 24;
    float x = day;
    // Plot the total amount of coins.
    float y = str2float (total[i]);
    plotdata.push_back (make_pair (x, y));
    //to_tty ({float2string (y)});
  }
  return plotdata;
}


vector <pair <float, float> > chart_rates (string exchange, string market, string coin, int days)
{
  // Storage for the data to plot.
  vector <pair <float, float> > plotdata;
  
  
  // If the requested market exists, take the rates straight from the database.
  if (allrates_exists (exchange, market, coin)) {
    
    // The second where to start the chart.
    // This will be day 0 on the X-axis.
    int starting_second = seconds_since_epoch () - (days * 3600 * 24);
    
    // Storage for the rates.
    vector <int> seconds;
    vector <float> bids, asks;
    allrates_get (exchange, market, coin, seconds, bids, asks);
    
    // Iterate over the data to generate the data points.
    for (unsigned int i = 0; i < seconds.size (); i++) {
      
      // Skip non-existing data.
      if (seconds[i] < starting_second) continue;
      
      // The number of days will be on the X-axis.
      float day = seconds[i] - starting_second;
      day = day / 3600 / 24;
      day = day - days;
      float x = day;
      
      // The rate of the coin will be on the Y-axis.
      float y = (bids[i] + asks[i]) / 2;
      
      // Store this data point.
      plotdata.push_back (make_pair (x, y));
    }
    
  }
  
  
  // If the requested market does not exist,
  // it takes the rates of the coins on the Bitcoin market,
  // then it takes the rates of the Bitcoin on the desired market,
  // then it converts the rates of the coin to the desired market..
  else {
    
    // Get the rates of the coin on the Bitcoin market.
    to_output output ("");
    unordered_map <int, float> bitcoin_minute_asks, bitcoin_minute_bids, bitcoin_hour_asks, bitcoin_hour_bids;
    pattern_prepare (&output, exchange, bitcoin_id (), coin,
                     bitcoin_minute_asks, bitcoin_minute_bids, bitcoin_hour_asks, bitcoin_hour_bids);
    
    // Get the rates of the Bitcoin on the desired market.
    unordered_map <int, float> market_minute_asks, market_minute_bids, market_hour_asks, market_hour_bids;
    vector <string> exchanges = exchange_get_names ();
    for (auto exchange : exchanges) {
      if (!allrates_exists (exchange, market, bitcoin_id ())) continue;
      pattern_prepare (&output, exchange, market, bitcoin_id (),
                       market_minute_asks, market_minute_bids, market_hour_asks, market_hour_bids);
      break;
    }
    
    // The minute where to start the chart.
    // This will be day 0 on the X-axis.
    int starting_minute = 0 - (days * 60 * 24);
    
    // Iterate over the data to generate the data points.
    for (int i = starting_minute; i <= 0; i++) {
      
      // The number of days will be on the X-axis.
      float day = (i - starting_minute);
      day = day / 60 / 24;
      day = day - days;
      float x = day;
      
      // The rate of the coin will be on the Y-axis.
      float coin_btc_rate = (bitcoin_minute_bids[i] + bitcoin_minute_asks[i]) / 2;
      float btc_market_rate = (market_minute_bids[i] + market_minute_asks[i]) / 2;
      float y = coin_btc_rate * btc_market_rate;
      
      // If the conversion did not work, don't store this data point.
      if (y == 0) continue;
      
      // Store this data point.
      plotdata.push_back (make_pair (x, y));
    }
    
  }
  
  // Done.
  return plotdata;
}


// This function tries to locate an order ID based on the input variables.
// If it finds the order ID, it will pass this in $order_id.
// If successful, it returns true, else it returns false.
bool locate_trade_order_id (void * output,
                            const string & exchange, const string & market, const string & coin, float rate,
                            string & order_id)
{
  // The bundled output object.
  to_output * out = (to_output *) output;
  
  out->add ({"Looking for order with uncertain ID", coin, "@", market, "@", exchange, "rate", float2string (rate)});
  
  // Get the open orders at the exchange, if there's any at all.
  // Since it looks in the open orders rather than in the order history,
  // if the order placed has been fulfilled, it won't find that order here.
  // So there's room for improvement here.
  vector <string> identifiers, markets, coin_abbrevs, coins, buy_sells, dates;
  vector <float> quantities, rates;
  string error;
  exchange_get_open_orders (exchange, identifiers, markets, coin_abbrevs, coins, buy_sells, quantities, rates, dates, error);
  // The above could be cached so as to not query the same exchange twice.
  // But in real life there will only be one uncertain buy order at a given time.
  // So even if the exchange API call result would be cached,
  // it is highly unlikely any gains would be made here.
  
  if (!error.empty ()) return false;
  
  // Iterate over the open orders at the exchange.
  for (unsigned int oo = 0; oo < identifiers.size(); oo++) {
    
    // Get more data about this open order.
    string open_market = markets [oo];
    string open_coin = coins [oo];
    float open_rate = rates [oo];
    
    // If market or coin differ, skip this entry.
    if (market != open_market) continue;
    if (coin != open_coin) continue;
    
    // Do not check on the quantity,
    // because some exchanges only pass the remaining quantity,
    // which may differ a lot from the order quantity.
    
    // If the order is too old, skip it.
    // Only consider recent orders.
    // This is to prevent mixup with older similar-looking orders.
    int order_seconds = exchange_parse_date (exchange, dates[oo]);
    int order_hours_age = (seconds_since_epoch () - order_seconds) / 3600;
    if (order_hours_age > 5) {
      out->add ({"Skipping order ID", identifiers [oo], "since it is", to_string (order_hours_age), "hours old"});
      continue;
    }
    
    // Check that the rates match.
    // Apply a slight margin.
    if (rate > (open_rate * 1.02)) continue;
    if (rate < (open_rate * 0.98)) continue;
    
    // At this stage it is very likely that this open order is the order looked for.
    order_id = identifiers [oo];
    out->add ({"Order ID", order_id, "has been located"});
    
    // Success.
    return true;
  }

  // Failed to locate the order ID.
  return false;
}


mutex cached_approximate_rates_mutex;
// Storage format: Exchange + market = value.
unordered_map <string, int> cached_approximate_rates_seconds;
// Storage format: [exchange + market] = value consisting of [coin] = value.
// The reason for this specific storage format is this:
// * A whole market can be cleared easily.
unordered_map <string, unordered_map <string, float> > cached_approximate_rates_seconds_bids;
unordered_map <string, unordered_map <string, float> > cached_approximate_rates_seconds_asks;


// This uses a memory cache and a timing mechanism.
// It gets the current stored rates.
// If a rate is outdated, it gets updated values from the exchange.
// It returns true if all went well and the rates are current.
bool get_cached_approximate_rate (const string & exchange, const string & market, const string & coin,
                                  float & ask, float & bid)
{
  // Clear values.
  ask = 0;
  bid = 0;
  
  string key = exchange + market;
  
  // Check the current age of the rates stored in memory.
  int age = 0;
  cached_approximate_rates_mutex.lock ();
  age = seconds_since_epoch () - cached_approximate_rates_seconds [key];
  cached_approximate_rates_mutex.unlock ();
  
  // Chgeck if the rates are aged by now.
  if (age >= 10) {
    
    // Obtain fresh summary rates from the exchange.
    vector <string> coins;
    vector <float> bids;
    vector <float> asks;
    exchange_get_market_summaries_via_satellite (exchange, market, coins, bids, asks);
    
    // Multiple threads can access the lookup containers simultaneously.
    cached_approximate_rates_mutex.lock ();
    
    // Clear containers here.
    // So if the exchange provided no rates, due to an error,
    // the containers won't contain outdated rates.
    // And since the age is stil stored for this exchange and market,
    // the exchange won't be overloaded with multiple queries to retry.
    cached_approximate_rates_seconds_bids[key].clear();
    cached_approximate_rates_seconds_asks[key].clear();
    
    // Store the rates in the fast lookup containers.
    for (unsigned int i = 0; i < coins.size(); i++) {
      string coin = coins[i];
      cached_approximate_rates_seconds_bids [key][coin] = bids[i];
      cached_approximate_rates_seconds_asks [key][coin] = asks[i];
    }
    
    // Record the new age of this lot of rates.
    cached_approximate_rates_seconds [key] = seconds_since_epoch ();
    
    cached_approximate_rates_mutex.unlock ();
  }
  
  // Get the rates from memory.
  cached_approximate_rates_mutex.lock ();
  bid = cached_approximate_rates_seconds_bids [key][coin];
  ask = cached_approximate_rates_seconds_asks [key][coin];
  cached_approximate_rates_mutex.unlock ();
  
  // Check the rates.
  // A rate of zero means there was an error somewhere.
  if (bid == 0) return false;
  if (ask == 0) return false;
  
  // Good rates.
  return true;
}


bool get_cached_approximate_order_book (bool buyers, bool sellers,
                                        const string & exchange,
                                        const string & market,
                                        const string & coin,
                                        vector <float> & quantities,
                                        vector <float> & rates)
{
  // Clear the caller's containers.
  quantities.clear ();
  rates.clear ();
  
  // The first step is to get the cached or live rate.
  // If that works out, the exchange is live.
  float ask, bid;
  bool success = get_cached_approximate_rate (exchange, market, coin, ask, bid);
  
  // It the cached or live rate could not be obtained, there's a failure somewhere.
  // Bail out right away.
  if (!success) {
    //to_stdout ({"Failed to get the cached or live rate for", coin, "@", market, "@", exchange});
    return false;
  }
  
  // The live price for the order books: The average of the cached live bid and ask prices.
  float live_price = (ask + bid) / 2;
  
  // Get the stored order book for the buyers or the sellers,
  // and the reference price stored along with it.
  float stored_price;
  if (buyers) buyers_get (exchange, market, coin, stored_price, quantities, rates);
  if (sellers) sellers_get (exchange, market, coin, stored_price, quantities, rates);
  
  // If the live price is still the same, or nearly the same,
  // as the price stored along with the order book,
  // that means that the order books are considered still good enough to be used.
  // So consider the processing complete in that case.
  float percentage = abs (live_price - stored_price) / stored_price * 100;
  if (percentage <= 0.2) return true;
  
  string buyers_sellers;
  if (buyers) buyers_sellers = "buyers";
  if (sellers) buyers_sellers = "sellers";
  
  // At this stage, it needs to fetch an updated order book from the exchange.
  //to_stdout ({"Updating the cached approximate", buyers_sellers, "for", coin, "@", market, "@", exchange});
  if (buyers) exchange_get_buyers_via_satellite (exchange, market, coin, quantities, rates, true, nullptr);
  if (sellers) exchange_get_sellers_via_satellite (exchange, market, coin, quantities, rates, true, nullptr);
  
  // Whatever order book it got, errors or not, stored it in the cache.
  if (buyers) buyers_set (exchange, market, coin, live_price, quantities, rates);
  if (sellers) sellers_set (exchange, market, coin, live_price, quantities, rates);

  if (quantities.empty ()) {
    to_stdout ({"Failed to update the cached approximate", buyers_sellers, "for", coin, "@", market, "@", exchange});
  }
  
  // Everything's OK if there's data.
  return !quantities.empty ();
}


bool get_cached_approximate_buyers (const string & exchange,
                                    const string & market,
                                    const string & coin,
                                    vector <float> & quantities,
                                    vector <float> & rates)
{
  return get_cached_approximate_order_book (true, false, exchange, market, coin, quantities, rates);
}


bool get_cached_approximate_sellers (const string & exchange,
                                     const string & market,
                                     const string & coin,
                                     vector <float> & quantities,
                                     vector <float> & rates)
{
  return get_cached_approximate_order_book (false, true, exchange, market, coin, quantities, rates);
}


// Converts the amount of $bitcoins to a value in Euros.
float bitcoin2euro (float bitcoins)
{
  // Calculation of average of multiple exchanges.
  float average = 0;
  int count = 0;
  // The exchanges to query the rate from.
  vector <string> exchanges = { bitfinex_get_exchange (), kraken_get_exchange (), bl3p_get_exchange () };
  for (auto exchange : exchanges) {
    // Get the cached rate.
    float rate = rate_get_cached (exchange, euro_id (), bitcoin_id());
    // If a rate given, add it to the average calculation.
    if (rate > 0) {
      average += rate;
      count++;
    }
  }
  // Calculate the average.
  average /= count;
  // Do the conversion and pass it to the caller.
  return bitcoins * average;
}


