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
#include "exchanges.h"
#include "models.h"
#include "controllers.h"
#include "proxy.h"


int main (int argc, char *argv[])
{
  // Start up.
  initialize_program (argc, argv);

  
  SQL sql (front_bot_ip_address ());
  
  
  // Gather all the arguments passed to this program.
  vector <string> arguments;
  for (int i = 1; i < argc; i++) {
    arguments.push_back (argv[i]);
  }

  
  string execute = "execute";
  string arrive = "arrive";
  string comment = "comment";

  
  // Whether to set a transfer order as executed by the exchange where the withdrawal was made.
  if (in_array (execute, arguments)) {
    string exchange = arguments [1];
    string order_id = arguments [2];
    string transfer_id = arguments [3];
    vector <string> order_ids;
    vector <string> withdraw_exchanges;
    vector <string> coin_abbreviations;
    vector <string> deposit_exchanges;
    vector <float> quantities;
    transfers_get_non_executed_orders (sql, order_ids, withdraw_exchanges, coin_abbreviations, deposit_exchanges, quantities);
    bool order_found = false;
    for (unsigned int i = 0; i < order_ids.size (); i++) {
      if (order_id != order_ids[i]) continue;
      if (exchange != withdraw_exchanges[i]) continue;
      to_tty ({"order id", order_ids[i], to_string (quantities[i]), coin_abbreviations[i], "withdrawn from", withdraw_exchanges[i], "deposited to", deposit_exchanges[i], "set as executed" });
        order_found = true;
    }
    if (order_found) {
      transfers_set_order_executed (sql, exchange, order_id, transfer_id);
    } else {
      to_tty ({"Could not find non-executed order id", order_id});
    }
  }

  
  // Whether to set a transfer order as arrived at the exchange where the deposit was made.
  else if (in_array (arrive, arguments)) {
    // Input variables.
    string exchange = arguments [1];
    string order_id = arguments [2];
    string transfer_id = arguments [3];
    float quantity = 0;
    // Get the non-arrived orders.
    vector <string> order_ids;
    vector <string> withdraw_exchanges;
    vector <string> deposit_exchanges;
    vector <string> coin_abbreviations_withdraw;
    vector <string> coin_abbreviations_deposit;
    vector <float> quantities;
    vector <string> transaction_ids;
    transfers_get_non_arrived_orders (sql,
                                      order_ids,
                                      withdraw_exchanges,
                                      deposit_exchanges,
                                      coin_abbreviations_withdraw,
                                      coin_abbreviations_deposit,
                                      quantities,
                                      transaction_ids);
    // Check if the order exists as non-arrived.
    bool order_found = false;
    for (unsigned int i = 0; i < order_ids.size (); i++) {
      if (order_id != order_ids[i]) continue;
      if (exchange != deposit_exchanges[i]) continue;
      quantity = quantities[i];
      to_tty ({"order id", order_ids[i], to_string (quantities[i]), coin_abbreviations_deposit[i], "withdrawn from", withdraw_exchanges[i], "deposited to", deposit_exchanges[i], "set as arrived" });
      order_found = true;
    }
    if (order_found) {
      transfers_set_order_arrived (sql, exchange, order_id, transfer_id, quantity);
    } else {
      to_tty ({"Could not find non-arrived order id", order_id});
    }
  }

  
  // Look into whether to order a transfer of coins.
  else {

    
    // Find two existing exchanges in the arguments.
    vector <string> exchanges = exchange_get_names ();
    string exchange_withdraw;
    string exchange_deposit;
    for (auto argument : arguments) {
      if (in_array (argument, exchanges)) {
        if (exchange_withdraw.empty ()) exchange_withdraw = argument;
        else if (exchange_deposit.empty ()) exchange_deposit = argument;
      }
    }
    if (!exchange_withdraw.empty () && !exchange_deposit.empty ()) {
      cout << "Exchange to withdraw from: " << exchange_withdraw << endl;
      cout << "Exchange to deposit to: " << exchange_deposit << endl;
      
      
      // Read the coins known at both of the exchanges.
      // Find the coin identifier passed on the command line.
      vector <string> coin_ids_withdraw = exchange_get_coin_ids (exchange_withdraw);
      vector <string> coin_ids_deposit = exchange_get_coin_ids (exchange_deposit);
      vector <string> coin_ids = array_intersect (coin_ids_withdraw, coin_ids_deposit);
      vector <string> intersection = array_intersect (arguments, coin_ids);
      if (!intersection.empty ()) {
        string coin_id = intersection [0];
        string coin_abbreviation_withdraw = exchange_get_coin_abbrev (exchange_withdraw, coin_id);
        string coin_abbreviation_deposit = exchange_get_coin_abbrev (exchange_deposit, coin_id);
        string coin_name_withdraw = exchange_get_coin_name (exchange_withdraw, coin_id);
        string coin_name_deposit = exchange_get_coin_name (exchange_deposit, coin_id);
        cout << "Coin: " << coin_id << " from " << coin_abbreviation_deposit << " " << coin_name_deposit <<  " to " << coin_abbreviation_withdraw << " " << coin_name_withdraw << endl;
        
        
        // Look for the amount to transfer.
        float quantity = 0;
        for (auto argument : arguments) {
          if (is_float (argument)) {
            float value = str2float (argument);
            if (quantity == 0) {
              quantity = value;
            }
          }
        }
        if (quantity != 0) {
          cout << "Quantity: " << quantity << " " << coin_abbreviation_withdraw << endl;
        } else {
          cerr << "Please pass the quantity to transfer" << endl;
        }
        
        
        // Optionally update the order amount depending on the coin and the exchange.
        float old_quantity = quantity;
        wallets_update_withdrawal_amount (sql, exchange_withdraw, coin_id, quantity);
        if (quantity != old_quantity) {
          // Normally it uses "float2string", but this upsets Bittrex's Neo withdrawals.
          // So for consistency, it uses "to_string" here and when withdrawinng from Bittrex.
          to_stdout ({"The quantity was updated to", to_string (quantity), coin_id});
        }
        
        
        // Place the order.
        if (quantity > 0) {
          to_output output ("");
          order_transfer (&output, exchange_withdraw, exchange_deposit, coin_id, quantity, true);
        }
        
        
      } else {
        cerr << "Please pass one of the known coins at both of the exchanges:";
        for (auto coin_id : coin_ids) cerr << " " << coin_id;
        cerr << endl;
      }
    } else {
      if (arguments.empty ()) {
        cerr << "Usage: ./transfer execute exchange order-id txid" << endl;
        cerr << "Usage: ./transfer arrive exchange order-id txid" << endl;
        cerr << "Usage: ./transfer exchange-withdraw exchange-deposit coin quantity" << endl;
        cerr << "Please pass two of the known exchanges:";
        for (auto exchange : exchanges) cerr << " " << exchange;
        cerr << endl;
      }
    }
  }
  
  
  
  // Done.
  finalize_program ();
  return EXIT_SUCCESS;
}
