/*
 * Copyright (C) 2001-2019 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef MAPPER_NATPMP_H
#define MAPPER_NATPMP_H

#include "Mapper.h"

class Mapper_NATPMP : public Mapper
{
	public:
		Mapper_NATPMP(const string& localIp, int af);

		static const string name;

	private:
		bool init();
		void uninit();

		bool addMapping(int port, Protocol protocol, const string& description);
		bool removeMapping(int port, Protocol protocol);
		bool supportsProtocol(int af) const;

		int renewal() const { return lifetime / 2; }

		string getDeviceName() const;
		IpAddress getExternalIP();
		int getExternalPort() const { return publicPort; }

		const string &getName() const { return name; }

		string gateway;
		int lifetime; // in seconds
		int publicPort;
};

#endif
