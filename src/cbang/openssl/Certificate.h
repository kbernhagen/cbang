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

#include <cbang/SmartPointer.h>
#include <cbang/String.h>
#include <cbang/util/Serializable.h>

#include <iostream>

typedef struct x509_st X509;


namespace cb {
  class KeyPair;
  class CertificateContext;

  class Certificate : public Serializable {
    X509 *cert;

  public:
    Certificate(const Certificate &o);
    Certificate(X509 *cert = 0);
    Certificate(const std::string &pem);
    ~Certificate();

    Certificate &operator=(const Certificate &o);

    X509 *getX509() const {return cert;}

    bool hasPublicKey() const;
    KeyPair getPublicKey() const;
    void setPublicKey(const KeyPair &key);

    void setVersion(int version);
    int getVersion() const;

    void setSerial(long serial);
    long getSerial() const;

    uint64_t getNotBefore() const;
    void setNotBefore(uint64_t x = 0);
    bool isNotBeforeInFuture() const;

    uint64_t getNotAfter() const;
    void setNotAfter(uint64_t x);
    bool isNotAfterInPast() const;
    bool expiredIn(unsigned secs) const;

    void setIssuer(const Certificate &issuer);

    void addNameEntry(const std::string &name, const std::string &value);
    std::string getNameEntry(const std::string &name) const;

    bool hasExtension(const std::string &name) const;
    std::string getExtension(const std::string &name) const;
    std::string getExtension(const std::string &name,
                             const std::string &defaultValue) const;
    bool extensionHas(const std::string &name, const std::string &value,
                      const std::string &delims = String::DEFAULT_DELIMS);
    void addExtension(const std::string &name, const std::string &value,
                      CertificateContext *ctx = 0);
    static void addExtensionAlias(const std::string &alias,
                                  const std::string &name);

    bool checkHost(const std::string &hostname) const;
    bool checkEmail(const std::string &email) const;
    bool issued(const Certificate &o) const;

    void sign(const KeyPair &key, const std::string &digest = "sha256") const;
    void verify(const KeyPair &key) const;

    bool operator==(const Certificate &o) const;
    bool operator!=(const Certificate &o) const {return !(*this == o);}
    bool operator<(const Certificate &o) const;

    // From Serializable
    void read(std::istream &stream) override;
    void write(std::ostream &stream) const override;
  };
}
