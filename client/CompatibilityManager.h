/*
 * Copyright (C) 2011-2013 Alexey Solomin, a.rainman on gmail pt com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef COMPATIBILITY_MANGER_H
#define COMPATIBILITY_MANGER_H

#ifdef _WIN32

#include "typedefs.h"
#include "w.h"

#ifndef SORT_DIGITSASNUMBERS
#define SORT_DIGITSASNUMBERS 0x00000008
#endif

class CompatibilityManager
{
	public:
		// Call this function as soon as possible (immediately after the start of the program).
		static void init();
		
		static bool isIncompatibleSoftwareFound()
		{
			return !g_incopatibleSoftwareList.empty();
		}
		static const string& getIncompatibleSoftwareList()
		{
			return g_incopatibleSoftwareList;
		}
		static string getIncompatibleSoftwareMessage();
		static string getGlobalMemoryStatusMessage();
		static string generateFullSystemStatusMessage();
		static string getStats();
		static string getNetworkStats();
		static string getDefaultPath();
		static string& getStartupInfo()
		{
			return g_startupInfo;
		}
		static LONG getComCtlVersion()
		{
			return g_comCtlVersion;
		}
		static DWORDLONG getTotalPhysMemory()
		{
			return g_TotalPhysMemory;
		}
		static DWORDLONG getFreePhysMemory()
		{
			return g_FreePhysMemory;
		}
		static bool updatePhysMemoryStats();

		static WORD getDllPlatform(const string& fullpath);
		static void reduceProcessPriority();
		static void restoreProcessPriority();
		
		static FINDEX_INFO_LEVELS findFileLevel;
		static DWORD findFileFlags;
		
#if defined(OSVER_WIN_XP) || defined(OSVER_WIN_VISTA)
		static DWORD compareFlags;
#else
		static constexpr DWORD compareFlags = SORT_DIGITSASNUMBERS;
#endif

	private:
		static DWORD g_oldPriorityClass;
		static string g_incopatibleSoftwareList;
		static string g_startupInfo;

		static LONG g_comCtlVersion;
		static DWORDLONG g_TotalPhysMemory;
		static DWORDLONG g_FreePhysMemory;

		static LONG getComCtlVersionFromOS();
		static void generateSystemInfoForApp();
		static bool getGlobalMemoryStatus(MEMORYSTATUSEX* status);

	public:
		static void detectIncompatibleSoftware();

		static string getSpeedInfo();
		static string getDiskSpaceInfo(bool onlyTotal = false);
		static string getDiskInfo();
		static TStringList findVolumes();
		static string getCPUInfo();
		static uint64_t getTickCount();
		static uint64_t getSysUptime() { return getTickCount() / 1000; }
		static bool setThreadName(HANDLE h, const char* name);
};

#endif // _WIN32

#endif // COMPATIBILITY_MANGER_H
