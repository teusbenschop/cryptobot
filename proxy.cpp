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


#include "proxy.h"
#include "models.h"


/*
 
Most of the exchanges limit the rate at which the API calls are made.
Some of the exchange set the limit lower and some set them higher.
Yobit for example very regularly says: "Ddos, 20-50 mins".
This type of error leads to a lower trade volume.
Because if the coin rates cannot be obtained at any given time, no arbitrage can be done.
To work around this, the bot will use multiple IP address to make the calls from.
The exchange will then accept a higher API call rate from this bot
because the exchange will see the calls coming from different IP addresses,
not knowing that they come from one and the same bot.
This applies only to the public calls, not to the authenticated calls.
Because authenticated calls carry a key that the exchange can link to an account.
Public calls don't carry such information.
The bot uses multiple anonymous proxy servers.


Setup of VPS at RAMnode:

ssh root@xxx.xxx.xxx.xxx - provide the password given by RAMnode

passwd - Change it to:
rjPf7ibnwWrjPf7ibnwWrjPf7ibnwWrjPf7ibnwWrjPf7ibnwWrjPf7ibnwWrjPf7ibnwWrjPf7ibnwWrjPf7ibnwWrjPf7ibnwWrjPf7ibnwW


With the exchanges Bittrex / Cryptopia / Yobit and Bitfinex,
four exchanges, and with two proxy servers,
the proxy server at RAMnode had nearly used up all of its bandwith
within about half a month.
If there's going to be more exchanges added,
and consequently a higher proxy bandwidth usage,
it becomes less expensive to run proxy servers elsewhere.
For example running them at PCExtreme.

There was a proxy in Atlanta, via RAMnode, that had often timeout errors.
Pinging it gave a latency of more than 100 ms. It was removed.

Add IPv6 addresses manually to each satellite.
Include the automatically assigned IPv6 address in the list too.
Add the following to /etc/rc.local:

proxy01:
 
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2527/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2528/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2529/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:252a/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:252b/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:252c/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:252d/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:252e/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:252f/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2531/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2532/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2533/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2534/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2535/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2536/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2537/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2538/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2539/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:253a/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:253b/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:253c/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:253d/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:253e/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:253f/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2541/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2542/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2543/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2544/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2545/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2546/64 dev ens3
ip -6 addr add 2a00:f10:402:781:49e:24ff:fe00:2547/64 dev ens3

proxy02:
 
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:13f/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:141/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:142/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:143/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:144/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:145/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:146/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:147/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:148/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:149/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:14a/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:14b/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:14c/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:14d/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:14e/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:14f/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:151/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:152/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:153/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:154/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:155/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:156/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:157/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:158/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:159/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:15a/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:15b/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:15c/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:15d/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:15e/64 dev ens3
ip -6 addr add 2a00:f10:401:0:466:aeff:fe00:15f/64 dev ens3

proxy03:

ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:117/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:118/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:119/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:11a/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:11b/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:11c/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:11d/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:11e/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:11f/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:121/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:122/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:123/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:124/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:125/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:126/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:127/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:128/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:129/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:12a/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:12b/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:12c/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:12d/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:12e/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:12f/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:13a/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:132/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:133/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:134/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:135/64 dev ens3
ip -6 addr add 2a00:f10:401:0:4b8:40ff:fe00:136/64 dev ens3

 
proxy04:

ip -6 addr add 2a00:f10:401:0:402:beff:fe00:157/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:158/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:159/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:15a/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:15b/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:15c/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:15d/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:15e/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:15f/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:161/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:162/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:163/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:164/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:165/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:166/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:167/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:168/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:169/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:16a/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:16b/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:16c/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:16d/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:16e/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:16f/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:171/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:172/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:173/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:174/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:175/64 dev ens3
ip -6 addr add 2a00:f10:401:0:402:beff:fe00:176/64 dev ens3

proxy05:

ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:279/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:27a/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:27b/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:27c/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:27d/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:27e/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:27f/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:281/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:282/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:283/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:284/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:285/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:286/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:287/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:288/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:289/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:28a/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:28b/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:28c/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:28d/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:28e/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:28f/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:291/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:292/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:293/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:294/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:295/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:296/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:297/64 dev ens3
ip -6 addr add 2a00:f10:400:2:4d9:52ff:fe00:298/64 dev ens3

 The information above is no longer accurate.
 When adding extra IPv6 addresees, there's lots of timeouts.
 See this email from PCExtreme:

 Bij de nieuwe versie wordt er door de software ook security groups op ipv6 toegepast. We ervaarde gisteren wat issues met een server welke meerdere ipv6 ip's aannam. Dit is nooit de bedoeling geweest en stond in onze templates ook niet aan. Als u dit corrigeert vermoed ik dat alles weer goed functioneert.
 Mijn vermoeden is dat je dit aan moet passen, het gedrag v/d timeout zal hoogst waarschijnlijk komen omdat uw server een extra adres heeft aangenomen en via dit adres de DNS probeert te resolven.

 */




mutex proxy_satellite_update_mutex;
vector <string> proxy_servers_ipv4;
vector <string> proxy_servers_ipv6;
vector <string> proxy_servers_ipv46;
vector <string> satellites;


vector <string> satellite_hosts ()
{
  vector <string> hosts = {
    // front bot
    "11.11.11.11",
    // proxy01 @ pcextreme
    ///"11.11.11.11",
    // proxy02 @ pcextreme
    // "11.11.11.11",
    // proxy03 @ pcextreme
    // "11.11.11.11",
    // proxy04 @ pcextreme
    // "11.11.11.11",
    // proxy05 @ pcextreme
    // "11.11.11.11",
  };
  return hosts;
}


void proxy_test (const string & host, bool ipv6, bool verbose)
{
  // Contact a website via the proxy.
  // These proxy servers are often used for API calls to the exchanges for getting the order books.
  // Getting the order books is a time-sensitive operation.
  // For that reason, this tests the proxy in hurried mode.
  // It means that if the proxy does not respond with the desired data very soon,
  // it will be discarded for just now.
  string error;
  string html = http_call ("http://website.nl", error, host, verbose, false, "", {}, true, false, "");
  if (!error.empty ()) {
    to_tty ({host, error});
    to_failures (failures_type_api_error (), "", "", 0, {"Testing proxy", host, ":", error});
    return;
  }
  
  // The returned html should be long enough.
  // The call to website returned a length of 4863 bytes.
  if (html.size () < 3000) {
    to_stdout ({"The length of the content of a website accessed via proxy", host, "is only", to_string (html.size()), "which is too short"});
    return;
  }

  // Store this proxy as a live one.
  proxy_satellite_update_mutex.lock ();
  //to_tty ({host});
  if (ipv6) {
    proxy_servers_ipv6.push_back (host);
  } else {
    proxy_servers_ipv4.push_back (host);
  }
  proxy_servers_ipv46.push_back (host);
  proxy_satellite_update_mutex.unlock ();
}


void satellite_test (const string & host)
{
  // Test the heartbeat of the satellite.
  // These satellites are often used for API calls to the exchanges for getting the order books.
  // Getting the order books is a time-sensitive operation.
  // For that reason, this tests the proxy in hurried mode.
  // It means that if the proxy does not respond with the desired data very soon,
  // it will be discarded for just now.
  // Do it in a hurry, to be sure the connection is really good.
  string request = "heartbeat";
  bool hurry = true;
  string error;
  string result = satellite_request (host, request, hurry, error);

  // There should be no error, if the satellite can be contacted.
  if (!error.empty ()) {
    to_tty ({host, error});
    to_failures (failures_type_api_error (), "", "", 0, {"Testing satellite", host, ":", error});
    return;
  }
  
  // The satellite's response should be good.
  if (result != "OK") {
    to_tty ({host, error});
    to_failures (failures_type_api_error (), "", "", 0, {"Testing satellite", host, ":", result});
    return;
  }

  // Store this satellite as a live one.
  proxy_satellite_update_mutex.lock ();
  satellites.push_back (host);
  proxy_satellite_update_mutex.unlock ();
}


void proxy_satellite_register_live_ones ()
{
  // The possible IPv4 proxy hosts and satellites.
  vector <string> possible_hosts_ipv4 = satellite_hosts ();
  // possible_hosts_ipv4 = { "localhost" };
  /*
  vector <string> possible_hosts_ipv6 = {
    "[2a00:f10:402:781:49e:24ff:fe00:2527]", // proxy01 @ pcextreme
    "[2a00:f10:401:0:466:aeff:fe00:13f]", // proxy02 @ pcextreme
    "[2a00:f10:401:0:4b8:40ff:fe00:117]", // proxy03 @ pcextreme
    "[2a00:f10:401:0:402:beff:fe00:157]", // proxy04 @ pcextreme
    "[2a00:f10:400:2:4d9:52ff:fe00:279]", // proxy05 @ pcextreme
  };
   */
  // Store parallel jobs.
  vector <thread *> jobs;
  /*
  // Test all possible IPv4 proxies in parallel threads.
  for (auto host : possible_hosts_ipv4) {
    host += ":8888";
    thread * job = new thread (proxy_test, host, false, false);
    jobs.push_back (job);
  }
  // Test all possible IPv6 proxies in parallel threads.
  for (auto host : possible_hosts_ipv6) {
    host += ":8888";
    thread * job = new thread (proxy_test, host, true, false);
    jobs.push_back (job);
  }
   */
  // Test all satellites in parallel threads.
  for (auto host : possible_hosts_ipv4) {
    thread * job = new thread (satellite_test, host);
    jobs.push_back (job);
  }
  // Wait till the threads are ready.
  for (auto job : jobs) {
    job->join ();
  }
}


map <string, size_t> proxy_selector;


// Returns the proxy host to use.
string proxy_get_ipv4 (const string & exchange)
{
  // Any available proxy hosts?
  if (proxy_servers_ipv4.empty ()) return "";

  // Take next proxy, rotating through the available hosts.
  // The rotation depends on the name of the exchanges.
  // The goal is that each exchange rotates sequentially through the proxies,
  // rather than that the entire bot rotates through them.
  string proxy;
  proxy_satellite_update_mutex.lock ();
  proxy_selector [exchange]++;
  if (proxy_selector [exchange] >= proxy_servers_ipv4.size()) {
    proxy_selector [exchange] = 0;
  }
  proxy = proxy_servers_ipv4[proxy_selector[exchange]];
  proxy_satellite_update_mutex.unlock ();
  
  // Done.
  return proxy;
}


// Returns the proxy host to use.
string proxy_get_ipv46 (const string & exchange)
{
  // Any available proxy hosts?
  if (proxy_servers_ipv46.empty ()) return "";
  
  // Take next proxy, rotating through the available hosts.
  // The rotation depends on the name of the exchanges.
  // The goal is that each exchange rotates sequentially through the proxies,
  // rather than that the entire bot rotates through them.
  string proxy;
  proxy_satellite_update_mutex.lock ();
  proxy_selector [exchange]++;
  if (proxy_selector [exchange] >= proxy_servers_ipv46.size()) {
    proxy_selector [exchange] = 0;
  }
  proxy = proxy_servers_ipv46[proxy_selector[exchange]];
  proxy_satellite_update_mutex.unlock ();
  
  // Done.
  return proxy;
}


map <string, size_t> satellite_selector;


// Returns the satellite to use.
string satellite_get (const string & exchange)
{
  // Any available live satellites?
  if (satellites.empty ()) return "";
  
  // Take next satellite, rotating through the available hosts.
  // The rotation depends on the name of the exchanges.
  // The goal is that each exchange rotates sequentially through the satellites,
  // rather than that the entire bot rotates through them.
  string satellite;
  proxy_satellite_update_mutex.lock ();
  satellite_selector [exchange]++;
  if (satellite_selector [exchange] >= satellites.size()) {
    satellite_selector [exchange] = 0;
  }
  satellite = satellites [satellite_selector [exchange]];
  proxy_satellite_update_mutex.unlock ();
  
  // Done.
  return satellite;
}


vector <string> ipv4_interfaces;
vector <string> ipv6_interfaces;
// Cloudflare rate limiting applies to one IPv4 address or to an entire IPv6/64 block.
// This is what Cloudflare says.
// It means that having multiple IPv6 addresses on one droplet
// is no use to bypass Cloudflare rate limiting.
// So a host like Yobit, that is accessible via IPv6,
// the satellite should also access it via IPv4,
// to spread the originating addresses Cloudflare sees.
// It was tested whether it makes any difference whether a satellite has only one IPv6 address,
// or multiple of them.
// It appears that more IPv6 addresses reduce the Cloudflare rate limiting.
vector <string> ipv46_interfaces;


// Register the device's IP addresses.
void register_ip_interfaces ()
{
  struct ifaddrs * ifAddrStruct=NULL;
  struct ifaddrs * ifa=NULL;
  void * tmpAddrPtr=NULL;
  
  getifaddrs(&ifAddrStruct);
  
  for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr) {
      continue;
    }
    string interface;
    string address;
    // Check whether it is IPv4.
    if (ifa->ifa_addr->sa_family == AF_INET) {
      // Check it is a valid IPv4 address.
      tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
      char addressBuffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
      interface = ifa->ifa_name;
      address = addressBuffer;
    }
    // Check whether it is IPv6.
    else if (ifa->ifa_addr->sa_family == AF_INET6) {
      // Check it is a valid IPv6 address.
      tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
      char addressBuffer[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
      interface = ifa->ifa_name;
      address = addressBuffer;
    }
    // Skip loopback interface.
    if (interface == "lo") continue;
    if (address.find ("127.") == 0) continue;
    if (address == "::1") continue;
    // Skip private IPv4 address.
    if (address.find ("10.") == 0) continue;
    // Skip private IPv6 address.
    if (address.find ("fe80") == 0) continue;
    if (address.empty ()) continue;
    // Check IPv4 or IPv6.
    bool is_ipv6 = address.find (":") != string::npos;
    // Store the address in its proper containers.
    if (is_ipv6) ipv6_interfaces.push_back (address);
    else ipv4_interfaces.push_back (address);
    // Always store it in the container with both IPv4 and IPv6 interfaces.
    ipv46_interfaces.push_back (address);
    //to_tty ({address});
  }

  if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
}


mutex interface_mutex;
map <string, size_t> interface_selector;


string get_ipv46_interface (const string & host)
{
  // Any available IPv4 or IPv6 interfaces?
  if (ipv46_interfaces.empty ()) return "";
  
  // Take next interface, rotating through the available ones.
  // The rotation depends on the name of the host.
  // The goal is that each host rotates sequentially through the interfaces,
  // rather than that the entire bot rotates through them.
  string interface;
  interface_mutex.lock ();
  interface_selector [host]++;
  if (interface_selector [host] >= ipv46_interfaces.size()) {
    interface_selector [host] = 0;
  }
  interface = ipv46_interfaces[interface_selector[host]];
  interface_mutex.unlock ();
  
  // Done.
  return interface;
}


// The IP addresses of the front and back bots.
string front_bot_ip_address ()
{
  return "11.11.11.11";
}
string back_bot_ip_address ()
{
  return "192.168.2.3";
}


// The names of the front and back bots.
string front_bot_name ()
{
  return "frontbot";
}
string back_bot_name ()
{
  return "backbot";
}


// Whether a host is accessible through IPv6.
unordered_map <string, bool> ipv6_hosts;


void satellite_register_known_hosts ()
{
  vector <string> known_hosts = {
    "api.bitfinex.com",
    "bittrex.com",
    "api.bl3p.eu",
    "www.cryptopia.co.nz",
    "api.kraken.com",
    "poloniex.com",
    "tradesatoshi.com",
    "yobit.net",
  };
  
  // Check if there's an IPv6 interface at all.
  if (ipv6_interfaces.empty ()) return;
  string ipv6_iface = ipv6_interfaces [0];
  
  for (auto host : known_hosts) {

    // Contact the host via a local IPv6 interface.
    string error;
    bool verbose = false;
    string html = http_call (host, error, "", verbose, false, "", {}, false, false, ipv6_iface);
    if (error.empty ()) {
      // It succeeded via IPv6: Record that.
      ipv6_hosts [host] = true;
      // Feedback.
      to_stdout ({"Host", host, "via IPv6"});
    }
  }
}


string satellite_get_interface (string url)
{
  // Remove the scheme http(s):// from the URL.
  size_t pos = url.find ("://");
  if (pos != string::npos) {
    url.erase (0, pos + 3);
  }
  // Extract the host.
  pos = url.find (":");
  if (pos == string::npos) pos = url.find ("/");
  if (pos == string::npos) pos = url.length () + 1;
  string host = url.substr (0, pos);
  
  // Can the host use an IPv6 interface?
  proxy_satellite_update_mutex.lock ();
  bool ipv6 = ipv6_hosts [host];
  proxy_satellite_update_mutex.unlock ();
  
  // If the host is accessible via IPv6, return an IPv6 interface.
  if (ipv6) {
    return get_ipv46_interface (host);
  }

  // The host does not support IPv6, so return nothing.
  return "";
}
