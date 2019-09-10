/*
* Copyright 2005-2022 Scott Maxwell
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to
* deal in the Software without restriction, including without limitation the
* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
* sell copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>

#define IS_LITTLE_ENDIAN 1
#define IS_BIG_ENDIAN 0

namespace TokenStream {

// define our endian swapping base types
class UInt16_Swap { // NOLINT
  uint16_t m_val = 0;

 public:
  constexpr UInt16_Swap() = default;
  constexpr UInt16_Swap(uint16_t v) : m_val(Swap(v)) {}
  constexpr UInt16_Swap(const UInt16_Swap& rhs) // NOLINT
      :
      m_val(rhs.m_val) {}

  UInt16_Swap& operator=(uint16_t v) {
    m_val = Swap(v);
    return *this;
  }
  UInt16_Swap& operator=(const UInt16_Swap& rhs) = default;
  constexpr operator uint16_t() const {
    return Swap(m_val);
  }
  constexpr static uint16_t Swap(uint16_t val) {
    return (val << 8u) | (val >> 8u);
  }
  constexpr bool operator!() const {
    return !m_val;
  }
};

class Int16_Swap { // NOLINT
  uint16_t m_val = 0;

 public:
  constexpr Int16_Swap() = default;
  constexpr Int16_Swap(int16_t v) : m_val(Swap(v)) {}
  constexpr Int16_Swap(const Int16_Swap& rhs) // NOLINT
      :
      m_val(rhs.m_val) {}

  Int16_Swap& operator=(int16_t v) {
    m_val = Swap(v);
    return *this;
  }
  Int16_Swap& operator=(const Int16_Swap& rhs) = default;
  constexpr operator int16_t() const {
    return Swap(m_val);
  }
  constexpr static int16_t Swap(int16_t val) {
    return UInt16_Swap::Swap(val);
  }
  constexpr bool operator!() const {
    return !m_val;
  }
};

class UInt32_Swap { // NOLINT
  uint32_t m_val = 0;

 public:
  constexpr UInt32_Swap() = default;
  constexpr UInt32_Swap(uint32_t v) : m_val(Swap(v)) {}
  constexpr UInt32_Swap(const UInt32_Swap& rhs) // NOLINT
      :
      m_val(rhs.m_val) {}

  UInt32_Swap& operator=(uint32_t v) {
    m_val = Swap(v);
    return *this;
  }
  UInt32_Swap& operator=(const UInt32_Swap& rhs) = default;
  constexpr operator uint32_t() const {
    return Swap(m_val);
  }
  constexpr static uint32_t Swap(uint32_t val) {
    return (val << 24u) | ((val << 8u) & 0xff'0000u) | ((val >> 8u) & 0xff00u) | (val >> 24u);
  }
  constexpr bool operator!() const {
    return !m_val;
  }
};

class Int32_Swap { // NOLINT
  uint32_t m_val = 0;

 public:
  constexpr Int32_Swap() = default;
  constexpr Int32_Swap(int32_t v) : m_val(Swap(v)) {}
  constexpr Int32_Swap(const Int32_Swap& rhs) // NOLINT
      :
      m_val(rhs.m_val) {}

  Int32_Swap& operator=(int32_t v) {
    m_val = Swap(v);
    return *this;
  }
  Int32_Swap& operator=(const Int32_Swap& rhs) = default;
  constexpr operator int32_t() const {
    return Swap(m_val);
  }
  constexpr static int32_t Swap(int32_t val) {
    return UInt32_Swap::Swap(val);
  }
  constexpr bool operator!() const {
    return !m_val;
  }
};

class UInt64_Swap { // NOLINT
  uint64_t m_val = 0;

 public:
  constexpr UInt64_Swap() = default;
  constexpr UInt64_Swap(uint64_t v) : m_val(Swap(v)) {}
  constexpr UInt64_Swap(const UInt64_Swap& rhs) // NOLINT
      :
      m_val(rhs.m_val) {}

  UInt64_Swap& operator=(uint64_t v) {
    m_val = Swap(v);
    return *this;
  }
  UInt64_Swap& operator=(const UInt64_Swap& rhs) = default;
  constexpr operator uint64_t() const {
    return Swap(m_val);
  }
  constexpr static uint64_t Swap(uint64_t val) {
    return (val << 56u) | ((val << 40u) & 0xff0000'00000000u) | ((val << 24u) & 0xff00'00000000u) |
        ((val << 8u) & 0xff'00000000u) | ((val >> 8u) & 0xff000000) | ((val >> 24u) & 0xff0000u) |
        ((val >> 40u) & 0xff00u) | (val >> 56u);
  }
  constexpr bool operator!() const {
    return !m_val;
  }
};

class Int64_Swap { // NOLINT
  uint64_t m_val = 0;

 public:
  constexpr Int64_Swap() = default;
  constexpr Int64_Swap(int64_t v) : m_val(Swap(v)) {}
  constexpr Int64_Swap(const Int64_Swap& rhs) // NOLINT
      :
      m_val(rhs.m_val) {}

  Int64_Swap& operator=(int64_t v) {
    m_val = Swap(v);
    return *this;
  }
  Int64_Swap& operator=(const Int64_Swap& rhs) = default;
  constexpr operator int64_t() const {
    return Swap(m_val);
  }
  constexpr static int64_t Swap(int64_t val) {
    return UInt64_Swap::Swap(val);
  }
  constexpr bool operator!() const {
    return !m_val;
  }
};

class F32_Swap { // NOLINT
  struct Union {
    union {
      float f;
      uint32_t i{};
    };
    constexpr Union() // NOLINT
    {}
    constexpr Union(float value) : f(value) {}
    constexpr Union(uint32_t value) // NOLINT
        :
        i(value) {}
    constexpr Union(UInt32_Swap value) // NOLINT
        :
        i(value) {}
  };
  UInt32_Swap m_val;

 public:
  constexpr F32_Swap() // NOLINT
  {}
  constexpr F32_Swap(float v) : m_val(Union(v).i) {}
  constexpr F32_Swap(const F32_Swap& rhs) // NOLINT
      :
      m_val(rhs.m_val) {}

  F32_Swap& operator=(float v) {
    m_val = Union(v).i;
    return *this;
  }
  F32_Swap& operator=(const F32_Swap& rhs) = default;

  constexpr operator float() const {
    return Union(m_val).f;
  }
  constexpr bool operator!() const {
    return !m_val;
  }
};

class F64_Swap { // NOLINT
  struct Union {
    union {
      double f;
      uint64_t i{};
    };
    constexpr Union() // NOLINT
    {}
    constexpr Union(double value) : f(value) {}
    constexpr Union(uint64_t value) // NOLINT
        :
        i(value) {}
    constexpr Union(UInt64_Swap value) // NOLINT
        :
        i(value) {}
  };
  UInt64_Swap m_val;

 public:
  constexpr F64_Swap() // NOLINT
  {}
  constexpr F64_Swap(double v) : m_val(Union(v).i) {}
  constexpr F64_Swap(const F64_Swap& rhs) // NOLINT
      :
      m_val(rhs.m_val) {}

  F64_Swap& operator=(double v) {
    m_val = Union(v).i;
    return *this;
  }
  F64_Swap& operator=(const F64_Swap& rhs) = default;

  constexpr operator double() const {
    return Union(m_val).f;
  }
  constexpr bool operator!() const {
    return !m_val;
  }
};

#if IS_LITTLE_ENDIAN
using uint64_le = uint64_t;
using uint32_le = uint32_t;
using uint16_le = uint16_t;
using int64_le = int64_t;
using int32_le = int32_t;
using int16_le = int16_t;
using f32_le = float;
using f64_le = double;

using uint64_be = UInt64_Swap;
using uint32_be = UInt32_Swap;
using uint16_be = UInt16_Swap;
using int64_be = Int64_Swap;
using int32_be = Int32_Swap;
using int16_be = Int16_Swap;
using f32_be = F32_Swap;
using f64_be = F64_Swap;

#else
using uint64_le = UInt64_Swap;
using uint32_le = UInt32_Swap;
using uint16_le = UInt16_Swap;
using int64_le = Int64_Swap;
using int32_le = Int32_Swap;
using int16_le = Int16_Swap;
using f32_le = F32_Swap;
using f64_le = F64_Swap;

using uint64_be = uint64_t;
using uint32_be = uint32_t;
using uint16_be = uint16_t;
using int64_be = int64_t;
using int32_be = int32_t;
using int16_be = int16_t;
using f32_be = float;
using f64_be = double;
#endif

} // namespace TokenStream
