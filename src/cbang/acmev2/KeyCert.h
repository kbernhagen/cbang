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

#include <cbang/openssl/KeyPair.h>
#include <cbang/openssl/CertificateChain.h>

#include <functional>


namespace cb {
  class CSR;

  namespace ACMEv2 {
    class KeyCert {
      KeyPair key;
      CertificateChain chain;
      std::vector<std::string> domains;
      double waitUntil = 0;

    public:
      KeyCert(const std::string &domains, const KeyPair &key,
              const CertificateChain &chain = CertificateChain());
      virtual ~KeyCert() {}

      const KeyPair &getKey() const {return key;}
      CertificateChain &getChain() {return chain;}
      const CertificateChain &getChain() const {return chain;}

      void addDomain(const std::string &domain) {domains.push_back(domain);}
      const std::vector<std::string> &getDomains() const {return domains;}

      double getWaitUntil() const {return waitUntil;}
      void setWaitUntil(double waitUntil) {this->waitUntil = waitUntil;}

      bool hasCert() const {return chain.size();}
      bool expiredIn(unsigned secs) const;

      virtual SmartPointer<CSR> makeCSR() const;
    };
  }
}
