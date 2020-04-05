#include "format.h"

#include <string>

using std::string;

// Done: Complete this helper function
// INPUT: Long int measuring seconds
// OUTPUT: HH:MM:SS
// REMOVE: [[maybe_unused]] once you define the function
string Format::ElapsedTime(long seconds) {
  string hour, min, sec;
  long h, m, s;
  h = seconds / 3600;
  m = (seconds % 3600) / 60;
  s = (seconds % 3600) % 60;
  hour = std::to_string(h);
  min = std::to_string(m);
  sec = std::to_string(s);
  string result = hour + ":" + min + ":" + sec;
  return result;
}