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

/** @file
*  Contains the Writer class definition.
*  Writer is meant to be used in conjunction with
*  Reader save objects' persistent data.
*/

#include <TokenStream/TokenStream.h>
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define ASSERT_TOKEN_SET() TS_ASSERT(m_nextToken.IsValid() || !m_stream->tellp(), "Token not set")

namespace TokenStream {

class MemoryWriter;

template<typename T>
struct ValueWithDefaultStruct {
  T m_value;
  T m_default;
  constexpr ValueWithDefaultStruct(T value, T defaultValue) :
      m_value(value), m_default(defaultValue) {}
};

//! @brief Create a helper pair for writing values to a stream with defaults.
//! @param value The value to write.
//! @param defaultValue The default for this field.
//! @note When you write this to a stream, if value==defaultValue && trimDefaults==true, nothing will be written.
template<typename T>
constexpr ValueWithDefaultStruct<T> ValueWithDefault(T value, T defaultValue) {
  return ValueWithDefaultStruct<T>(value, defaultValue);
}

template<typename T>
struct StringValueWithDefaultStruct {
  const T& m_value;
  const typename T::value_type* m_default;
  constexpr StringValueWithDefaultStruct(const T& value,
                                         const typename T::value_type* defaultValue) :
      m_value(value), m_default(defaultValue) {}
};

//! @brief Create a helper pair for writing values to a stream with defaults.
//! @param value The value to write.
//! @param defaultValue The default for this field.
//! @note When you write this to a stream, if value==defaultValue && trimDefaults==true, nothing will be written.
inline StringValueWithDefaultStruct<std::string> ValueWithDefault(const std::string& value,
                                                                  const char* defaultValue) {
  return {value, defaultValue};
}

//! @brief Create a helper pair for writing values to a stream with defaults.
//! @param value The value to write.
//! @param defaultValue The default for this field.
//! @note When you write this to a stream, if value==defaultValue && trimDefaults==true, nothing will be written.
inline StringValueWithDefaultStruct<std::wstring> ValueWithDefault(const std::wstring& value,
                                                                   const wchar_t* defaultValue) {
  return {value, defaultValue};
}

/*! @brief Writes objects into a TokenStream
     *
     * @code
     * void PersonalRecord::Write(Writer& writer)
     * {
     *   writer << Token(BirthMonthToken) << m_birthMonth;
     *   writer << Token(BirthDayToken) << m_birthDay;
     *   writer << Token(BirthYearToken) << m_birthYear;
     *   writer << Token(SocialSecurityNumberToken) << m_social;
     *   writer << Token(FirstNameToken) << m_firstName;
     *   writer << Token(LastNameToken) << m_lastName;
     * }
     * @endcode
     *
     * @warning By default, he \e Put methods will not write anything if the data being
     * written is the default value. For instance, PutByte(someToken, 0) will not
     * modify the stream. This is done to keep the stream as small as possible.
     * As a result, \b you \b must \b clear all members of your object before
     * using Reader to retrieve their values.
     *
     * @note You can now change trimDefaults to \e false to disable that functionality.
     *
     * @see Token
     * @see Serializable
     * @see MemoryWriter
     * @see Binary
     * @see Reader
     * @see Reader::SubStream
     */
class Writer { // NOLINT
 public:
  // Find out whether T has `void WriteToTokenStream(Writer&)` method, or a Helper<T> with a Write(const T& o, Writer& writer) method or a WriteSingleValue(const T& o, Writer&, const T* defaultValue) method
  template<typename>
  struct has_write_to_token_stream_method_Void {
    typedef void type;
  };
  template<typename>
  struct has_object_writer_helper_Void {
    typedef void type;
  };
  template<typename>
  struct has_value_writer_helper_Void {
    typedef void type;
  };
  template<typename T, typename Sfinae = void>
  struct has_write_to_token_stream_method : std::false_type {};
  template<typename T>
  struct has_write_to_token_stream_method<T,
                                          typename has_write_to_token_stream_method_Void<
                                              decltype(std::declval<const T&>().WriteToTokenStream(
                                                  *static_cast<Writer*>(nullptr)))>::type>
      : std::true_type {};
  template<typename T, typename Sfinae = void>
  struct has_object_writer_helper : std::false_type {};
  template<typename T>
  struct has_object_writer_helper<
      T,
      typename has_object_writer_helper_Void<decltype(Helper<T>::Write(
          *static_cast<const T*>(nullptr), *static_cast<Writer*>(nullptr)))>::type>
      : std::true_type {};
  template<typename T, typename Sfinae = void>
  struct has_value_writer_helper : std::false_type {};
  template<typename T>
  struct has_value_writer_helper<
      T,
      typename has_value_writer_helper_Void<decltype(Helper<T>::WriteSingleValue(
          *static_cast<const T*>(nullptr),
          *static_cast<Writer*>(nullptr),
          static_cast<const T*>(nullptr)))>::type> : std::true_type {};
  template<typename T>
  struct has_custom_writer {
    static constexpr bool value = std::is_base_of<Serializable, T>::value ||
        has_write_to_token_stream_method<T>::value || has_object_writer_helper<T>::value;
  };

  //! @brief Creates Writer that will output to the specified stream.
  //! @param stream Any generic stream object.
  //! @param writer Inherit parameters from other writer.
  explicit Writer(std::ostream& stream, const Writer& writer) :
      m_userData(writer.m_userData), m_stream(&stream), m_trimDefaults(writer.m_trimDefaults) {}

  //! @brief Creates Writer that will output to the specified stream.
  //! @param stream Any generic stream object.
  //! @param trimDefaults If \e true, default values will not be written. If false, tokens with 0-len will be written for default values.
  explicit Writer(std::ostream& stream, bool trimDefaults = true) :
      m_stream(&stream), m_trimDefaults(trimDefaults) {}

  //! @brief Creates Writer that will output to the specified stream.
  //! @param stream Any generic stream object.
  //! @param writer Inherit parameters from other writer.
  //! @param trimDefaults If \e true, default values will not be written. If false, tokens with 0-len will be written for default values.
  explicit Writer(std::ostream& stream, const Writer& writer, bool trimDefaults) :
      m_userData(writer.m_userData), m_stream(&stream), m_trimDefaults(trimDefaults) {}

  //! @brief Do not allow move semantics for the stream. We need it to stick around externally.
  explicit Writer(std::ostream&& stream, bool trimDefaults = true) = delete;

  // no copying
  Writer(const Writer&) = delete;
  Writer& operator=(const Writer&) = delete;

  //! @brief Use this for user data that is needed during serialization
  void* m_userData = nullptr;

  //@{
  //! @brief Writes a Token.
  //! @param token Token to write.
  //! @pre Previous Put command must not have been a token.
  //! @pre \p token < 0xff
  //! @post Next Put must be the data attached to the token.
  //! @note Nothing is actually written until the subsequent Put command.
  //! This is because no data is written for default values.
  Writer& PutToken(Token token) {
    TS_ASSERT(!m_nextToken.IsValid(), "Token already set");
    m_nextToken = token;
    return *this;
  }
  Writer& operator<<(Token token) {
    return PutToken(token);
  }
  //@}

  //! @brief Writes a 1-byte value to the stream.
  //! @param token A Token
  //! @param value Any 8-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, uint8_t value, uint8_t defaultValue = 0);

  //! @brief Writes a 1-byte value to the stream.
  //! @param token A Token
  //! @param value Any 8-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, uint8_t value, uint32_t defaultValue) {
    return Put(token, value, static_cast<uint8_t>(defaultValue));
  }

  //! @brief Writes a 1-byte value to the stream.
  //! @param token A Token
  //! @param value Any 8-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, uint8_t value, int defaultValue) {
    return Put(token, value, static_cast<uint8_t>(defaultValue));
  }

  //! @brief Writes a 1-byte value to the stream.
  //! @param value Any 8-bit value
  //! @pre You must have written a token immediately preceding this
  //! @note If value==0 && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(uint8_t value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, value);
  }

  //! @brief Writes a 1-byte value to the stream.
  //! @param token A Token
  //! @param value Any 8-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, int8_t value, int8_t defaultValue = 0);

  //! @brief Writes a 1-byte value to the stream.
  //! @param token A Token
  //! @param value Any 8-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, int8_t value, int defaultValue) {
    return Put(token, value, static_cast<int8_t>(defaultValue));
  }

  //! @brief Writes a 1-byte value to the stream.
  //! @param value Any 8-bit value
  //! @pre You must have written a token immediately preceding this
  //! @note If value==0 && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(int8_t value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, value);
  }

  //! @brief Writes a 1-byte boolean value to the stream.
  //! @param token A Token
  //! @param value A bool
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, bool value, bool defaultValue = false) {
    return Put(token, static_cast<uint8_t>(value), static_cast<uint8_t>(defaultValue));
  }

  //! @brief Writes a 1-byte boolean value to the stream.
  //! @param value Any 8-bit value
  //! @pre You must have written a token immediately preceding this
  //! @note If value==false && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(bool value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, static_cast<uint8_t>(value));
  }

  //! @brief Writes a 2-byte value to the stream.
  //! @param token A Token
  //! @param value Unsigned 16-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, uint16_t value, uint16_t defaultValue = 0);

  //! @brief Writes a 2-byte value to the stream.
  //! @param token A Token
  //! @param value Unsigned 16-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, uint16_t value, uint32_t defaultValue) {
    return Put(token, value, static_cast<uint16_t>(defaultValue));
  }

  //! @brief Writes a 2-byte value to the stream.
  //! @param token A Token
  //! @param value Unsigned 16-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, uint16_t value, int defaultValue) {
    return Put(token, value, static_cast<uint16_t>(defaultValue));
  }

  //! @brief Writes a 2-byte value to the stream.
  //! @param value Unsigned 16-bit value
  //! @pre You must have written a token immediately preceding this
  //! @note If value==0 && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(uint16_t value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, value, static_cast<uint16_t>(0));
  }

  //! @brief Writes a 2-byte value to the stream.
  //! @param token A Token
  //! @param value Signed 16-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, int16_t value, int16_t defaultValue = 0);

  //! @brief Writes a 2-byte value to the stream.
  //! @param token A Token
  //! @param value Signed 16-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, int16_t value, int defaultValue) {
    return Put(token, value, static_cast<int16_t>(defaultValue));
  }

  //! @brief Writes a 2-byte value to the stream.
  //! @param value Signed 16-bit value
  //! @pre You must have written a token immediately preceding this
  //! @note If value==0 && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(int16_t value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, value);
  }

  //! @brief Writes a 4-byte value to the stream.
  //! @param token A Token
  //! @param value Unsigned 32-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, uint32_t value, uint32_t defaultValue = 0);

  //! @brief Writes a 4-byte value to the stream.
  //! @param token A Token
  //! @param value Unsigned 32-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, uint32_t value, int defaultValue) {
    return Put(token, value, static_cast<uint32_t>(defaultValue));
  }

  //! @brief Writes a 4-byte value to the stream.
  //! @param token A Token
  //! @param value Unsigned 32-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, uint32_t value, long long defaultValue) {
    return Put(token, value, static_cast<uint32_t>(defaultValue));
  }

  //! @brief Writes a 4-byte value to the stream.
  //! @param value Unsigned 32-bit value
  //! @pre You must have written a token immediately preceding this
  //! @note If value==0 && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(uint32_t value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, value);
  }

  //! @brief Writes a 4-byte value to the stream.
  //! @param token A Token
  //! @param value Signed 32-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, int32_t value, int32_t defaultValue = 0);

  //! @brief Writes a 4-byte value to the stream.
  //! @param value Signed 32-bit value
  //! @pre You must have written a token immediately preceding this
  //! @note If value==0 && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(int32_t value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, value);
  }

  //! @brief Writes an 8-byte value to the stream.
  //! @param token A Token
  //! @param value Unsigned 64-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, uint64_t value, uint64_t defaultValue = 0);

  //! @brief Writes an 8-byte value to the stream.
  //! @param token A Token
  //! @param value Unsigned 64-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, uint64_t value, uint32_t defaultValue) {
    return Put(token, value, static_cast<uint64_t>(defaultValue));
  }

  //! @brief Writes an 8-byte value to the stream.
  //! @param token A Token
  //! @param value Unsigned 64-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, uint64_t value, int defaultValue) {
    return Put(token, value, static_cast<uint64_t>(defaultValue));
  }

  //! @brief Writes an 8-byte value to the stream.
  //! @param token A Token
  //! @param value Unsigned 64-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, uint64_t value, long long defaultValue) {
    return Put(token, value, static_cast<uint64_t>(defaultValue));
  }

  //! @brief Writes an 8-byte value to the stream.
  //! @param value Unsigned 64-bit value
  //! @pre You must have written a token immediately preceding this
  //! @note If value==0 && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(uint64_t value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, value);
  }

  //! @brief Writes an 8-byte value to the stream.
  //! @param token A Token
  //! @param value Signed 64-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, int64_t value, int64_t defaultValue = 0);

  //! @brief Writes an 8-byte value to the stream.
  //! @param token A Token
  //! @param value Signed 64-bit value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, int64_t value, int defaultValue) {
    return Put(token, value, static_cast<int64_t>(defaultValue));
  }

  //! @brief Writes an 8-byte value to the stream.
  //! @param value Signed 64-bit value
  //! @pre You must have written a token immediately preceding this
  //! @note If value==0 && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(int64_t value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, value);
  }

  //! @brief Writes an enum value to the stream.
  //! @param token A Token
  //! @param value A value of T.
  //! @param defaultValue A value of T.
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  template<class T, class = typename std::enable_if<std::is_enum<T>::value>::type>
  Writer& Put(Token token, T value, T defaultValue = static_cast<T>(0)) {
    return Put(token,
               static_cast<std::underlying_type_t<T>>(value),
               static_cast<std::underlying_type_t<T>>(defaultValue));
  }

  //! @brief Writes an enum value to the stream.
  //! @param value A value of T.
  //! @pre You must have written a token immediately preceding this
  //! @note If value==0 && trimDefaults==true, nothing will be written to the stream.
  template<class T, class = typename std::enable_if<std::is_enum<T>::value, Writer&>::type>
  typename std::enable_if<!std::is_same<uint64_t, std::underlying_type_t<T>>::value, Writer&>::type
  operator<<(T& value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, static_cast<std::underlying_type_t<T>>(value));
  }

  //! @brief Writes an enum value to the stream or a token. If you did not write a token and this enum has an underlying type of uint64_t, this will be treated as a PutToken
  //! @param value A value of T.
  //! @pre You must have written a token immediately preceding this, unless this is a token enum with an underlying type of uint64_t
  //! @note If value==0 && trimDefaults==true, nothing will be written to the stream.
  template<class T, class = typename std::enable_if<std::is_enum<T>::value, Writer&>::type>
  typename std::enable_if<std::is_same<uint64_t, std::underlying_type_t<T>>::value, Writer&>::type
  operator<<(T& value) {
    if (!m_nextToken.IsValid()) {
      return PutToken(value);
    }
    return Put(static_cast<std::underlying_type_t<T>>(value));
  }

  //! @brief Writes a 4-byte float to the stream.
  //! @param token A Token
  //! @param value Any 4-byte float value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, float value, float defaultValue = 0.f);

  //! @brief Writes a 4-byte float to the stream.
  //! @param value Any 4-byte float value
  //! @pre You must have written a token immediately preceding this
  //! @note If value==0.0f && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(float value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, value);
  }

  //! @brief Writes an 8-byte double to the stream.
  //! @param token A Token
  //! @param value Any 8-byte double value
  //! @param defaultValue Default Value
  //! @note If value==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, double value, double defaultValue = 0.0);

  //! @brief Writes an 8-byte double to the stream.
  //! @param value Any 8-byte double value
  //! @pre You must have written a token immediately preceding this
  //! @note If value==0.0 && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(double value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, value);
  }

  //! @brief Writes a string to the stream.
  //! @param token A Token
  //! @param str nullptr or a pointer to a 0-terminated string.
  //! @param defaultValue nullptr or a pointer to a 0-terminated string.
  //! @note If str==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, const char* str, const char* defaultValue = nullptr);

  //! @brief Writes a string to the stream.
  //! @param str nullptr or a pointer to a 0-terminated string.
  //! @pre You must have written a token immediately preceding this
  //! @note If str==nullptr or strlen(str)==0 && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(const char* str) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, str);
  }

  //! @brief Writes a wide string to the stream.
  //! @param token A Token
  //! @param str nullptr or a pointer to a 0-terminated string.
  //! @param defaultValue nullptr or a pointer to a 0-terminated string.
  //! @note If str==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, const wchar_t* str, const wchar_t* defaultValue = nullptr);

  //! @brief Writes a wide string to the stream.
  //! @param str nullptr or a pointer to a 0-terminated string.
  //! @pre You must have written a token immediately preceding this
  //! @note If str==nullptr or strlen(str)==0 && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(const wchar_t* str) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, str);
  }

  //! @brief Writes a string to the stream.
  //! @param token A Token
  //! @param str An std::string.
  //! @param defaultValue nullptr or a pointer to a 0-terminated string.
  //! @note If str==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, const std::string& str, const char* defaultValue = nullptr) {
    return Put(token, str.c_str(), defaultValue);
  }

  //! @brief Writes a wstring to the stream.
  //! @param token A Token
  //! @param str An std::wstring.
  //! @param defaultValue nullptr or a pointer to a 0-terminated string.
  //! @note If str==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, const std::wstring& str, const wchar_t* defaultValue = nullptr) {
    return Put(token, str.c_str(), defaultValue);
  }

  //! @brief Writes a string to the stream.
  //! @param token A Token
  //! @param str An std::string.
  //! @param defaultValue nullptr or a pointer to a 0-terminated string.
  //! @note If str==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, const std::string& str, const std::string& defaultValue) {
    return Put(token, str.c_str(), defaultValue.c_str());
  }

  //! @brief Writes a wstring to the stream.
  //! @param token A Token
  //! @param str An std::wstring.
  //! @param defaultValue nullptr or a pointer to a 0-terminated string.
  //! @note If str==defaultValue && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, const std::wstring& str, const std::wstring& defaultValue) {
    return Put(token, str.c_str(), defaultValue.c_str());
  }

  //! @brief Writes a string to the stream.
  //! @param str An std::string.
  //! @pre You must have written a token immediately preceding this
  //! @note If str is empty && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(const std::string& str) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, str);
  }

  //! @brief Writes a wstring to the stream.
  //! @param str An std::wstring.
  //! @pre You must have written a token immediately preceding this
  //! @note If str is empty && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(const std::wstring& str) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, str);
  }

  //! @brief Writes a binary string to the stream.
  //! @param token A Token
  //! @param block Pointer to a chunk of binary data.
  //! @param len Length of the binary data
  //! @pre len==0 || block!=nullptr
  //! @note If len==0 && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, const void* block, uint64_t len);

  //! @brief Writes a binary string to the stream.
  //! @param token A Token
  //! @param block A vector of uint8_t.
  //! @pre block must be either empty or contain a string of bytes.
  //! @note If block is empty && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, const Binary& block) {
    return Put(token, block.data(), block.size());
  }

  //! @brief Writes a binary string to the stream.
  //! @param block A vector of uint8_t.
  //! @pre You must have written a token immediately preceding this
  //! @note If block is empty && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(const Binary& block) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, block);
  }

  //! @brief Copies data from a std::istream to the stream.
  //! @param token A Token
  //! @param stream A std::ostream
  //! @note If stream is empty && trimDefaults==true, nothing will be written to the TokenStream.
  Writer& Put(Token token, std::istream& stream);
  Writer& Put(Token token, std::istream&& stream) {
    return Put(token, stream);
  }

  //! @brief Copies data from a MemoryWriter to the stream.
  //! @param token A Token
  //! @param memoryWriter A MemoryWriter
  //! @note If memoryWriter is empty && trimDefaults==true, nothing will be written to the TokenStream.
  Writer& Put(Token token, const MemoryWriter& memoryWriter);
  Writer& Put(Token token, const MemoryWriter&& memoryWriter) {
    return Put(token, memoryWriter);
  }

  //@{
  //! @brief Writes a serializable object to the stream.
  //! @param token A Token
  //! @param object Reference to a Serializable object.
  //! @param keepStubOnEmpty If true, honor trimDefaults while writing the object, but keep the 0-byte stub if the result is empty. We need these as placeholders in vectors.
  //! @note If !object && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token, const Serializable& object, bool keepStubOnEmpty = false);
  Writer& Put(Token token, const Serializable&& object, bool keepStubOnEmpty = false) {
    return Put(token, object, keepStubOnEmpty);
  }

  //@{
  //! @brief Writes an object to the stream using its Helper.
  //! @param token A Token
  //! @param object Reference to an object that has a Helper.
  //! @param keepStubOnEmpty If true, honor trimDefaults while writing the object, but keep the 0-byte stub if the result is empty. We need these as placeholders in vectors.
  //! @note If !object && trimDefaults==true, nothing will be written to the stream.

  template<typename T>
  typename std::enable_if<has_object_writer_helper<T>::value, Writer&>::type
  Put(Token token, const T& object, bool keepStubOnEmpty = false);

  template<typename T>
  typename std::enable_if<has_object_writer_helper<T>::value, Writer&>::type
  Put(Token token, const T&& object, bool keepStubOnEmpty = false) {
    return Put(token, object, keepStubOnEmpty);
  }

  template<typename T>
  typename std::enable_if<has_object_writer_helper<T>::value, Writer&>::type
  operator<<(const T& object) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, object);
  }
  //@}

  //@{
  //! @brief Writes a single-value object to the stream using its Helper.
  //! @param token A Token
  //! @param object Reference to an object that has a Helper with a single-value writer.
  //! @param defaultValue Skip writing if a defaultValue is not nullptr and object matches default.
  //! @note If !object && trimDefaults==true, nothing will be written to the stream.

  template<typename T>
  typename std::enable_if<has_value_writer_helper<T>::value, Writer&>::type
  Put(Token token, const T& object, const T* defaultValue = nullptr) {
    PutToken(token);
    Helper<T>::WriteSingleValue(object, this, defaultValue);
    return *this;
  }

  template<typename T>
  typename std::enable_if<has_value_writer_helper<T>::value, Writer&>::type
  Put(Token token, const T&& object, const T* defaultValue = nullptr) {
    return Put(token, object, defaultValue);
  }

  template<typename T>
  typename std::enable_if<has_value_writer_helper<T>::value, Writer&>::type
  operator<<(const T& object) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, object);
  }
  //@}

  //@{
  //! @brief Writes an object to the stream using its Helper.
  //! @param token A Token
  //! @param object Reference to an object that has a Helper.
  //! @param keepStubOnEmpty If true, honor trimDefaults while writing the object, but keep the 0-byte stub if the result is empty. We need these as placeholders in vectors.
  //! @note If !object && trimDefaults==true, nothing will be written to the stream.
  template<typename T>
  typename std::enable_if<has_write_to_token_stream_method<T>::value, Writer&>::type
  Put(Token token, const T& object, bool keepStubOnEmpty = false);

  template<typename T>
  typename std::enable_if<has_write_to_token_stream_method<T>::value, Writer&>::type
  Put(Token token, const T&& object, bool keepStubOnEmpty = false) {
    return Put(token, object, keepStubOnEmpty);
  }

  template<typename T>
  typename std::enable_if<has_write_to_token_stream_method<T>::value, Writer&>::type
  operator<<(const T& object) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, object);
  }
  //@}

  //! @brief Writes a serializable object to the stream.
  //! @param object Reference to a Serializable object.
  //! @pre You must have written a token immediately preceding this
  //! @note If !object && trimDefaults==true, nothing will be written to the stream.
  Writer& operator<<(const Serializable& object) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, object);
  }

  //! @brief Writes a std::pair to the stream
  //! @param token A Token
  //! @param object The std::pair. Both members must be serializable.
  //! @param keepStubOnEmpty If true, honor trimDefaults while writing the object, but keep the 0-byte stub if the result is empty. We need these as placeholders in vectors.
  //! @note If !object && trimDefaults==true, nothing will be written to the stream.
  template<typename First, typename Second>
  Writer& Put(Token token, const std::pair<First, Second>& object, bool keepStubOnEmpty = false);

  //! @brief Writes a std::pair to the stream.
  //! @param object The std::pair. Both members must be serializable.
  //! @pre You must have written a token immediately preceding this
  //! @note If !object && trimDefaults==true, nothing will be written to the stream.
  template<typename First, typename Second>
  Writer& operator<<(const std::pair<First, Second>& object) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, object);
  }

  //! @brief Writes a serializable object to the stream.
  //! @param token A Token
  //! @param object Reference to a Serializable object.
  //! @param tokenMap TokenMap to use for serialization
  //! @param keepStubOnEmpty If true, honor trimDefaults while writing the object, but keep the 0-byte stub if the result is empty. We need these as placeholders in vectors.
  //! @note If !object && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(Token token,
              const Serializable& object,
              const TokenMap& tokenMap,
              bool keepStubOnEmpty = false);

  //! @brief Writes a serializable object to the stream.
  //! @param object Reference to a Serializable object.
  //! @param tokenMap TokenMap to use for serialization
  //! @param keepStubOnEmpty If true, honor trimDefaults while writing the object, but keep the 0-byte stub if the result is empty. We need these as placeholders in vectors.
  //! @note If !object && trimDefaults==true, nothing will be written to the stream.
  Writer& Put(const Serializable& object, const TokenMap& tokenMap, bool keepStubOnEmpty = false) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, object, tokenMap, keepStubOnEmpty);
  }

  //! @brief Writes a container size hint that can be used by the reader to preallocate the following container.
  //! @param token A Token
  //! @param size Size of the next container.
  //! @note If size is 0 or 1, nothing will be written to the stream.
  Writer& PutContainerElementCount(Token token, uint64_t size);

  //! @brief Writes a vector of serializable objects to the stream.
  //! @param token A Token
  //! @param objects A vector of serializable objects.
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<template<typename, typename, typename...> class Map,
           typename Key,
           typename Value,
           typename... Params>
  Writer& PutMap(Token token, const Map<Key, Value, Params...>& objects) {
    auto size = objects.size();
    if (size) {
      PutContainerElementCount(token, size);
      for (const auto& i : objects) {
        Put(token, i, true);
      }
    } else if (!m_trimDefaults) {
      PutData(token);
    }
    m_nextToken.Clear();
    return *this;
  }

  //! @brief Writes a vector of serializable objects to the stream.
  //! @param token A Token
  //! @param objects A vector of serializable objects.
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<template<typename, typename...> class Container, typename T, typename... Params>
  typename std::enable_if<has_custom_writer<T>::value, Writer&>::type
  PutContainer(Token token, const Container<T, Params...>& objects) {
    auto size = objects.size();
    if (size) {
      PutContainerElementCount(token, size);
      for (const auto& i : objects) {
        Put(token, i, true);
      }
    } else if (!m_trimDefaults) {
      PutData(token);
    }
    m_nextToken.Clear();
    return *this;
  }

  //! @brief Writes a vector to the stream.
  //! @param token A Token
  //! @param items A vector of normal data (not Serializable objects).
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<template<typename, typename...> class Container, typename T, typename... Params>
  typename std::enable_if<!has_custom_writer<T>::value, Writer&>::type
  PutContainer(Token token, const Container<T, Params...>& items) {
    auto size = items.size();
    if (size) {
      TrimDefault state(*this, false);
      PutContainerElementCount(token, size);
      for (const auto& i : items) {
        Put(token, i);
      }
    } else if (!m_trimDefaults) {
      PutData(token);
    }
    m_nextToken.Clear();
    return *this;
  }

  //! @brief Writes a vector of serializable objects to the stream.
  //! @param token A Token
  //! @param objects A vector of serializable objects.
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<typename T>
  Writer& Put(Token token, const std::vector<T>& objects) {
    return PutContainer<std::vector, T>(token, objects);
  }

  //! @brief Writes a vector to the stream.
  //! @param itemsOrObjects A vector of normal data or Serializable objects.
  //! @pre You must have written a token immediately preceding this
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<typename T>
  Writer& operator<<(const std::vector<T>& itemsOrObjects) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, itemsOrObjects);
  }

  //! @brief Writes a list of serializable objects to the stream.
  //! @param token Token to write.
  //! @param objects A list of serializable objects.
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<typename T>
  Writer& Put(Token token, const std::list<T>& objects) {
    return PutContainer<std::list, T>(token, objects);
  }

  //! @brief Writes a list to the stream.
  //! @param itemsOrObjects A list of normal data or Serializable objects.
  //! @pre You must have written a token immediately preceding this
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<typename T>
  Writer& operator<<(const std::list<T>& itemsOrObjects) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, itemsOrObjects);
  }

  //! @brief Writes a list of serializable objects to the stream.
  //! @param token Token to write.
  //! @param objects A list of serializable objects.
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<typename T>
  Writer& Put(Token token, const std::set<T>& objects) {
    return PutContainer<std::set, T>(token, objects);
  }

  //! @brief Writes a list to the stream.
  //! @param itemsOrObjects A list of normal data or Serializable objects.
  //! @pre You must have written a token immediately preceding this
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<typename T>
  Writer& operator<<(const std::set<T>& itemsOrObjects) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, itemsOrObjects);
  }

  //! @brief Writes a list of serializable objects to the stream.
  //! @param token Token to write.
  //! @param objects A list of serializable objects.
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<typename T>
  Writer& Put(Token token, const std::unordered_set<T>& objects) {
    return PutContainer<std::unordered_set, T>(token, objects);
  }

  //! @brief Writes a list to the stream.
  //! @param itemsOrObjects A list of normal data or Serializable objects.
  //! @pre You must have written a token immediately preceding this
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<typename T>
  Writer& operator<<(const std::unordered_set<T>& itemsOrObjects) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, itemsOrObjects);
  }

  //! @brief Writes a set of serializable objects to the stream.
  //! @param token A Token
  //! @param objects A set of serializable objects.
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<typename Key, typename T>
  Writer& Put(Token token, const std::map<Key, T>& objects) {
    return PutMap<std::map, Key, T>(token, objects);
  }

  //! @brief Writes a set to the stream.
  //! @param itemsOrObjects A set of normal data or Serializable objects.
  //! @pre You must have written a token immediately preceding this
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<typename Key, typename T>
  Writer& operator<<(const std::map<Key, T>& itemsOrObjects) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, itemsOrObjects);
  }

  //! @brief Writes a set of serializable objects to the stream.
  //! @param token A Token
  //! @param objects A set of serializable objects.
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<typename Key, typename T>
  Writer& Put(Token token, const std::unordered_map<Key, T>& objects) {
    return PutMap<std::unordered_map, Key, T>(token, objects);
  }

  //! @brief Writes a set to the stream.
  //! @param itemsOrObjects A set of normal data or Serializable objects.
  //! @pre You must have written a token immediately preceding this
  //! @note If objects.size() == 0 && trimDefaults==true, nothing will be written to the stream.
  template<typename Key, typename T>
  Writer& operator<<(const std::unordered_map<Key, T>& itemsOrObjects) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, itemsOrObjects);
  }

  //! @brief Use this helper to specify a default value into a stream
  //! @param value Use ValueWithDefault() to make this value/default pair
  //! @pre You must have written a token immediately preceding this
  //! @note If value.m_value == value.m_default && trimDefaults==true, nothing will be written to the stream.
  template<typename T>
  Writer& operator<<(ValueWithDefaultStruct<T>&& value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, value.m_value, value.m_default);
  }

  //! @brief Use this helper to specify a default value into a stream
  //! @param value Use ValueWithDefault() to make this value/default pair
  //! @pre You must have written a token immediately preceding this
  //! @note If value.m_value == value.m_default && trimDefaults==true, nothing will be written to the stream.
  template<typename T>
  Writer& operator<<(StringValueWithDefaultStruct<T>&& value) {
    ASSERT_TOKEN_SET();
    return Put(m_nextToken, value.m_value, value.m_default);
  }

  size_t GetLength() const {
    return static_cast<size_t>(m_stream->tellp());
  }

  //! @brief Used to switch the TrimDefault state of a stream.
  //! This will allow you to change the state for the life of the TrimDefault object.
  class TrimDefault { // NOLINT
   public:
    //! @brief Temporarily change TrimDefault state of \p writer
    //! @param writer A \e Writer to switch. The constructor will remember the old state.
    //! @param skip \e true to skip outputting empty items, \e false to output zero length items
    TrimDefault(Writer& writer, bool skip) : m_writer{writer}, m_oldValue{writer.m_trimDefaults} {
      writer.m_trimDefaults = skip;
    }

    // No copying
    TrimDefault(const TrimDefault&) = delete;
    TrimDefault& operator=(const TrimDefault&) = delete;

    //! Old state will be restored
    ~TrimDefault() {
      m_writer.m_trimDefaults = m_oldValue;
    }

    //! Changes the value of the trim default state
    void Trim(bool skip) const {
      m_writer.m_trimDefaults = skip;
    }

   private:
    Writer& m_writer;
    bool m_oldValue;
  };

 private:
  void PutData(Token t) {
    PutData(t, nullptr, 0);
  }
  void PutData(Token t,
               const void* data,
               uint64_t len,
               bool removeLeadingZeros = false,
               bool handleExtendedSign = false);
  void PutDataHeader(Token t, uint64_t len);
  void WriteLengthEncoded(uint64_t value);

  std::ostream* m_stream;
  Token m_nextToken;
  bool m_trimDefaults = true;
  bool m_badStream = false;

  Token m_containerToken;
  uint64_t m_containerElementCount = 0;
  uint64_t m_containerElementIndex = 0;
};

/*! @brief Writes objects into a TokenStream using an internal memory buffer, with minimal allocations
     *
     * @code
     * void PersonalRecord::Write(Writer& writer)
     * {
     *   writer << Token(BirthMonthToken) << m_birthMonth;
     *   writer << Token(BirthDayToken) << m_birthDay;
     *   writer << Token(BirthYearToken) << m_birthYear;
     *   writer << Token(SocialSecurityNumberToken) << m_social;
     *   writer << Token(FirstNameToken) << m_firstName;
     *   writer << Token(LastNameToken) << m_lastName;
     * }
     * @endcode
     *
     * @warning By default, he \e Put methods will not write anything if the data being
     * written is the default value. For instance, PutByte(someToken, 0) will not
     * modify the stream. This is done to keep the stream as small as possible.
     * As a result, \b you \b must \b clear all members of your object before
     * using Reader to retrieve their values.
     *
     * @note You can now change trimDefaults to \e false to disable that functionality.
     *
     * @see Token
     * @see Serializable
     * @see Writer
     * @see Binary
     * @see Reader
     * @see Reader::SubStream
     */
class MemoryWriter : public Writer {
 public:
  //! @brief Creates MemoryWriter that will output to an internal memory buffer.
  //! @param trimDefaults If \e true, default values will not be written. If \e false, tokens with 0-len will be written for default values.
  explicit MemoryWriter(bool trimDefaults = true) : Writer{m_internalStream, trimDefaults} {}

  //! @brief Creates MemoryWriter that will output to an internal memory buffer.
  //! @param writer Inherit parameters from other writer.
  explicit MemoryWriter(const Writer& writer) : Writer{m_internalStream, writer} {}

  //! @brief Creates MemoryWriter that will output to an internal memory buffer.
  //! @param writer Inherit parameters from other writer.
  //! @param trimDefaults If \e true, default values will not be written. If \e false, tokens with 0-len will be written for default values.
  explicit MemoryWriter(const Writer& writer, bool trimDefaults) :
      Writer{m_internalStream, writer, trimDefaults} {}

  //! @brief Get an AZ::IO::Reader to access the stream.
  std::stringstream GetReader() const {
    std::stringstream tmp;
    tmp << m_internalStream.str();
    return tmp;
  }

 private:
  std::stringstream m_internalStream;
};

inline Writer& Writer::Put(Token token, const MemoryWriter& memoryWriter) {
  return Put(token, memoryWriter.GetReader());
}

template<typename First, typename Second>
Writer& Writer::Put(Token token, const std::pair<First, Second>& object, bool keepStubOnEmpty) {
  // ReSharper disable once CppClassIsIncomplete
  MemoryWriter subWriter{*this, m_trimDefaults};
  subWriter.Put(0, object.first);
  subWriter.Put(1, object.second);
  TrimDefault handleStub{*this, m_trimDefaults && !keepStubOnEmpty};
  Put(token, subWriter);
  return *this;
}

template<typename T>
typename std::enable_if<Writer::has_object_writer_helper<T>::value, Writer&>::type
Writer::Put(Token token, const T& object, bool keepStubOnEmpty) {
  // ReSharper disable once CppClassIsIncomplete
  MemoryWriter subWriter{*this, m_trimDefaults};
  Helper<T>::Write(object, subWriter);
  TrimDefault handleStub{*this, m_trimDefaults && !keepStubOnEmpty};
  Put(token, subWriter);
  return *this;
}

template<typename T>
typename std::enable_if<Writer::has_write_to_token_stream_method<T>::value, Writer&>::type
Writer::Put(Token token, const T& object, bool keepStubOnEmpty) {
  // ReSharper disable once CppClassIsIncomplete
  MemoryWriter subWriter{*this, m_trimDefaults};
  object.WriteToTokenStream(subWriter);
  TrimDefault handleStub{*this, m_trimDefaults && !keepStubOnEmpty};
  Put(token, subWriter);
  return *this;
}

} // namespace TokenStream

#undef ASSERT_TOKEN_SET
