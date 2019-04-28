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


#ifndef INCLUDED_SHARED_H
#define INCLUDED_SHARED_H


#include "libraries.h"
#ifdef HAVE_EXECINFO
#include <execinfo.h>
#endif


extern string program_path;
void initialize_program (int argc, char *argv[]);
void finalize_program ();


int seconds_since_epoch ();
int minutes_since_epoch ();
string timestamp ();
int seconds_within_minute ();
int minutes_within_hour ();
int hours_within_day ();
int day_within_week ();
int get_numerical_year (int seconds);
int get_numerical_month (int seconds);
int get_numerical_month_day (int seconds);
int get_seconds_since_epoch (int year, int month, int day);
long milliseconds_since_epoch ();


string http_call (const string & url,
                  string& error,
                  const string & proxy,
                  bool verbose,
                  bool post, const string & postdata,
                  const vector <pair <string, string> > & headers,
                  bool hurry, bool persist,
                  const string & interface);


string base64_encode (const string &binary);
string base64_decode (const string &ascii);
string url_encode (const string &value);
string hmac_sha512_raw (string key_in, string data_in);
string hmac_sha512_hexits (string key_in, string data_in);
string hmac_sha384_hexits (string key_in, string data_in);
string hmac_sha256_raw (string key_in, string data_in);
string md5_raw (string data);
string sha256_raw (const string& data);
string sha256_hexits (const string& data);
string trim (string s);
string str_replace (string search, string replace, string subject);
string str2lower (string value);
string str2upper (string value);
bool str2bool (string value);
bool is_float (string value);
string float2string (float value, int precision = 10);
string float2visual (float value);
float str2float (const string & s);
int str2int (const string & s);
void to_tty (vector <string> fragments);
void to_stdout (vector <string> fragments, bool lock = true);
class to_output
{
public:
  to_output (string tag);
  ~to_output ();
  void add (vector <string> fragments);
  void to_stderr (bool on);
  void clear ();
  void failure (const string & type, const vector <string> & fragments);
private:
  string mytag;
  string plaintag;
  vector <vector <string> > myfragments;
  bool send2stderr;
  bool stamp;
};
void to_stderr (vector <string> fragments, bool stamp = true);
class to_email
{
public:
  to_email (string subject);
  ~to_email ();
  void add (vector <string> fragments);
private:
  string mysubject;
  string mycontents;
};
void to_failures (string type, string exchange, string coin, float quantity, vector <string> fragments);

string bitcoin_id ();
string litecoin_id ();
string ethereum_id ();
string dogecoin_id ();
string usdollar_id ();
string ripple_id ();
string verge_id ();
string bitcoincash_id ();
string usdtether_id ();
string newzealanddollartoken_id ();
string euro_id ();
string waves_id ();
string russianrubble_id ();
string monero_id ();

string dirname (string path);
string basename (string path);
void file_put_contents (string filename, string contents);
string file_get_contents (string filename);
bool file_or_dir_exists (const string & path);
vector <string> scandir (string folder);
int filesize (const string & filename);
string implode (vector <string>& values, string delimiter);
vector <string> explode (string value, char delimiter);
string increasing_nonce ();

template <typename T>
bool in_array (const T & needle, const vector <T> & haystack)
{
  return (find (haystack.begin(), haystack.end(), needle) != haystack.end());
}

template <typename T>
vector <T> array_intersect (vector <T> a, vector <T> b)
{
  vector <T> result;
  set <T> aset (a.begin(), a.end());
  for (auto & item : b) {
    if (aset.find (item) != aset.end()) {
      result.push_back (item);
    }
  }
  return result;
}

template <typename T>
void array_remove (T needle, vector <T> & haystack)
{
  haystack.erase (std::remove (haystack.begin(), haystack.end(), needle), haystack.end());
}

void api_call_sequencer (atomic <long> & sequencer);
string string_fill (string s, int width, char filler);
bool program_running (char *argv[]);
int satellite_port ();
string satellite_request (const string & host, string request, bool hurry, string& error);
void plot (const vector <pair <float, float> > & data1,
           const vector <pair <float, float> > & data2,
           const string & title1, const string & title2,
           string & picturepath);
void plot2 (const vector <vector <pair <float, float> > > & datasets,
           const vector <string> & titles,
           string & picturepath);
string trade_order_fulfilled ();
bool has_floats_only (const string & line);
string hostname ();
float get_percentage_change (float oldvalue, float newvalue);
bool running_within_minute ();
string tmp_handelaar_path ();


#endif

