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

#include <map>
#include <vector>
// ReSharper disable once CppUnusedIncludeDirective
#include <cassert>
#define TS_ASSERT(condition, message) assert(condition) // NOLINT

namespace TokenStream {
class Reader;
class Writer;
class Serializable;
using Binary = std::vector<uint8_t>;

/** \brief Simple wrapper for an 8-64 bit token
    *
    *		Token is an 8-64 bit value used by the Reader and Writer.
    *		We need to encapsulate the token value in this class so that the Reader will know it is receiving a Token rather than a value.
    *		This class intentionally does *not* use explicit constructors so that we can easily use any numeric or enum type as a Token.
    *		
    *		@see Reader
    *		@see Writer
    */
class Token {
 public:
  //! Invalid token value
  static constexpr uint64_t InvalidTokenValue = 0xffff'ffff'ffff'ffff;

  //! Default constructor creates a token of undefined value
  constexpr Token() = default;

  //! Creates a token of value \e t
  constexpr Token(uint64_t t) : m_token(t) {}

  template<class T, class = typename std::enable_if<std::is_enum<T>::value>::type>
  constexpr Token(T t) : m_token(static_cast<std::underlying_type_t<T>>(t)) {}

  //! Returns the token as a uint64_t
  constexpr operator uint64_t() const {
    return m_token;
  }

  constexpr bool IsValid() const {
    return m_token != InvalidTokenValue;
  }

  void Clear() {
    *this = Token();
  }

 private:
  uint64_t m_token = InvalidTokenValue;
};

//! @brief A helper to package up the getter and setter for a single member. Used with \e TokenMap.
//! @note MemberAccessor is automatically created by the \e MAP_TOKEN and \e ENUMERATED_TOKEN macros.
//! @see MAP_TOKEN
//! @see ENUMERATED_TOKEN
struct MemberAccessor {
  using Getter = void (*)(Reader&, Serializable&);
  using Putter = void (*)(Writer&, const Serializable&);
  Getter Get;
  Putter Put;
};

//! @brief A helper you can define per type to serialize and deserialize externally to the type.
//! @code
//! namespace TokenStream {
//!     template<> struct Helper<DesignDataTest::MathUtil::Point> {
//!         enum class Token {
//!             X = 2,
//!             Y = 3,
//!         };
//!
//!         static void Read(DesignDataTest::MathUtil::Point& o, Reader& reader) {
//!             while (!reader.EOS()) {
//!                 switch (reader.GetToken<Token>()) {
//!                     case Token::X:
//!                         reader >> o.m_x;
//!                         break;
//!                     case Token::Y:
//!                         reader >> o.m_y;
//!                         break;
//!                 }
//!             }
//!         }
//!         static void Write(DesignDataTest::MathUtil::Point& o, Writer& writer) {
//!             writer.Put(Token::X, o.m_x);
//!             writer.Put(Token::Y, o.m_y);
//!         }
//!     };
//!     static_assert(Reader::has_reader_helper<DesignDataTest::MathUtil::Point>::value, "DesignDataTest::MathUtil::Point should have a valid read Helper");
//!     static_assert(Writer::has_writer_helper<DesignDataTest::MathUtil::Point>::value, "DesignDataTest::MathUtil::Point should have a valid write Helper");
//! }
//! @endcode
//! @note It is often a good idea to make the Helper for your type a friend of your type
template<typename T>
struct Helper;

//! @brief A helper used to create the const static token map for a structure. We need this instead of a raw map so that we can merge in parent maps.
//! @note TokenMap is automatically created by the \e TOKEN_MAP and \e ENUMERATED_TOKEN_MAP macros.
//! @see TOKEN_MAP
//! @see MAP_TOKEN
//! @see ENUMERATED_TOKEN
class TokenMap : public std::map<uint64_t, MemberAccessor> {
 public:
  TokenMap() = default;
  TokenMap(const TokenMap& parent, std::initializer_list<value_type> initializer) :
      map<uint64_t, MemberAccessor>(initializer) {
#ifdef AZ_ENABLE_TRACING
    for (auto item : parent) {
      TS_ASSERT(find(item.first) == end(), "Duplicate token found in parent's TokenMap");
    }
#endif
    insert(parent.begin(), parent.end());
  }
  TokenMap(std::initializer_list<value_type> initializer) :
      map<uint64_t, MemberAccessor>(initializer) {}
};

//! @brief Any type that needs to be serialized must derive from this. You can then either provide your own \e Read and \e Write methods, or define a token map.
class Serializable // NOLINT
{
 public:
  virtual ~Serializable() = default;

  //! @brief Write the object to a TokenStream::Writer
  //! @note The default implementation uses the TokenMap, which is probably what you want. But if you want to roll your own, it should look something like this.
  //! @code
  //! void EmployeeData::Write(TokenStream::Writer& writer) {
  //!     writer
  //!         << Token::BirthMonthToken << birthMonth
  //!         << Token::BirthDayToken << birthDay
  //!         << Token::BirthYearToken << birthYear
  //!         << Token::SocialSecurityNumberToken << socialSecurityNumber
  //!         << Token::FirstNameToken << firstName
  //!         << Token::LastNameToken << lastName;
  //! }
  //! @endcode
  virtual void Write(Writer& writer) const {
    Write(writer, GetTokenMap());
  }

  //! @brief Read the object from a TokenStream::Reader
  //! @note The default implementation uses the TokenMap, which is probably what you want. But if you want to roll your own, it should look something like this.
  //! @code
  //! void EmployeeData::Read(TokenStream::Reader& reader) {
  //!     while (!reader.EOS()) {
  //!         switch (reader.GetToken<Token>()) {
  //!         case Token::BirthMonthToken:
  //!             reader >> birthMonth;
  //!             break;
  //!
  //!         case Token::BirthDayToken:
  //!             reader >> birthDay;
  //!             break;
  //!
  //!         case Token::BirthYearToken:
  //!             reader >> birthYear;
  //!             break;
  //!
  //!         case Token::SocialSecurityNumberToken:
  //!             reader >> socialSecurityNumber;
  //!             break;
  //!
  //!         case Token::FirstNameToken:
  //!             reader >> firstName;
  //!             break;
  //!
  //!         case Token::LastNameToken:
  //!             reader >> lastName;
  //!             break;
  //!         }
  //!     }
  //! }
  //! @endcode
  virtual void Read(Reader& reader) {
    Read(reader, GetTokenMap());
  }

  void Write(Writer& writer, const TokenMap& tokenMap) const;
  void Read(Reader& reader, const TokenMap& tokenMap);

  //! @brief Returns the TokenMap used by the default implementations of \e Read and \e Write. Build this with the TOKEN_MAP macro.
  virtual const TokenMap& GetTokenMap() const {
    const static TokenMap dummy;
    return dummy;
  }
};
} // namespace TokenStream

// No need to use directly. Just use MAP_TOKEN and the compiler will figure it out.
#define MAP_TOKEN_NO_DEFAULT(tok, mem)                                                             \
  {                                                                                                \
    static_cast<uint64_t>(tok), {                                                                  \
      [](TokenStream::Reader& reader, Serializable& o) { reader >> reinterpret_cast<T&>(o).mem; }, \
          [](TokenStream::Writer& writer, const Serializable& o) {                                 \
            writer << reinterpret_cast<const T&>(o).mem;                                           \
          }                                                                                        \
    }                                                                                              \
  }

// No need to use directly. Just use MAP_TOKEN and the compiler will figure it out.
#define MAP_TOKEN_WITH_DEFAULT(tok, mem, def)                                                      \
  {                                                                                                \
    static_cast<uint64_t>(tok), {                                                                  \
      [](TokenStream::Reader& reader, Serializable& o) { reader >> reinterpret_cast<T&>(o).mem; }, \
          [](TokenStream::Writer& writer, const Serializable& o) {                                 \
            writer << TokenStream::ValueWithDefault(reinterpret_cast<const T&>(o).mem, def);       \
          }                                                                                        \
    }                                                                                              \
  }

// No need to use directly. Just use ENUMERATED_TOKEN and the compiler will figure it out.
#define ENUMERATED_TOKEN_NO_DEFAULT(mem)                                                           \
  {                                                                                                \
    static_cast<uint64_t>(Token::mem), {                                                           \
      [](TokenStream::Reader& reader, Serializable& o) { reader >> reinterpret_cast<T&>(o).mem; }, \
          [](TokenStream::Writer& writer, const Serializable& o) {                                 \
            writer << reinterpret_cast<const T&>(o).mem;                                           \
          }                                                                                        \
    }                                                                                              \
  }

// No need to use directly. Just use ENUMERATED_TOKEN and the compiler will figure it out.
#define ENUMERATED_TOKEN_WITH_DEFAULT(mem, def)                                                    \
  {                                                                                                \
    static_cast<uint64_t>(Token::mem), {                                                           \
      [](TokenStream::Reader& reader, Serializable& o) { reader >> reinterpret_cast<T&>(o).mem; }, \
          [](TokenStream::Writer& writer, const Serializable& o) {                                 \
            writer << TokenStream::ValueWithDefault(reinterpret_cast<const T&>(o).mem, def);       \
          }                                                                                        \
    }                                                                                              \
  }

// Magic macros for differentiating between usages with and without the final default parameter
#define TOKEN_FUNC_CHOOSER(arg1, arg2, arg3, ...) arg3
#define TOKEN_MACRO_CHOOSER(argsWithParenthesis) TOKEN_FUNC_CHOOSER argsWithParenthesis
#define MAP_TOKEN_MACRO_CHOOSER(...)                                                               \
  TOKEN_MACRO_CHOOSER((__VA_ARGS__, MAP_TOKEN_WITH_DEFAULT, MAP_TOKEN_NO_DEFAULT, ))
#define ENUMERATED_TOKEN_MACRO_CHOOSER(...)                                                        \
  TOKEN_MACRO_CHOOSER((__VA_ARGS__, ENUMERATED_TOKEN_WITH_DEFAULT, ENUMERATED_TOKEN_NO_DEFAULT, ))

//! @brief Use to map a token (any 8-64 bit value or a TokenStream::Token instance) to the members of a base class
//! @param tok Any 8-64 bit value or a TokenStream::Token instance
//! @param baseClassName The name of the base class to wrap in its own token
//! @note This is safer than using DERIVED_TOKEN_MAP because it will use an additional nesting level to
//!       keep the token space separate, but it will take a bit more space.
#define MAP_BASE_TOKEN(tok, baseClassName)                                                         \
  {                                                                                                \
    static_cast<uint64_t>(tok), {                                                                  \
      [](TokenStream::Reader& reader, Serializable& o) {                                           \
        TokenStream::Reader::SubStream subStream{reader};                                          \
        reinterpret_cast<T&>(o).Read(reader,                                                       \
                                     reinterpret_cast<T&>(o).baseClassName::GetTokenMap());        \
      },                                                                                           \
          [](TokenStream::Writer& writer, const Serializable& o) {                                 \
            writer.Put(reinterpret_cast<const T&>(o),                                              \
                       reinterpret_cast<const T&>(o).baseClassName::GetTokenMap());                \
          }                                                                                        \
    }                                                                                              \
  }

//! @brief Use to map a token (any 8-64 bit value or a TokenStream::Token instance) to a member
//! @param tok Any 8-64 bit value or a TokenStream::Token instance
//! @param member The undecorated name of the member to serialize
//! @param default (optional) The default value for the member
#define MAP_TOKEN(tok, ...) MAP_TOKEN_MACRO_CHOOSER(__VA_ARGS__)(tok, __VA_ARGS__)

//! @brief To use this form, the structure must contain an `enum class Token : uint64_t` with values that match the names of the members
//! @param member The undecorated name of the member to serialize. There must be a Token::<name> with the identical name.
//! @param default (optional) The default value for the member
#define ENUMERATED_TOKEN(...) ENUMERATED_TOKEN_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

//! @brief Create the token map for this type
//! @param mappedOfTokens The contents should be a series of either \e MAP_TOKEN or \e ENUMERATED_TOKEN entries
/*! @code
*  TOKEN_MAP(
*      MAP_TOKEN(0, name)
*      MAP_TOKEN(1, street)
*      MAP_TOKEN(2, city)
*      MAP_TOKEN(3, state)
*      MAP_TOKEN(4, zip)
*  )
*  @endcode
*/
#define TOKEN_MAP(...)                                                                             \
  const TokenStream::TokenMap& GetTokenMap() const override {                                      \
    using T = std::remove_const<std::remove_reference<decltype(*this)>::type>::type;               \
    static const TokenStream::TokenMap tokenMap{__VA_ARGS__};                                      \
    return tokenMap;                                                                               \
  }

//! @brief Create the token map for this type, including tokens from the base class' token map
//! @param baseClass The base of this class that has its own token map to include
//! @param mappedOfTokens The contents should be a series of either \e MAP_TOKEN or \e ENUMERATED_TOKEN entries
//! @note Tokens must not overlap!
/*! @code
*  DERIVED_TOKEN_MAP(
*      Person,
*      ENUMERATED_TOKEN(jobTitle)
*      ENUMERATED_TOKEN(manager)
*  )
*  @endcode
*/
#define DERIVED_TOKEN_MAP(baseClassName, ...)                                                      \
  const TokenStream::TokenMap& GetTokenMap() const override {                                      \
    using T = std::remove_const<std::remove_reference<decltype(*this)>::type>::type;               \
    static const TokenStream::TokenMap tokenMap(baseClassName::GetTokenMap(), {__VA_ARGS__});      \
    return tokenMap;                                                                               \
  }
