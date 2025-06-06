/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
                Copyright (c) 2003-2021, Cauldron Development LLC
                               All rights reserved.

         The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
       as published by the Free Software Foundation, either version 2.1 of
               the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
          but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                 Lesser General Public License for more details.

         You should have received a copy of the GNU Lesser General Public
                 License along with the C! library.  If not, see
                         <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
           You may request written permission by emailing the authors.

                  For information regarding this software email:
                                 Joseph Coffland
                          joseph@cauldrondevelopment.com

\******************************************************************************/

#pragma once

#include "Exception.h"

#include <string>
#include <vector>
#include <istream>
#include <sstream>
#include <functional>
#include <cstdint>
#include <cstdarg>


namespace cb {
  /// Used for convenient conversion of basic data types to and from std::string
  class String : public std::string {
  public:
    static const std::string DEFAULT_DELIMS;
    static const std::string DEFAULT_LINE_DELIMS;
    static const std::string LETTERS_LOWER_CASE;
    static const std::string LETTERS_UPPER_CASE;
    static const std::string LETTERS;
    static const std::string NUMBERS;
    static const std::string ALPHA_NUMERIC;

    /// Copy at most @param n bytes from @param s.
    String(const char *s, size_type n);

    // Pass through constructors
    /// See std::string
    String(const char *s) : std::string(s) {}

    /// See std::string
    String(const std::string &s) : std::string(s) {}

    /// See std::string
    String(const std::string &s, size_type pos, size_type n = npos) :
      std::string(s, pos, n) {}

    /// See std::string
    String(size_type n, char c) : std::string(n, c) {}

    // Conversion constructors
    explicit String(double x, int precision);

#define CBANG_STRING_PT(NAME, TYPE, DESC) explicit String(TYPE x);
#include "StringParseTypes.def"

    // Formatting
    [[gnu::format(printf, 1, 2)]]
    static std::string printf(const char *format, ...);
    [[gnu::format(printf, 1, 0)]]
    static std::string vprintf(const char *format, va_list ap);

    // Tokenizing
    static unsigned tokenize(const std::string &s,
                             std::vector<std::string> &tokens,
                             const std::string &delims = DEFAULT_DELIMS,
                             bool allowEmpty = false,
                             unsigned maxTokens = ~0);
    static unsigned tokenizeLine(std::istream &stream,
                                 std::vector<std::string> &tokens,
                                 const std::string &delims = DEFAULT_DELIMS,
                                 const std::string &lineDelims =
                                 DEFAULT_LINE_DELIMS,
                                 unsigned maxLength = 1024);

    // Parsing
    template <typename T> static bool parse(const std::string &s, T &value,
                                            bool full);

#define CBANG_STRING_PT(NAME, TYPE, DESC)                               \
    static TYPE parse##NAME(const std::string &s, TYPE &value, bool full);
#include "StringParseTypes.def"

#define CBANG_STRING_PT(NAME, TYPE, DESC)                               \
    static TYPE parse##NAME(const std::string &s, bool full = false);
#include "StringParseTypes.def"

    template <typename T> static T parse(const std::string &s, bool full);

    // Type tests
#define CBANG_STRING_PT(NAME, TYPE, DESC)                       \
    static TYPE is##NAME(const std::string &s, bool full);
#include "StringParseTypes.def"

    static bool isInteger(const std::string &s);
    static bool isNumber(const std::string &s);

    // Transformations
    static std::string trimLeft(const std::string &s,
                                const std::string &delims = DEFAULT_DELIMS);
    static std::string trimRight(const std::string &s,
                                 const std::string &delims = DEFAULT_DELIMS);
    static std::string trim(const std::string &s,
                            const std::string &delims = DEFAULT_DELIMS);
    static std::string toUpper(const std::string &s);
    static std::string toLower(const std::string &s);
    static std::string capitalize(const std::string &s);
    static std::ostream &fill(std::ostream &stream, const std::string &str,
                              unsigned currentColumn = 0,
                              unsigned indent = 0,
                              unsigned maxColumn = 80);
    static std::string fill(const std::string &str,
                            unsigned currentColumn = 0,
                            unsigned indent = 0,
                            unsigned maxColumn = 80);

    static bool endsWith(const std::string &s, const std::string &part);
    static bool startsWith(const std::string &s, const std::string &part);
    static std::string bar(const std::string &title = "", unsigned width = 80,
                           const std::string &chars = "*");
    static std::string hexdump(const char *data, unsigned size);
    static std::string hexdump(const uint8_t *data, unsigned size);
    static std::string hexdump(const std::string &s);
    static char hexNibble(int x, bool lower = true);
    static std::string hexEncode(const std::string &s);
    static std::string hexEncode(const char *data, unsigned length);
    static std::string escapeRE(const std::string &s);
    static std::string escapeC(char c);
    static void escapeC(std::string &result, char c);
    static std::string escapeC(const std::string &s);
    static std::string unescapeC(const std::string &s);


    template <class I> static
    void join(std::ostream &stream, I begin, I end,
              const std::string &delim = " ") {
      for (I it = begin; it != end; it++) {
        if (it != begin) stream << delim;
        stream << *it;
      }
    }


    template <class I> static
    std::string join(I begin, I end, const std::string &delim = " ") {
      std::ostringstream stream;
      join(stream, begin, end, delim);
      return stream.str();
    }


    template <class T> static
    std::string join(const T &s, const std::string &delim = " ") {
      return join(s.begin(), s.end(), delim);
    }


    template <class T> static
    void join(std::ostream &stream, const T &s,
              const std::string &delim = " ") {
      join(stream, s.begin(), s.end(), delim);
    }


    static std::string ellipsis(const std::string &s, unsigned width = 80);

    /// Regular expression find
    static std::size_t find(const std::string &s, const std::string &pattern,
                            std::vector<std::string> *groups = 0);
    /// Replace a single character
    static std::string replace(const std::string &s, char search, char replace);
    /// Regular expression replace
    static std::string replace(const std::string &s, const std::string &search,
                               const std::string &replace);
    static std::string transcode(const std::string &s,
                                 const std::string &search,
                                 const std::string &replace);

    // Formatting
    using format_cb_t = std::function<std::string (
      const std::string &id, const std::string &spec)>;
    std::string format(format_cb_t cb);
  };
}
