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

#include <TokenStream/Reader.h>
#include <TokenStream/TokenStream.h>
#include <TokenStream/Writer.h>
#include <map>
#include <memory>
#include <utility>

namespace TokenStream {

/** @brief Object to allow you to create generic serializable structures manually instead of with pre-constructed C++ structs
 *
 * @code
 *  TokenStream::Generic employee;
 *  employee.Add(BirthMonthToken, 9);
 *  employee.Add(BirthDayToken, 21);
 *  employee.Add(BirthYearToken, 1992);
 *  employee.Add(SocialSecurityNumberToken, "800-55-1212");
 *  employee.Add(FirstNameToken, "Ford");
 *  employee.Add(LastNameToken, "Prefect");
 *  TokenStream::MemoryWriter writer;
 *  employee.Write(writer);
 * @endcode
 *
 * @see MemoryWriter
 */
class Generic : public Serializable {
 public:
  class MemberBase;

  //! @brief Write object to a Writer
  void Write(Writer& writer) const override;

  //! @brief Read the object from a Reader. You need to have added all members you
  //! expect to read before calling Read. Any unrecognized tokens will be skipped.
  void Read(Reader& reader) override;

  //! @brief Add a token/value pair. Type is automatically deduced. Specify it manually if you want to be specific.
  template<typename T>
  Generic& Add(Token token, T value) {
    m_members[token] = std::make_shared<Member<T>>(value);
    return *this;
  }

  //! @brief Add a token/value pair and a defaultValue. Type is automatically deduced. Specify it manually if you want to be specific.
  template<typename T>
  Generic& Add(Token token, T value, T defaultValue) {
    m_members[token] = std::make_shared<MemberWithDefault<T>>(value, defaultValue);
    return *this;
  }

  //! @brief Get a pointer to a MemberBase object or nullptr if not present. You will need to do reinterpret_cast<TokenStream::Generic::Member<T>> to the value type you want.
  MemberBase* operator[](Token t);

  //! @brief Get the value associated with the specified token. The token MUST be in the map when you call this.
  template<typename T>
  T at(Token t) {
    return reinterpret_cast<Member<T>*>(operator[](t))->value;
  }

  //! @brief internal helper base class
  // ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
  class MemberBase {
   public:
    virtual ~MemberBase() = default;

    virtual void Get(Reader& reader) = 0;
    virtual void Put(Token t, Writer& writer) = 0;
  };

  //! @brief internal helper class to hold a generic value to serialize
  template<typename T>
  // ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
  struct Member : MemberBase {
    explicit Member(T value) : value(std::move(value)) {}

    void Get(Reader& reader) override {
      reader >> value;
    }

    void Put(Token t, Writer& writer) override {
      writer.Put(t, value);
    }

    T value;
  };

  //! @brief internal helper class to hold a generic value and default to serialize
  template<typename T>
  // ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
  struct MemberWithDefault : Member<T> {
    MemberWithDefault(T value, T defaultValue) : Member<T>{value}, defaultValue{defaultValue} {}
    void Put(Token t, Writer& writer) override {
      writer.Put(t, this->value, defaultValue);
    }

    T defaultValue;
  };

 private:
  std::map<Token, std::shared_ptr<MemberBase>> m_members;
};

//! @brief Automatically use std::string instead of const char*
template<>
inline Generic& Generic::Add(Token token, const char* value) {
  return Add(token, std::string(value));
}

} // namespace TokenStream
