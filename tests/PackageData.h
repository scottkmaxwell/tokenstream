#pragma once
#include <TokenStream/Reader.h>
#include <TokenStream/TokenStream.h>
#include <TokenStream/Writer.h>
#include <string>

namespace InstallationExample {

enum class OsType : uint8_t {
  Any = 0,
  Windows = 1,
  Mac = 2,
  Linux = 3,
  Ios = 4,
  Android = 5,
  XboxOne = 6,
  PS4 = 7
};
enum class CompressionType : uint8_t { Default = 0, None = 1, Zip = 2, BZ2 = 3, LZMA = 4, LZO = 5 };
enum class InstallConditionType : uint8_t { Differs, Initial, Absent, Newer };
using TimestampType = uint32_t;

// specifies the critical information for a single file
struct FileData : TokenStream::Serializable {
  enum class Token : uint64_t {
    name,
    priority,
    timestamp,
    compressedSize,
    uncompressedSize,
    crc,
    compression,
    uninstallOnly,
    installIf,
    testCondition,
    languages,
    os,
    executable,
    link,
    redistributable,
    uri,
    offset
  };

  std::string name; // filename on the filesystem
  int32_t priority =
      0; // installation priority. Conditional plug-ins and redist files have higher numbers.
  TimestampType timestamp = 0;
  uint32_t compressedSize = 0;
  uint32_t uncompressedSize = 0;
  uint32_t crc = 0;
  CompressionType compression = CompressionType::
      Default; // if default, use the default compression type specified in the Package
  bool uninstallOnly = false; // do not install this file but clean it up if the user uninstalls
  InstallConditionType installIf = InstallConditionType::Differs;
  std::string
      testCondition; // the name of a Conditional plug-in to run. Install if true is returned.
  std::set<std::string>
      languages; // list of language codes. Only install if the list is empty or if the list contains the language of the user.
  std::vector<OsType>
      os; // OS filter. Only install if the list is empty or if the list contains the OS of the user.
  bool executable = false; // true for all executables
  std::string link;        // if this is a symlink, the relative path to link to
  bool redistributable =
      false; // true for all redist files. These need to be run during install and completed before launch.
  std::string
      uri; // normally empty but can be set if we need to point into some other file on the CDN
  uint64_t offset =
      0; // normally zero but if m_uri is set, this can give the offset into the file on the CDN

  // include an example with a default
  TOKEN_MAP(ENUMERATED_TOKEN(name),
            ENUMERATED_TOKEN(priority),
            ENUMERATED_TOKEN(timestamp),
            ENUMERATED_TOKEN(compressedSize),
            ENUMERATED_TOKEN(uncompressedSize),
            ENUMERATED_TOKEN(crc),
            ENUMERATED_TOKEN(compression),
            ENUMERATED_TOKEN(uninstallOnly),
            ENUMERATED_TOKEN(installIf),
            ENUMERATED_TOKEN(testCondition),
            ENUMERATED_TOKEN(languages),
            ENUMERATED_TOKEN(os),
            ENUMERATED_TOKEN(executable),
            ENUMERATED_TOKEN(link),
            ENUMERATED_TOKEN(redistributable),
            ENUMERATED_TOKEN(uri),
            ENUMERATED_TOKEN(offset))
};

struct FolderData : TokenStream::Serializable {
  enum class Token : uint64_t {
    path = 1,
    maxPriority = 2,
    os = 3,
    onCondition = 4,
    folders = 5,
    files = 6
  };

  void Write(TokenStream::Writer& writer) const override;
  void Read(TokenStream::Reader& reader) override;

  std::vector<FolderData> folders; // token=5
  std::vector<FileData> files;     // token=6
  std::string path;                // token=1
  int32_t maxPriority = 0;         // token=2
  std::vector<OsType> os;          // token=3
  std::string onCondition;         // token=4
};

// an external package can either be a component or a wrapper
// a good example of a wrapper is Cider for Mac to allow running Windows titles on macOs
struct ExternalPackageData : TokenStream::Serializable {
  enum class Token : uint64_t { uri, launchParameters, childPath, os, vars, folders };

  std::string uri;              // uri to external manifest
  std::string launchParameters; // if uri is to a wrapper, add these launch parameters
  std::string childPath;        // if uri is to a wrapper, this can override the child path
  std::vector<OsType> os;       // os filter for the external package

  // vars are name/value macro pairs that can be used in text strings in the remainder of the
  // manifest. Reference a macro by using the name wrapped in ${}, e.g. ${root}
  std::unordered_map<std::string, std::string> vars; // var overrides for the external package
  std::vector<FolderData>
      folders; // additional folders and files to overlay into the external package

  TOKEN_MAP(ENUMERATED_TOKEN(uri),
            ENUMERATED_TOKEN(launchParameters),
            ENUMERATED_TOKEN(childPath),
            ENUMERATED_TOKEN(os),
            ENUMERATED_TOKEN(vars),
            ENUMERATED_TOKEN(folders))
};

struct RequirementsData : TokenStream::Serializable {
  enum class Token : uint64_t { minimumRam, minimumOsVersion };

  uint32_t minimumRam = 0;
  float minimumOsVersion = 0.f;

  TOKEN_MAP(ENUMERATED_TOKEN(minimumRam), ENUMERATED_TOKEN(minimumOsVersion))
};

// this is the top level for any manifest
struct PackageData : TokenStream::Serializable {
  enum class Token : uint64_t {
    name,
    packagerVersion,
    timestamp,
    description,
    reserve,
    packageSize,
    fileCount,
    executable,
    workingDirectory,
    launchParameters,
    childPath,
    isWrapper,
    languages,
    compression,
    vars,
    requirements,
    externalPackages,
    folders
  };

  std::wstring name;            // name of the product
  uint16_t packagerVersion = 0; // version of the package builder used to create the manifest
  TimestampType timestamp = 0;  // time when this was package built
  std::string description;      // user-friendly description of the package
  int32_t reserve =
      0; // ensure that this number of MB of hard drive space is still available after install
  uint32_t packageSize = 0; // total byte size of package once installed
  uint32_t fileCount = 0;   // total number of files in the package
  std::string executable;   // path the the primary executable
  std::string workingDirectory =
      "."; // working directory is package root by default but this can specify an alternative as a relative path
  std::string launchParameters; // parameter to pass to the primary executable on launch
  std::string
      childPath; // if this package is a wrapper for other content, this is the path where the other content should be installed
  bool isWrapper =
      false; // if this package is meant to be used as an external wrapper digest (e.g. Cider), set this to true
  std::vector<std::string> languages; // list of supported languages
  CompressionType compression =
      CompressionType::LZMA; // default compression type for the package. defaults to LZMA.

  // vars are name/value macro pairs that can be used in text strings in the remainder of the
  // manifest. Reference a macro by using the name wrapped in ${}, e.g. ${root}
  std::map<std::string, std::string> vars;

  std::vector<RequirementsData> requirements;
  std::vector<ExternalPackageData> externalPackages;
  std::vector<FolderData> folders;

  TOKEN_MAP(ENUMERATED_TOKEN(name),
            ENUMERATED_TOKEN(packagerVersion),
            ENUMERATED_TOKEN(timestamp),
            ENUMERATED_TOKEN(description),
            ENUMERATED_TOKEN(reserve),
            ENUMERATED_TOKEN(packageSize),
            ENUMERATED_TOKEN(fileCount),
            ENUMERATED_TOKEN(executable),
            ENUMERATED_TOKEN(workingDirectory, "."),
            ENUMERATED_TOKEN(launchParameters),
            ENUMERATED_TOKEN(childPath),
            ENUMERATED_TOKEN(isWrapper),
            ENUMERATED_TOKEN(languages),
            ENUMERATED_TOKEN(compression, CompressionType::LZMA),
            ENUMERATED_TOKEN(vars),
            ENUMERATED_TOKEN(requirements),
            ENUMERATED_TOKEN(externalPackages),
            ENUMERATED_TOKEN(folders))
};

struct SecurePackageData : PackageData {
  enum class Token : uint64_t {
    base,
    signature,
    algorithm,
  };
  enum class Algorithm { SHA1, SHA256 };

  std::vector<uint8_t> signature;
  Algorithm algorithm = Algorithm::SHA1;
  TOKEN_MAP(MAP_BASE_TOKEN(Token::base, PackageData),
            ENUMERATED_TOKEN(signature),
            ENUMERATED_TOKEN(algorithm))
};

} // namespace InstallationExample
