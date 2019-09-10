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

#if _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include "EndianTypes.h"
#include <TokenStream/Reader.h>
#include <algorithm>

#define VERIFIED_READ(byte_count, location)                                                        \
  do {                                                                                             \
    m_stream.read(reinterpret_cast<char*>(location), byte_count);                                  \
    auto read = static_cast<size_t>(m_stream.gcount());                                            \
    if (read != (byte_count)) {                                                                    \
      TS_ASSERT(read == (byte_count), "Failed to read from TokenStream");                          \
      m_lastToken = Token::InvalidTokenValue;                                                      \
      m_badStream = true;                                                                          \
    }                                                                                              \
  } while (false) // NOLINT

namespace TokenStream {

Reader::Reader(std::istream& stream) : m_stream{stream} {
  const auto currentPosition = m_stream.tellg();
  m_stream.seekg(0, std::ios::end);
  m_context.m_end = static_cast<size_t>(m_stream.tellg() - currentPosition);
  m_stream.seekg(currentPosition, std::ios::beg);
}

uint64_t Reader::ReadLengthEncoded(bool forToken) {
  uint8_t v;

  VERIFY_TOKENSTREAM(!PastEOS(1), 0);
  if (m_badStream) {
    return 0;
  }
  VERIFIED_READ(1, &v);
  if (m_badStream) {
    return 0;
  }
  ++m_offset;
  // Handle the special 0xf8 value
  if (v == 0xf8) {
    if (!forToken) {
      m_badStream = true;
      return 0;
    }
    m_nextContainerElementCount = DecodeLength();
    return DecodeToken();
  }

  // Anything without the high-bit set is just a 1-byte length
  if (v < 0x80) {
    return v;
  }

  // Anything under 0xf8 is a 2-byte length with the high bit set
  if (v < 0xf8) {
    VERIFY_TOKENSTREAM(!PastEOS(1), 0);
    uint16_t v16 = v & 0x7fu;
    v16 <<= 8u;
    VERIFIED_READ(1, &v);
    if (m_badStream) {
      return 0;
    }
    m_offset += 1;
    v16 |= v;
    return v16;
  }

  // 0xf8 and above means that the low-3 bits contains a byte count for the length (1-8)
  // Since we already handle the 1 byte length cases, we can treat 0xf8 as a special control value

  // Otherwise, get my byte count and start grabbing and shifting
  m_remainingInElement = v - 0xf7;
  VERIFY_TOKENSTREAM(!PastEOS(m_remainingInElement), 0);

  return FetchPartial<uint64_be>();
}

Token Reader::GetToken() {
  if (m_badStream) {
    return Token::InvalidTokenValue;
  }
  if (m_tokenPushed) {
    m_tokenPushed = false;
    return m_lastToken;
  }
  if (m_remainingInElement) {
    Skip();
    if (EOS()) {
      return Token::InvalidTokenValue;
    }
  }

  m_nextContainerElementCount = 0;
  bool updateContainerElementEnd = false;

  // If we are in the middle of reading items in a container, and we just reached the end of the current item
  if (m_context.m_containerElementEnd && m_context.m_containerElementEnd == m_offset) {
    VERIFY_TOKENSTREAM(!PastEOS(1), Token::InvalidTokenValue);

    // Grab the stored container token
    m_lastToken = m_context.m_containerToken;
    // If this was the last item in the container, clear out all the container tracking fields
    if (++m_context.m_containerElementIndex == m_context.m_containerElementCount) {
      m_context.m_containerElementCount = m_context.m_containerElementIndex =
          m_context.m_containerElementEnd = 0;
      m_context.m_containerToken = Token::InvalidTokenValue;
    }
    // Otherwise, we'll need to update the end marker after we get the size
    else {
      updateContainerElementEnd = true;
    }
  }
  // Not the beginning of the next element in a container so just get the token
  else {
    VERIFY_TOKENSTREAM(!PastEOS(2), Token::InvalidTokenValue);

    m_lastToken = Token(DecodeToken());
    // But if that is the first item in a container, set up the container tracking fields
    if (m_nextContainerElementCount > 1) {
      m_context.m_containerToken = m_lastToken;
      m_context.m_containerElementCount = m_nextContainerElementCount;
      m_context.m_containerElementIndex = 1;
      updateContainerElementEnd = true;
    }
  }

  if (m_badStream) {
    m_lastToken = Token::InvalidTokenValue;
    return Token::InvalidTokenValue;
  }

  m_remainingInElement = DecodeLength();

  // If we are processing an item in a container, set the end marker
  if (updateContainerElementEnd) {
    m_context.m_containerElementEnd = m_offset + m_remainingInElement;
  }

  if (m_badStream) {
    return Token::InvalidTokenValue;
  }

  VERIFY_TOKENSTREAM(!PastEOS(m_remainingInElement), Token::InvalidTokenValue);

  return {m_lastToken};
}

int8_t Reader::GetByte() {
  uint8_t t;
  Fetch(&t);
  return t;
}

uint8_t Reader::GetUByte() {
  uint8_t t;
  Fetch(&t);
  return t;
}

int16_t Reader::GetWord() {
  return FetchPartial<int16_be>(true);
}

uint16_t Reader::GetUWord() {
  return FetchPartial<uint16_be>();
}

int32_t Reader::GetLong() {
  return FetchPartial<int32_be>(true);
}

uint32_t Reader::GetULong() {
  return FetchPartial<uint32_be>();
}

int64_t Reader::GetLongLong() {
  return FetchPartial<int64_be>(true);
}

uint64_t Reader::GetULongLong() {
  return FetchPartial<uint64_be>();
}

float Reader::GetFloat() {
  return FetchPartial<f32_le>();
}

double Reader::GetDouble() {
  return FetchPartial<f64_le>();
}

void Reader::Fetch(uint8_t* v) {
  if (!m_remainingInElement) {
    return;
  }
  VERIFIED_READ(m_remainingInElement, v);

  m_offset += m_remainingInElement;
  m_remainingInElement = 0;
}

std::string Reader::GetString() {
  std::string ret;
  *this >> ret;
  return ret;
}

Reader& Reader::operator>>(std::string& rhs) {
  const auto len = m_remainingInElement;
  if (len) {
    rhs.resize(static_cast<std::string::size_type>(len));
    Fetch(reinterpret_cast<uint8_t*>(const_cast<char*>(rhs.data())));
    if (m_badStream) {
      rhs.resize(0);
      return *this;
    }
    TS_ASSERT(rhs.length() == len, "String is the wrong length");
  } else {
    rhs.clear();
  }

  return *this;
}

std::wstring Reader::GetWideString() {
  std::wstring ret;
  *this >> ret;

  return ret;
}

Reader& Reader::operator>>(std::wstring& rhs) {
  const auto len = m_remainingInElement;
  if (len) {
    std::string buffer;
    buffer.resize(len);
    Fetch(reinterpret_cast<uint8_t*>(const_cast<char*>(buffer.data())));
    if (m_badStream) {
      rhs.resize(0);
      return *this;
    }
#if _WIN32
    const auto w_len = MultiByteToWideChar(CP_UTF8, 0, buffer.c_str(), -1, 0, 0);
    rhs.resize(static_cast<std::wstring::size_type>(w_len) - 1);
    MultiByteToWideChar(
        CP_UTF8, 0, buffer.c_str(), -1, const_cast<wchar_t*>(rhs.data()), w_len - 1);
#else
    auto* oldLocaleStr = setlocale(LC_ALL, nullptr);
    const std::string oldLocale = oldLocaleStr ? oldLocaleStr : "";
    setlocale(LC_ALL, "en_US.utf8");
    size_t w_len = mbstowcs(nullptr, buffer.c_str(), len);
    rhs.resize(w_len);
    mbstowcs(const_cast<wchar_t*>(rhs.data()), buffer.data(), len);
    if (!oldLocale.empty()) {
      setlocale(LC_ALL, oldLocale.c_str());
    }
#endif
  } else {
    rhs.clear();
  }

  return *this;
}

void Reader::GetSerializable(Serializable& object) {
  if (!m_offset) {
    m_remainingInElement = DecodeLength();
    if (m_badStream) {
      return;
    }
  }
  SubStream sub{*this};
  object.Read(*this);
}

void Reader::GetSerializable(Serializable& object, const TokenMap& tokenMap) {
  if (!m_offset) {
    m_remainingInElement = DecodeLength();
    if (m_badStream) {
      return;
    }
  }
  SubStream sub{*this};
  object.Read(*this, tokenMap);
}

Binary Reader::GetBlock() {
  Binary ret;
  if (m_remainingInElement) {
    ret.resize(static_cast<std::vector<uint8_t>::size_type>(m_remainingInElement));
    Fetch(ret.data());
    if (m_badStream) {
      ret.resize(0);
    }
  }
  return ret;
}

void Reader::SkipBytes(size_t bytes) {
  m_tokenPushed = false;
  m_remainingInElement = 0;
  if (m_badStream || !bytes) {
    return;
  }
  try {
    m_stream.seekg(bytes, std::ios_base::cur);
    m_offset += bytes;
  } catch (std::ios_base::failure&) {
    SkipBytesByReading(bytes);
  }
}

void Reader::SkipBytesByReading(size_t bytes) {
  uint8_t buffer[0x400];
  while (bytes && !m_badStream) {
    const auto count = std::min(bytes, sizeof buffer);
    VERIFIED_READ(count, buffer);
    m_offset += count;
    bytes -= count;
  }
}

Reader::SubStream::SubStream(Reader& reader) : m_reader{reader}, m_oldContext{reader.m_context} {
  reader.m_context = SubStreamContext{reader.m_offset + reader.m_remainingInElement};
  reader.m_remainingInElement = 0;
}

Reader::SubStream::~SubStream() {
  m_reader.SkipBytes(m_reader.m_context.m_end - m_reader.m_offset);
  m_reader.m_context = m_oldContext;
}

} // namespace TokenStream
