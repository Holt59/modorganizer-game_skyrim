#include "gameskyrim.h"

#include "skyrimbsainvalidation.h"
#include "skyrimscriptextender.h"
#include "skyrimdataarchives.h"
#include "skyrimsavegameinfo.h"

#include "executableinfo.h"
#include "pluginsetting.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>

#include <QtDebug>

#include <Windows.h>
#include <winver.h>

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace MOBase;

GameSkyrim::GameSkyrim()
{
}

bool GameSkyrim::init(IOrganizer *moInfo)
{
  if (!GameGamebryo::init(moInfo)) {
    return false;
  }
  m_ScriptExtender = std::shared_ptr<ScriptExtender>(new SkyrimScriptExtender(this));
  m_DataArchives = std::shared_ptr<DataArchives>(new SkyrimDataArchives());
  m_BSAInvalidation = std::shared_ptr<BSAInvalidation>(new SkyrimBSAInvalidation(m_DataArchives, this));
  m_SaveGameInfo = std::shared_ptr<SaveGameInfo>(new SkyrimSaveGameInfo(this));
  return true;
}

QString GameSkyrim::gameName() const
{
  return "Skyrim";
}

QList<ExecutableInfo> GameSkyrim::executables() const
{
  return QList<ExecutableInfo>()
      << ExecutableInfo("SKSE", findInGameFolder(m_ScriptExtender->loaderName()))
      << ExecutableInfo("SBW", findInGameFolder("SBW.exe"))
      << ExecutableInfo("Skyrim", findInGameFolder(getBinaryName()))
      << ExecutableInfo("Skyrim Launcher", findInGameFolder(getLauncherName()))
      << ExecutableInfo("BOSS", findInGameFolder("BOSS/BOSS.exe"))
      << ExecutableInfo("LOOT", getLootPath())
      << ExecutableInfo("Creation Kit", findInGameFolder("CreationKit.exe")).withSteamAppId("202480")
  ;
}

QString GameSkyrim::name() const
{
  return "Skyrim Support Plugin";
}

QString GameSkyrim::author() const
{
  return "Tannin";
}

QString GameSkyrim::description() const
{
  return tr("Adds support for the game Skyrim");
}

MOBase::VersionInfo GameSkyrim::version() const
{
  return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

bool GameSkyrim::isActive() const
{
  return qApp->property("managed_game").value<IPluginGame*>() == this;
}

QList<PluginSetting> GameSkyrim::settings() const
{
  return QList<PluginSetting>();
}

void GameSkyrim::initializeProfile(const QDir &path, ProfileSettings settings) const
{
  if (settings.testFlag(IPluginGame::MODS)) {
    copyToProfile(localAppFolder() + "/Skyrim", path, "plugins.txt");
    copyToProfile(localAppFolder() + "/Skyrim", path, "loadorder.txt");
  }

  if (settings.testFlag(IPluginGame::CONFIGURATION)) {
    if (settings.testFlag(IPluginGame::PREFER_DEFAULTS)
        || !QFileInfo(myGamesPath() + "/skyrim.ini").exists()) {
      copyToProfile(gameDirectory().absolutePath(), path, "skyrim_default.ini", "skyrim.ini");
    } else {
      copyToProfile(myGamesPath(), path, "skyrim.ini");
    }

    copyToProfile(myGamesPath(), path, "skyrimprefs.ini");
  }
}

QString GameSkyrim::savegameExtension() const
{
  return "ess";
}

QString GameSkyrim::steamAPPId() const
{
  return "72850";
}

QStringList GameSkyrim::getPrimaryPlugins() const
{
  return { "skyrim.esm", "update.esm" };
}

QString GameSkyrim::getBinaryName() const
{
  return "TESV.exe";
}

QString GameSkyrim::getGameShortName() const
{
  return "Skyrim";
}

QStringList GameSkyrim::getIniFiles() const
{
  return { "skyrim.ini", "skyrimprefs.ini" };
}

QStringList GameSkyrim::getDLCPlugins() const
{
  return { "Dawnguard.esm", "Dragonborn.esm", "HearthFires.esm",
           "HighResTexturePack01.esp", "HighResTexturePack02.esp", "HighResTexturePack03.esp" };
}

namespace {
//Note: This is ripped off from shared/util. And in an upcoming move, the fomod
//installer requires something similar. I suspect I should abstract this out
//into gamebryo (or lower level)

VS_FIXEDFILEINFO GetFileVersion(const std::wstring &fileName)
{
  DWORD handle = 0UL;
  DWORD size = ::GetFileVersionInfoSizeW(fileName.c_str(), &handle);
  if (size == 0) {
    throw std::runtime_error("failed to determine file version info size");
  }

  std::vector<char> buffer(size);
  handle = 0UL;
  if (!::GetFileVersionInfoW(fileName.c_str(), handle, size, buffer.data())) {
    throw std::runtime_error("failed to determine file version info");
  }

  void *versionInfoPtr = nullptr;
  UINT versionInfoLength = 0;
  if (!::VerQueryValue(buffer.data(), L"\\", &versionInfoPtr, &versionInfoLength)) {
    throw std::runtime_error("failed to determine file version");
  }

  return *static_cast<VS_FIXEDFILEINFO*>(versionInfoPtr);
}

}

IPluginGame::LoadOrderMechanism GameSkyrim::getLoadOrderMechanism() const
{
  try {
    std::wstring fileName = gameDirectory().absoluteFilePath(getBinaryName()).toStdWString().c_str();
    VS_FIXEDFILEINFO versionInfo = ::GetFileVersion(fileName);
    if ((versionInfo.dwFileVersionMS > 0x10004) || // version >= 1.5.x?
        ((versionInfo.dwFileVersionMS == 0x10004) && (versionInfo.dwFileVersionLS >= 0x1A0000))) { // version >= ?.4.26
      return LoadOrderMechanism::PluginsTxt;
    }
  } catch (const std::exception &e) {
    qCritical() << "TESV.exe is invalid: " << e.what();
  }
  return LoadOrderMechanism::FileTime;
}


int GameSkyrim::getNexusModOrganizerID() const
{
  return 1334;
}

int GameSkyrim::getNexusGameID() const
{
  return 110;
}
