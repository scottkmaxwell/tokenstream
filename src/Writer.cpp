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

#include "EndianTypes.h"
#include <TokenStream/Writer.h>
#include <algorithm>
#if _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <cwchar>
#endif

#define VERIFIED_WRITE(byte_count, location, ...)                                                  \
  do {                                                                                             \
    m_stream->write(reinterpret_cast<const char*>(location), byte_count);                          \
    if (!m_stream) {                                                                               \
      TS_ASSERT(m_stream, "Failed to write to TokenStream");                                       \
      m_badStream = true;                                                                          \
      return __VA_ARGS__;                                                                          \
    }                                                                                              \
  } while (false)
#define ASSERT(test) TS_ASSERT(test, "Failed: " #test)
#define PUTNUMBER(token, value, defaultValue, conversionType, extendedSign)                        \
  do {                                                                                             \
    if (!m_trimDefaults || (value) != (defaultValue)) {                                            \
      conversionType conv = value;                                                                 \
      PutData(token, &conv, sizeof conv, true, extendedSign);                                      \
    } else {                                                                                       \
      m_nextToken = Token::InvalidTokenValue;                                                      \
    }                                                                                              \
  } while (false)

namespace TokenStream {

Writer& Writer::Put(Token token, uint8_t value, uint8_t defaultValue) {
  PUTNUMBER(token, value, defaultValue, uint8_t, false);
  return *this;
}

Writer& Writer::Put(Token token, int8_t value, int8_t defaultValue) {
  PUTNUMBER(token, value, defaultValue, uint8_t, false);
  return *this;
}

Writer& Writer::Put(Token token, uint16_t value, uint16_t defaultValue) {
  PUTNUMBER(token, value, defaultValue, uint16_be, false);
  return *this;
}

Writer& Writer::Put(Token token, int16_t value, int16_t defaultValue) {
  PUTNUMBER(token, value, defaultValue, int16_be, true);
  return *this;
}

Writer& Writer::Put(Token token, uint32_t value, uint32_t defaultValue) {
  PUTNUMBER(token, value, defaultValue, uint32_be, false);
  return *this;
}

Writer& Writer::Put(Token token, int32_t value, int32_t defaultValue) {
  PUTNUMBER(token, value, defaultValue, int32_be, true);
  return *this;
}

Writer& Writer::Put(Token token, uint64_t value, uint64_t defaultValue) {
  PUTNUMBER(token, value, defaultValue, uint64_be, false);
  return *this;
}

Writer& Writer::Put(Token token, int64_t value, int64_t defaultValue) {
  PUTNUMBER(token, value, defaultValue, int64_be, true);
  return *this;
}

Writer& Writer::Put(Token token, float value, float defaultValue) {
  PUTNUMBER(token, value, defaultValue, f32_le, false);
  return *this;
}

Writer& Writer::Put(Token token, double value, double defaultValue) {
  PUTNUMBER(token, value, defaultValue, f64_le, false);
  return *this;
}

Writer& Writer::Put(Token token, const char* str, const char* defaultValue) {
  auto needsWrite = !m_trimDefaults;
  if (!needsWrite) {
    if (defaultValue && defaultValue[0]) {
      needsWrite = !str || !str[0] || strcmp(str, defaultValue) != 0;
    } else {
      needsWrite = str && str[0];
    }
  }
  if (needsWrite) {
    if (!str || !str[0]) {
      TrimDefault force{*this, false};
      PutData(token);
    } else {
      const auto len = strlen(str);
      PutData(token, str, len);
    }
  }
  m_nextToken = Token::InvalidTokenValue;
  return *this;
}

Writer& Writer::Put(Token token, const wchar_t* str, const wchar_t* defaultValue) {
  auto needsWrite = !m_trimDefaults;
  if (!needsWrite) {
    if (defaultValue && defaultValue[0]) {
      needsWrite = !str || !str[0] || wcscmp(str, defaultValue) != 0;
    } else {
      needsWrite = str && str[0];
    }
  }
  if (needsWrite) {
    if (!str || !str[0]) {
      TrimDefault force{*this, false};
      PutData(token);
    } else {
#if _WIN32
      char fixedBuffer[0x400];
      std::string dynamicBuffer;
      auto* buffer = fixedBuffer;
      const size_t len = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
      if (len > sizeof fixedBuffer) {
        dynamicBuffer.resize(len - 1);
        buffer = const_cast<char*>(dynamicBuffer.data());
      }
      WideCharToMultiByte(CP_UTF8, 0, str, -1, buffer, static_cast<int>(len), nullptr, nullptr);
      PutData(token, buffer, len - 1);
#else
      auto* oldLocaleStr = setlocale(LC_ALL, nullptr);
      const std::string oldLocale = oldLocaleStr ? oldLocaleStr : "";
      setlocale(LC_ALL, "en_US.utf8");
      mbstate_t state;
      auto s_str = &str;
      size_t len = wcsrtombs(nullptr, s_str, 0, &state);
      char fixedBuffer[0x400];
      std::string dynamicBuffer;
      auto* buffer = fixedBuffer;
      if (len > sizeof fixedBuffer) {
        dynamicBuffer.resize(len);
        buffer = const_cast<char*>(dynamicBuffer.data());
      }
      s_str = &str;
      wcsrtombs(buffer, s_str, len, &state);
      PutData(token, buffer, len);
      if (!oldLocale.empty()) {
        setlocale(LC_ALL, oldLocale.c_str());
      }
#endif
    }
  }
  m_nextToken = Token::InvalidTokenValue;
  return *this;
}

Writer& Writer::Put(Token token, const Serializable& object, bool keepStubOnEmpty) {
  MemoryWriter subWriter{*this};
  object.Write(subWriter);
  TrimDefault handleStub{*this, m_trimDefaults && !keepStubOnEmpty};
  Put(token, subWriter);
  return *this;
}

Writer& Writer::Put(Token token,
                    const Serializable& object,
                    const TokenMap& tokenMap,
                    bool keepStubOnEmpty) {
  MemoryWriter subWriter{*this};
  object.Write(subWriter, tokenMap);
  TrimDefault handleStub{*this, m_trimDefaults && !keepStubOnEmpty};
  Put(token, subWriter);
  return *this;
}

Writer& Writer::Put(Token token, const void* block, uint64_t len) {
  ASSERT(block || !len);
  PutData(token, block, len);
  return *this;
}

Writer& Writer::PutContainerElementCount(Token token, uint64_t size) {
  if (m_badStream || size < 2) {
    return *this;
  }
  m_containerToken = token;
  m_containerElementCount = size;
  m_containerElementIndex = 0;
  auto len = static_cast<uint8_t>(0xf8);

  VERIFIED_WRITE(sizeof len, &len, *this);
  WriteLengthEncoded(size);
  return *this;
}

void Writer::WriteLengthEncoded(uint64_t value) {
  if (m_badStream) {
    return;
  }
  if (value < 0x80) {
    auto byteLen = static_cast<uint8_t>(value);
    VERIFIED_WRITE(sizeof byteLen, &byteLen, );
    return;
  }
  if (value < 0x7800) {
    uint16_be wordLen = static_cast<uint16_t>(value) | 0x8000u;
    VERIFIED_WRITE(sizeof wordLen, &wordLen, );
    return;
  }

  uint64_be beValue = value;
  auto block = reinterpret_cast<const uint8_t*>(&beValue);
  uint8_t len = sizeof beValue;
  while (len > 1 && !*block) {
    len--;
    block++;
  }
  uint8_t markedLen = len + 0xf7;
  VERIFIED_WRITE(sizeof markedLen, &markedLen, );
  VERIFIED_WRITE(len, block, );
}

void Writer::PutDataHeader(Token token, uint64_t len) {
  m_nextToken = Token::InvalidTokenValue;
  if (len || !m_trimDefaults) {
    // If we are writing elements of a container
    if (m_containerToken != Token::InvalidTokenValue) {
      // Make sure the token matches
      if (token != m_containerToken) {
        m_badStream = true;
        return;
      }
      // Write the token for the first item only
      if (m_containerElementIndex == 0) {
        WriteLengthEncoded(token);
      }
      // Clear the token for the last item
      if (++m_containerElementIndex == m_containerElementCount) {
        m_containerToken = Token::InvalidTokenValue;
      }
    }
    // If we have an invalid token and this is not the first item, we have a problem
    else if (token == Token::InvalidTokenValue && m_stream->tellp()) {
      m_badStream = true;
      return;
    }
    // Write the token if valid.
    // This check should only fail if we are writing the first item in the stream with no token.
    else if (token != Token::InvalidTokenValue) {
      WriteLengthEncoded(token);
    }
    WriteLengthEncoded(len);
  }
}

Writer& Writer::Put(Token token, std::istream& stream) {
  if (m_badStream) {
    return *this;
  }
  stream.seekg(0, std::ios::end);
  auto len = stream.tellg();
  stream.seekg(0, std::ios::beg);
  PutDataHeader(token, len);
  if (len) {
    char buffer[0x1000];
    while (len) {
      const auto bytesToRead = std::min(len, static_cast<std::ostream::pos_type>(sizeof buffer));
      stream.read(buffer, bytesToRead);
      const auto read = stream.gcount();
      if (!read) {
        TS_ASSERT(read, "Failed to read ostream for copy to TokenStream");
        m_badStream = true;
        return *this;
      }
      m_stream->write(buffer, read);
      if (!m_stream) {
        TS_ASSERT(m_stream, "Failed to write to TokenStream");
        m_badStream = true;
        return *this;
      }
      len -= read;
    }
  }
  return *this;
}

void Writer::PutData(
    Token token, const void* data, uint64_t len, bool removeLeadingZeros, bool handleExtendedSign) {
  if (m_badStream) {
    return;
  }
  auto block = static_cast<const uint8_t*>(data);
  if (len && removeLeadingZeros) {
    if (handleExtendedSign && *block == 0xff) {
      while (len > 1 && *block == 0xff && (block[1] & 0x80u) == 0x80u) {
        len--;
        block++;
      }
    } else {
      while (len > 1 && !*block) {
        len--;
        block++;
      }
      if (handleExtendedSign && len && block != data && (*block & 0x80u) == 0x80u) {
        block--;
        len++;
      }
    }
  }
  PutDataHeader(token, len);
  if (len) {
    ASSERT(block);
    VERIFIED_WRITE(len, block, );
  }
}

} // namespace TokenStream
