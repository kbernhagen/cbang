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

#include "Value.h"

#include <limits>


namespace cb {
  namespace JSON {
    namespace Num {
      template <typename T>
      struct Type {
        enum {
          SignedInt =
          (std::numeric_limits<T>::is_integer &&
           std::numeric_limits<T>::is_signed) ? 1 : 0
        };
      };

      template <typename T, typename X, int _T, int _X>
      struct Imp {
        static inline bool InRange(X x) {
          return (X)std::numeric_limits<T>::min() <= x ||
            x <= (X)std::numeric_limits<T>::max();
        }
      };

      template <typename T, typename X>
      struct Imp<T, X, 1, 0> {
        static inline bool InRange(X x) {
          return x <= (X)std::numeric_limits<T>::max();
        }
      };

      template <typename T, typename X>
      struct Imp<T, X, 0, 1> {
        static inline bool InRange(X x) {
          return 0 <= x && (T)x <= std::numeric_limits<T>::max();
        }
      };


      template <typename T, typename X>
      static inline bool InRange(X x) {
        return Imp<T, X, Type<T>::SignedInt, Type<X>::SignedInt>::InRange(x);
      }
    }


    template <typename T>
    class NumberValue : public Value {
    protected:
      T value;

    public:
      NumberValue(const T &value = 0) : value(value) {}

      const T &getValue() const {return value;}

      operator const T &() const {return value;}

      // From Value
      ValueType getType() const override {return JSON_NUMBER;}
      bool isInteger() const override {return false;}
      bool isNumber() const override {return true;}
      ValuePtr copy(bool deep = false) const override
      {return new NumberValue<T>(value);}
      double getNumber() const override {return value;}


#define CBANG_NUM_FUNCS(TYPE, SHORT, LONG)                              \
      bool is##SHORT() const override {return Num::InRange<TYPE>(value);} \
                                                                        \
      TYPE get##SHORT() const override {                                \
        if (!is##SHORT())                                               \
          CBANG_TYPE_ERROR("Value " << value << " is not a " #LONG);    \
                                                                        \
        return (TYPE)value;                                             \
      }

      CBANG_NUM_FUNCS(int8_t,   S8,   8-bit signed integer);
      CBANG_NUM_FUNCS(uint8_t,  U8,   8-bit unsigned integer);
      CBANG_NUM_FUNCS(int16_t,  S16, 16-bit signed integer);
      CBANG_NUM_FUNCS(uint16_t, U16, 16-bit unsigned integer);
      CBANG_NUM_FUNCS(int32_t,  S32, 32-bit signed integer);
      CBANG_NUM_FUNCS(uint32_t, U32, 32-bit unsigned integer);
      CBANG_NUM_FUNCS(int64_t,  S64, 64-bit signed integer);
      CBANG_NUM_FUNCS(uint64_t, U64, 64-bit unsigned integer);

#undef CBANG_NUM_FUNCS

      bool toBoolean() const override {return value;}
      void write(Sink &sink) const override {sink.write(value);}
    };


    typedef NumberValue<double>   Number;
    typedef NumberValue<uint64_t> U64;
    typedef NumberValue<int64_t>  S64;

    template<> inline bool U64::isInteger() const {return true;}
    template<> inline bool S64::isInteger() const {return true;}
  }
}
