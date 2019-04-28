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


#include "shared.h"
#include "locks.h"
#include "bittrex.h"
#include "cryptopia.h"
#include "bl3p.h"
#include "sql.h"
#include "models.h"
#include "bitfinex.h"
#include "poloniex.h"
#include "exchanges.h"
#include "proxy.h"
#ifdef HAVE_LIBPROC
#include <libproc.h>
#endif
#include <sqlite3.h>


#ifdef HAVE_EXECINFO
// http://stackoverflow.com/questions/77005/how-to-generate-a-stacktrace-when-my-gcc-c-app-crashes
// To add linker flag -rdynamic is essential.
void backtrace_output (int sig)
{
  (void) sig;
  
  // Information.
  cerr << "Writing backtrace" << endl;
  
  void *array[30];
  size_t size;
  
  // Get void*'s for all entries on the stack
  size = backtrace (array, 30);
  
  // Write entries to stderr.
  backtrace_symbols_fd (array, size, 2);
  
  exit (1);
}
#endif


// Global variable, e.g. /home/joe/cryptobot/lab
string program_path;

// Global variable: Minute in the hour the program was started.
string starting_minute_in_hour;

void initialize_program (int argc, char *argv[])
{
  // Not used.
  (void) argc;
  (void) argv;

#ifdef HAVE_EXECINFO
  // Handler for logging segmentation fault.
  signal (SIGSEGV, backtrace_output);
  // Handler for logging abort.
  signal (SIGABRT, backtrace_output);
#endif

  thread_setup ();
  curl_global_init (CURL_GLOBAL_ALL);

#ifdef HAVE_LIBPROC
  {
    // The following way to get the program path works on macOS.
    pid_t pid = getpid ();
    char pathbuf [2048];
    int ret = proc_pidpath (pid, pathbuf, sizeof (pathbuf));
    if (ret > 0 ) {
      program_path = pathbuf;
    }
  }
#else
  {
    // The /proc/self is a symbolic link to the process-ID subdir of /proc, e.g.
    // /proc/4323 when the pid of the process of this program is 4323.
    // Inside /proc/<pid> there is a symbolic link to the executable that is
    // running as this <pid>.  This symbolic link is called "exe". So if we
    // read the path where the symlink /proc/self/exe points to we have the
    // full path of the executable.
    char fullpath[2048];
    int length = readlink("/proc/self/exe", fullpath, sizeof(fullpath));
    fullpath[length] = '\0';
    program_path = fullpath;
  }
#endif

  // Use SQLite in multi-threaded mode.
  if (!sqlite3_threadsafe ()) {
    cerr << "SQLite is not threadsafe" << endl;
  }
  sqlite3_config (SQLITE_CONFIG_MULTITHREAD);
  
  // The minute within the current hour that this program was started.
  starting_minute_in_hour = to_string (minutes_within_hour ());
  
  // Test live proxies and satellites to use.
  proxy_satellite_register_live_ones ();
}


extern const char * __progname;


void finalize_program ()
{
  // OpenSSL stuff.
  thread_cleanup ();
  
  // Output the hurried calls statistics, as that's important for successful arbitrage trading.
  // It used to output the statistics in the cryptobot.cpp only.
  // But this led to a situation that if another bot failed to contact the satellites,
  // and this again led to IP bans by some exchanges,
  // and it was only noticed after a few days.
  // So now it always outputs those failed hurried calls.
  {
    vector <string> exchanges = exchange_get_names ();
    for (auto & exchange : exchanges) {
      int passes = exchange_get_hurried_call_passes (exchange);
      if (passes) to_stdout ({"Number of succeeded time-sensitive calls to exchange", exchange, "is", to_string (passes)});
      int fails = exchange_get_hurried_call_fails (exchange);
      if (fails) to_stdout ({"Number of failed time-sensitive calls to exchange", exchange, "is", to_string (fails)});
    }
  }
}


int seconds_since_epoch ()
{
  auto now = chrono::system_clock::now();
  auto epoch = now.time_since_epoch();
  auto seconds = chrono::duration_cast<chrono::seconds>(epoch);
  return seconds.count ();
}


int minutes_since_epoch ()
{
  auto now = chrono::system_clock::now();
  auto epoch = now.time_since_epoch();
  auto minutes = chrono::duration_cast<chrono::minutes>(epoch);
  return minutes.count ();
}


string timestamp ()
{
  // Use the same time format as in SQL DATESTAMP.
  // That makes comparison easier as both use the same format.
  auto now = chrono::system_clock::now();
  auto in_time_t = chrono::system_clock::to_time_t(now);
  stringstream ss;
  ss << put_time (localtime(&in_time_t), "%F %T");
  return ss.str();
}


// Gets the second within the minute from the seconds since the Unix epoch.
// It runs from 0 to 59.
int seconds_within_minute ()
{
  time_t tt = seconds_since_epoch ();
  tm utc_tm = *gmtime(&tt);
  int second = utc_tm.tm_sec;
  return second;
}


// Gets the minute within the hour from the seconds since the Unix epoch.
// It runs from 0 to 59.
int minutes_within_hour ()
{
  time_t tt = seconds_since_epoch ();
  tm utc_tm = *gmtime(&tt);
  int minute = utc_tm.tm_min;
  return minute;
}


// Gets the hour within the day from the seconds since the Unix epoch.
// It runs from 0 to 23.
int hours_within_day ()
{
  time_t tt = seconds_since_epoch ();
  tm utc_tm = *gmtime(&tt);
  int hour = utc_tm.tm_hour;
  return hour;
}


// The numerical day of the week: 1 (for Sunday) through 7 (for Saturday)
int day_within_week ()
{
  time_t tt = seconds_since_epoch ();
  tm utc_tm = *gmtime(&tt);
  int day = utc_tm.tm_wday;
  day++;
  return day;
}


// A C++ equivalent for PHP's date ("Y") function.
// A full numeric representation of a year, 4 digits: 2014.
int get_numerical_year (int seconds)
{
  time_t tt = seconds;
  tm utc_tm = *gmtime(&tt);
  // Get years since 1900, and correct to get years since birth of Christ.
  int year = utc_tm.tm_year + 1900;
  return year;
}


// A C++ equivalent for PHP's date ("n") function.
// Numeric representation of a month: 1 through 12.
int get_numerical_month (int seconds)
{
  time_t tt = seconds;
  tm utc_tm = *gmtime(&tt);
  int month = utc_tm.tm_mon + 1;
  return month;
}


// The numerical day of the month from 1 to 31.
int get_numerical_month_day (int seconds)
{
  time_t tt = seconds;
  tm utc_tm = *gmtime(&tt);
  int day = utc_tm.tm_mday;
  return day;
}


// Returns the seconds since the Unix epoch for $year and $month and $day.
int get_seconds_since_epoch (int year, int month, int day)
{
  int seconds = 0;
  bool done = false;
  bool hit = false;
  int iterations = 0;
  do {
    seconds += 86400;
    int myyear = get_numerical_year (seconds);
    int mymonth = get_numerical_month (seconds);
    int myday = get_numerical_month_day (seconds);
    if ((year == myyear) && (month == mymonth)) hit = true;
    done = ((year == myyear) && (month == mymonth) && (day == myday));
    if (hit) if (month != mymonth) done = true;
    iterations++;
    if (iterations > 50000) done = true;
  } while (!done);
  return seconds;
}


long milliseconds_since_epoch ()
{
  auto now = chrono::system_clock::now ();
  auto duration = now.time_since_epoch ();
  auto milliseconds = chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  return milliseconds;
}


size_t curl_write_function (void *ptr, size_t size, size_t count, void *stream)
{
  ((string *) stream)->append ((char *) ptr, 0, size * count);
  return size * count;
}


// Makes a http(s) call.
// $url: The URL to call.
// $error: Where it can store a possible error.
// $proxy: Which proxy to use for the call.
// $verbose: Verbose calls.
// $post: Whether to POST rather than GET.
// $postdata: The data to POST.
// $headers: Extra http headers.
// $hurry: Whether the caller is in a hurry to get the response.
// $interface: The interface or IP address to connect from.
string http_call (const string & url,
                  string& error,
                  const string & proxy,
                  bool verbose,
                  bool post, const string & postdata,
                  const vector <pair <string, string> > & headers,
                  bool hurry, bool persist,
                  const string & interface)
{
  error.clear ();
  string response;
  
  // It used to repeat a failed call up to three times.
  // But this has been switched off now.
  // Repeating a failed call only costs time.
  // With a time-sensitive operation like arbitrage, anything that costs time should be eliminated.

  CURL *curl = curl_easy_init ();
  if (curl) {
  
    // Set URL.
    curl_easy_setopt (curl, CURLOPT_URL, url.c_str());
    
    // Optionally set the proxy to use.
    if (!proxy.empty ()) {
      curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
    }
    
    // Optionally set the interface to use.
    if (!interface.empty ()) {
      curl_easy_setopt(curl, CURLOPT_INTERFACE, interface.c_str());
    }
    
    // Set up the response writer.
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, curl_write_function);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, &response);
    
    // Follow moved location.
    curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    // Optional verbose output
    if (verbose) curl_easy_setopt (curl, CURLOPT_VERBOSE, 1L);

    // Set a timeout on establishing a connection.
    if (hurry) {
      // Arbitrage is time-sensitive, so pick a relatively short timeout.
      curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, 2L);
    } else if (persist) {
      // Some important connections needs to be more persistent.
      curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, 15L);
    } else {
      // Normal non-hurried operation: Take a reasonable connection timeout value.
      curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, 5L);
    }
    // Set a transfer timeout for normal speeds.
    if (hurry) {
      // Arbitrage is time-sensitive, so pick a relatively short timeout.
      // If there's an exchange that does not repond fast enough when the order book is requested,
      // and when the timeout is relatively short,
      // this exchange will automatically be excluded from arbitrage during this iteration.
      // The exchange might have a problem or else be overloaded.
      // So automatically skipping this exchange when it has those problem, that is a good thing.
      curl_easy_setopt (curl, CURLOPT_TIMEOUT, 6L);
    } else if (persist) {
      // Some important connections needs to be more persistent.
      // When placing trade orders on some smaller exchanges this may time out, at times.
      // For such cases it needs a longer timeout to succeed.
      curl_easy_setopt (curl, CURLOPT_TIMEOUT, 90L);
      /*
      to_stdout ({"Set curl timeout to 90 seconds for URL", url});
      to_stdout ({"Set curl timeout to 90 seconds for POST", postdata});
      {
        vector <string> msg = { "Set curl timeout to 90 seconds for headers" };
        for (auto header : headers) {
          msg.push_back (header.first);
          msg.push_back (header.second);
        }
        to_stdout (msg);
      }
       */
    } else {
      // Set a reasonable timeout value for non-hurried operations.
      curl_easy_setopt (curl, CURLOPT_TIMEOUT, 30L);
    }
    // Timing out may use signals. Disable that.
    curl_easy_setopt (curl, CURLOPT_NOSIGNAL, 1L);
    
    // Don't check the secure certificate.
    // There has been a case with Cryptopia being misconfigured,
    // so the certificate could not be verified,
    // which resulted in no trades being possible anymore.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    
    // Optional POST data.
    if (post) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
    }
    
    // Optional extra header.
    struct curl_slist *list = NULL;
    for (auto header : headers) {
      string line = header.first + ": " + header.second;
      list = curl_slist_append (list, line.c_str ());
    }
    if (list) {
      curl_easy_setopt (curl, CURLOPT_HTTPHEADER, list);
    }
    
    // Fetch data.
    CURLcode res = curl_easy_perform (curl);
    if (res == CURLE_OK) {
      error.clear ();
    } else {
      response.clear ();
      error = curl_easy_strerror (res);
    }

    // Clean up.
    if (list) curl_slist_free_all (list);
    curl_easy_cleanup (curl);
  }
  
  // Done.
  return response;
}


static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


string base64_encode (const string &binary)
{
  if (binary.size() > (numeric_limits<string::size_type>::max() / 4u) * 3u) {
    throw length_error("Converting too large a string to base64.");
  }
  
  const size_t binlen = binary.size();
  // Use = signs so the end is properly padded.
  string retval ((((binlen + 2) / 3) * 4), '=');
  size_t outpos = 0;
  int bits_collected = 0;
  unsigned int accumulator = 0;
  const string::const_iterator binend = binary.end();
  
  for (string::const_iterator i = binary.begin(); i != binend; ++i) {
    accumulator = (accumulator << 8) | (*i & 0xffu);
    bits_collected += 8;
    while (bits_collected >= 6) {
      bits_collected -= 6;
      retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
    }
  }
  if (bits_collected > 0) { // Any trailing bits that are missing.
    assert(bits_collected < 6);
    accumulator <<= 6 - bits_collected;
    retval[outpos++] = b64_table[accumulator & 0x3fu];
  }
  assert(outpos >= (retval.size() - 2));
  assert(outpos <= retval.size());
  return retval;
}


static const char reverse_table[128] = {
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
  64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
  64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
};


string base64_decode (const string &ascii)
{
  string retval;
  const string::const_iterator last = ascii.end();
  int bits_collected = 0;
  unsigned int accumulator = 0;
  
  for (string::const_iterator i = ascii.begin(); i != last; ++i) {
    const int c = *i;
    if (isspace(c) || c == '=') {
      // Skip whitespace and padding. Be liberal in what you accept.
      continue;
    }
    if ((c > 127) || (c < 0) || (reverse_table[c] > 63)) {
      throw invalid_argument("This contains characters not legal in a base64 encoded string.");
    }
    accumulator = (accumulator << 6) | reverse_table[c];
    bits_collected += 6;
    if (bits_collected >= 8) {
      bits_collected -= 8;
      retval += static_cast<char>((accumulator >> bits_collected) & 0xffu);
    }
  }
  return retval;
}


string url_encode (const string &value)
{
  ostringstream escaped;
  escaped.fill('0');
  escaped << hex;
  
  for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
    string::value_type c = (*i);
    
    // Keep alphanumeric and other accepted characters intact
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << c;
      continue;
    }
    
    // Any other characters are percent-encoded
    escaped << uppercase;
    escaped << '%' << setw(2) << int((unsigned char) c);
    escaped << nouppercase;
  }
  
  return escaped.str();
}


string hmac_sha512_raw (string key_in, string data_in)
{
  // The key to hash.
  char key[1024];
  memset (key, 0, sizeof (key));
  strcpy (key, key_in.c_str());
  // The data to hash using HMAC.
  char data [1024];
  memset (data, 0, sizeof (data));
  for (size_t i = 0; i < data_in.size(); i++) data [i] = data_in [i];
  // Using output buffer for thread safety.
  unsigned char md[EVP_MAX_MD_SIZE];
  // Using sha512 hash engine.
  // Key or data may end with a null character, so strlen() cannot be used.
  unsigned char* digest = HMAC(EVP_sha512(), key, key_in.size(), (unsigned char*)data, data_in.size(), md, NULL);
  // SHA512 produces 512 bits that is 64 bytes of raw digest length.
  string raw;
  for (int i = 0; i < 64; i++) raw += digest [i];
  return raw;
}


string hmac_sha512_hexits (string key_in, string data_in)
{
  // The key to hash.
  char key[1024];
  memset (key, 0, sizeof (key));
  strcpy (key, key_in.c_str());
  // The data to hash using HMAC.
  char data [1024];
  memset (data, 0, sizeof (data));
  for (size_t i = 0; i < data_in.size(); i++) data [i] = data_in [i];
  // Using output buffer for thread safety.
  unsigned char md[EVP_MAX_MD_SIZE];
  // Using sha512 hash engine.
  // Key or data may end with a null character, so strlen() cannot be used.
  unsigned char* digest = HMAC(EVP_sha512(), key, key_in.size(), (unsigned char*)data, data_in.size(), md, NULL);
  // SHA512 produces 512 bits that is 64 bytes which gives 128 bytes of hash length.
  char hexits[128];
  memset (hexits, 0, sizeof (hexits));
  for (int i = 0; i < 64; i++) sprintf (&hexits[i*2], "%02x", (unsigned int)digest[i]);
  // Resulting hexits.
  return string (hexits);
}


string hmac_sha384_hexits (string key_in, string data_in)
{
  // The key to hash.
  char key[1024];
  memset (key, 0, sizeof (key));
  strcpy (key, key_in.c_str());
  // The data to hash using HMAC.
  char data [1024];
  memset (data, 0, sizeof (data));
  for (size_t i = 0; i < data_in.size(); i++) data [i] = data_in [i];
  // Using output buffer for thread safety.
  unsigned char md[EVP_MAX_MD_SIZE];
  // Using sha384 hash engine.
  // Key or data may end with a null character, so strlen() cannot be used.
  unsigned char* digest = HMAC(EVP_sha384(), key, key_in.size(), (unsigned char*)data, data_in.size(), md, NULL);
  // SHA384 produces 384 bits that is 48 bytes which gives 96 bytes of hash length.
  char hexits[96];
  memset (hexits, 0, sizeof (hexits));
  for (int i = 0; i < 48; i++) sprintf (&hexits[i*2], "%02x", (unsigned int)digest[i]);
  // Resulting hexits.
  return string (hexits);
}


string hmac_sha256_raw (string key_in, string data_in)
{
  // The key to hash.
  char key[1024];
  memset (key, 0, sizeof (key));
  strcpy (key, key_in.c_str());
  // The data to hash using HMAC.
  char data [1024];
  memset (data, 0, sizeof (data));
  for (size_t i = 0; i < data_in.size(); i++) data [i] = data_in [i];
  // Using output buffer for thread safety.
  unsigned char md[EVP_MAX_MD_SIZE];
  // Using sha256 hash engine.
  // Key or data may end with a null character, so strlen() cannot be used.
  unsigned char* digest = HMAC(EVP_sha256(), key, key_in.size(), (unsigned char*)data, data_in.size(), md, NULL);
  // SHA512 produces 256 bits that is 32 bytes of raw digest length.
  string raw;
  for (int i = 0; i < 32; i++) raw += digest [i];
  return raw;
}


string md5_raw (string data)
{
  unsigned char m [MD5_DIGEST_LENGTH];
  char* buffer = (char *) data.c_str ();
  unsigned long size = data.length ();
  MD5 ((unsigned char*) buffer, size, m);
  string m2 (m, m + sizeof m / sizeof m[0] );
  return m2;
}


string sha256_raw (const string & data)
{
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, data.c_str(), data.size());
  SHA256_Final(hash, &sha256);
  string sha;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sha += hash [i];
  }
  return sha;
}


// printf "compute sha256" | openssl sha256
string sha256_hexits (const string& data)
{
  unsigned char outputbuffer [SHA256_DIGEST_LENGTH];
  SHA256((const unsigned char *)data.c_str(), data.length(), outputbuffer);
  char hexits [SHA256_DIGEST_LENGTH * 2];
  memset (hexits, 0, sizeof (hexits));
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sprintf (&hexits[i*2], "%02x", (unsigned int)outputbuffer[i]);
  }
  return string (hexits);
}


#define cryptopia "cryptopia"
#define bl3p "bl3p"


string trim (string s)
{
  if (s.length () == 0)
    return s;
  // Strip spaces, tabs, new lines, carriage returns.
  size_t beg = s.find_first_not_of(" \t\n\r");
  size_t end = s.find_last_not_of(" \t\n\r");
  // No non-spaces
  if (beg == string::npos)
    return "";
  return string (s, beg, end - beg + 1);
}


string str_replace (string search, string replace, string subject)
{
  size_t offposition = subject.find (search);
  while (offposition != string::npos) {
    subject.replace (offposition, search.length (), replace);
    offposition = subject.find (search, offposition + replace.length ());
  }
  return subject;
}


string str2lower (string value)
{
  transform (value.begin(), value.end(), value.begin(), ::tolower);
  return value;
}


string str2upper (string value)
{
  transform (value.begin(), value.end(), value.begin(), ::toupper);
  return value;
}


bool str2bool (string value)
{
  if (value == "true") return true;
  if (value == "TRUE") return true;
  bool b;
  istringstream (value) >> b;
  return b;
}


bool is_float (string value)
{
  istringstream iss (value);
  float f;
  // The flag noskipws considers leading whitespace invalid.
  iss >> noskipws >> f;
  // Check the entire string was consumed and if either failbit or badbit is set
  return iss.eof() && !iss.fail();
}


// When using "to_string (float)" to convert a float to a string,
// the resulting string may be rounded up.
// For example "to_string (0.00000059)" would yield "0.000001".
// When placing limit order for low-priced coins, this conversion is far too inaccurate.
// It would lead to accumulating open orders at the exchange,
// or it would lead to losses when buying coins.
// So the function below is much more accurate.
// It should be used in all critical float-to-string conversion,
// in place of "to_string ()".
string float2string (float value, int precision)
{
  stringstream ss;
  ss << fixed << setprecision (precision);
  ss << value;
  return ss.str ();
}


string float2visual (float value)
{
  int precision = 8;
  if (value >= 0.00001) precision = 7;
  if (value >= 0.0001) precision = 6;
  if (value >= 0.001) precision = 5;
  if (value >= 0.01) precision = 4;
  if (value >= 0.1) precision = 3;
  if (value >= 1) precision = 2;
  if (value >= 10) precision = 1;
  if (value >= 100) precision = 0;
  stringstream ss;
  ss << fixed << setprecision (precision);
  ss << value;
  return ss.str ();
}


float str2float (const string & s)
{
  float f = 0;
  istringstream r (s);
  r >> f;
  return f;
}


int str2int (const string & s)
{
  int i = 0;
  istringstream r (s);
  r >> i;
  return i;
}


// If the binary runs in a terminal, it sends the fragments to the terminal.
// Else it does nothing.
void to_tty (vector <string> fragments)
{
  // No messages means: No output.
  if (fragments.empty ()) return;
  // Send to the terminal.
  if (isatty (fileno (stdout))) {
    for (auto fragment : fragments) {
      cout << fragment << " ";
    }
    cout << endl;
  }
}


mutex standard_out_mutex;


void to_stdout (vector <string> fragments, bool lock)
{
  // No messages means: No output.
  if (fragments.empty ()) return;
  // Whether it runs in a terminal: This affects the behaviour.
  bool tty = isatty (fileno (stdout));
  // Buffer to write.
  string buffer;
  // In a script, add the time stamp and host name.
  // Plus the minute this program started.
  if (!tty) {
    buffer.append (timestamp ());
    buffer.append (" ");
    buffer.append (hostname ());
    buffer.append (" ");
    buffer.append (starting_minute_in_hour);
  }
  // Output the fragments.
  for (auto fragment : fragments) {
    if (!buffer.empty ()) buffer.append (" ");
    buffer.append (fragment);
  }
  // Always new line at the end.
  buffer.append ("\n");
  // Lock to be sure there's clear output in case simultaneous processes output at the same time.
  if (lock) standard_out_mutex.lock ();
  try {
    // Append it to the logfile.
    ofstream file;
    file.open (tmp_handelaar_path (), ios::binary | ios::app);
    file << buffer;
    file.close ();
    // In case of running in terminal, also output it visibly.
    if (tty) cout << buffer;
  } catch (...) {
    // If it failed to write to file, output it to the standard output.
    cout << buffer;
  }
  // Unlock the output writer.
  if (lock) standard_out_mutex.unlock ();
}


// This object sends text to stdout.
// It bundles all output into one uninterrupted sequence of lines.
// This is helpful for clarity as there may be multiple processes logging their output simultaneously.
to_output::to_output (string tag)
{
  if (!tag.empty ()) {
    mytag = timestamp () + " " + hostname () + " " + starting_minute_in_hour + " " + tag;
    plaintag = tag;
  }
  send2stderr = false;
  stamp = true;
}


to_output::~to_output ()
{
  // No messages means: No output.
  if (myfragments.empty ()) return;

  // Lock the output so all lines stay together nicely in the logfile.
  standard_out_mutex.lock ();
  
  // Opener and closer.
  if (!mytag.empty ()) {
    myfragments.insert (myfragments.begin (), {mytag, "begin"});
    myfragments.push_back ({timestamp (), hostname (), starting_minute_in_hour, plaintag, "end"});
  }

  // Whether it runs in a terminal: This affects the behaviour.
  bool tty = isatty (fileno (stdout));

  // Iterate over the fragments.
  for (auto & fragment : myfragments) {
    
    // Buffer to write.
    string buffer;

    // Output the fragments.
    for (auto bit : fragment) {
      if (!buffer.empty ()) buffer.append (" ");
      buffer.append (bit);
    }
    // Always new line at the end.
    buffer.append ("\n");

    // In a script, leave the time stamp added.
    // If this will get emailed via the crontab, there's no need for a date to be added.
    // Because the email has a date of its own.
    // Still, there's two reasons for adding the date:
    // * In different time zones, to match this error event with the standard output, using the date to match.
    // * If reading through Gmail, gmail online collapses repeating text.
    //   By adding the date, it won't collapse the text, since the texts are different now.
    // The timestamp and hostname and starting minute can get disabled if desired.
    if (tty || !stamp) buffer.erase (0, 20 + hostname ().size () + 1 + starting_minute_in_hour.size() + 1);

    // Conditions for sending it to standard error.
    if (send2stderr) {
      // Both in a script, and in a terminal, send it to standard error.
      // So it will be emailed right away since errors may need immediate action.
      cerr << buffer;
    }

    // Any of the following conditions sends it to the logfile.
    // 1. Within scripts.
    // 2. When not sending to standard error.
    if (!tty || !send2stderr) {
      try {
        // Append it to the logfile.
        ofstream file;
        file.open (tmp_handelaar_path (), ios::binary | ios::app);
        file << buffer;
        file.close ();
        // In case of running in terminal, also output it visibly.
        if (tty) cout << buffer;
      } catch (...) {
        // If it failed to write to file, output it to the standard output.
        cout << buffer;
      }
    }
  }
  
  // Unlock the output.
  standard_out_mutex.unlock ();
}


mutex output_add_mutex;


void to_output::add (vector <string> fragments)
{
  // No messages means: Do nothing.
  if (fragments.empty ()) return;
  // Add the current timestamp.
  // The purposes for adding it here, and not right at the end, while doing the output, is this:
  // Doing it now gives a more realistic idea of how long the individual step of bundled output took.
  // Add the hostname too, so the logbook indicates which bot this entry is from.
  fragments.insert (fragments.begin(), timestamp () + " " + hostname () + " " + starting_minute_in_hour);
  // Store the messages.
  // It used to lock a private mutex for thread safety.
  // But this led to undefined crashes at times.
  // Rather than looking into why, it now just skips that step.
  // But that led to terrible crashes again.
  // So now it uses a global mutex instead.
  output_add_mutex.lock ();
  myfragments.push_back (fragments);
  output_add_mutex.unlock ();
}


void to_output::to_stderr (bool on)
{
  send2stderr = on;
}


void to_output::clear ()
{
  myfragments.clear ();
}


void to_output::failure (const string & type, const vector <string> & fragments)
{
  // No messages means: No output.
  if (fragments.empty ()) return;
  // Add it to the object.
  add (fragments);
  // Create the message to store in the failures database.
  string message;
  for (auto fragment : fragments) {
    if (!message.empty ()) message.append (" ");
    message.append (fragment);
  }
  // Store the message.
  SQL sql (front_bot_ip_address ());
  failures_store (sql, type, "", "", 0, message);
}


mutex standard_err_mutex;


// This outputs to standard error.
// Only very important errors to be sent this way.
// Examples are errors that require immediate human monitoring or action.
// Like a security breach.
void to_stderr (vector <string> fragments, bool stamp)
{
  // No messages means: No output.
  if (fragments.empty ()) return;
  // Detect whether running in a terminal.
  bool tty = isatty (fileno (stdout));
  // Within scripts: Send it to standard out also.
  if (!tty) to_stdout (fragments);
  // Buffer to write.
  string buffer;
  // In a script, add the time stamp and the hostname.
  // Strictly speaking, since this will get emailed in the current setup, there's no need for a date added.
  // Because the email's date is also find.
  // Still, there's two reasons for adding the date:
  // * In different time zones, to match this error event with the standard output, using the date to match.
  // * If reading through Gmail, gmail online collapses repeating text.
  //   By adding the date, it won't collapse the text, since the texts are different now.
  // The timestamp and hostname can get disabled as needed.
  if (!tty) {
    if (stamp) buffer.append (timestamp () + " " + hostname () + " " + starting_minute_in_hour);
  }
  // Output the fragments.
  for (auto fragment : fragments) {
    if (!buffer.empty ()) buffer.append (" ");
    buffer.append (fragment);
  }
  // Lock to be sure there's clear output in case simultaneous processes output at the same time.
  standard_err_mutex.lock ();
  // Both in a script, and in a terminal, send it to standard error.
  // So it will be emailed right away since errors may need immediate action.
  cerr << buffer << endl;
  // Unlock the output writer.
  standard_err_mutex.unlock ();
}


to_email::to_email (string subject)
{
  mysubject = subject;
  mycontents.append (timestamp () + " " + hostname () + " " + starting_minute_in_hour + "\n\n");
}


to_email::~to_email ()
{
  string filename = "/tmp/" + increasing_nonce ();
  file_put_contents (filename, mycontents);
  string command = "cat " + filename + " | mail -s '" + mysubject + "' `whoami`";
  int result = system (command.c_str());
  (void) result;
}


void to_email::add (vector <string> fragments)
{
  // Send it to standard out also.
  to_stdout (fragments);
  // Add the fragment to the object's buffer for mailing later.
  string line;
  for (auto fragment : fragments) {
    if (!line.empty ()) line.append (" ");
    line.append (fragment);
  }
  mycontents.append (line);
  mycontents.append ("\n");
}


void to_failures (string type, string exchange, string coin, float quantity, vector <string> fragments)
{
  // No messages means: No output.
  if (fragments.empty ()) return;
  // Detect whether running in a terminal.
  bool tty = isatty (fileno (stdout));
  // Within scripts: Send it to standard out also.
  if (!tty) to_stdout (fragments);
  // Create the message to store in the failures database.
  string message;
  for (auto fragment : fragments) {
    if (!message.empty ()) message.append (" ");
    message.append (fragment);
  }
  // Store the message.
  SQL sql (front_bot_ip_address ());
  failures_store (sql, type, exchange, coin, quantity, message);
}


string bitcoin_id ()
{
  return "bitcoin";
}


string litecoin_id ()
{
  return "litecoin";
}


string ethereum_id ()
{
  return "ethereum";
}


string dogecoin_id ()
{
  return "dogecoin";
}


string usdollar_id ()
{
  return "usdollar";
}


string ripple_id ()
{
  return "ripple";
}


string verge_id ()
{
  return "verge";
}


string bitcoincash_id ()
{
  return "bitcoincash";
}


string usdtether_id ()
{
  return "usdtether";
}


string newzealanddollartoken_id ()
{
  return "newzealanddollartoken";
}


string euro_id ()
{
  return "euro";
}


string waves_id ()
{
  return "waves";
}


string russianrubble_id ()
{
  return "russianrubble";
}


string monero_id ()
{
  return "monero";
}


// C++ replacement for the dirname function, see http://linux.die.net/man/3/dirname.
// The BSD dirname is not thread-safe, see the implementation notes on $ man 3 dirname.
string dirname (string path)
{
  if (!path.empty ()) {
    if (path.find_last_of ("/") == path.length () - 1) {
      // Remove trailing slash.
      path = path.substr (0, path.length () - 1);
    }
    size_t pos = path.find_last_of ("/");
    if (pos != string::npos) path = path.substr (0, pos);
    else path = "";
  }
  if (path.empty ()) path = ".";
  return path;
}


// C++ replacement for the basename function, see http://linux.die.net/man/3/basename.
// The BSD basename is not thread-safe, see the warnings in $ man 3 basename.
string basename (string path)
{
  if (!path.empty ()) {
    if (path.find_last_of ("/") == path.length () - 1) {
      // Remove trailing slash.
      path = path.substr (0, path.length () - 1);
    }
    size_t pos = path.find_last_of ("/");
    if (pos != string::npos) path = path.substr (pos + 1);
  }
  return path;
}


void file_put_contents (string filename, string contents)
{
  try {
    ofstream file;
    file.open(filename, ios::binary | ios::trunc);
    file << contents;
    file.close ();
  } catch (...) {
  }
}


string file_get_contents (string filename)
{
  try {
    ifstream ifs(filename.c_str(), ios::in | ios::binary | ios::ate);
    streamoff filesize = ifs.tellg();
    if (filesize == 0) return "";
    ifs.seekg(0, ios::beg);
    vector <char> bytes((int)filesize);
    ifs.read(&bytes[0], (int)filesize);
    return string(&bytes[0], (int)filesize);
  }
  catch (...) {
    return "";
  }
}


bool file_or_dir_exists (const string & path)
{
  struct stat buffer;
  return (stat (path.c_str(), &buffer) == 0);
}


// Scans the directory for files it contains.
vector <string> scandir (string folder)
{
  // Storage.
  vector <string> files;
  // Open folder and iterate over it.
  DIR * dir = opendir (folder.c_str());
  if (dir) {
    struct dirent * direntry;
    while ((direntry = readdir (dir)) != NULL) {
      string name = direntry->d_name;
      // Exclude short-hand directory names.
      if (name == ".") continue;
      if (name == "..") continue;
      // Exclude developer temporal files.
      if (name == ".deps") continue;
      if (name == ".dirstamp") continue;
      // Exclude macOS files.
      if (name == ".DS_Store") continue;
      // Store the name.
      files.push_back (name);
    }
    closedir (dir);
  }
  // Sort the entries.
  sort (files.begin(), files.end());
  // Remove . and ..
  array_remove (string("."), files);
  array_remove (string(".."), files);
  // Done.
  return files;
}


// Returns the size of $filename on disk in bytes.
int filesize (const string & filename)
{
  struct stat buf;
  int rc = stat (filename.c_str (), &buf);
  return rc == 0 ? buf.st_size : 0;
}


string implode (vector <string>& values, string delimiter)
{
  string full;
  for (vector<string>::iterator it = values.begin (); it != values.end (); ++it)
  {
    full += (*it);
    if (it != values.end ()-1) full += delimiter;
  }
  return full;
}


vector <string> explode (string value, char delimiter)
{
  vector <string> result;
  istringstream iss (value);
  for (string token; getline (iss, token, delimiter); )
  {
    result.push_back (move (token));
  }
  return result;
}


// Returns a strictly increasing nonce.
string increasing_nonce ()
{
  auto now = chrono::system_clock::now ();
  auto duration = now.time_since_epoch ();
  auto microseconds = chrono::duration_cast<std::chrono::microseconds>(duration).count();
  string usecs = to_string (microseconds % 1000000);
  usecs = string (8 - usecs.length(), '0') + usecs;
  string secs = to_string (seconds_since_epoch ());
  return secs + usecs;
}


// This functions provides for delays so API calls that use increasing nonces get called in sequence.
// Before this it used another method to ensure increasing nonces:
// It used a mutex lock around the whole API call.
// This had a couple of problem at times:
// When the exchange was overloaded, or otherwise slow to respond,
// this could causes all subsequence calls to be delayed unnessarily.
// And since arbitrage trading is time-sensitive,
// this at times led to losses due to this type of trading, rather than gains.
// The limit trade order was just placed way too late, and the market prices had changes in the mean time.
// This new method just fires the calls off in sequence, with a slight delays between them.
// It does this indepent from how fast or slow the API call returns.
void api_call_sequencer (atomic <long> & sequencer)
{
  // The current number of milliseconds since the Unix epoch.
  long milliseconds = milliseconds_since_epoch ();
  // First call since app start:
  if (sequencer == 0) {
    // Initialize the sequencer.
    sequencer = milliseconds;
    // No need to delay.
    return;
  }
  // The last call is more than 300 milliseconds ago:
  if (milliseconds > sequencer + 300) {
    // Update the sequencer to the current time.
    sequencer = milliseconds;
    // No need to delay.
    return;
  }
  // The last call is very recent: Update the sequencer.
  sequencer += 300;
  // Delay for some milliseconds.
  int delay = sequencer - milliseconds;
  if (delay > 0) {
    this_thread::sleep_for (chrono::milliseconds (delay));
  }
  // Done.
}


string string_fill (string s, int width, char filler)
{
  stringstream ss;
  ss << setw (width) << setfill (filler) << s;
  return ss.str ();
}


// Lists the running processes.
// Here is some sample output:
// Running the bot separately:
// macOS: 36591 s001  S+     0:00.02 ./rates
// Linux:  2924 pts/0    S+     0:00 ./rates
// Linux:  3120 pts/0    S+     0:00 cryptobot/rates
// Running the bot from the crontab on Linux:
// 30989 ?        Ss     0:00 /bin/sh -c cryptobot/rates
// 30991 ?        Sl     0:08 cryptobot/rates
vector <string> get_active_processes ()
{
  string pipe = "/tmp/" + increasing_nonce ();
  string command = "ps ax > " + pipe + " 2>&1";
  int result = system (command.c_str());
  (void) result;
  string output = file_get_contents (pipe);
  unlink (pipe.c_str());
  vector <string> processes = explode (output, '\n');
  return processes;
}


// Returns the pid(s) of the program passed
vector <string> pidof (const string & process)
{
  string pipe = "/tmp/" + increasing_nonce ();
  string command = "pidof " + process + " > " + pipe + " 2>&1";
  int result = system (command.c_str());
  (void) result;
  string output = file_get_contents (pipe);
  unlink (pipe.c_str());
  output = trim (output);
  vector <string> pids = explode (output, ' ');
  return pids;
}


bool program_running (char *argv[])
{
  // Flag.
  bool is_running = false;

  // The program name is argument 0 passed to the binary, by the shell.
  string program = basename (argv[0]);
  
  vector <string> pids = pidof (program);

  // Normally there's one program process.
  // So if there's more than one, the previous one still runs.
  if (pids.size() > 1) is_running = true;

  // If there's too many of these processes, it indicates something is stuck somewhere.
  if (pids.size() >= 10) {
    // Error message to inform the operator.
    to_stderr ({"There is", to_string (pids.size()), "processes of", program, "running, so killing them all"});
    // Kill all of them.
    string command = "killall " + program;
    int result = system (command.c_str());
    (void) result;
  }
  
  // Done.
  return is_running;
}


int satellite_port ()
{
  return 1234;
}


// Sends a request to the satellite and reads the response.
string satellite_request (const string & host, string request, bool hurry, string& error)
{
  bool connection_healthy = true;
  
  // Resolve the host.
  struct addrinfo hints;
  struct addrinfo * address_results = nullptr;
  bool address_info_resolved = false;
  memset (&hints, 0, sizeof (struct addrinfo));
  // Allow IPv4 and IPv6.
  hints.ai_family = AF_UNSPEC;
  // TCP/IP socket.
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  // Select protocol that matches with the socket type.
  hints.ai_protocol = 0;
  // The 'service' is actually the port number.
  string service = to_string (satellite_port ());
  // Get a list of address structures. There can be several of them.
  int res = getaddrinfo (host.c_str(), service.c_str (), &hints, &address_results);
  if (res != 0) {
    error = host + ": ";
    error.append (gai_strerror (res));
    connection_healthy = false;
  } else {
    address_info_resolved = true;
  }
  
  // Connection setup.
  int sock = 0;
  // Iterate over the list of address structures.
  vector <string> errors;
  struct addrinfo * rp = NULL;
  if (connection_healthy) {
    for (rp = address_results; rp != NULL; rp = rp->ai_next) {
      // Try to get a socket for this address structure.
      sock = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      // If it fails, try the next one.
      if (sock < 0) {
        string err = "Creating socket: ";
        err.append (strerror (errno));
        errors.push_back (err);
        continue;
      }
      // Try to connect.
      int res = connect (sock, rp->ai_addr, rp->ai_addrlen);
      // Test and record error.
      if (res < 0) {
        string err = host + ": ";
        err.append (strerror (errno));
        errors.push_back (err);
      }
      // If success: Done.
      if (res != -1) break;
      // Failure: Socket should be closed.
      if (sock) {
        close (sock);
      }
      sock = 0;
    }
  }
  
  // Check whether no address succeeded.
  if (connection_healthy) {
    if (rp == NULL) {
      error = implode (errors, " | ");
      connection_healthy = false;
    }
  }
  
  // No longer needed: Only to be freed when the address was resolved.
  if (address_info_resolved) freeaddrinfo (address_results);
  
  // Set a timeout on the network connection.
  if (connection_healthy) {
    // Normal or hurried timeout.
    int timeout = 30;
    if (hurry) timeout = 6;
    // Linux: Timeout value is a struct timeval, address passed to setsockopt() is const void *
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
    setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
  }

  // Send the request.
  if (connection_healthy) {
    request.append ("\n");
    if (send (sock, request.c_str(), request.length(), 0) != (int) request.length ()) {
      error = "Sending request: ";
      error.append (strerror (errno));
      connection_healthy = false;
    }
  }
  
  // Read the response.
  string response;
  if (connection_healthy) {
    bool reading = true;
    char cur;
    do {
      int ret = read(sock, &cur, 1);
      if (ret > 0) {
        response += cur;
      } else if (ret < 0) {
        error = "Receiving: ";
        error.append (strerror (errno));
        connection_healthy = false;
      } else {
        // Probably EOF.
        reading = false;
      }
    } while (reading && connection_healthy);
  }
  
  // Only close the socket if it was open.
  if (sock > 0) {
    close (sock);
  }
  
  // Done.
  if (!connection_healthy) response.clear ();
  return response;
}


void plot (const vector <pair <float, float> > & data1,
           const vector <pair <float, float> > & data2,
           const string & title1, const string & title2,
           string & picturepath)
{
  // Storage for the contents to plot.
  vector <string> contents;

  // Create the file with the first set of data to plot.
  for (auto element : data1) {
    contents.push_back (float2string (element.first) + " " + float2string (element.second));
  }
  string datafile1 = "/tmp/" + title1 + "data1.txt";
  file_put_contents (datafile1, implode (contents, "\n"));

  // Create the file with the first set of data to plot.
  contents.clear ();
  for (auto element : data2) {
    contents.push_back (float2string (element.first) + " " + float2string (element.second));
  }
  string datafile2 = "/tmp/" + title2 + "data2.txt";
  file_put_contents (datafile2, implode (contents, "\n"));

  // The important part of the filename, given as 'title' here.
  // It used variable $title1 as this important part.
  // But this led to problems.
  // The issue was in that title when drawing a double graph.
  // There could be another graph using that same $title1 again, altough a different $title2.
  // So the title was not unique.
  // The solution is this:
  // It should use both $title1 and $title2 in the filenames.
  string title (title1);
  if (!title2.empty ()) {
    title.append ("-");
    title.append (title2);
  }

  // The output location for the picture will be in ../cryptodata/graphs.
  picturepath = dirname (dirname (program_path));
  picturepath.append ("/cryptodata/graphs");
  picturepath.append ("/" + title + ".png");
  
  // Create the file with the plot commands.
  contents.clear ();
  contents.push_back ("set terminal pngcairo size 800,600 enhanced font 'Verdana,10'");
  contents.push_back ("set output '" + picturepath + "'");
  string plotcommand = "plot '" + datafile1 + "' title '" + title1 + "' with linespoints";
  if (!data2.empty ()) plotcommand.append (", '" + datafile2 + "' title '" + title2 + "' with linespoints");
  contents.push_back (plotcommand);
  string plotfile = "/tmp/" + title + "plot.txt";
  file_put_contents (plotfile, implode (contents, "\n"));
  
  // Run the plotter.
  string command = "gnuplot '" + plotfile + "' > /dev/null 2>&1";
  int status = system (command.c_str());
  (void) status;
  
  // Clean the intermediary files out.
  unlink (datafile1.c_str());
  unlink (datafile2.c_str());
  unlink (plotfile.c_str());
}


void plot2 (const vector <vector <pair <float, float> > > & datasets,
            const vector <string> & titles,
            string & picturepath)
{
  // Content size checks.
  if (datasets.empty ()) return;
  if (datasets.size () != titles.size()) return;

  // The output location for the picture will be in ../cryptodata/graphs.
  // The path to the graph depends on all available titles.
  picturepath = dirname (dirname (program_path));
  picturepath.append ("/cryptodata/graphs");
  vector <string> titlescopy = titles;
  string imploded_titles = implode (titlescopy, "-");
  picturepath.append ("/" + imploded_titles + ".png");

  // The paths to the data files.
  vector <string> datafiles;

  // Iterate over the data sets and the titles.
  for (size_t i = 0; i < datasets.size(); i++) {

    // The current data set and associated title.
    vector <pair <float, float> > data1 = datasets [i];
    string title = titles [i];

    // Storage for the contents to plot.
    vector <string> datacontents;

    // Create the file with the set of data to plot.
    for (auto element : data1) {
      datacontents.push_back (float2string (element.first) + " " + float2string (element.second));
    }
    string datafile1 = "/tmp/" + title + "data1.txt";
    file_put_contents (datafile1, implode (datacontents, "\n"));
    datafiles.push_back (datafile1);
  }
  
  // Create the file with the plot commands.
  vector <string> commands;
  commands.clear ();
  commands.push_back ("set terminal pngcairo size 800,600 enhanced font 'Verdana,10'");
  commands.push_back ("set output '" + picturepath + "'");
  string plotcommand;
  for (size_t i = 0; i < datasets.size(); i++) {
    if (i == 0) {
      plotcommand = "plot '" + datafiles[i] + "' title '" + titles[i] + "' with linespoints";
    } else {
      plotcommand.append (", '" + datafiles[i] + "' title '" + titles[i] + "' with linespoints");
    }
  }
  commands.push_back (plotcommand);
  string plotfile = "/tmp/" + imploded_titles + "plot.txt";
  file_put_contents (plotfile, implode (commands, "\n"));
  
  // Run the plotter.
  string command = "gnuplot '" + plotfile + "' > /dev/null 2>&1";
  int status = system (command.c_str());
  (void) status;
  
  // Clean the intermediary files out.
  for (auto datafile : datafiles) unlink (datafile.c_str());
  unlink (plotfile.c_str());
}


// When a limit trade order gets fulfilled, it is to use the string provided below.
string trade_order_fulfilled ()
{
  return "tradefulfilled";
}


bool has_floats_only (const string & line)
{
  size_t pos = line.find_first_not_of ("0123456789. ");
  return pos == string::npos;
}


// Get the name of the current host.
string hostname ()
{
  char hostname[255];
  gethostname(hostname, 255);
  return hostname;
}


// See https://www.mathsisfun.com/data/percentage-difference-vs-error.html
float get_percentage_change (float oldvalue, float newvalue)
{
  return (newvalue - oldvalue) / abs (oldvalue) * 100;
}


// Whether the bot is still running within the current minute.
bool running_within_minute ()
{
  // At the start of the binary, store the current minute within the hour once.
  static int starting_minute = minutes_within_hour ();
  // Check that the current minute is still the same as the one it started with.
  // It used not to check on the minute.
  // It times this caused two instances of the bot to run in parallel.
  // This led to inaccurate funds assessment and simultaneous sales, one of which could fail.
  if (minutes_within_hour () != starting_minute) return false;
  // Check that it is not too near the end of the minute.
  return (seconds_within_minute () <= 55);
}


string tmp_handelaar_path ()
{
  return "/tmp/handelaar.log";
}
