/*
 * Copyright (C) 2011-2017 FlylinkDC++ Team http://flylinkdc.com
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

#include "stdinc.h"
#include "IpGuard.h"
#include "Util.h"
#include "AppPaths.h"
#include "SettingsManager.h"
#include "LogManager.h"
#include "ConfCore.h"

IpGuard ipGuard;

IpGuard::IpGuard() : enabled(false), isWhiteList(false), cs(RWLock::create())
{
}

void IpGuard::updateSettings() noexcept
{
	auto ss = SettingsManager::instance.getCoreSettings();
	ss->lockRead();
	bool optEnabled = ss->getBool(Conf::ENABLE_IPGUARD);
	bool optWhiteList = ss->getBool(Conf::IPGUARD_DEFAULT_DENY);
	ss->unlockRead();

	enabled = optEnabled;

	WRITE_LOCK(*cs);
	isWhiteList = optWhiteList;
}

string IpGuard::getFileName()
{
	return Util::getConfigPath() + "IPGuard.ini";
}

void IpGuard::load() noexcept
{
	WRITE_LOCK(*cs);
	ipList.clear();
	auto addLine = [this](const string& s) -> bool
	{
		IpList::ParseLineResult out;
		int result = IpList::parseLine(s, out);
		if (!result)
		{
			if (!ipList.addRange(out.start, out.end, 0, result))
				LogManager::message("Error adding data from IPGuard.ini: " + IpList::getErrorText(result) + " [" + s + "]", false);
		}
		else if (result != IpList::ERR_LINE_SKIPPED)
			LogManager::message("Error parsing IPGuard.ini: " + IpList::getErrorText(result) + " [" + s + "]", false);
		return true;
	};
	try
	{
		File f(getFileName(), File::READ, File::OPEN);
		Util::readTextFile(f, addLine);
	}
	catch (Exception& e)
	{
		LogManager::message("Could not load IPGuard.ini: " + e.getError(), false);
		ipList.clear();
	}
}

void IpGuard::clear() noexcept
{
	WRITE_LOCK(*cs);
	ipList.clear();
}

bool IpGuard::isBlocked(uint32_t addr) const noexcept
{
	READ_LOCK(*cs);
	uint64_t payload;
	if (ipList.find(addr, payload))
		return !isWhiteList;
	return isWhiteList;
}
