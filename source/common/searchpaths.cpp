//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) 2019 Christoph Oelckers


This is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
//
// Search path management. Scan all directories for potential game content and return a list with all proper matches
//

#include <filesystem>
#include "m_crc32.h"
#include "i_specialpaths.h"
#include "compat.h"
#include "gameconfigfile.h"
#include "cmdlib.h"
#include "utf8.h"
#include "sc_man.h"
#include "resourcefile.h"
#include "printf.h"
#include "common.h"
#include "gamecontrol.h"




// These two structs need to be expoted
struct GrpInfo
{
	FString name;
	FString scriptname;
	FString dirname;
	FString defname;
	FString rtsname;
	uint32_t CRC = 0;
	uint32_t dependencyCRC = 0;
	size_t size = 0;
	int flags = 0;
	TArray<FString> loadfiles;
	TArray<FString> loadart;
};

				
struct GrpEntry
{
	FString FileName;
	GrpInfo FileInfo;
	uint32_t FileIndex;
};


namespace fs = std::filesystem;


fs::path AbsolutePath(const char* path)
{
	FString dirpath = MakeUTF8(path);	// convert into clean UTF-8 - the input here may easily be 8 bit encoded.
	fs::path fpath = fs::u8path(dirpath.GetChars());
	return fs::absolute(fpath);
}


void AddSearchPath(TArray<FString>& searchpaths, const char* path)
{
	try
	{
		auto fpath = AbsolutePath(path);
		if (fs::is_directory(fpath))
		{
			FString apath = fpath.u8string().c_str();
			if (searchpaths.Find(apath) == searchpaths.Size())
				searchpaths.Push(apath);
		}
	}
	catch (fs::filesystem_error &)
	{
	}
}

#ifndef _WIN32
//-------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------

static void G_AddSteamPaths(TArray<FString> &searchpaths, const char *basepath)
{
	FString path;

    // Duke Nukem 3D: Megaton Edition (Steam)
    path.Format("%s/steamapps/common/Duke Nukem 3D/gameroot", basepath);
	AddSearchPath(searchpaths, path);
    path.Format("%s/steamapps/common/Duke Nukem 3D/gameroot/addons/dc", basepath);
	AddSearchPath(searchpaths, path);
    path.Format("%s/steamapps/common/Duke Nukem 3D/gameroot/addons/nw", basepath);
	AddSearchPath(searchpaths, path);
    path.Format("%s/steamapps/common/Duke Nukem 3D/gameroot/addons/vacation", basepath);
	AddSearchPath(searchpaths, path);

    // Duke Nukem 3D (3D Realms Anthology (Steam) / Kill-A-Ton Collection 2015)
#ifdef __APPLE__
    path.Format("%s/steamapps/common/Duke Nukem 3D/Duke Nukem 3D.app/drive_c/Program Files/Duke Nukem 3D", basepath);
	AddSearchPath(searchpaths, path);
#endif

    // NAM (Steam)
#ifdef __APPLE__
    path.Format("%s/steamapps/common/Nam/Nam.app/Contents/Resources/Nam.boxer/C.harddisk/NAM", basepath);
#else
    path.Format("%s/steamapps/common/Nam/NAM", basepath);
#endif
	AddSearchPath(searchpaths, path);

    // WWII GI (Steam)
    path.Format("%s/steamapps/common/World War II GI/WW2GI", basepath);
	AddSearchPath(searchpaths, path);

	// Shadow Warrior Classic Redux - Steam
	static char const s_SWCR_Steam[] = "steamapps/common/Shadow Warrior Classic/gameroot";
	path.Format("%s/%s", basepath, s_SWCR_Steam);
	AddSearchPath(searchpaths, path);
	//path.Format("%s/%s/addons", basepath, s_SWCR_Steam);
	//AddSearchPath(searchpaths, path);
	//path.Format("%s/%s/classic/MUSIC", basepath, s_SWCR_Steam);
	//AddSearchPath(searchpaths, path);

	// Shadow Warrior Classic (1997) - Steam
	static char const s_SWC_Steam[] = "steamapps/common/Shadow Warrior Original/gameroot";
	path.Format("%s/%s", basepath, s_SWC_Steam);
	AddSearchPath(searchpaths, path);
	//path.Format("%s/%s/MUSIC", basepath, s_SWC_Steam);
	//AddSearchPath(searchpaths, path);

	// Shadow Warrior (Classic) - 3D Realms Anthology - Steam
#if defined EDUKE32_OSX
	path.Format("%s/steamapps/common/Shadow Warrior DOS/Shadow Warrior.app/Contents/Resources/sw", basepath);
	AddSearchPath(searchpaths, path);
#endif

}


static TArray<FString>* g_searchpaths;

static void AddAnItem(const char* item)
{
	AddSearchPath(*g_searchpaths, item);
}



#ifndef __APPLE__

//-------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------

void G_AddExternalSearchPaths(TArray<FString> &searchpaths)
{
	FString path;
    char *homepath = Bgethomedir();

    path.Format("%s/.steam/steam", homepath);
    G_AddSteamPaths(searchpaths, buf);

    path.Format("%s/.steam/steam/steamapps/libraryfolders.vdf", homepath);
	g_searchpaths = &searchpaths;
    G_ParseSteamKeyValuesForPaths(searchpaths, buf, AddAnItem);
}

#else

//-------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------
void G_AddExternalSearchPaths(TArray<FString> &searchpaths)
{
    char *applications[] = { osx_getapplicationsdir(0), osx_getapplicationsdir(1) };
    char *support[] = { osx_getsupportdir(0), osx_getsupportdir(1) };

	FString path;

    char buf[BMAX_PATH];
    int32_t i;

	g_searchpaths = &searchpaths;
    for (i = 0; i < 2; i++)
    {
        path.Format("%s/Steam", support[i]);
        G_AddSteamPaths(searchpaths, buf);

        path.Format("%s/Steam/steamapps/libraryfolders.vdf", support[i]);
        G_ParseSteamKeyValuesForPaths(searchpaths, buf, AddAnItem);

        // Duke Nukem 3D: Atomic Edition (GOG.com)
        path.Format("%s/Duke Nukem 3D.app/Contents/Resources/Duke Nukem 3D.boxer/C.harddisk", applications[i]);
		AddSearchPath(searchpaths, path);

		// Shadow Warrior Classic Complete - GOG.com
		static char const s_SWC_GOG[] = "Shadow Warrior Complete/Shadow Warrior.app/Contents/Resources/Shadow Warrior.boxer/C swarrior_files.harddisk";
		path.Format("%s/%s", applications[i], s_SWC_GOG);
		AddSearchPath(searchpaths, path);
		//path.Format("%s/%s/MUSIC", applications[i], s_SWC_GOG);
		//addsearchpath(buf);

		// Shadow Warrior Classic Redux - GOG.com
		static char const s_SWCR_GOG[] = "Shadow Warrior Classic Redux/Shadow Warrior Classic Redux.app/Contents/Resources/gameroot";
		path.Format("%s/%s", applications[i], s_SWCR_GOG);
		AddSearchPath(searchpaths, path);
		//path.Format("%s/%s/music", applications[i], s_SWCR_GOG);
		//addsearchpath(buf);

    }

    for (i = 0; i < 2; i++)
    {
        Xfree(applications[i]);
        Xfree(support[i]);
    }
}

#endif
#else
//-------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------

void G_AddExternalSearchPaths(TArray<FString> &searchpaths)
{

    char buf[BMAX_PATH] = {0};
    DWORD bufsize;

    // Duke Nukem 3D: 20th Anniversary World Tour (Steam)
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 434050)", "InstallLocation", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
    }

    // Duke Nukem 3D: Megaton Edition (Steam)
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 225140)", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        strncpy(suffix, "/gameroot", remaining);
		AddSearchPath(searchpaths, buf);
        strncpy(suffix, "/gameroot/addons/dc", remaining);
		AddSearchPath(searchpaths, buf);
        strncpy(suffix, "/gameroot/addons/nw", remaining);
		AddSearchPath(searchpaths, buf);
        strncpy(suffix, "/gameroot/addons/vacation", remaining);
		AddSearchPath(searchpaths, buf);
    }

    // Duke Nukem 3D (3D Realms Anthology (Steam) / Kill-A-Ton Collection 2015)
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 359850)", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        strncpy(suffix, "/Duke Nukem 3D", remaining);
		AddSearchPath(searchpaths, buf);
    }

    // Duke Nukem 3D: Atomic Edition (GOG.com)
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue("SOFTWARE\\GOG.com\\GOGDUKE3D", "PATH", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
    }

    // Duke Nukem 3D (3D Realms Anthology)
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue("SOFTWARE\\3DRealms\\Duke Nukem 3D", NULL, buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        strncpy(suffix, "/Duke Nukem 3D", remaining);
		AddSearchPath(searchpaths, buf);
    }

    // 3D Realms Anthology
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue("SOFTWARE\\3DRealms\\Anthology", NULL, buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        strncpy(suffix, "/Duke Nukem 3D", remaining);
		AddSearchPath(searchpaths, buf);
    }

    // NAM (Steam)
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 329650)", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        strncpy(suffix, "/NAM", remaining);
		AddSearchPath(searchpaths, buf);
    }

    // WWII GI (Steam)
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 376750)", "InstallLocation", buf, &bufsize))
    {
        char * const suffix = buf + bufsize - 1;
        size_t const remaining = sizeof(buf) - bufsize;

        strncpy(suffix, "/WW2GI", remaining);
		AddSearchPath(searchpaths, buf);
    }

    // Redneck Rampage (GOG.com)
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue("SOFTWARE\\GOG.com\\GOGREDNECKRAMPAGE", "PATH", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
    }

    // Redneck Rampage Rides Again (GOG.com)
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue("SOFTWARE\\GOG.com\\GOGCREDNECKRIDESAGAIN", "PATH", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
    }
	
    // Blood: One Unit Whole Blood (Steam)
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 299030)", "InstallLocation", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
    }

    // Blood: One Unit Whole Blood (GOG.com)
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue("SOFTWARE\\GOG.com\\GOGONEUNITONEBLOOD", "PATH", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
    }

    // Blood: Fresh Supply (Steam)
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 1010750)", "InstallLocation", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
        strncat(buf, R"(\addons\Cryptic Passage)", 23);
		AddSearchPath(searchpaths, buf);
    }

    // Blood: Fresh Supply (GOG.com)
    bufsize = sizeof(buf);
    if (Paths_ReadRegistryValue(R"(SOFTWARE\Wow6432Node\GOG.com\Games\1374469660)", "path", buf, &bufsize))
    {
		AddSearchPath(searchpaths, buf);
        strncat(buf, R"(\addons\Cryptic Passage)", 23);
		AddSearchPath(searchpaths, buf);
    }

	bufsize = sizeof(buf);
	if (Paths_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 225160)", "InstallLocation", buf, &bufsize))
	{
		char* const suffix = buf + bufsize - 1;
		size_t const remaining = sizeof(buf) - bufsize;

		strncpy(suffix, "/gameroot", remaining);
		AddSearchPath(searchpaths, buf);
		//strncpy(suffix, "/gameroot/addons", remaining);
		//addsearchpath_user(buf, SEARCHPATH_REMOVE);
		//strncpy(suffix, "/gameroot/classic/MUSIC", remaining);
		//addsearchpath(buf);
	}

	// Shadow Warrior Classic (1997) - Steam
	bufsize = sizeof(buf);
	if (Paths_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 238070)", "InstallLocation", buf, &bufsize))
	{
		char* const suffix = buf + bufsize - 1;
		DWORD const remaining = sizeof(buf) - bufsize;

		strncpy(suffix, "/gameroot", remaining);
		AddSearchPath(searchpaths, buf);
		//strncpy(suffix, "/gameroot/MUSIC", remaining);
		//addsearchpath(buf);
	}

	// Shadow Warrior (Classic) - 3D Realms Anthology - Steam
	bufsize = sizeof(buf);
	if (Paths_ReadRegistryValue(R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 358400)", "InstallLocation", buf, &bufsize))
	{
		char* const suffix = buf + bufsize - 1;
		DWORD const remaining = sizeof(buf) - bufsize;

		strncpy(suffix, "/Shadow Warrior", remaining);
		AddSearchPath(searchpaths, buf);
	}

	// Shadow Warrior Classic Complete - GOG.com
	bufsize = sizeof(buf);
	if (Paths_ReadRegistryValue("SOFTWARE\\GOG.com\\GOGSHADOWARRIOR", "PATH", buf, &bufsize))
	{
		AddSearchPath(searchpaths, buf);
	}

	// Shadow Warrior - 3D Realms Anthology
	bufsize = sizeof(buf);
	if (Paths_ReadRegistryValue("SOFTWARE\\3DRealms\\Shadow Warrior", NULL, buf, &bufsize))
	{
		char* const suffix = buf + bufsize - 1;
		DWORD const remaining = sizeof(buf) - bufsize;

		strncpy(suffix, "/Shadow Warrior", remaining);
		AddSearchPath(searchpaths, buf);
	}

	// 3D Realms Anthology
	bufsize = sizeof(buf);
	if (Paths_ReadRegistryValue("SOFTWARE\\3DRealms\\Anthology", NULL, buf, &bufsize))
	{
		char* const suffix = buf + bufsize - 1;
		DWORD const remaining = sizeof(buf) - bufsize;

		strncpy(suffix, "/Shadow Warrior", remaining);
		AddSearchPath(searchpaths, buf);
	}

}
#endif

//==========================================================================
//
//
//
//==========================================================================

void CollectSubdirectories(TArray<FString> &searchpath, const char *dirmatch)
{
	try
	{
		FString dirpath = MakeUTF8(dirmatch);	// convert into clean UTF-8
		dirpath.Truncate(dirpath.Len() - 2);	// remove the '/*'
		fs::path path = AbsolutePath(dirpath.GetChars());
		if (fs::exists(path) && fs::is_directory(path))
		{
			for (const auto& entry : fs::directory_iterator(path))
			{
				if (fs::is_directory(entry.status()))
				{
					FString newdir = absolute(entry.path()).u8string().c_str();
					if (searchpath.Find(newdir) == searchpath.Size())
						searchpath.Push(newdir);
				}
			}
		}
	}
	catch (fs::filesystem_error &)
	{
		// Just ignore this path if it caused an error.
	}
}

//==========================================================================
//
// CollectSearchPaths
//
// collect all paths in a local array for easier management
//
//==========================================================================

TArray<FString> CollectSearchPaths()
{
	TArray<FString> searchpaths;
	
	if (GameConfig->SetSection("GameSearch.Directories"))
	{
		const char *key;
		const char *value;

		while (GameConfig->NextInSection(key, value))
		{
			if (stricmp(key, "Path") == 0)
			{
				FString nice = NicePath(value);
				if (nice.Len() > 0)
				{
#ifdef _WIN32
					if (isalpha(nice[0] && nice[1] == ':' && nice[2] != '/')) continue;	// ignore drive relative paths because they are meaningless.
#endif
					// A path ending with "/*" means to add all subdirectories.
					if (nice[nice.Len()-2] == '/' && nice[nice.Len()-1] == '*')
					{
						CollectSubdirectories(searchpaths, nice);
					}
					// Checking Steam via a list entry allows easy removal if not wanted.
					else if (nice.CompareNoCase("$STEAM") == 0)
					{
						G_AddExternalSearchPaths(searchpaths);
					}
					else
					{
						AddSearchPath(searchpaths, nice);
					}
				}
			}
		}
	}
	// Unify and remove trailing slashes
	for (auto &str : searchpaths)
	{
		str.Substitute("\\", "/");
		str.Substitute("//", "/");	// Double slashes can happen when constructing paths so just get rid of them here.
		if (str.Back() == '/') str.Truncate(str.Len() - 1);
	}
	return searchpaths;
}

//==========================================================================
//
//
//
//==========================================================================

struct FileEntry
{
	FString FileName;
	uintmax_t FileLength;
	uint64_t FileTime;
	uint32_t CRCValue;
	uint32_t Index;
};

TArray<FileEntry> CollectAllFilesInSearchPath()
{
	TArray<FileEntry> filelist;
	auto paths = CollectSearchPaths();
	int index = 0;
	for(auto &path : paths)
	{
		auto fpath = fs::u8path(path.GetChars());
		if (fs::exists(fpath) && fs::is_directory(fpath))
		{
			for (const auto& entry : fs::directory_iterator(fpath))
			{
				if (fs::is_regular_file(entry.status()))
				{
					filelist.Reserve(1);
					auto& flentry = filelist.Last();
					flentry.FileName = absolute(entry.path()).u8string().c_str();
					flentry.FileLength = entry.file_size();
					flentry.FileTime = entry.last_write_time().time_since_epoch().count();
					flentry.Index = index++; // to preserve order when working on the list.
					filelist.Push(flentry);
				}
			}
		}
	}
	return filelist;
}

//==========================================================================
//
//
//
//==========================================================================

static TArray<FileEntry> LoadCRCCache(void)
{
	auto cachepath = M_GetAppDataPath(false) + "/grpcrccache.txt";
	FScanner sc;
	TArray<FileEntry> crclist;

	try
	{
		sc.OpenFile(cachepath);
		while (sc.GetString())
		{
			crclist.Reserve(1);
			auto flentry = crclist.Last();
			flentry.FileName = sc.String;
			sc.MustGetString();
			flentry.FileLength = strtoull(sc.String, nullptr, 0);	// Cannot use sc.Number because that's only 32 bit.
			sc.MustGetString();
			flentry.FileTime = strtoull(sc.String, nullptr, 0);	// Cannot use sc.Number because that's only 32 bit.
			sc.MustGetString();
			flentry.CRCValue = strtoull(sc.String, nullptr, 0);	// Cannot use sc.Number because that's only 32 bit.
		}
	}
	catch (std::runtime_error &)
	{
		// If there's a parsing error, return what we got and discard the rest.
	}
	return crclist;
}

//==========================================================================
//
//
//
//==========================================================================

static TArray<GrpInfo> ParseGrpInfo(const char *fn, FileReader &fr)
{
	TArray<GrpInfo> groups;
	TMap<FString, uint32_t> CRCMap;
	TMap<FString, int> FlagMap;
	
	FlagMap.Insert("GAMEFLAG_DUKE", GAMEFLAG_DUKE);
	FlagMap.Insert("GAMEFLAG_NAM", GAMEFLAG_NAM);
	FlagMap.Insert("GAMEFLAG_NAPALM", GAMEFLAG_NAPALM);
	FlagMap.Insert("GAMEFLAG_WW2GI", GAMEFLAG_WW2GI);
	FlagMap.Insert("GAMEFLAG_ADDON", GAMEFLAG_ADDON);
	FlagMap.Insert("GAMEFLAG_SHAREWARE", GAMEFLAG_SHAREWARE);
	FlagMap.Insert("GAMEFLAG_DUKEBETA", GAMEFLAG_DUKEBETA); // includes 0x20 since it's a shareware beta
	FlagMap.Insert("GAMEFLAG_FURY", GAMEFLAG_FURY);
	FlagMap.Insert("GAMEFLAG_RR", GAMEFLAG_RR);
	FlagMap.Insert("GAMEFLAG_RRRA", GAMEFLAG_RRRA);
	FlagMap.Insert("GAMEFLAG_BLOOD", GAMEFLAG_BLOOD);
	
	FScanner sc;
	auto mem = fr.Read();
	sc.OpenMem(fn, (const char *)mem.Data(), mem.Size());
	
	while (sc.GetToken())
	{
		sc.TokenMustBe(TK_Identifier);
		if (sc.Compare("CRC"))
		{
			sc.MustGetToken('{');
			while (!sc.CheckToken('}'))
			{
				sc.MustGetToken(TK_Identifier);
				FString key = sc.String;
				sc.MustGetToken(TK_IntConst);
				if (sc.BigNumber < 0 || sc.BigNumber >= UINT_MAX)
				{
					sc.ScriptError("CRC hash %s out of range", sc.String);
				}
				CRCMap.Insert(key, (uint32_t)sc.BigNumber);
			}
		}
		if (sc.Compare("grpinfo"))
		{
			groups.Reserve(1);
			auto grp = groups.Last();
			sc.MustGetToken('{');
			while (!sc.CheckToken('}'))
			{
				sc.MustGetToken(TK_Identifier);
				if (sc.Compare("name"))
				{
					sc.MustGetToken(TK_StringConst);
					grp.name = sc.String;
				}
				else if (sc.Compare("scriptname"))
				{
					sc.MustGetToken(TK_StringConst);
					grp.scriptname = sc.String;
				}
				else if (sc.Compare("loaddirectory"))
				{
					sc.MustGetToken(TK_StringConst);
					grp.dirname = sc.String;
				}
				else if (sc.Compare("defname"))
				{
					sc.MustGetToken(TK_StringConst);
					grp.defname = sc.String;
				}
				else if (sc.Compare("rtsname"))
				{
					sc.MustGetToken(TK_StringConst);
					grp.rtsname = sc.String;
				}
				else if (sc.Compare("crc"))
				{
					sc.MustGetAnyToken();
					if (sc.TokenType == TK_IntConst)
					{
						grp.CRC = (uint32_t)sc.BigNumber;
					}
					else if (sc.TokenType == TK_Identifier)
					{
						auto ip = CRCMap.CheckKey(sc.String);
						if (ip) grp.CRC = *ip;
						else sc.ScriptError("Unknown CRC value %s", sc.String);
					}
					else sc.TokenMustBe(TK_IntConst);
				}
				else if (sc.Compare("dependency"))
				{
					sc.MustGetAnyToken();
					if (sc.TokenType == TK_IntConst)
					{
						grp.dependencyCRC = (uint32_t)sc.BigNumber;
					}
					else if (sc.TokenType == TK_Identifier)
					{
						auto ip = CRCMap.CheckKey(sc.String);
						if (ip) grp.dependencyCRC = *ip;
						else sc.ScriptError("Unknown CRC value %s", sc.String);
					}
					else sc.TokenMustBe(TK_IntConst);
				}
				else if (sc.Compare("size"))
				{
					sc.MustGetToken(TK_IntConst);
					grp.size = sc.BigNumber;
				}
				else if (sc.Compare("flags"))
				{
					do
					{
						sc.MustGetAnyToken();
						if (sc.TokenType == TK_IntConst)
						{
							grp.flags |= sc.Number;
						}
						else if (sc.TokenType == TK_Identifier)
						{
							auto ip = FlagMap.CheckKey(sc.String);
							if (ip) grp.dependencyCRC |= *ip;
							else sc.ScriptError("Unknown flag value %s", sc.String);
						}
						else sc.TokenMustBe(TK_IntConst);
					}
					while (sc.CheckToken('|'));
				}
				else if (sc.Compare("loadgrp"))
				{
					do
					{
						sc.MustGetToken(TK_StringConst);
						grp.loadfiles.Push(sc.String);
					}
					while (sc.CheckToken(','));
				}
				else if (sc.Compare("loadart"))
				{
					do
					{
						sc.MustGetToken(TK_StringConst);
						grp.loadfiles.Push(sc.String);
					}
					while (sc.CheckToken(','));
				}
				else sc.ScriptError(nullptr);
			}
		}
		else sc.ScriptError(nullptr);
	}
	return groups;
}

//==========================================================================
//
//
//
//==========================================================================

TArray<GrpInfo> ParseAllGrpInfos(TArray<FileEntry>& filelist)
{
	TArray<GrpInfo> groups;
	extern FString progdir;
	// This opens the base resource only for reading the grpinfo from it.
	std::unique_ptr<FResourceFile> engine_res;
	FString baseres = progdir + "demolition.pk3";
	engine_res.reset(FResourceFile::OpenResourceFile(baseres, true, true));
	if (engine_res)
	{
		auto basegrp = engine_res->FindLump("demolition/demolition.grpinfo");
		if (basegrp)
		{
			auto fr = basegrp->NewReader();
			if (fr.isOpen())
			{
				groups = ParseGrpInfo("demolition/demolition.grpinfo", fr);
			}
		}
	}
	for (auto& entry : filelist)
	{
		auto lowerstr = entry.FileName.MakeLower();
		if (lowerstr.Len() >= 8)
		{
			const char* exten = lowerstr.GetChars() + lowerstr.Len() - 8;
			if (!stricmp(exten, ".grpinfo"))
			{
				// parse it.
				FileReader fr;
				if (fr.OpenFile(entry.FileName))
				{
					auto g = ParseGrpInfo(entry.FileName, fr);
					groups.Append(g);
				}
			}
		}
	}
	return groups;
}

//==========================================================================
//
//
//
//==========================================================================
					
void GetCRC(FileEntry *entry, TArray<FileEntry> &CRCCache)
{
	for (auto &ce : CRCCache)
	{
		// File size, modification date snd name all must match exactly to pick an entry.
		if (entry->FileLength == ce.FileLength && entry->FileTime == ce.FileTime && entry->FileName.Compare(ce.FileName))
		{
			entry->CRCValue = ce.CRCValue;
			return;
		}
	}
	FileReader f;
	if (f.OpenFile(entry->FileName))
	{
		TArray<uint8_t> buffer(65536, 1);
		uint32_t crcval = 0;
		size_t b;
		do
		{
			b = f.Read(buffer.Data(), buffer.Size());
			if (b > 0) crcval = AddCRC32(crcval, buffer.Data(), b);
		}
		while (b == buffer.Size());
		entry->CRCValue = crcval;
		CRCCache.Push(*entry);
	}
}

GrpInfo *IdentifyGroup(FileEntry *entry, TArray<GrpInfo *> &groups)
{
	for (auto g : groups)
	{
		if (entry->FileLength == g->size && entry->CRCValue == g->CRC)
			return g;
	}
	return nullptr;
}

//==========================================================================
//
//
//
//==========================================================================

TArray<GrpEntry> GrpScan()
{
	TArray<GrpEntry> foundGames;
	
	TArray<FileEntry *> sortedFileList;
	TArray<GrpInfo *> sortedGroupList;
	
	auto allFiles = CollectAllFilesInSearchPath();
	auto allGroups = ParseAllGrpInfos(allFiles);
	auto cachedCRCs = LoadCRCCache();
	
	// Remove all unnecessary content from the file list. Since this contains all data from the search path's directories it can be quite large.
	// Sort both lists by file size so that we only need to pass over each list once to weed out all unrelated content. Go backward to avoid too much item movement
	// (most will be deleted anyway.)
	for (auto &f : allFiles) sortedFileList.Push(&f);
	for (auto &g : allGroups) sortedGroupList.Push(&g);
	
	std::sort(sortedFileList.begin(), sortedFileList.end(), [](FileEntry* lhs, FileEntry* rhs) { return lhs->FileLength < rhs->FileLength; });
	std::sort(sortedGroupList.begin(), sortedGroupList.end(), [](GrpInfo* lhs, GrpInfo* rhs) { return lhs->size < rhs->size; });

	int findex = sortedFileList.Size();
	int gindex = sortedGroupList.Size();
	int cindex = sortedGroupList.Size();

	while (findex > 0 || gindex > 0)
	{
		if (sortedFileList[findex]->FileLength > sortedGroupList[gindex]->size)
		{
			// File is larger than the largest known group so it cannot be a candidate.
			sortedFileList.Delete(findex--);
		}
		else if (sortedFileList[findex]->FileLength < sortedGroupList[gindex]->size)
		{
			// The largest available file is smaller than this group so we cannot possibly have it.
			sortedGroupList.Delete(gindex--);
		}
		else
		{
			// We found a matching file. Skip over all other entries of the same size so we can analyze those later as well
			while (findex > 0 && sortedFileList[findex]->FileLength == sortedFileList[findex-1]->FileLength) findex--;
			while (gindex > 0 && sortedGroupList[gindex]->size == sortedGroupList[gindex-1]->size) gindex--;
		}
	}
	if (sortedGroupList.Size() == 0 || sortedFileList.Size() == 0)
	for (auto entry : sortedFileList)
	{
		GetCRC(entry, cachedCRCs);
		auto grp = IdentifyGroup(entry, sortedGroupList);
		if (grp)
		{
			foundGames.Reserve(1);
			auto fg = foundGames.Last();
			fg.FileInfo = *grp;
			fg.FileName = entry->FileName;
			fg.FileIndex = entry->Index;
		}
	}
	
	// One last thing: We must check for all addons if their dependency is present.
	for( int i = foundGames.Size()-1; i >= 0; i--)
	{
		auto crc = foundGames[i].FileInfo.dependencyCRC;
		if (crc != 0)
		{
			bool found = false;
			for (auto fg : foundGames)
			{
				if (fg.FileInfo.CRC == crc)
				{
					found = true;
					break;
				}
			}
			if (!found) foundGames.Delete(i); // Dependent add-on without dependency cannot be played.
		}
	}
	
	// Do we have anything left? If not, error out
	if (foundGames.Size() == 0)
	{
		I_Error("No supported games found. Please check your search paths.");
	}
	return foundGames;
}