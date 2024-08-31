/*
 * Copyright (C) 2001-2017 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_ADC_SUPPORTS_H
#define DCPLUSPLUS_DCPP_ADC_SUPPORTS_H

#include "AdcCommand.h"

class Identity;
class AdcSupports
{
	public:
		static const string CLIENT_PROTOCOL;
		static const string SECURE_CLIENT_PROTOCOL;
		static const string ADCS_FEATURE;
		static const string TCP4_FEATURE;
		static const string TCP6_FEATURE;
		static const string UDP4_FEATURE;
		static const string UDP6_FEATURE;
		static const string NAT0_FEATURE;
		static const string SEGA_FEATURE;
		static const string CCPM_FEATURE;
		static const string SUD1_FEATURE;
		static const string BASE_SUPPORT;
		static const string BAS0_SUPPORT;
		static const string TIGR_SUPPORT;
		static const string UCM0_SUPPORT;
		static const string BLO0_SUPPORT;
		static const string ZLIF_SUPPORT;

		enum KnownSupports
		{
			SEGA_FEATURE_BIT = 1,
		};

		static string getSupports(const Identity& id);
		static void setSupports(Identity& id, const StringList& su, const string& source, uint32_t* parsedFeatures);
		static void setSupports(Identity& id, const string& su, const string& source, uint32_t* parsedFeatures);
#if defined(BL_FEATURE_COLLECT_UNKNOWN_FEATURES) || defined(BL_FEATURE_COLLECT_UNKNOWN_TAGS)
		static string getCollectedUnknownTags();
#endif
};

class NmdcSupports
{
	public:
		enum Status
		{
			NORMAL   = 0x01,
			AWAY     = 0x02,
			SERVER   = 0x04,
			FIREBALL = 0x08,
			TLS      = 0x10,
			NAT0     = 0x20, // non-standard and incompatible with some hubs
			AIRDC    = 0x40
		};
		static void setStatus(Identity& id, const char statusChar, const char modeChar, const string& connection);
};

#endif // DCPLUSPLUS_DCPP_ADC_SUPPORTS_H
