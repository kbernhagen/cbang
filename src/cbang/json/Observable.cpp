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

#include "Observable.h"

using namespace std;
using namespace cb::JSON;



void ObservableBase::setParentRef(Value *parent, const ValuePtr &key) {
  if (this->parent) THROW("Parent already set");
  this->parent = parent;
  this->key    = key;
}


void ObservableBase::incParentRef() {
  if (key->isNumber()) key = Factory().create(key->getU64() + 1);
}


void ObservableBase::decParentRef() {
  if (key->isNumber()) key = Factory().create(key->getU64() - 1);
}


void ObservableBase::_notify(list<ValuePtr> &change) {
  notify(change);

  if (parent) {
    change.push_front(key);
    dynamic_cast<ObservableBase *>(parent)->_notify(change);
  }
}


void ObservableBase::_notify(unsigned index, const ValuePtr &value) {
  list<ValuePtr> change;

  if (value.isSet()) change.push_back(value);
  else change.push_back(Factory().createNull());

  change.push_front(Factory().create(index));
  _notify(change);
}


void ObservableBase::_notify(const string &key, const ValuePtr &value) {
  list<ValuePtr> change;

  if (value.isSet()) change.push_back(value);
  else change.push_back(Factory().createNull());

  change.push_front(Factory().create(key));
  _notify(change);
}
