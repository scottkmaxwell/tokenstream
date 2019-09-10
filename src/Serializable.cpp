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

#include <TokenStream/Reader.h>
#include <TokenStream/TokenStream.h>
#include <TokenStream/Writer.h>

namespace TokenStream {

void Serializable::Write(Writer& writer, const TokenMap& tokenMap) const {
  for (auto& kv : tokenMap) {
    writer.PutToken(kv.first);
    kv.second.Put(writer, *this);
  }
}

void Serializable::Read(Reader& reader, const TokenMap& tokenMap) {
  if (!tokenMap.empty()) {
    while (!reader.EOS()) {
      const auto token = reader.GetToken();
      const auto i = tokenMap.find(token);
      if (i != tokenMap.end()) {
        i->second.Get(reader, *this);
      }
    }
  }
}

} // namespace TokenStream
