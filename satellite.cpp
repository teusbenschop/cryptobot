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
#include "exchanges.h"
#include "shared.h"
#include "io.h"
#include "proxy.h"
#include "controllers.h"
#include "models.h"
#include "sqlite.h"


// This is the satellite server.
// It serves the market summaries, the buyers, and the sellers, via http.

// The initial reason for having this server is as follows:
// The bot was going to use more and more CPU.
// So there was a need to rent a more powerful virtual server.
// Or to put a dedicated server in a datacenter.
// Since the bot is not a public server that should be online all of the time,
// it made sense to put the bot on a server at home.
// That would be less expensive than putting the bot on a server in a datacenter.
// But the problem with this approach was that the server used a lot of data
// when downloading the market summaries and the rates.
// That is when the plan came up to leave the part that uses a lot of data in the cloud,
// and move the bot itself, that requires a lot of CPU power, to a server at home.
// So the bot at home fetches the rates from the online server.
// The online rates server receives a lot of bytes from the various exchanges,
// strips out whatever is not needed, and then server the data in bare form to the bot.
// This way the data usage of the bot at home is reduced a lot.

// Through the libcurl option VERBOSE,
// it can be seen that there's a huge lot of data being transferred for a single call,
// because it is HTTPS,
// and other stuff is added.
// So if using the satellite, it saves a huge lot of data to the main bot.

// Library libcurl is using a lot of CPU.
// When using this satellite, the CPU usage of libcurl is offloaded to that satellite.

// Initially the server was left running continually.
// But then it happened that the Bitfinex websocket connection got stuck.
// So it kept giving the same rates, and limit trade orders kept being placed.
// This led to the wrong arbitrage trading.
// So now the server is started at the start of every minute.
// It quits on its own just before the end of every minute.

// If there are connection issues while switching to the next satellite,
// a possible solution would be this:
// Two satellites both running at different ports.
// If a satellite on one port starts, it signals the one on the other port, to shut down.
// It does not shut down straightwawy.
// It gives a different response to the heartbeat, so the clients know to use another satellite.
// The satellite shuts down after a few minutes.
// The bot will have switched already to the second satellite.

// The bot used to get the order books through various proxy servers.
// It now get the same information through this satellite.
// The number of succeeded hurried calls to get the order books has improved a lot now.
// The failed hurried call count is now low or zero.
// This may also be related to the 10+ IPv6 global addresses per satellite.
// The former proxy server had only one IPv6 address.


#include "libraries.h"
#include "shared.h"
#include "io.h"


bool binary_listener_healthy = true;
mutex binary_server_mutex;
unordered_map <string, int> binary_server_addresses;
atomic <int> total_client_count (0);
atomic <int> active_thread_count (0);


// Processes a single request from a web client.
void process_binary_client_request (int connfd, string clientaddress)
{
  // One extra active thread has started.
  active_thread_count++;
  
  try {
    
    // Statistics.
    total_client_count++;
    
    // Connection healthy flag.
    bool connection_healthy = true;
    
    // Read the client's request.
    // It is not possible to read the request till EOF.
    // The EOF does not come right away.
    // The client keeps the connection open for receiving the response.
    // The binary protocol consists of one line of input.
    // Read one line of data from the client.
    char inputbuffer [1024];
    // Fix valgrind unitialized value message.
    memset (&inputbuffer, 0, 1024);
    {
      unsigned int i = 0;
      char character = '\0';
      // Limit the input length, for dealing with rogue clients.
      while ((i < sizeof (inputbuffer) - 1) && (character != '\n')) {
        int n = recv (connfd, &character, 1, 0);
        if (n > 0) {
          inputbuffer[i] = character;
          i++;
        } else {
          character = '\n';
        }
      }
      // Terminate the C string with a NULL character.
      inputbuffer[i] = '\0';
    }
    
    if (connection_healthy) {
      
      string reply;
      
      vector <string> elements = explode (trim(inputbuffer), ' ');
      
      // Heartbeat.
      if (elements.size () == 1) {
        if (elements[0] == "heartbeat") {
          reply = "OK";
        }
      }
      
      // Server shutdown.
      if (elements.size () == 3) {
        if (elements[0] == "next") {
          if (elements[1] == "instance") {
            if (elements[2] == "waiting") {
              reply = "OK";
              binary_listener_healthy = false;
            }
          }
        }
      }
      
      // Getting market summaries.
      if (elements.size () == 3) {
        if (elements[0] == "summaries") {
          string exchange = elements[1];
          string market = elements[2];
          vector <string> coins;
          vector <float> bids, asks;
          string error;
          // Call the API.
          exchange_get_market_summaries (exchange, market, coins, bids, asks, &error);
          // Response container.
          vector <string> response;
          // Assemble the reply.
          for (unsigned int i = 0; i < coins.size (); i++) {
            string line = coins[i] + " " + float2string (bids[i]) + " " + float2string (asks[i]);
            response.push_back (line);
          }
          // Error handling:
          // If there's an error, the response to the caller consists of one line: The error.
          if (!error.empty ()) response = { error };
          // Done.
          reply = implode (response, "\n");
        }
      }
      
      // Getting buyers and sellers.
      if (elements.size () == 4) {
        bool get_buyers = (elements[0] == "buyers");
        bool get_sellers = (elements[0] == "sellers");
        if (get_buyers || get_sellers) {
          string exchange = elements[1];
          string market = elements[2];
          string coin = elements[3];
          vector <float> quantities, rates;
          string error;
          // Call the API.
          // No hurried calls here: The client determines the measure of hurry:
          // The client disconnects when in a hurry, and the response is not yet in.
          if (get_buyers) exchange_get_buyers (exchange, market, coin, quantities, rates, false, error);
          if (get_sellers) exchange_get_sellers (exchange, market, coin, quantities, rates, false, error);
          // Response container.
          vector <string> response;
          for (unsigned int i = 0; i < quantities.size (); i++) {
            // Limit the number of entries. It saves bandwidth.
            if (i >= 10) continue;
            // Add this entry to the response.
            string line = float2string (quantities[i]) + " " + float2string (rates[i]);
            response.push_back (line);
          }
          // Error handling:
          // If there's an error, the response to the caller consists of one line: The error.
          if (!error.empty ()) response = { error };
          // Done.
          reply = implode (response, "\n");
        }
      }
      
      // Get the distinct rates seconds.
      if (elements.size () == 5) {
        if (elements[0] == "rates") {
          if (elements[1] == "between") {
            if (elements[2].size() == 10) {
              int second_from = stoi (elements[2]);
              if (elements[3] == "and") {
                if (elements[4].size() == 10) {
                  int second_to = stoi (elements[4]);
                  // Read all the files in the database folder.
                  string databases_folder = SqliteDatabase::path ("");
                  vector <string> files = scandir (databases_folder);
                  // Storage for the database with the seconds in their names.
                  vector <int> seconds_databases;
                  // Iterate over all files, and strip bits off,
                  // so it remains with the clear second.
                  for (auto file : files) {
                    if (file.find ("allrates") != string::npos) {
                      file.erase (0, 9);
                      file.erase (10);
                      seconds_databases.push_back (stoi (file));
                    }
                  }
                  // Storage for the seconds there's databases for.
                  vector <string> existing_seconds;
                  // Iterate over the database and check they fall within range.
                  for (auto second : seconds_databases) {
                    if (second < second_from) continue;
                    if (second > second_to) continue;
                    existing_seconds.push_back (to_string (second));
                  }
                  // Add at least one string in case there's no seconds.
                  // If there's no seconds, there will be no reply,
                  // and then the satellite will mark the client as rogue.
                  existing_seconds.push_back ("OK");
                  // Assemble the reply.
                  reply = implode (existing_seconds, "\n");
                }
              }
            }
          }
        }
      }
      
      // Serve a rates database for a particular second.
      if (elements.size () == 3) {
        if (elements[0] == "get") {
          if (elements[1] == "rates") {
            if (elements[2].size() == 10) {
              int second = stoi (elements[2]);
              // Send the whole database in raw format to the client.
              string database = allrates_database (second);
              string path = SqliteDatabase::path (database);
              reply = file_get_contents (path);
            }
          }
        }
      }

      // Serve and rotate the log file.
      if (elements.size () == 3) {
        if (elements[0] == "serve") {
          if (elements[1] == "rotate") {
            if (elements[2] == "logs") {
              reply = "OK\n";
              string path = tmp_handelaar_path ();
              reply.append (file_get_contents (path));
              unlink (path.c_str());
            }
          }
        }
      }

      // Record this IP addresss whether it is a normal client or a rogue one.
      int addition = -1;
      if (!reply.empty ()) addition = 1;
      binary_server_mutex.lock ();
      binary_server_addresses [clientaddress] += addition;
      binary_server_mutex.unlock ();
      
      // Send response to the client.
      const char * output = reply.c_str();
      // The C function strlen () fails on null characters in the reply, so use string::size() instead.
      size_t length = reply.size ();
      send (connfd, output, length, 0);
    }
    
  } catch (exception & e) {
    to_stderr ({"Internal error:", e.what ()});
  } catch (exception * e) {
    to_stderr ({"Internal error:", e->what ()});
  } catch (...) {
    to_stderr ({"A general internal error occurred"});
  }
  
  // Done: Close.
  shutdown (connfd, SHUT_RDWR);
  close (connfd);
  
  // Decrease the number of active threads.
  active_thread_count--;
}


void binary_server ()
{
  binary_listener_healthy = true;
  
  // This server uses BSD sockets.
  // Create a listening socket.
  // This represents an endpoint.
  // This prepares to accept incoming connections on.
  // It listens on address family AF_INET6 for both IPv4 and IPv6.
  int listenfd = socket (AF_INET6, SOCK_STREAM, 0);
  if (listenfd < 0) {
    to_stdout ({"Error opening socket:", strerror (errno)});
    binary_listener_healthy = false;
  }
  
  // Eliminate "Address already in use" error from bind.
  // The function is used to allow the local address to  be reused
  // when the server is restarted before the required wait time expires.
  int optval = 1;
  int result = setsockopt (listenfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &optval, sizeof (int));
  if (result != 0) {
    to_stdout ({"Error setting socket option:", strerror (errno)});
    binary_listener_healthy = false;
  }
  
  // The listening socket will be an endpoint for all requests to a port on this host.
  // It listens on any IPv6 address.
  struct sockaddr_in6 serveraddr;
  memset (&serveraddr, 0, sizeof (serveraddr));
  serveraddr.sin6_flowinfo = 0;
  serveraddr.sin6_family = AF_INET6;
  serveraddr.sin6_addr = in6addr_any;
  serveraddr.sin6_port = htons (satellite_port ());
  result = mybind (listenfd, (struct sockaddr *) &serveraddr, sizeof (serveraddr));
  if (result != 0) {
    to_stdout ({"Error binding server to socket:", strerror (errno)});
    binary_listener_healthy = false;
  }
  
  // Make it a listening socket ready to queue and accept many connection requests
  // before the system starts rejecting the incoming requests.
  int backlog = 128;
  result = listen (listenfd, backlog);
  if (result != 0) {
    to_stdout ({"Error listening on socket:", strerror (errno)});
    binary_listener_healthy = false;
  }
  
  // Keep waiting for, accepting, and processing connections.
  while (binary_listener_healthy) {
    
    // Socket and file descriptor for the client connection.
    struct sockaddr_in6 clientaddr6;
    socklen_t clientlen = sizeof (clientaddr6);
    int connfd = accept (listenfd, (struct sockaddr *)&clientaddr6, &clientlen);
    if (connfd > 0) {
      
      // Socket send and receive timeout.
      // It was tested that it disconnects an idle client after the specified timeout.
      struct timeval tv;
      tv.tv_sec = 60;
      tv.tv_usec = 0;
      setsockopt (connfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
      setsockopt (connfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
      
      // The client's remote IPv6 address in hexadecimal digits separated by colons.
      // IPv4 addresses are mapped to IPv6 addresses.
      string clientaddress;
      char remote_address[256];
      inet_ntop (AF_INET6, &clientaddr6.sin6_addr, remote_address, sizeof (remote_address));
      clientaddress = remote_address;
      
      // Handle dropping incoming connections from rogue clients.
      binary_server_mutex.lock ();
      int count = binary_server_addresses [clientaddress];
      binary_server_mutex.unlock ();
      if (count < 0) {
        shutdown (connfd, SHUT_RDWR);
        close (connfd);
        continue;
      }
      
      // Handle this request in a thread, enabling parallel requests.
      thread job (process_binary_client_request, connfd, clientaddress);
      job.detach ();
      
    } else {
      to_stdout ({"Error accepting connection on socket:", strerror (errno)});
    }
  }
  
  // Close listening socket, freeing it for any next server process.
  close (listenfd);
}


void satellite_emergency_exit_timer ()
{
  this_thread::sleep_for (chrono::minutes (1));
  exit (0);
}


int main (int argc, char *argv[])
{
  // Initialize the program, but not the proxy servers.
  initialize_program (argc, argv);
  
  to_stdout ({"Starting satellite"});

  // Register the device's public IP addresses.
  register_ip_interfaces ();
  
  // Test the known hosts for IPv6 access.
  satellite_register_known_hosts ();
  
  // This satellite contacts a possible previous instance of this satellite.
  // It waits till the previous instance has quit.
  // Then it will proceed to act as the new satellite.
  bool previous_satellite_live = true;
  int tries = 0;
  do {
    string error;
    satellite_request ("localhost", "next instance waiting", false, error);
    previous_satellite_live = error.empty ();
    tries++;
    to_tty ({"try", to_string (tries), error});
    this_thread::sleep_for (chrono::milliseconds (100));
  } while (previous_satellite_live && (tries < 500));
  
  // Run the http server.
  binary_server ();
  
  // Start emergency exit timer.
  thread job (satellite_emergency_exit_timer);
  job.detach ();

  // After the server shuts down and no longer accepts new connections,
  // wait till all client processors have completed.
  // So, it does not close or interrupt ongoing client requests: It completes them.
  while (active_thread_count) this_thread::sleep_for (chrono::milliseconds (100));
  
  to_stdout ({"Stopping satellite", "-", "done", to_string (total_client_count), "requests"});

  finalize_program ();
  
  return EXIT_SUCCESS;
}
