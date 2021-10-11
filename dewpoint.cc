// Copyright 2021 Benjamin Barenblat
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include <getopt.h>
#include <locale.h>
#include <math.h>
#include <stdlib.h>

#include <iostream>
#include <optional>
#include <string_view>

namespace {

constexpr std::string_view kShortUsage =
    "Usage: dewpoint TEMPERATURE HUMIDITY\n";

constexpr std::string_view kHelp =
    R"(Compute the dew point from a given temperature and humidity. Temperatures are
interpreted by default according to the current locale; humidity is interpreted
as a percentage.

Options:
      -c, --celsius, --centigrade
                              use the Celsius temperature scale
      -f, --fahrenheit        use the Fahrenheit temperature scale
      --help                  display this help and exit
)";

constexpr std::string_view kAskForHelp =
    "Try 'dewpoint --help' for more information\n";

enum {
  kHelpLongOption = 128,
};

bool LocaleUsesFahrenheit(std::string_view locale) {
  int underscore = locale.find('_');
  int dot = locale.find('.');
  if (underscore == std::string_view::npos || dot == std::string_view::npos ||
      dot <= underscore) {
    return false;
  }
  std::string_view territory =
      locale.substr(underscore + 1, dot - underscore - 1);
  return territory == "US" /* United States */ ||
         territory == "LR" /* Liberia */ ||
         territory == "FM" /* Micronesia */ ||
         territory == "KY" /* Cayman Islands */ ||
         territory == "MH" /* Marshall Islands */ ||
         territory == "PW" /* Palau */;
}

std::optional<float> ReadFloat(const char* s) {
  char* end;
  float f = strtof(s, &end);
  if (*end != '\0') {
    return std::nullopt;
  }
  return f;
}

// Approximates the dew point of air at the specified temperature and relative
// humidity using the Magnus formula. For a complete derivation, see equation 8
// of Mark G. Lawrence, "The Relationship Between Relative Humidity and the
// Dewpoint Temperature in Moist Air," Bulletin of the American Meteorological
// Society 86(2) (February 2005), 225-234,
// https://doi.org/10.1175/BAMS-86-2-225.
float DewPoint(float celsius, float humidity) {
  constexpr float kA = 17.625;
  constexpr float kB = 243.04;

  float x = log(humidity * 0.01f) + kA * celsius / (kB + celsius);
  return kB * x / (kA - x);
}

float DewPointFahrenheit(float fahrenheit, float humidity) {
  return 9.0f / 5.0f * DewPoint(5.0f / 9.0f * (fahrenheit - 32.0f), humidity) +
         32.0f;
}

}  // namespace

int main(int argc, char* argv[]) {
  setlocale(LC_ALL, "");

  std::string measurement_locale = "C";
  if (const char* s = setlocale(LC_MEASUREMENT, ""); s != nullptr) {
    measurement_locale = s;
  }

  const bool locale_uses_fahrenheit = LocaleUsesFahrenheit(measurement_locale);
  bool use_fahrenheit = locale_uses_fahrenheit;

  static option long_options[] = {
      {"celsius", no_argument, nullptr, 'c'},
      {"centigrade", no_argument, nullptr, 'c'},
      {"fahrenheit", no_argument, nullptr, 'f'},
      {"help", no_argument, nullptr, kHelpLongOption},
      {nullptr, 0, nullptr, 0},
  };
  while (true) {
    int c = getopt_long(argc, argv, "cf", long_options, nullptr);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'c':
        use_fahrenheit = false;
        break;
      case 'f':
        use_fahrenheit = true;
        break;
      case kHelpLongOption:
        std::cout << kShortUsage << kHelp
                  << "\nYour current measurement locale is "
                  << measurement_locale << ", which uses "
                  << (locale_uses_fahrenheit ? "Fahrenheit" : "Celsius")
                  << " by\ndefault.\n";
        return 0;
      case '?':
        std::cerr << kAskForHelp;
        return 1;
      default:
        std::cerr << "Internal error; please report.\n";
        return 1;
    }
  }

  if (optind != argc - 2) {
    std::cerr << kShortUsage << kAskForHelp;
    return 1;
  }

  std::optional<float> temperature = ReadFloat(argv[1]);
  if (!temperature.has_value()) {
    std::cerr << "dewpoint: invalid temperature \"" << argv[1] << "\"\n"
              << kAskForHelp;
    return 1;
  }

  std::optional<float> humidity = ReadFloat(argv[2]);
  if (!humidity.has_value() || *humidity <= 0.0f) {
    std::cerr << "dewpoint: invalid humidity \"" << argv[1] << "\"\n"
              << kAskForHelp;
    return 1;
  }

  float dew_point = use_fahrenheit ? DewPointFahrenheit(*temperature, *humidity)
                                   : DewPoint(*temperature, *humidity);
  std::cout << static_cast<int>(rintf(dew_point)) << '\n';
}
