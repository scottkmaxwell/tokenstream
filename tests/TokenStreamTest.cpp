#include "PackageData.h"
#include <TokenStream/Generic.h>
#include <TokenStream/Reader.h>
#include <TokenStream/Writer.h>
#include <gtest/gtest.h>

using namespace InstallationExample;

SecurePackageData MakeTestPackageWithStructure() {
  SecurePackageData package;
  package.name = L"Quake";
  package.packagerVersion = 1;
  package.timestamp = 1000;
  package.description = "The best game ever!";
  package.reserve = -0x88;
  package.fileCount = 0xc0;

  package.vars.emplace("root", R"(c:\example\game)");
  package.vars.emplace("cert", "Mycert.cert");
  package.vars.emplace("repeat", "");
  package.vars.emplace("", "");
  package.vars.emplace("", "");

  package.languages.emplace_back("en");
  package.languages.emplace_back();
  package.languages.emplace_back("de");

  RequirementsData requirements;
  requirements.minimumOsVersion = 10.1f;
  requirements.minimumRam = 1000;
  package.requirements.push_back(requirements);

  FolderData folder;
  folder.path = "bin";

  FileData file;
  file.name = "Quake.exe";
  file.timestamp = 0x12345678;
  file.compressedSize = 10000;
  file.uncompressedSize = 100000;
  file.crc = 0x87654321;
  file.languages = {"en", "de"};
  file.os = {OsType::Windows};
  file.executable = true;
  folder.files.emplace_back(file);

  file.name = "Quake2.exe";
  file.timestamp = 0x12345679;
  file.compressedSize = 100000;
  file.uncompressedSize = 1000000;
  file.crc = 0x87654343;
  file.languages.clear();
  file.os = {OsType::Windows, OsType::Mac};
  file.executable = true;
  folder.files.emplace_back(file);

  package.folders.emplace_back(folder);
  return package;
}

TokenStream::Generic MakeTestPackageWithGeneric() {
  TokenStream::Generic package;
  package.Add(PackageData::Token::name, "Quake");
  package.Add(PackageData::Token::packagerVersion, 1);
  package.Add(PackageData::Token::timestamp, 1000);
  package.Add(PackageData::Token::description, "The best game ever!");
  package.Add(PackageData::Token::reserve, -0x88);
  package.Add<uint32_t>(PackageData::Token::fileCount, 0xc0);
  package.Add(PackageData::Token::compression, CompressionType::LZMA, CompressionType::LZMA);

  std::vector<TokenStream::Generic> varVector;
  varVector.reserve(4);
  {
    varVector.emplace_back();
    auto& var = varVector.back();
    var.Add(0, "root");
    var.Add(1, R"(c:\example\game)");
  }
  {
    varVector.emplace_back();
    auto& var = varVector.back();
    var.Add(0, "cert");
    var.Add(1, "Mycert.cert");
  }
  {
    varVector.emplace_back();
    auto& var = varVector.back();
    var.Add(0, "repeat");
  }
  { varVector.emplace_back(); }
  package.Add(PackageData::Token::vars, varVector);

  const std::vector<std::string> languageVector{"en", "", "de"};
  package.Add(PackageData::Token::languages, languageVector);

  // In the structure, requirements is a std::vector but we are populating
  // it with only a single entry. Just adding one of these to our generic
  // rather than adding a vector should produce equivalent results. This
  // will test that that is true.
  TokenStream::Generic requirements;
  requirements.Add(RequirementsData::Token::minimumOsVersion, 10.1f);
  requirements.Add(RequirementsData::Token::minimumRam, 1000);
  package.Add(PackageData::Token::requirements, requirements);

  std::vector<TokenStream::Generic> foldersVector;
  foldersVector.emplace_back();
  auto& folder = foldersVector.back();
  folder.Add(FolderData::Token::path, "bin");

  std::vector<TokenStream::Generic> filesVector;
  filesVector.reserve(2);
  {
    filesVector.emplace_back();
    auto& file = filesVector.back();
    file.Add(FileData::Token::name, "Quake.exe");
    file.Add(FileData::Token::timestamp, 0x12345678);
    file.Add(FileData::Token::compressedSize, 10000);
    file.Add(FileData::Token::uncompressedSize, 100000);
    file.Add(FileData::Token::crc, 0x87654321);
    file.Add(FileData::Token::languages, std::vector<std::string>{"en", "de"});
    file.Add(FileData::Token::os, std::vector<OsType>{OsType::Windows});
    file.Add(FileData::Token::executable, true);
  }
  {
    filesVector.emplace_back();
    auto& file = filesVector.back();
    file.Add(FileData::Token::name, "Quake2.exe");
    file.Add(FileData::Token::timestamp, 0x12345679);
    file.Add(FileData::Token::compressedSize, 100000);
    file.Add(FileData::Token::uncompressedSize, 1000000);
    file.Add(FileData::Token::crc, 0x87654343);
    file.Add(FileData::Token::os, std::vector<OsType>{OsType::Windows, OsType::Mac});
    file.Add(FileData::Token::executable, true);
  }
  folder.Add(FolderData::Token::files, filesVector);
  package.Add(PackageData::Token::folders, foldersVector);

  TokenStream::Generic securePackage;
  securePackage.Add(SecurePackageData::Token::base, package);
  return securePackage;
}

#define ASSERT_EQUAL(x) EXPECT_EQ(package.x, package2.x) // NOLINT

TEST(TokenStreamTest, StuctureSerializationTest) {
  auto package = MakeTestPackageWithStructure();

  TokenStream::MemoryWriter writer;
  package.Write(writer);
  EXPECT_EQ(227u, writer.GetLength());

  SecurePackageData package2;
  auto readerString = writer.GetReader().str();
  EXPECT_EQ(227u, readerString.length());

  auto memoryReader{writer.GetReader()};
  TokenStream::Reader reader{memoryReader};
  package2.Read(reader);

  ASSERT_EQUAL(name);
  ASSERT_EQUAL(packagerVersion);
  ASSERT_EQUAL(timestamp);
  ASSERT_EQUAL(description);
  ASSERT_EQUAL(reserve);
  ASSERT_EQUAL(fileCount);

  ASSERT_EQUAL(vars.size());
  for (const auto& var : package.vars) {
    EXPECT_EQ(var.second, package2.vars[var.first]);
  }

  ASSERT_EQUAL(languages);

  ASSERT_EQUAL(requirements.size());
  ASSERT_EQUAL(requirements[0].minimumOsVersion);
  ASSERT_EQUAL(requirements[0].minimumRam);

  ASSERT_EQUAL(folders.size());
  ASSERT_EQUAL(folders[0].path);
  ASSERT_EQUAL(folders[0].files.size());

  ASSERT_EQUAL(folders[0].files[0].name);
  ASSERT_EQUAL(folders[0].files[0].timestamp);
  ASSERT_EQUAL(folders[0].files[0].compressedSize);
  ASSERT_EQUAL(folders[0].files[0].uncompressedSize);
  ASSERT_EQUAL(folders[0].files[0].crc);
  ASSERT_EQUAL(folders[0].files[0].languages);
  ASSERT_EQUAL(folders[0].files[0].os);
  ASSERT_EQUAL(folders[0].files[0].executable);

  ASSERT_EQUAL(folders[0].files[1].name);
  ASSERT_EQUAL(folders[0].files[1].timestamp);
  ASSERT_EQUAL(folders[0].files[1].compressedSize);
  ASSERT_EQUAL(folders[0].files[1].uncompressedSize);
  ASSERT_EQUAL(folders[0].files[1].crc);
  ASSERT_EQUAL(folders[0].files[1].languages);
  ASSERT_EQUAL(folders[0].files[1].os);
  ASSERT_EQUAL(folders[0].files[1].executable);

  EXPECT_EQ(writer.GetLength(), 227u);
}

TEST(TokenStreamTest, GenericWriteStructureReadSerializationTest) {
  auto package = MakeTestPackageWithStructure();
  auto packageGeneric = MakeTestPackageWithGeneric();

  TokenStream::MemoryWriter writer;
  packageGeneric.Write(writer);
  EXPECT_EQ(227u, writer.GetLength());

  SecurePackageData package2;
  auto memoryReader{writer.GetReader()};
  TokenStream::Reader reader{memoryReader};
  package2.Read(reader);

  ASSERT_EQUAL(name);
  ASSERT_EQUAL(packagerVersion);
  ASSERT_EQUAL(timestamp);
  ASSERT_EQUAL(description);
  ASSERT_EQUAL(reserve);
  ASSERT_EQUAL(fileCount);

  ASSERT_EQUAL(vars.size());
  for (const auto& var : package.vars) {
    EXPECT_EQ(var.second, package2.vars[var.first]);
  }

  ASSERT_EQUAL(languages);

  ASSERT_EQUAL(requirements.size());
  ASSERT_EQUAL(requirements[0].minimumOsVersion);
  ASSERT_EQUAL(requirements[0].minimumRam);

  ASSERT_EQUAL(folders.size());
  ASSERT_EQUAL(folders[0].path);
  ASSERT_EQUAL(folders[0].files.size());

  ASSERT_EQUAL(folders[0].files[0].name);
  ASSERT_EQUAL(folders[0].files[0].timestamp);
  ASSERT_EQUAL(folders[0].files[0].compressedSize);
  ASSERT_EQUAL(folders[0].files[0].uncompressedSize);
  ASSERT_EQUAL(folders[0].files[0].crc);
  ASSERT_EQUAL(folders[0].files[0].languages);
  ASSERT_EQUAL(folders[0].files[0].os);
  ASSERT_EQUAL(folders[0].files[0].executable);

  ASSERT_EQUAL(folders[0].files[1].name);
  ASSERT_EQUAL(folders[0].files[1].timestamp);
  ASSERT_EQUAL(folders[0].files[1].compressedSize);
  ASSERT_EQUAL(folders[0].files[1].uncompressedSize);
  ASSERT_EQUAL(folders[0].files[1].crc);
  ASSERT_EQUAL(folders[0].files[1].languages);
  ASSERT_EQUAL(folders[0].files[1].os);
  ASSERT_EQUAL(folders[0].files[1].executable);

  EXPECT_EQ(writer.GetLength(), 227u);
}

#undef ASSERT_EQUAL
#define ASSERT_EQUAL(x, T) EXPECT_EQ(package.at<T>(x), package2.at<T>(x)) // NOLINT

TEST(TokenStreamTest, GenericSerializationTest) {
  TokenStream::Generic package;
  package.Add(PackageData::Token::name, "Quake");
  package.Add(PackageData::Token::packagerVersion, 1);
  package.Add(PackageData::Token::timestamp, 1000);
  package.Add(PackageData::Token::description, "The best game ever!");
  package.Add(PackageData::Token::reserve, -0x88);
  package.Add<uint32_t>(PackageData::Token::fileCount, 0xc0);
  package.Add(PackageData::Token::compression, CompressionType::LZMA, CompressionType::LZMA);

  TokenStream::MemoryWriter writer;
  package.Write(writer);

  //EXPECT_EQ(writer.GetLength(), 41);

  TokenStream::Generic package2;
  package2.Add(PackageData::Token::name, "");
  package2.Add(PackageData::Token::packagerVersion, 0);
  package2.Add(PackageData::Token::timestamp, 0);
  package2.Add(PackageData::Token::description, "");
  package2.Add(PackageData::Token::reserve, 0);
  package2.Add<uint32_t>(PackageData::Token::fileCount, 0);
  package2.Add(PackageData::Token::compression, InstallationExample::CompressionType::LZMA);

  auto memoryReader{writer.GetReader()};
  TokenStream::Reader reader{memoryReader};
  package2.Read(reader);

  ASSERT_EQUAL(PackageData::Token::name, std::string);
  ASSERT_EQUAL(PackageData::Token::packagerVersion, int);
  ASSERT_EQUAL(PackageData::Token::timestamp, int);
  ASSERT_EQUAL(PackageData::Token::description, std::string);
  ASSERT_EQUAL(PackageData::Token::reserve, int);
  ASSERT_EQUAL(PackageData::Token::fileCount, uint32_t);
  ASSERT_EQUAL(PackageData::Token::compression, InstallationExample::CompressionType);

  EXPECT_EQ(writer.GetLength(), 42u);
}
