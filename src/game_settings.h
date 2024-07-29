#ifndef LOOT_GUI_STATE_GAME_GAME_SETTINGS
#define LOOT_GUI_STATE_GAME_GAME_SETTINGS

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "loot/enum/game_type.h"

namespace loot
{
constexpr inline std::string_view NEHRIM_STEAM_REGISTRY_KEY =
    "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App "
    "1014940\\InstallLocation";
static constexpr const char* DEFAULT_MASTERLIST_BRANCH = "v0.23";

enum struct GameId : uint8_t
{
  tes3,
  tes4,
  nehrim,
  tes5,
  enderal,
  tes5se,
  enderalse,
  tes5vr,
  fo3,
  fonv,
  fo4,
  fo4vr,
  starfield
};

GameType GetGameType(const GameId gameId);

float GetMinimumHeaderVersion(const GameId gameId);

std::filesystem::path GetPluginsFolderName(GameId gamiId);

std::string ToString(const GameId gameId);

bool SupportsLightPlugins(const GameType gameType);

std::filesystem::path GetMasterFilename(const GameId gameId);

std::string GetGameName(const GameId gameId);

std::string GetDefaultMasterlistRepositoryName(GameId gameId);

std::string GetDefaultMasterlistUrl(const std::string& repositoryName);
std::string GetDefaultMasterlistUrl(const GameId gameId);

class GameSettings
{
public:
  GameSettings() = default;
  explicit GameSettings(const GameId gameId, std::filesystem::path  lootFolder = "");

  bool operator==(const GameSettings& rhs) const;  // Compares names and folder names.

  GameId Id() const;
  GameType Type() const;
  std::string Name() const;  // Returns the game's name, eg. "TES IV: Oblivion".
  std::filesystem::path FolderName() const;
  std::filesystem::path Master() const;
  float MinimumHeaderVersion() const;
  std::string MasterlistSource() const;
  std::filesystem::path GamePath() const;
  std::filesystem::path GameLocalPath() const;
  std::filesystem::path DataPath() const;

  GameSettings& SetName(const std::string& name);
  GameSettings& SetMaster(const std::filesystem::path& masterFile);
  GameSettings& SetMinimumHeaderVersion(float minimumHeaderVersion);
  GameSettings& SetMasterlistSource(const std::string& source);
  GameSettings& SetGamePath(const std::filesystem::path& path);
  GameSettings& SetGameLocalPath(const std::filesystem::path& GameLocalPath);
  GameSettings& SetGameLocalFolder(const std::filesystem::path& folderName);

private:
  GameId id_{GameId::tes4};
  GameType type_{GameType::tes4};
  std::string name_;
  std::filesystem::path masterFile_;
  float minimumHeaderVersion_{0.0f};

  std::filesystem::path lootFolderName_;

  std::string masterlistSource_;

  std::filesystem::path gamePath_;  // Path to the game's folder.
  std::filesystem::path gameLocalPath_;
};
}  // namespace loot

#endif
