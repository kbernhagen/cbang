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

#include "URLPatternMatcher.h"

#include <cbang/String.h>
#include <cbang/util/Regex.h>

using namespace std;
using namespace cb::HTTP;


URLPatternMatcher::URLPatternMatcher(
  const string &pattern, const cb::SmartPointer<RequestHandler> &child) :
  RE2PatternMatcher(toRE2Pattern(pattern), child) {}


string URLPatternMatcher::toRE2Pattern(const string &pattern) {
  if (pattern.empty() || pattern[0] == '^') return pattern;

  vector<string> parts;
  String::tokenize(pattern, parts, "/");

  // TODO allow escaping \, { and }

  string rePattern;
  for (auto &part: parts) {
    auto start = part.find('{');
    if (start != string::npos) {
      auto end = part.find_last_of('}');
      if (end == string::npos)
        THROW("Mismatched { in URL pattern: " << pattern);

      string head = part.substr(0, start);
      string var  = part.substr(start + 1, end - start - 1);
      string tail = part.substr(end + 1);

      if ((head + var + tail).find_first_of("{}") != string::npos)
        THROW("Invalid URL pattern: " << pattern);

      rePattern += "/" + head + "(?P<" + var + ">[^/]*)" + tail;

    } else rePattern += "/" + part;
  }

  if (!pattern.empty() && pattern.back() == '/') rePattern += "/";

  return rePattern;
}
