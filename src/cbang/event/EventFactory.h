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

#include "EventFlag.h"

#include <cbang/SmartPointer.h>
#include <cbang/net/SocketType.h>

#include <functional>


namespace cb {
  namespace Event {
    class Base;
    class Event;

    class EventFactory : public EventFlag {
      Base &base;

    public:
      EventFactory(Base &base) : base(base) {}


      template <class T> struct Callback {
        typedef void (T::*member_t)(Event &, int, unsigned);
        typedef void (T::*bare_member_t)();
      };

      typedef std::function<void (Event &, int, unsigned)> callback_t;
      typedef std::function<void ()> bare_callback_t;


      SmartPointer<Event> newEvent(callback_t cb,
                                   unsigned flags = EVENT_PERSIST);
      SmartPointer<Event> newEvent(socket_t fd, callback_t cb,
                                   unsigned flags = EVENT_PERSIST);
      SmartPointer<Event> newSignal(int signal, callback_t cb,
                                    unsigned flags = EVENT_PERSIST);

      // Bare Callbacks
      callback_t bind(bare_callback_t cb)
      {return [cb] (Event &, int, unsigned) {cb();};}

      SmartPointer<Event> newEvent(bare_callback_t cb,
                                   unsigned flags = EVENT_PERSIST)
      {return newEvent(bind(cb), flags);}

      SmartPointer<Event>
      newEvent(socket_t fd, bare_callback_t cb, unsigned flags = EVENT_PERSIST)
        {return newEvent(fd, bind(cb), flags);}

      SmartPointer<Event> newSignal(int signal, bare_callback_t cb,
                                    unsigned flags = EVENT_PERSIST)
      {return newSignal(signal, bind(cb), flags);}


      // Member Callbacks
      template <class T>
      callback_t bind(T *obj, typename Callback<T>::member_t member) {
        using namespace std::placeholders;
        return std::bind(member, obj, _1, _2, _3);
      }

      template <class T> SmartPointer<Event> newEvent(
        T *obj, typename Callback<T>::member_t member,
        unsigned flags = EVENT_PERSIST)
      {return newEvent(bind(obj, member), flags);}

      template <class T>
      SmartPointer<Event> newEvent(socket_t fd, T *obj,
                                   typename Callback<T>::member_t member,
                                   unsigned flags = EVENT_PERSIST)
        {return newEvent(fd, bind(obj, member), flags);}

      template <class T> SmartPointer<Event>
      newSignal(int signal, T *obj, typename Callback<T>::member_t member,
                unsigned flags = EVENT_PERSIST)
      {return newSignal(signal, bind(obj, member), flags);}


      // Bare Member Callbacks
      template <class T>
      callback_t bind(T *obj, typename Callback<T>::bare_member_t member) {
        return bind(std::bind(member, obj));
      }

      template <class T>
      SmartPointer<Event> newEvent(
        T *obj, typename Callback<T>::bare_member_t member,
        unsigned flags = EVENT_PERSIST)
      {return newEvent(bind(obj, member), flags);}

      template <class T>
      SmartPointer<Event> newEvent(socket_t fd, T *obj,
                                   typename Callback<T>::bare_member_t member,
                                   unsigned flags)
        {return newEvent(fd, bind(obj, member), flags);}

      template <class T> SmartPointer<Event> newSignal(
        int signal, T *obj, typename Callback<T>::bare_member_t member,
        unsigned flags = EVENT_PERSIST)
      {return newSignal(signal, bind(obj, member), flags);}
    };
  }
}
