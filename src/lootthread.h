#ifndef LOOTTHREAD_H
#define LOOTTHREAD_H

#include "game_settings.h"
#include <lootcli/lootcli.h>

#include <curlpp/cURLpp.hpp>

namespace loot
{
class Game;
}

namespace lootcli
{

loot::LogLevel toLootLogLevel(lootcli::LogLevels level);
lootcli::LogLevels fromLootLogLevel(loot::LogLevel level);

class LOOTWorker
{
public:
  explicit LOOTWorker();

  void setGame(const std::string& gameName);
  void setGamePath(const std::filesystem::path& gamePath);
  void setOutput(const std::filesystem::path& outputPath);
  void setPluginListPath(const std::filesystem::path& pluginListPath);
  void
  setLanguageCode(const std::string& language_code);  // Will add this when I figure out
                                                      // how languages work on MO
  void setLogLevel(loot::LogLevel level);

  void setUpdateMasterlist(bool update);

  int run();

private:
  void progress(Progress p);
  void log(loot::LogLevel level, const std::string& message) const;

  void GetFile(const char* szUrl, const std::filesystem::path& szFileName);
  void getSettings(const std::filesystem::path& file);
  std::string getOldDefaultRepoUrl(loot::GameId gameType);
  std::optional<std::filesystem::path> GetLocalFolder(const toml::table& table);
  bool IsNehrim(const toml::table& table);
  bool IsEnderal(const toml::table& table, const std::filesystem::path& expectedLocalFolder);
  bool IsEnderal(const toml::table& table);
  bool IsEnderalSE(const toml::table& table);
  bool isLocalPath(const std::filesystem::path& location, const std::filesystem::path& filename);
  bool isBranchCheckedOut(const std::filesystem::path& localGitRepo,
                          const std::string& branch);
  std::optional<std::string> migrateMasterlistRepoSettings(loot::GameId gameType,
                                                           std::string url,
                                                           std::string branch);
  std::string migrateMasterlistSource(const std::string& source);

  std::filesystem::path gamePath() const;
  std::filesystem::path masterlistPath() const;
  std::filesystem::path settingsPath() const;
  std::filesystem::path userlistPath() const;
  std::filesystem::path l10nPath() const;
  std::filesystem::path dataPath() const;

private:
  // void handleErr(unsigned int resultCode, const char *description);
  bool sort(loot::Game& game);
  // const char *lootErrorString(unsigned int errorCode);
  // template <typename T> T resolveVariable(HMODULE lib, const char *name);
  // template <typename T> T resolveFunction(HMODULE lib, const char *name);

private:
  loot::GameId m_GameId;
  std::string m_Language;
  std::string m_GameName;
  std::filesystem::path m_GamePath;
  std::filesystem::path m_OutputPath;
  std::filesystem::path m_PluginListPath;
  loot::LogLevel m_LogLevel;
  bool m_UpdateMasterlist;
  mutable std::recursive_mutex mutex_;
  loot::GameSettings m_GameSettings;
  std::chrono::high_resolution_clock::time_point m_startTime;

  std::string createJsonReport(loot::GameInterface& game,
                               const std::vector<std::string>& sortedPlugins) const;

  QJsonArray createPlugins(loot::GameInterface& game,
                           const std::vector<std::string>& sortedPlugins) const;

  QJsonValue createMessages(const std::vector<loot::Message>& list) const;
  QJsonValue createDirty(const std::vector<loot::PluginCleaningData>& data) const;
  QJsonValue createClean(const std::vector<loot::PluginCleaningData>& data) const;

  QJsonValue createIncompatibilities(loot::GameInterface& game,
                                     const std::vector<loot::File>& data) const;

  QJsonValue createMissingMasters(loot::GameInterface& game,
                                  const std::string& pluginName) const;
};

}  // namespace lootcli

#endif  // LOOTTHREAD_H
