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

#include <TokenStream/Generic.h>
#include <TokenStream/Reader.h>
#include <TokenStream/Writer.h>

namespace TokenStream {

void Generic::Read(Reader& reader) {
  while (!reader.EOS()) {
    const auto t = reader.GetToken();
    auto member = m_members.find(t);
    if (member != m_members.end()) {
      member->second->Get(reader);
    }
  }
}

void Generic::Write(Writer& writer) const {
  for (const auto& i : m_members) {
    i.second->Put(i.first, writer);
  }
}

Generic::MemberBase* Generic::operator[](Token t) {
  const auto member = m_members.find(t);
  if (member != m_members.end()) {
    return member->second.get();
  }
  return nullptr;
}

} // namespace TokenStream
