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

#include "Sink.h"
#include "ValueType.h"

#include <vector>
#include <set>


namespace cb {
  namespace JSON {
    class NullSink : public Sink {
    protected:
      bool allowDuplicates;

      std::vector<ValueType> stack;
      typedef std::set<std::string> keys_t;
      std::vector<keys_t> keyStack;

      bool canWrite = true;

    public:
      NullSink(bool allowDuplicates = false) :
        allowDuplicates(allowDuplicates) {}

      bool getAllowDuplicates() const {return allowDuplicates;}
      void setAllowDuplicates(bool x) {allowDuplicates = x;}

      // From Sink
      unsigned getDepth() const override {return stack.size();}
      void close() override;
      void reset() override;
      void writeNull() override {assertCanWrite();}
      void writeBoolean(bool value) override {assertCanWrite();}
      void write(double value) override {assertCanWrite();}
      void write(const std::string &value) override {assertCanWrite();}
      bool inList() const override;
      void beginList(bool simple = false) override;
      void beginAppend() override;
      void endList() override;
      bool inDict() const override;
      void beginDict(bool simple = false) override;
      bool has(const std::string &key) const override;
      void beginInsert(const std::string &key) override;
      void endDict() override;

    protected:
      void assertCanWrite();
      void assertWriteNotPending();
    };
  }
}
