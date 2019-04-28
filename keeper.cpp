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
#include "shared.h"
#include "proxy.h"


int main (int argc, char *argv[])
{
  initialize_program (argc, argv);

  // Gather the IP addresses of the satellites and the front bot.
  vector <string> hosts = satellite_hosts ();
  hosts.push_back ("11.11.11.11");

  // Iterate over the IP addresses.
  for (auto host : hosts) {

    // Assemble the call to the satellite.
    vector <string> call = { "serve", "rotate", "logs" };

    // Call the satellite.
    string error;
    bool hurry = false;
    string contents = satellite_request (host, implode (call, " "), hurry, error);

    // Error handler.
    if (!error.empty ()) {
      to_stdout ({"While fetching logbook from", host, "it gives error", error});
    }

    // Process the response.
    else {
      
      // Remove the first three characters, as these contain OK\n.
      contents.erase (0, 3);
      
      // Append the response to the local logbook.
      try {
        ofstream file;
        file.open (tmp_handelaar_path (), ios::binary | ios::app);
        file << contents;
        file.close ();
      } catch (...) {
        // If it failed to write to file, output it to the standard output.
        cout << contents;
      }
      
    }
  }
  
  finalize_program ();
  return EXIT_SUCCESS;
}
