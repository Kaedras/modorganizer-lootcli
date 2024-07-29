#include "game_settings.h"

#include <QStandardPaths>
#include <utility>

namespace fs = std::filesystem;
namespace loot
{
static constexpr float MORROWIND_MINIMUM_HEADER_VERSION  = 1.2f;
static constexpr float OBLIVION_MINIMUM_HEADER_VERSION   = 0.8f;
static constexpr float SKYRIM_FO3_MINIMUM_HEADER_VERSION = 0.94f;
static constexpr float SKYRIM_SE_MINIMUM_HEADER_VERSION  = 1.7f;
static constexpr float FONV_MINIMUM_HEADER_VERSION       = 1.32f;
static constexpr float FO4_MINIMUM_HEADER_VERSION        = 0.95f;
static constexpr float STARFIELD_MINIMUM_HEADER_VERSION  = 0.96f;

GameType GetGameType(const GameId gameId)
{
  switch (gameId) {
  case GameId::tes3:
    return GameType::tes3;
  case GameId::tes4:
  case GameId::nehrim:
    return GameType::tes4;
  case GameId::tes5:
  case GameId::enderal:
    return GameType::tes5;
  case GameId::tes5se:
  case GameId::enderalse:
    return GameType::tes5se;
  case GameId::tes5vr:
    return GameType::tes5vr;
  case GameId::fo3:
    return GameType::fo3;
  case GameId::fonv:
    return GameType::fonv;
  case GameId::fo4:
    return GameType::fo4;
  case GameId::fo4vr:
    return GameType::fo4vr;
  case GameId::starfield:
    return GameType::starfield;
  default:
    throw std::logic_error("Unrecognised game ID");
  }
}

float GetMinimumHeaderVersion(const GameId gameId)
{
  switch (gameId) {
  case GameId::tes3:
    return MORROWIND_MINIMUM_HEADER_VERSION;
  case GameId::tes4:
  case GameId::nehrim:
    return OBLIVION_MINIMUM_HEADER_VERSION;
  case GameId::tes5:
  case GameId::enderal:
    return SKYRIM_FO3_MINIMUM_HEADER_VERSION;
  case GameId::tes5se:
  case GameId::tes5vr:
  case GameId::enderalse:
    return SKYRIM_SE_MINIMUM_HEADER_VERSION;
  case GameId::fo3:
    return SKYRIM_FO3_MINIMUM_HEADER_VERSION;
  case GameId::fonv:
    return FONV_MINIMUM_HEADER_VERSION;
  case GameId::fo4:
  case GameId::fo4vr:
    return FO4_MINIMUM_HEADER_VERSION;
  case GameId::starfield:
    return STARFIELD_MINIMUM_HEADER_VERSION;
  default:
    throw std::logic_error("Unrecognised game ID");
  }
}

std::filesystem::path GetPluginsFolderName(GameId gameId)
{
  switch (gameId) {
  case GameId::tes3:
    return "Data Files";
  case GameId::tes4:
  case GameId::nehrim:
  case GameId::tes5:
  case GameId::enderal:
  case GameId::tes5se:
  case GameId::enderalse:
  case GameId::tes5vr:
  case GameId::fo3:
  case GameId::fonv:
  case GameId::fo4:
  case GameId::fo4vr:
  case GameId::starfield:
    return "Data";
  default:
    throw std::logic_error("Unrecognised game ID");
  }
}

std::string ToString(const GameId gameId)
{
  switch (gameId) {
  case GameId::tes3:
    return "Morrowind";
  case GameId::tes4:
    return "Oblivion";
  case GameId::nehrim:
    return "Nehrim";
  case GameId::tes5:
    return "Skyrim";
  case GameId::enderal:
    return "Enderal";
  case GameId::tes5se:
    return "Skyrim Special Edition";
  case GameId::enderalse:
    return "Enderal Special Edition";
  case GameId::tes5vr:
    return "Skyrim VR";
  case GameId::fo3:
    return "Fallout3";
  case GameId::fonv:
    return "FalloutNV";
  case GameId::fo4:
    return "Fallout4";
  case GameId::fo4vr:
    return "Fallout4VR";
  case GameId::starfield:
    return "Starfield";
  default:
    throw std::logic_error("Unrecognised game ID");
  }
}

bool SupportsLightPlugins(const GameType gameType)
{
  return gameType == GameType::tes5se || gameType == GameType::tes5vr ||
         gameType == GameType::fo4 || gameType == GameType::fo4vr;
}

std::filesystem::path GetMasterFilename(const GameId gameId)
{
  switch (gameId) {
  case GameId::tes3:
    return "Morrowind.esm";
  case GameId::tes4:
    return "Oblivion.esm";
  case GameId::nehrim:
    return "Nehrim.esm";
  case GameId::tes5:
  case GameId::tes5se:
  case GameId::tes5vr:
  case GameId::enderal:
  case GameId::enderalse:
    return "Skyrim.esm";
  case GameId::fo3:
    return "Fallout3.esm";
  case GameId::fonv:
    return "FalloutNV.esm";
  case GameId::fo4:
  case GameId::fo4vr:
    return "Fallout4.esm";
  case GameId::starfield:
    return "Starfield.esm";
  default:
    throw std::logic_error("Unrecognised game ID");
  }
}

std::string GetGameName(const GameId gameId)
{
  switch (gameId) {
  case GameId::tes3:
    return "TES III: Morrowind";
  case GameId::tes4:
    return "TES IV: Oblivion";
  case GameId::nehrim:
    return "Nehrim - At Fate's Edge";
  case GameId::tes5:
    return "TES V: Skyrim";
  case GameId::enderal:
    return "Enderal: Forgotten Stories";
  case GameId::tes5se:
    return "TES V: Skyrim Special Edition";
  case GameId::enderalse:
    return "Enderal: Forgotten Stories (Special Edition)";
  case GameId::tes5vr:
    return "TES V: Skyrim VR";
  case GameId::fo3:
    return "Fallout 3";
  case GameId::fonv:
    return "Fallout: New Vegas";
  case GameId::fo4:
    return "Fallout 4";
  case GameId::fo4vr:
    return "Fallout 4 VR";
  case GameId::starfield:
    return "Starfield";
  default:
    throw std::logic_error("Unrecognised game ID");
  }
}

std::string GetDefaultMasterlistRepositoryName(const GameId gameId)
{
  switch (gameId) {
  case GameId::tes3:
    return "morrowind";
  case GameId::tes4:
  case GameId::nehrim:
    return "oblivion";
  case GameId::tes5:
    return "skyrim";
  case GameId::enderal:
  case GameId::enderalse:
    return "enderal";
  case GameId::tes5se:
    return "skyrimse";
  case GameId::tes5vr:
    return "skyrimvr";
  case GameId::fo3:
    return "fallout3";
  case GameId::fonv:
    return "falloutnv";
  case GameId::fo4:
    return "fallout4";
  case GameId::fo4vr:
    return "fallout4vr";
  case GameId::starfield:
    return "starfield";
  default:
    throw std::logic_error("Unrecognised game type");
  }
}

std::string GetDefaultMasterlistUrl(const std::string& repositoryName)
{
  return std::string("https://raw.githubusercontent.com/loot/") + repositoryName + "/" +
         DEFAULT_MASTERLIST_BRANCH + "/masterlist.yaml";
}

std::string GetDefaultMasterlistUrl(const GameId gameId)
{
  const auto repoName = GetDefaultMasterlistRepositoryName(gameId);

  return GetDefaultMasterlistUrl(repoName);
}

GameSettings::GameSettings(const GameId gameId, std::filesystem::path lootFolder)
    : id_(gameId), type_(GetGameType(gameId)), name_(GetGameName(gameId)),
      masterFile_(GetMasterFilename(gameId)),
      minimumHeaderVersion_(GetMinimumHeaderVersion(gameId)),
      lootFolderName_(std::move(lootFolder)),
      masterlistSource_(GetDefaultMasterlistUrl(gameId))
{}

bool GameSettings::operator==(const GameSettings& rhs) const
{
  return name_ == rhs.Name() || lootFolderName_ == rhs.FolderName();
}

GameId GameSettings::Id() const
{
  return id_;
}

GameType GameSettings::Type() const
{
  return type_;
}

std::string GameSettings::Name() const
{
  return name_;
}

std::filesystem::path GameSettings::FolderName() const
{
  return lootFolderName_;
}

std::filesystem::path GameSettings::Master() const
{
  return masterFile_;
}

float GameSettings::MinimumHeaderVersion() const
{
  return minimumHeaderVersion_;
}

std::string GameSettings::MasterlistSource() const
{
  return masterlistSource_;
}

std::filesystem::path GameSettings::GamePath() const
{
  return gamePath_;
}

std::filesystem::path GameSettings::GameLocalPath() const
{
  return gameLocalPath_;
}

std::filesystem::path GameSettings::DataPath() const
{
  return gamePath_ / GetPluginsFolderName(id_);
}

GameSettings& GameSettings::SetName(const std::string& name)
{
  name_ = name;
  return *this;
}

GameSettings& GameSettings::SetMaster(const std::filesystem::path& masterFile)
{
  masterFile_ = masterFile;
  return *this;
}

GameSettings& GameSettings::SetMinimumHeaderVersion(float mininumHeaderVersion)
{
  minimumHeaderVersion_ = mininumHeaderVersion;
  return *this;
}

GameSettings& GameSettings::SetMasterlistSource(const std::string& source)
{
  masterlistSource_ = source;
  return *this;
}

GameSettings& GameSettings::SetGamePath(const std::filesystem::path& path)
{
  gamePath_ = path;
  return *this;
}

GameSettings& GameSettings::SetGameLocalPath(const std::filesystem::path& path)
{
  gameLocalPath_ = path;
  return *this;
}

GameSettings& GameSettings::SetGameLocalFolder(const std::filesystem::path& folderName)
{
  QDir appData =
      QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first();
  auto appDataPath = appData.filesystemPath();
  gameLocalPath_   = appDataPath / fs::path(folderName);
  return *this;
}
}  // namespace loot
