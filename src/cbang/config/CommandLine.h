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

#include "Option.h"
#include "Options.h"

#include <cbang/SmartPointer.h>

#include <string>
#include <map>
#include <vector>
#include <ostream>

namespace cb {
  /// Represents a set of command line options
  class CommandLine : public Options {
    const Options *keywords      = 0;
    std::string name;
    std::string usageArgs;
    std::vector<std::string> usageExtras;
    bool allowConfigAsFirstArg   = false;
    bool allowSingleDashLongOpts = false;
    bool allowExtraOpts          = false;
    bool allowPositionalArgs     = true;
    bool warnOnInvalidArgs       = false;
    bool showKeywordOpts         = true;

    std::vector<const char *> licenseText;
    std::vector<std::string> positionalArgs;

  public:
    CommandLine();

    // From Options
    using Options::add;
    void add(const std::string &name, SmartPointer<Option> option) override;
    const SmartPointer<Option> &get(const std::string &key) const override;

    void setKeywordOptions(const Options *options) {keywords = options;}
    void setUsageArgs(const std::string &s) {usageArgs = s;}
    void addUsageLine(const std::string &line) {usageExtras.push_back(line);}
    void addLicenseText(const char *text) {licenseText.push_back(text);}

    void setAllowConfigAsFirstArg  (bool x) {allowConfigAsFirstArg = x;}
    void setAllowSingleDashLongOpts(bool x) {allowSingleDashLongOpts = x;}
    void setAllowExtraOpts         (bool x) {allowExtraOpts = x;}
    void setAllowPositionalArgs    (bool x) {allowPositionalArgs = x;}
    void setWarnOnInvalidArgs      (bool x) {warnOnInvalidArgs = x;}
    void setShowKeywordOpts        (bool x) {showKeywordOpts = x;}

    const std::vector<std::string> &getPositionalArgs() const
    {return positionalArgs;}

    using cb::Serializable::parse;
    int parse(int argc, char *argv[]);
    int parse(const std::vector<std::string> &args);

    void usage(std::ostream &stream, const std::string &name) const;

  private:
    int usageAction(Option &option);
    int jsonHelpAction();
    int licenseAction();
    int incVerbosityAction();
    int quietAction();
  };
};
