#include "PackageData.h"
#include <TokenStream/Reader.h>
#include <TokenStream/Writer.h>

namespace InstallationExample {
using namespace TokenStream;

void FolderData::Read(Reader& reader) {
  while (!reader.EOS()) {
    switch (reader.GetToken<Token>()) {
      case Token::path:
        reader >> path;
        break;

      case Token::maxPriority:
        reader >> maxPriority;
        break;

      case Token::os:
        reader >> os;
        break;

      case Token::onCondition:
        reader >> onCondition;
        break;

      case Token::folders:
        reader >> folders;
        break;

      case Token::files:
        reader >> files;
        break;
    }
  }
}

void FolderData::Write(Writer& writer) const {
  writer << Token::path << path << Token::maxPriority << maxPriority << Token::os << os
         << Token::onCondition << onCondition << Token::folders << folders << Token::files << files;
}

} // namespace InstallationExample
