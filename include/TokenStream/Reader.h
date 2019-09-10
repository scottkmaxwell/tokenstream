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
 *  Contains the Reader class definition.
 *  Reader is meant to be used in conjunction with
 *  Writer and Reader::SubStream to restore objects'
 *  persistent data.
 */

#include <TokenStream/TokenStream.h>
#include <functional>
#include <istream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define VERIFY_TOKENSTREAM(x, on_fail)                                                             \
  do {                                                                                             \
    if (!(x)) {                                                                                    \
      TS_ASSERT(x, "Invalid TokenStream");                                                         \
      m_lastToken = Token::InvalidTokenValue;                                                      \
      m_badStream = true;                                                                          \
      return on_fail;                                                                              \
    }                                                                                              \
  } while (false) // NOLINT

namespace TokenStream {

/*! @brief Reads an object stored as a TokenStream by Writer
    *
    * Typically you will create a loop in which you read a token,
    * then \e stream the data associated with that token.
    * @code
    *   AZ::IO::MemoryStream readerInput(start,end);
    *   Reader reader{ readerInput };
    *   while (!reader.EOS()) {
    *       switch (reader.GetToken<Token>()) {
    *           case Token::FirstName:
    *               reader >> m_firstName;
    *               break;
    *           case Token::LastName:
    *               reader >> m_lastName;
    *               break;
    *           case Token::BirthMonth:
    *               reader >> m_birthMonth;
    *               break;
    *           case Token::BirthDay:
    *               reader >> m_birthDay;
    *               break;
    *           case Token::BirthYear:
    *               reader >> m_birthYear;
    *               break;
    *           case Token::SocialSecurityNumber:
    *               reader >> m_social;
    *               break;
    *       }
    *   }
    * @endcode
    *
    * @par
    * \b Warning: By default, the \e Writer methods will not write anything if the data being
    * written is the default value. For instance, Writer::PutByte(someToken, 0) will not
    * modify the stream. This is done to keep the stream as small as possible.
    * As a result, \b you \b must \b clear all members of your object before
    * using Reader to retrieve their values.
    *
    * @note You can turn disable the "trim default" feature of Writer if desired.
    *
    * @see Token
    * @see Reader::SubStream
    * @see Binary
    * @see Writer
    */
class Reader { // NOLINT
  // This data is saved and restored by the SubStream class to handle context narrowing and restoring
  struct SubStreamContext {
    Token m_containerToken;
    size_t m_containerElementCount = 0;
    size_t m_containerElementIndex = 0;
    size_t m_containerElementEnd = 0;
    size_t m_end;
    explicit SubStreamContext(size_t end = 0) : m_end(end) {}
  };

 public:
  template<typename>
  struct has_reserve_method_Void {
    typedef void type;
  };
  template<typename T, typename Sfinae = void>
  struct has_reserve_method : std::false_type {};
  template<typename T>
  struct has_reserve_method<T,
                            typename has_reserve_method_Void<decltype(std::declval<T&>().reserve(
                                *static_cast<typename T::size_type*>(nullptr)))>::type>
      : std::true_type {};

  template<typename>
  struct has_emplace_back_method_Void {
    typedef void type;
  };
  template<typename T, typename Sfinae = void>
  struct has_emplace_back_method : std::false_type {};
  template<typename T>
  struct has_emplace_back_method<
      T,
      typename has_emplace_back_method_Void<decltype(std::declval<T&>().emplace_back())>::type>
      : std::true_type {};

  // Find out whether T has `void ReadFromTokenStream(Reader&)` method or a Helper<T> with a Read method
  template<typename>
  struct has_read_from_token_stream_method_Void {
    typedef void type;
  };
  template<typename>
  struct has_object_reader_helper_Void {
    typedef void type;
  };
  template<typename>
  struct has_value_reader_helper_Void {
    typedef void type;
  };
  template<typename T, typename Sfinae = void>
  struct has_read_from_token_stream_method : std::false_type {};
  template<typename T>
  struct has_read_from_token_stream_method<
      T,
      typename has_read_from_token_stream_method_Void<
          decltype(std::declval<T&>().ReadFromTokenStream(*static_cast<Reader*>(nullptr)))>::type>
      : std::true_type {};
  template<typename T, typename Sfinae = void>
  struct has_object_reader_helper : std::false_type {};
  template<typename T>
  struct has_object_reader_helper<
      T,
      typename has_object_reader_helper_Void<decltype(Helper<T>::Read(
          *static_cast<T*>(nullptr), *static_cast<Reader*>(nullptr)))>::type> : std::true_type {};
  template<typename T, typename Sfinae = void>
  struct has_value_reader_helper : std::false_type {};
  template<typename T>
  struct has_value_reader_helper<
      T,
      typename has_value_reader_helper_Void<decltype(Helper<T>::ReadSingleValue(
          *static_cast<T*>(nullptr), *static_cast<Reader*>(nullptr)))>::type> : std::true_type {};

  //! @brief Creates Reader that will operate on the specified stream
  //! @param stream A generic stream that contains the binary data to parse
  //! std::istream is a base class that you can override to stream data into the Reader.
  //! If you simply want to read data from a buffer, use the AZ::IO::MemoryStream class.
  explicit Reader(std::istream& stream);

  //! @brief Do not allow move semantics for the stream. We need it to stick around externally.
  explicit Reader(std::istream&&) = delete;

  // no copying
  Reader(const Reader&) = delete;
  Reader& operator=(const Reader&) = delete;

  //! @brief Retrieves the next token and updates the stream pointer
  //! @post Stream pointer will be updated.
  Token GetToken();

  //! @brief Retrieves the next token and updates the stream pointer
  //! @post Stream pointer will be updated.
  template<typename T>
  T GetToken() {
    const auto t = GetToken();
    return static_cast<T>(static_cast<uint64_t>(t));
  }

  //! @brief Retrieves the most recent token and updates the stream pointer
  Token GetLastToken() const {
    return m_lastToken;
  }

  //! @brief Retrieves the most recent token and updates the stream pointer
  void PushLastToken() {
    m_tokenPushed = true;
  }

  //@{
  //! @brief Retrieves a single-byte value.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  bool GetBool() {
    return GetByte() == 1;
  }
  Reader& operator>>(bool& rhs) {
    rhs = GetByte() == 1;
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves a single-byte value.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  int8_t GetByte();
  uint8_t GetUByte();
  Reader& operator>>(int8_t& rhs) {
    rhs = GetByte();
    return *this;
  }
  Reader& operator>>(uint8_t& rhs) {
    rhs = GetUByte();
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves a 2-byte value.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  int16_t GetWord();
  uint16_t GetUWord();
  Reader& operator>>(int16_t& rhs) {
    rhs = GetWord();
    return *this;
  }
  Reader& operator>>(uint16_t& rhs) {
    rhs = GetUWord();
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves a 4-byte value.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  int32_t GetLong();
  uint32_t GetULong();
  Reader& operator>>(int32_t& rhs) {
    rhs = GetLong();
    return *this;
  }
  Reader& operator>>(uint32_t& rhs) {
    rhs = GetULong();
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves an 8-byte value.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  int64_t GetLongLong();
  uint64_t GetULongLong();
  Reader& operator>>(int64_t& rhs) {
    rhs = GetLongLong();
    return *this;
  }
  Reader& operator>>(uint64_t& rhs) {
    rhs = GetULongLong();
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves an 4-byte float.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  float GetFloat();
  Reader& operator>>(float& rhs) {
    rhs = GetFloat();
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves an 8-byte double.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  double GetDouble();
  Reader& operator>>(double& rhs) {
    rhs = GetDouble();
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves an enum value.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<class T, class = typename std::enable_if<std::is_enum<T>::value>::type>
  T GetEnum() {
    std::underlying_type_t<T> temp;
    this >> temp;
    return static_cast<T>(temp);
  }
  template<class T, class = typename std::enable_if<std::is_enum<T>::value>::type>
  Reader& operator>>(T& rhs) {
    std::underlying_type_t<T> temp;
    *this >> temp;
    rhs = static_cast<T>(temp);
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves a Serializable object.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  void GetSerializable(Serializable& object);

  Reader& operator>>(Serializable& object) {
    GetSerializable(object);
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves an object using its ReadFromTokenStream method.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<typename T>
  typename std::enable_if<has_read_from_token_stream_method<T>::value, void>::type
  GetWithReadFromTokenStreamMethod(T& object) {
    if (!m_offset) {
      m_remainingInElement = DecodeLength();
      if (m_badStream) {
        return;
      }
    }
    SubStream sub{*this};
    object.ReadFromTokenStream(*this);
  }

  template<class T>
  typename std::enable_if<has_read_from_token_stream_method<T>::value, Reader&>::type
  operator>>(T& object) {
    GetWithReadFromTokenStreamMethod(object);
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves an object using a Helper.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<typename T>
  typename std::enable_if<has_object_reader_helper<T>::value, void>::type GetWithHelper(T& object) {
    if (!m_offset) {
      m_remainingInElement = DecodeLength();
      if (m_badStream) {
        return;
      }
    }
    SubStream sub{*this};
    Helper<T>::Read(object, *this);
  }

  template<class T>
  typename std::enable_if<has_object_reader_helper<T>::value, Reader&>::type operator>>(T& object) {
    GetWithHelper(object);
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves an object using a Helper.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<typename T>
  typename std::enable_if<has_value_reader_helper<T>::value, void>::type GetWithHelper(T& object) {
    Helper<T>::ReadSingleValue(object, *this);
  }

  template<class T>
  typename std::enable_if<has_value_reader_helper<T>::value, Reader&>::type operator>>(T& object) {
    GetWithHelper(object);
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves a Serializable object.
  //! @param object Object to read into
  //! @param tokenMap TokenMap to use for read
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  void GetSerializable(Serializable& object, const TokenMap& tokenMap);
  //@}

  //! @brief Retrieves a vector of values.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<template<typename, typename...> class Container, typename T, typename... Params>
  typename std::enable_if<has_reserve_method<Container<T, Params...>>::value &&
                              has_emplace_back_method<Container<T, Params...>>::value,
                          void>::type
  GetContainer(Container<T, Params...>& vec) {
    const auto containerToken = m_lastToken;
    if (m_nextContainerElementCount) {
      vec.reserve(static_cast<size_t>(m_nextContainerElementCount));
    }
    do {
      vec.emplace_back();
      T& item = vec.back();
      *this >> item;
      if (EOS()) {
        return;
      }
    } while (GetToken() == containerToken);
    PushLastToken();
  }

  //! @brief Retrieves a vector of values.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<template<typename, typename...> class Container, typename T, typename... Params>
  typename std::enable_if<!has_reserve_method<Container<T, Params...>>::value &&
                              has_emplace_back_method<Container<T, Params...>>::value,
                          void>::type
  GetContainer(Container<T, Params...>& vec) {
    const auto containerToken = m_lastToken;
    do {
      vec.emplace_back();
      T& item = vec.back();
      *this >> item;
      if (EOS()) {
        return;
      }
    } while (GetToken() == containerToken);
    PushLastToken();
  }

  //! @brief Retrieves a vector of values.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<template<typename, typename...> class Container, typename T, typename... Params>
  typename std::enable_if<has_reserve_method<Container<T, Params...>>::value &&
                              !has_emplace_back_method<Container<T, Params...>>::value,
                          void>::type
  GetContainer(Container<T, Params...>& vec) {
    const auto containerToken = m_lastToken;
    if (m_nextContainerElementCount) {
      vec.reserve(static_cast<size_t>(m_nextContainerElementCount));
    }
    do {
      T item;
      *this >> item;
      vec.insert(item);
      if (EOS()) {
        return;
      }
    } while (GetToken() == containerToken);
    PushLastToken();
  }

  //! @brief Retrieves a vector of values.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<template<typename, typename...> class Container, typename T, typename... Params>
  typename std::enable_if<!has_reserve_method<Container<T, Params...>>::value &&
                              !has_emplace_back_method<Container<T, Params...>>::value,
                          void>::type
  GetContainer(Container<T, Params...>& vec) {
    const auto containerToken = m_lastToken;
    do {
      T item;
      *this >> item;
      vec.insert(item);
      if (EOS()) {
        return;
      }
    } while (GetToken() == containerToken);
    PushLastToken();
  }

  //@{
  //! @brief Retrieves a vector of values.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<class T, typename... Params>
  void GetContainer(std::vector<T, Params...>& vec) {
    GetContainer<std::vector, T, Params...>(vec);
  }
  template<class T, typename... Params>
  Reader& operator>>(std::vector<T, Params...>& vec) {
    GetContainer<std::vector, T, Params...>(vec);
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves a list of values.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<class T, typename... Params>
  void GetContainer(std::list<T, Params...>& lst) {
    GetContainer<std::list, T, Params...>(lst);
  }
  template<class T, typename... Params>
  Reader& operator>>(std::list<T, Params...>& lst) {
    GetContainer<std::list, T, Params...>(lst);
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves a set of values.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<class T, typename... Params>
  void GetContainer(std::set<T, Params...>& lst) {
    GetContainer<std::set, T, Params...>(lst);
  }
  template<class T, typename... Params>
  Reader& operator>>(std::set<T, Params...>& lst) {
    GetContainer<std::set, T, Params...>(lst);
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves a set of values.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<class T, typename... Params>
  void GetContainer(std::unordered_set<T, Params...>& lst) {
    GetContainer<std::unordered_set, T, Params...>(lst);
  }
  template<class T, typename... Params>
  Reader& operator>>(std::unordered_set<T, Params...>& lst) {
    GetContainer<std::unordered_set, T, Params...>(lst);
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves a std::pair.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<typename First, typename Second>
  void GetPair(std::pair<First, Second>& pair) {
    if (!m_offset) {
      m_remainingInElement = DecodeLength();
      if (m_badStream) {
        return;
      }
    }
    SubStream sub{*this};
    while (!EOS()) {
      switch (GetToken()) {
        case 0:
          *this >> pair.first;
          break;
        case 1:
          *this >> pair.second;
          break;
        default:
          break;
      }
    }
  }
  template<typename First, typename Second>
  Reader& operator>>(std::pair<First, Second>& pair) {
    GetPair(pair);
    return *this;
  }

  //! @brief Retrieves a vector of values.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<template<typename, typename, typename...> class Map,
           typename Key,
           typename Value,
           typename... Params>
  void GetPairMap(Map<Key, Value, Params...>& container) {
    const auto containerToken = m_lastToken;
    do {
      auto item = std::pair<Key, Value>();
      *this >> item;
      container.emplace(std::move(item));
      if (EOS()) {
        return;
      }
    } while (GetToken() == containerToken);
    PushLastToken();
  }

  //@{
  //! @brief Retrieves a map of values.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<typename Key, typename Value, typename... Params>
  void GetContainer(std::map<Key, Value, Params...>& container) {
    return GetPairMap<std::map, Key, Value, Params...>(container);
  }
  template<typename Key, typename Value, typename... Params>
  Reader& operator>>(std::map<Key, Value, Params...>& container) {
    GetPairMap<std::map, Key, Value, Params...>(container);
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves a map of values.
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  template<typename Key, typename Value, typename... Params>
  void GetContainer(std::unordered_map<Key, Value, Params...>& container) {
    return GetPairMap<std::unordered_map, Key, Value, Params...>(container);
  }
  template<typename Key, typename Value, typename... Params>
  Reader& operator>>(std::unordered_map<Key, Value, Params...>& container) {
    GetPairMap<std::unordered_map, Key, Value, Params...>(container);
    return *this;
  }
  //@}

  //@{
  //! @brief Retrieves a string.
  //! @returns String object containing or referring to data
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  std::string GetString();
  Reader& operator>>(std::string& rhs);
  //@}

  //@{
  //! @brief Retrieves a string.
  //! @returns String object containing or referring to data
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  std::wstring GetWideString();
  Reader& operator>>(std::wstring& rhs);
  //@}

  //@{
  //! @brief Retrieves a block.
  //! @returns Binary object containing or referring to data
  //! @pre EOS() == false
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  Binary GetBlock();
  Reader& operator>>(Binary& rhs) {
    rhs = GetBlock();
    return *this;
  }
  //@}

  //@{
  size_t NextContainerElementCount() const {
    return m_nextContainerElementCount;
  }

  //! @brief Skips the data associated with the token just retrieved
  //! @pre Last method must have been GetToken() or a \b const method.
  //! @post EOS() || stream points to the next token
  void Skip() {
    SkipBytes(m_remainingInElement);
  }

  //! @brief Check to see if we are at or past the end of the stream.
  //! @returns \e true if End Of Stream
  bool EOS() const {
    return m_badStream || (m_offset >= m_context.m_end && !m_tokenPushed);
  }

  //! @brief Check to see if we will be past the end of the stream after processing \e offset bytes.
  //! @param offset Number of bytes past current location to check.
  //! @return \e true if streamPointer+offset past (but not at) the end-of-stream
  bool PastEOS(size_t offset = 0) const {
    return m_context.m_end && m_offset + offset > m_context.m_end;
  }

  //! @brief Used for unit testing only. Returns \e true if exactly at the end-of-stream.
  bool VerifyEOS() const {
    return m_context.m_end && m_offset == m_context.m_end;
  }

  /*! @brief Used to handle nested data. SubStream will modify the EOS indicator for the specified \e Reader until the SubStream goes out of scope.
        *
        * @code
        * void Address::Get(Reader& reader)
        * {
        *   SubStream subStream { reader };
        *   while (!reader.EOS()) {
        *       switch (reader.GetToken()) {
        *           case StreetToken:
        *               reader >> m_street;
        *               break;
        *           case CityToken:
        *               reader >> m_city;
        *               break;
        *           case StateToken:
        *               reader >> m_state;
        *               break;
        *           case ZipToken:
        *               reader >> m_zip;
        *               break;
        *       }
        *   }
        * }
        * @endcode
        *
        * @see Writer::SubStream
        */
  class SubStream { // NOLINT
   public:
    //! @brief Change \p reader to only expose the SubStream
    //! @param reader A \e Reader to substream. The constructor will modify the EOS of \e reader to correspond to the end of this block.
    //! @pre reader.EOS() == false
    //! @pre Last Reader method must have been GetToken() or a \b const method.
    //! @post reader points to first token of substream
    explicit SubStream(Reader& reader);

    // no copying
    SubStream(const SubStream&) = delete;
    SubStream& operator=(const SubStream&) = delete;

    //! @brief Any bytes remaining in SubStream will be skipped and previous EOS pointer will be restored so that the stream can continue reading from the data after this substream.
    ~SubStream();

   private:
    Reader& m_reader;
    SubStreamContext m_oldContext;
  };
  friend class SubStream;

 protected:
  template<typename T>
  T FetchPartial(bool signExtend = false) {
    T result = 0;
    VERIFY_TOKENSTREAM(m_remainingInElement <= sizeof(T), 0);
    auto dest = reinterpret_cast<uint8_t*>(&result) + sizeof result - m_remainingInElement;
    auto fill = static_cast<int>(sizeof result - m_remainingInElement);
    Fetch(dest);
    if (m_badStream) {
      return 0;
    }
    if (signExtend && fill && (*dest & 0x80) == 0x80) {
      do {
        *--dest = 0xff;
        --fill;
      } while (fill);
    }
    return result;
  }

  void Fetch(uint8_t* v);
  uint64_t ReadLengthEncoded(bool forToken);
  uint64_t DecodeToken() {
    return ReadLengthEncoded(true);
  }
  size_t DecodeLength() {
    return static_cast<size_t>(ReadLengthEncoded(false));
  }
  void SkipBytes(size_t bytes);

 private:
  void SkipBytesByReading(size_t bytes);

  std::istream& m_stream;
  size_t m_offset = 0;
  size_t m_remainingInElement = 0;
  size_t m_nextContainerElementCount = 0;
  Token m_lastToken;
  SubStreamContext m_context;

  bool m_tokenPushed = false;
  bool m_badStream = false;
};

} // namespace TokenStream
