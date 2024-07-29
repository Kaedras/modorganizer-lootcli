#include "lootthread.h"
#include "game_settings.h"
#include "version.h"
#include <QStandardPaths>
#include <codecvt>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <fstream>

// using namespace loot;
namespace fs = std::filesystem;

using std::lock_guard;
using std::recursive_mutex;

namespace lootcli
{
static const std::set<std::string> oldDefaultBranches({"master", "v0.7", "v0.8",
                                                       "v0.10", "v0.13", "v0.14",
                                                       "v0.15", "v0.17", "v0.18"});
static const std::regex GITHUB_REPO_URL_REGEX =
    std::regex(R"(^https://github\.com/([^/]+)/([^/]+?)(?:\.git)?/?$)",
               std::regex::ECMAScript | std::regex::icase);

std::string toString(loot::MessageType type)
{
  switch (type) {
  case loot::MessageType::say:
    return "info";
  case loot::MessageType::warn:
    return "warn";
  case loot::MessageType::error:
    return "error";
  default:
    return "unknown";
  }
}

LOOTWorker::LOOTWorker()
    : m_GameId(loot::GameId::tes5), m_GameName("Skyrim"),
      m_LogLevel(loot::LogLevel::info)
{
  // initialize curlpp, this must only be done once
  cURLpp::initialize();
}

std::string ToLower(std::string text)
{
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });

  return text;
}

void LOOTWorker::setGame(const std::string& gameName)
{
  static std::map<std::string, loot::GameId> gameMap = {
      {"morrowind", loot::GameId::tes3},     {"oblivion", loot::GameId::tes4},
      {"fallout3", loot::GameId::fo3},       {"fallout4", loot::GameId::fo4},
      {"fallout4vr", loot::GameId::fo4vr},   {"falloutnv", loot::GameId::fonv},
      {"skyrim", loot::GameId::tes5},        {"skyrimse", loot::GameId::tes5se},
      {"skyrimvr", loot::GameId::tes5vr},    {"nehrim", loot::GameId::nehrim},
      {"enderal", loot::GameId::enderal},    {"enderalse", loot::GameId::enderalse},
      {"starfield", loot::GameId::starfield}};

  auto iter = gameMap.find(ToLower(gameName));

  if (iter != gameMap.end()) {
    m_GameId   = iter->second;
    m_GameName = loot::ToString(m_GameId);
  } else {
    throw std::runtime_error("invalid game name \"" + gameName + "\"");
  }
}

void LOOTWorker::setGamePath(const std::filesystem::path& gamePath)
{
  m_GamePath = gamePath;
}

void LOOTWorker::setOutput(const std::filesystem::path& outputPath)
{
  m_OutputPath = outputPath;
}

void LOOTWorker::setUpdateMasterlist(bool update)
{
  m_UpdateMasterlist = update;
}

void LOOTWorker::setPluginListPath(const std::filesystem::path& pluginListPath)
{
  m_PluginListPath = pluginListPath;
}

void LOOTWorker::setLanguageCode(const std::string& languageCode)
{
  m_Language = languageCode;
}

void LOOTWorker::setLogLevel(loot::LogLevel level)
{
  m_LogLevel = level;
}

fs::path GetLOOTAppData()
{
  QDir appData =
      QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).first();
  auto path = appData.filesystemPath();
  return path / "LOOT";
}

fs::path LOOTWorker::gamePath() const
{
  return GetLOOTAppData() / "games" / m_GameSettings.FolderName();
}

fs::path LOOTWorker::masterlistPath() const
{
  return gamePath() / "masterlist.yaml";
}

fs::path LOOTWorker::userlistPath() const
{
  return gamePath() / "userlist.yaml";
}
fs::path LOOTWorker::settingsPath() const
{
  return GetLOOTAppData() / "settings.toml";
}

fs::path LOOTWorker::l10nPath() const
{
  return GetLOOTAppData() / "resources" / "l10n";
}

fs::path LOOTWorker::dataPath() const
{
  return m_GameSettings.DataPath();
}

void LOOTWorker::getSettings(const fs::path& file)
{
  lock_guard<recursive_mutex> guard(mutex_);
  // Don't use cpptoml::parse_file() as it just uses a std stream,
  // which don't support UTF-8 paths on Windows.
  std::ifstream in(file);
  if (!in.is_open())
    throw std::runtime_error(file.string() + " could not be opened for parsing");

  const auto settings = toml::parse(in, file.string());
  const auto games    = settings["games"];
  if (games.is_array_of_tables()) {
    for (const auto& game : *games.as_array()) {
      try {
        if (!game.is_table()) {
          throw std::runtime_error("games array element is not a table");
        }
        auto gameTable = *game.as_table();

        using loot::GameId;
        using loot::GameSettings;

        auto id = gameTable["gameId"].value<std::string>();
        if (!id) {
          throw std::runtime_error(
              "'gameId' and 'type' keys both missing from game settings table");
        }
        const auto gameType = *id;
        GameId gameId;

        if (gameType == "Morrowind") {
          gameId = GameId::tes3;
        } else if (gameType == "Oblivion") {
          // The Oblivion game type is shared between Oblivon and Nehrim.
          gameId = IsNehrim(gameTable) ? GameId::nehrim : GameId::tes4;
        } else if (gameType == "Skyrim") {
          // The Skyrim game type is shared between Skyrim and Enderal.
          gameId = IsEnderal(gameTable) ? GameId::enderal : GameId::tes5;
        } else if (gameType == "SkyrimSE" || gameType == "Skyrim Special Edition") {
          // The Skyrim SE game type is shared between Skyrim SE and Enderal SE.
          gameId = IsEnderalSE(gameTable) ? GameId::enderalse : GameId::tes5se;
        } else if (gameType == "Skyrim VR") {
          gameId = GameId::tes5vr;
        } else if (gameType == "Fallout3") {
          gameId = GameId::fo3;
        } else if (gameType == "FalloutNV") {
          gameId = GameId::fonv;
        } else if (gameType == "Fallout4") {
          gameId = GameId::fo4;
        } else if (gameType == "Fallout4VR") {
          gameId = GameId::fo4vr;
        } else {
          throw std::runtime_error(
              "invalid value for 'type' key in game settings table");
        }

        auto folder = gameTable["folder"].value<std::string>();
        if (!folder) {
          throw std::runtime_error("'folder' key missing from game settings table");
        }

        const auto type = gameTable["type"].value<std::string>();

        // SkyrimSE was a previous serialised value for GameType::tes5se,
        // and the game folder name LOOT created for that game type.
        if (type && *type == "SkyrimSE" && *folder == *type) {
          folder = "Skyrim Special Edition";
        }

        GameSettings newSettings(gameId, folder.value());

        if (newSettings.Type() == m_GameSettings.Type()) {

          auto name = gameTable["name"].value<std::string>();
          if (name) {
            newSettings.SetName(*name);
          }

          auto master = gameTable["master"].value<std::string>();
          if (master) {
            newSettings.SetMaster(*master);
          }

          const auto minimumHeaderVersion =
              gameTable["minimumHeaderVersion"].value<double>();
          if (minimumHeaderVersion) {
            newSettings.SetMinimumHeaderVersion((float)*minimumHeaderVersion);
          }

          auto source = gameTable["masterlistSource"].value<std::string>();
          if (source) {
            newSettings.SetMasterlistSource(migrateMasterlistSource(*source));
          } else {
            auto url    = gameTable["repo"].value<std::string>();
            auto branch = gameTable["branch"].value<std::string>();
            auto migratedSource =
                migrateMasterlistRepoSettings(newSettings.Id(), *url, *branch);
            if (migratedSource.has_value()) {
              newSettings.SetMasterlistSource(migratedSource.value());
            }
          }

          auto path = gameTable["path"].value<std::string>();
          if (path) {
            newSettings.SetGamePath(std::filesystem::u8path(*path));
          }

          auto localPath   = gameTable["local_path"].value<std::string>();
          auto localFolder = gameTable["local_folder"].value<std::string>();
          if (localPath && localFolder) {
            throw std::runtime_error(
                "Game settings have local_path and local_folder set, use only one.");
          } else if (localPath) {
            newSettings.SetGameLocalPath(std::filesystem::u8path(*localPath));
          } else if (localFolder) {
            newSettings.SetGameLocalFolder(*localFolder);
          }

          m_GameSettings = newSettings;
          break;
        }
      } catch (...) {
        // Skip invalid games.
      }
    }
  }

  if (m_Language.empty()) {
    m_Language = settings["language"].value_or(loot::MessageContent::DEFAULT_LANGUAGE);
  }
}

std::optional<std::filesystem::path>
LOOTWorker::GetLocalFolder(const toml::table& table)
{
  const auto localPath   = table["local_path"].value<std::string>();
  const auto localFolder = table["local_folder"].value<std::string>();

  if (localFolder.has_value()) {
    return localFolder;
  }

  if (localPath.has_value()) {
    return std::filesystem::u8path(*localPath).filename();
  }

  return std::nullopt;
}

bool LOOTWorker::IsNehrim(const toml::table& table)
{
  const auto installPath = table["path"].value<std::string>();

  if (installPath.has_value() && !installPath.value().empty()) {
    const auto path = std::filesystem::u8path(installPath.value());
    if (std::filesystem::exists(path)) {
      return std::filesystem::exists(path / "NehrimLauncher.exe");
    }
  }

  // Fall back to using heuristics based on the existing settings.
  // Return true if any of these heuristics return a positive match.
  const auto gameName           = table["name"].value<std::string>();
  const auto masterFilename     = table["master"].value<std::string>();
  const auto isBaseGameInstance = table["isBaseGameInstance"].value<bool>();
  const auto folder             = table["folder"].value<std::string>();

  return
      // Nehrim uses a different main master file from Oblivion.
      (masterFilename.has_value() &&
       masterFilename.value() == loot::GetMasterFilename(loot::GameId::nehrim)) ||
      // Game name probably includes "nehrim".
      (gameName.has_value() && boost::icontains(gameName.value(), "nehrim")) ||
      // LOOT folder name probably includes "nehrim".
      (folder.has_value() && boost::icontains(folder.value(), "nehrim")) ||
      // Between 0.18.1 and 0.19.0 inclusive, LOOT had an isBaseGameInstance
      // game setting that was false for Nehrim, Enderal and Enderal SE.
      (isBaseGameInstance.has_value() && !isBaseGameInstance.value());
}

bool LOOTWorker::IsEnderal(const toml::table& table,
                           const std::filesystem::path& expectedLocalFolder)
{
  const auto installPath = table["path"].value<std::string>();

  if (installPath.has_value() && !installPath.value().empty()) {
    const auto path = std::filesystem::u8path(installPath.value());
    if (std::filesystem::exists(path)) {
      return std::filesystem::exists(path / "Enderal Launcher.exe");
    }
  }

  // Fall back to using heuristics based on the existing settings.
  // Return true if any of these heuristics return a positive match.
  const auto gameName           = table["name"].value<std::string>();
  const auto isBaseGameInstance = table["isBaseGameInstance"].value<bool>();
  const auto localFolder        = GetLocalFolder(table);
  const auto folder             = table["folder"].value<std::string>();

  return
      // Enderal and Enderal SE use different local folders than their base
      // games.
      (localFolder.has_value() && localFolder.value() == expectedLocalFolder) ||
      // Game name probably includes "enderal".
      (gameName.has_value() && boost::icontains(gameName.value(), "enderal")) ||
      // LOOT folder name probably includes "enderal".
      (folder.has_value() && boost::icontains(folder.value(), "enderal")) ||
      // Between 0.18.1 and 0.19.0 inclusive, LOOT had an isBaseGameInstance
      // game setting that was false for Nehrim, Enderal and Enderal SE.
      (isBaseGameInstance.has_value() && !isBaseGameInstance.value());
}

bool LOOTWorker::IsEnderal(const toml::table& table)
{
  return IsEnderal(table, "enderal");
}

bool LOOTWorker::IsEnderalSE(const toml::table& table)
{
  return IsEnderal(table, "Enderal Special Edition");
}

std::string LOOTWorker::getOldDefaultRepoUrl(loot::GameId GameId)
{
  switch (GameId) {
  case loot::GameId::tes3:
    return "https://github.com/loot/morrowind.git";
  case loot::GameId::tes4:
    return "https://github.com/loot/oblivion.git";
  case loot::GameId::tes5:
    return "https://github.com/loot/skyrim.git";
  case loot::GameId::tes5se:
    return "https://github.com/loot/skyrimse.git";
  case loot::GameId::tes5vr:
    return "https://github.com/loot/skyrimvr.git";
  case loot::GameId::fo3:
    return "https://github.com/loot/fallout3.git";
  case loot::GameId::fonv:
    return "https://github.com/loot/falloutnv.git";
  case loot::GameId::fo4:
    return "https://github.com/loot/fallout4.git";
  case loot::GameId::fo4vr:
    return "https://github.com/loot/fallout4vr.git";
  default:
    throw std::runtime_error(
        "Unrecognised game type: " +
        std::to_string(static_cast<std::underlying_type_t<loot::GameId>>(GameId)));
  }
}

bool LOOTWorker::isLocalPath(const std::filesystem::path& location,
                             const std::filesystem::path& filename)
{
  if (location.string().starts_with("http://") ||
      location.string().starts_with("https://")) {
    return false;
  }

  // Could be a local path. Only return true if it points to a non-bare
  // Git repository that currently has the given branch checked out and
  // the given filename exists in the repo root.

  auto filePath = location / filename;

  if (!std::filesystem::is_regular_file(filePath)) {
    return false;
  }

  auto headFilePath = location / ".git" / "HEAD";

  return std::filesystem::is_regular_file(headFilePath);
}

bool LOOTWorker::isBranchCheckedOut(const std::filesystem::path& localGitRepo,
                                    const std::string& branch)
{
  auto headFilePath = localGitRepo / ".git" / "HEAD";

  std::ifstream in(headFilePath);
  if (!in.is_open()) {
    return false;
  }

  std::string line;
  std::getline(in, line);
  in.close();

  return line == "ref: refs/heads/" + branch;
}

std::optional<std::string>
LOOTWorker::migrateMasterlistRepoSettings(loot::GameId GameId, std::string url,
                                          std::string branch)
{

  if (oldDefaultBranches.count(branch) == 1) {
    // Update to the latest masterlist branch.
    log(loot::LogLevel::info, "Updating masterlist repository branch from " + branch +
                                  " to " + loot::DEFAULT_MASTERLIST_BRANCH);
    branch = loot::DEFAULT_MASTERLIST_BRANCH;
  }

  if (GameId == loot::GameId::tes5vr && url == "https://github.com/loot/skyrimse.git") {
    // Switch to the VR-specific repository (introduced for LOOT v0.17.0).
    auto newUrl = "https://github.com/loot/skyrimvr.git";
    log(loot::LogLevel::info,
        "Updating masterlist repository URL from" + url + " to " + newUrl);
    url = newUrl;
  }

  if (GameId == loot::GameId::fo4vr && url == "https://github.com/loot/fallout4.git") {
    // Switch to the VR-specific repository (introduced for LOOT v0.17.0).
    auto newUrl = "https://github.com/loot/fallout4vr.git";
    log(loot::LogLevel::info,
        "Updating masterlist repository URL from " + url + " to " + newUrl);
    url = newUrl;
  }

  auto filename = "masterlist.yaml";
  if (isLocalPath(url, filename)) {
    auto localRepoPath = std::filesystem::u8path(url);
    if (!isBranchCheckedOut(localRepoPath, branch)) {
      log(loot::LogLevel::warning,
          "The URL " + url +
              " is a local Git repository path but the configured branch " + branch +
              " is not checked out. LOOT will use the path as the masterlist "
              "source, but there may be unexpected differences in the loaded "
              "metadata if the " +
              branch +
              " branch is not manually checked out before the "
              "next time the masterlist is updated.");
    }

    return (localRepoPath / filename).string();
  }

  std::smatch regexMatches;
  std::regex_match(url, regexMatches, GITHUB_REPO_URL_REGEX);
  if (regexMatches.size() != 3) {
    log(loot::LogLevel::warning,
        "Cannot migrate masterlist repository settings as the URL does not "
        "point to a repository on GitHub.");
    return std::nullopt;
  }

  auto githubOwner = regexMatches.str(1);
  auto githubRepo  = regexMatches.str(2);

  return "https://raw.githubusercontent.com/" + githubOwner + "/" + githubRepo + "/" +
         branch + "/masterlist.yaml";
}

std::string LOOTWorker::migrateMasterlistSource(const std::string& source)
{
  static const std::vector<std::string> officialMasterlistRepos = {
      "morrowind", "oblivion",  "skyrim",   "skyrimse",   "skyrimvr",
      "fallout3",  "falloutnv", "fallout4", "fallout4vr", "enderal"};

  for (const auto& repo : officialMasterlistRepos) {
    for (const auto& branch : oldDefaultBranches) {
      const auto url = "https://raw.githubusercontent.com/loot/" + repo + "/" + branch +
                       "/masterlist.yaml";

      if (source == url) {
        const auto newSource = loot::GetDefaultMasterlistUrl(repo);

        log(loot::LogLevel::info,
            "Migrating masterlist source from " + source + " to " + newSource);

        return newSource;
      }
    }
  }

  return source;
}

void LOOTWorker::GetFile(const char* szUrl,                        // Full URL
                         const std::filesystem::path& szFileName)  // Local file name
{
  try {
    std::ofstream ostream(szFileName);

    cURLpp::Easy request;

    request.setOpt(cURLpp::Options::WriteStream(&ostream));
    request.setOpt(cURLpp::Options::Url(szUrl));
    request.setOpt(cURLpp::Options::UserAgent("lootcli/1.5.0"));

    request.perform();

  } catch (...) {
    throw;
  }
}

std::string escape(const std::string& s)
{
  return boost::replace_all_copy(s, "\"", "\\\"");
}

int LOOTWorker::run()
{
  m_startTime = std::chrono::high_resolution_clock::now();

  {
    // Do some preliminary locale / UTF-8 support setup here, in case the settings file
    // reading requires it.
    // Boost.Locale initialisation: Specify location of language dictionaries.
    boost::locale::generator gen;
    gen.add_messages_path(l10nPath().string());
    gen.add_messages_domain("loot");

    // Boost.Locale initialisation: Generate and imbue locales.
    std::locale::global(gen("en.UTF-8"));
  }

  loot::SetLoggingCallback([&](loot::LogLevel level, const char* message) {
    log(level, message);
  });

  try {
    fs::path profile(m_PluginListPath);
    profile = profile.parent_path();

    m_GameSettings = loot::GameSettings(m_GameId, loot::ToString(m_GameId));

    fs::path settings = settingsPath();

    if (fs::exists(settings))
      getSettings(settings);

    m_GameSettings.SetGamePath(m_GamePath);

    std::unique_ptr<loot::GameInterface> gameHandle = CreateGameHandle(
        m_GameSettings.Type(), m_GameSettings.GamePath(), profile);

    if (!GetLOOTAppData().empty()) {
      // Make sure that the LOOT game path exists.
      auto lootGamePath = gamePath();
      if (!fs::is_directory(lootGamePath)) {
        if (fs::exists(lootGamePath)) {
          throw loot::FileAccessError(
              "Could not create LOOT folder for game, the path exists but is not "
              "a directory");
        }

        std::vector<fs::path> legacyGamePaths{GetLOOTAppData() /
                                              m_GameSettings.FolderName()};

        if (m_GameSettings.Id() == loot::GameId::tes5se) {
          // LOOT v0.10.0 used SkyrimSE as its folder name for Skyrim SE, so
          // migrate from that if it's present.
          legacyGamePaths.insert(legacyGamePaths.begin(),
                                 GetLOOTAppData() / "SkyrimSE");
        }

        for (const auto& legacyGamePath : legacyGamePaths) {
          if (fs::is_directory(legacyGamePath)) {
            log(loot::LogLevel::info,
                "Found a folder for this game in the LOOT data folder, "
                "assuming "
                "that it's a legacy game folder and moving into the correct "
                "subdirectory...");

            fs::create_directories(lootGamePath.parent_path());
            fs::rename(legacyGamePath, lootGamePath);
            break;
          }
        }

        fs::create_directories(lootGamePath);
      }
    }

    if (m_Language != loot::MessageContent::DEFAULT_LANGUAGE) {
      log(loot::LogLevel::debug, "initialising language settings");
      log(loot::LogLevel::debug, "selected language: " + m_Language);

      // Boost.Locale initialisation: Generate and imbue locales.
      boost::locale::generator gen;
      std::locale::global(gen(m_Language + ".UTF-8"));
    }

    if (true) {
      progress(Progress::CheckingMasterlistExistence);
      if (!fs::exists(masterlistPath())) {
        fs::create_directories(masterlistPath().parent_path());
      }

      progress(Progress::UpdatingMasterlist);

      log(loot::LogLevel::info, "Downloading latest masterlist file from " +
                                    m_GameSettings.MasterlistSource() + " to " +
                                    masterlistPath().string());
      try {
        GetFile(m_GameSettings.MasterlistSource().c_str(),
                masterlistPath());
      } catch (const std::exception& ex) {
        log(loot::LogLevel::error, std::format("GetFile failed: {}", ex.what()));
        return 1;
      }
    }

    progress(Progress::LoadingLists);

    fs::path userlist = userlistPath();
    gameHandle->GetDatabase().LoadLists(
        masterlistPath(), fs::exists(userlist) ? userlistPath() : fs::path());

    progress(Progress::ReadingPlugins);
    gameHandle->LoadCurrentLoadOrderState();
    std::vector<std::filesystem::path> pluginsList;
    for (auto plugin : gameHandle->GetLoadOrder()) {
      std::filesystem::path pluginPath(plugin);
      pluginsList.push_back(pluginPath);
    }

    progress(Progress::SortingPlugins);
    std::vector<std::string> sortedPlugins = gameHandle->SortPlugins(pluginsList);

    progress(Progress::WritingLoadorder);

    std::ofstream outf(m_PluginListPath);
    if (!outf) {
      log(loot::LogLevel::error,
          std::format("failed to open {} to rewrite it", m_PluginListPath.string()));
      return 1;
    }
    outf << "# This file was automatically generated by Mod Organizer." << std::endl;
    for (const std::string& plugin : sortedPlugins) {
      outf << plugin << std::endl;
    }
    outf.close();

    progress(Progress::ParsingLootMessages);
    std::ofstream(m_OutputPath) << createJsonReport(*gameHandle, sortedPlugins);
  } catch (std::system_error& e) {
    log(loot::LogLevel::error, e.what());
    return 1;
  } catch (const std::exception& e) {
    log(loot::LogLevel::error, e.what());
    return 1;
  }

  progress(Progress::Done);

  return 0;
}

void set(QJsonObject& o, const char* e, const QJsonValue& v)
{
  if (v.isObject() && v.toObject().isEmpty()) {
    return;
  }

  if (v.isArray() && v.toArray().isEmpty()) {
    return;
  }

  if (v.isString() && v.toString().isEmpty()) {
    return;
  }

  o[e] = v;
}

std::string
LOOTWorker::createJsonReport(loot::GameInterface& game,
                             const std::vector<std::string>& sortedPlugins) const
{
  QJsonObject root;

  set(root, "messages", createMessages(game.GetDatabase().GetGeneralMessages(true)));
  set(root, "plugins", createPlugins(game, sortedPlugins));

  const auto end = std::chrono::high_resolution_clock::now();

  set(root, "stats",
      QJsonObject{{"time", qint64(std::chrono::duration_cast<std::chrono::milliseconds>(
                                      end - m_startTime)
                                      .count())},
                  {"lootcliVersion", LOOTCLI_VERSION_STRING},
                  {"lootVersion", QString::fromStdString(loot::GetLiblootVersion())}});

  QJsonDocument doc(root);
  return doc.toJson(QJsonDocument::Indented).toStdString();
}

template <class Container>
QJsonArray createStringArray(const Container& c)
{
  QJsonArray array;

  for (auto&& e : c) {
    array.push_back(QString::fromStdString(e));
  }

  return array;
}

QJsonArray
LOOTWorker::createPlugins(loot::GameInterface& game,
                          const std::vector<std::string>& sortedPlugins) const
{
  QJsonArray plugins;

  for (auto&& pluginName : sortedPlugins) {

    auto plugin = game.GetPlugin(pluginName);

    QJsonObject o;
    o["name"] = QString::fromStdString(pluginName);

    if (auto metaData = game.GetDatabase().GetPluginMetadata(pluginName, true, true)) {
      set(o, "incompatibilities",
          createIncompatibilities(game, metaData->GetIncompatibilities()));
      set(o, "messages", createMessages(metaData->GetMessages()));
      set(o, "dirty", createDirty(metaData->GetDirtyInfo()));
      set(o, "clean", createClean(metaData->GetCleanInfo()));
    }

    set(o, "missingMasters", createMissingMasters(game, pluginName));

    if (plugin->LoadsArchive()) {
      o["loadsArchive"] = true;
    }

    if (plugin->IsMaster()) {
      o["isMaster"] = true;
    }

    if (plugin->IsLightPlugin()) {
      o["isLightMaster"] = true;
    }

    // don't add if the name is the only thing in there
    if (o.size() > 1) {
      plugins.push_back(o);
    }
  }

  return plugins;
}

QJsonValue LOOTWorker::createMessages(const std::vector<loot::Message>& list) const
{
  QJsonArray messages;

  for (loot::Message m : list) {
    auto simpleMessage = loot::SelectMessageContent(m.GetContent(), m_Language);
    if (simpleMessage.has_value()) {
      messages.push_back(QJsonObject{
          {"type", QString::fromStdString(toString(m.GetType()))},
          {"text", QString::fromStdString(simpleMessage.value().GetText())}});
    }
  }

  return messages;
}

QJsonValue
LOOTWorker::createDirty(const std::vector<loot::PluginCleaningData>& data) const
{
  QJsonArray array;

  for (const auto& d : data) {
    QJsonObject o{
        {"crc", static_cast<qint64>(d.GetCRC())},
        {"itm", static_cast<qint64>(d.GetITMCount())},
        {"deletedReferences", static_cast<qint64>(d.GetDeletedReferenceCount())},
        {"deletedNavmesh", static_cast<qint64>(d.GetDeletedNavmeshCount())},
    };

    set(o, "cleaningUtility", QString::fromStdString(d.GetCleaningUtility()));
    auto simpleMessage = loot::SelectMessageContent(
        loot::Message(loot::MessageType::say, d.GetDetail()).GetContent(), m_Language);
    if (simpleMessage.has_value()) {
      set(o, "info", QString::fromStdString(simpleMessage.value().GetText()));
    } else {
      set(o, "info", QString::fromStdString(""));
    }

    array.push_back(o);
  }

  return array;
}

QJsonValue
LOOTWorker::createClean(const std::vector<loot::PluginCleaningData>& data) const
{
  QJsonArray array;

  for (const auto& d : data) {
    QJsonObject o{
        {"crc", static_cast<qint64>(d.GetCRC())},
    };

    set(o, "cleaningUtility", QString::fromStdString(d.GetCleaningUtility()));
    auto simpleMessage = loot::SelectMessageContent(
        loot::Message(loot::MessageType::say, d.GetDetail()).GetContent(), m_Language);
    if (simpleMessage.has_value()) {
      set(o, "info", QString::fromStdString(simpleMessage.value().GetText()));
    } else {
      set(o, "info", QString::fromStdString(""));
    }

    array.push_back(o);
  }

  return array;
}

QJsonValue
LOOTWorker::createIncompatibilities(loot::GameInterface& game,
                                    const std::vector<loot::File>& data) const
{
  QJsonArray array;

  for (auto&& f : data) {
    const auto n = static_cast<std::string>(f.GetName());
    if (!game.GetPlugin(n)) {
      continue;
    }

    const auto name        = QString::fromStdString(n);
    const auto displayName = QString::fromStdString(f.GetDisplayName());

    QJsonObject o{{"name", name}};

    if (displayName != name) {
      set(o, "displayName", displayName);
    }

    array.push_back(std::move(o));
  }

  return array;
}

QJsonValue LOOTWorker::createMissingMasters(loot::GameInterface& game,
                                            const std::string& pluginName) const
{
  QJsonArray array;

  for (auto&& master : game.GetPlugin(pluginName)->GetMasters()) {
    if (!game.GetPlugin(master)) {
      array.push_back(QString::fromStdString(master));
    }
  }

  return array;
}

void LOOTWorker::progress(Progress p)
{
  std::cout << "[progress] " << static_cast<int>(p) << "\n";
  std::cout.flush();
}

std::string escapeNewlines(const std::string& s)
{
  auto ss = boost::replace_all_copy(s, "\n", "\\n");
  boost::replace_all(ss, "\r", "\\r");
  return ss;
}

void LOOTWorker::log(loot::LogLevel level, const std::string& message) const
{
  if (level < m_LogLevel) {
    return;
  }

  const auto ll        = fromLootLogLevel(level);
  const auto levelName = logLevelToString(ll);

  std::cout << "[" << levelName << "] " << escapeNewlines(message) << "\n";
  std::cout.flush();
}

loot::LogLevel toLootLogLevel(lootcli::LogLevels level)
{
  using L  = loot::LogLevel;
  using LC = lootcli::LogLevels;

  switch (level) {
  case LC::Trace:
    return L::trace;
  case LC::Debug:
    return L::debug;
  case LC::Info:
    return L::info;
  case LC::Warning:
    return L::warning;
  case LC::Error:
    return L::error;
  default:
    return L::info;
  }
}

lootcli::LogLevels fromLootLogLevel(loot::LogLevel level)
{
  using L  = loot::LogLevel;
  using LC = lootcli::LogLevels;

  switch (level) {
  case L::trace:
    return LC::Trace;

  case L::debug:
    return LC::Debug;

  case L::info:
    return LC::Info;

  case L::warning:
    return LC::Warning;

  case L::error:  // fall-through
  case L::fatal:
    return LC::Error;

  default:
    return LC::Info;
  }
}

}  // namespace lootcli
