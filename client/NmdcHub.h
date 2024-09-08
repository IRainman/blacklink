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

#ifndef DCPLUSPLUS_DCPP_NMDC_HUB_H
#define DCPLUSPLUS_DCPP_NMDC_HUB_H

#include "forward.h"
#include "User.h"
#include "Text.h"
#include "Client.h"
#include "AntiFlood.h"

class ClientManager;

class NmdcHub : public Client, private Flags
{
	public:
		using Client::send;
		using Client::connect;

		static ClientBasePtr create(const string& hubURL, const string& address, uint16_t port, bool secure);
		void connect(const OnlineUserPtr& user, const string& token, bool forcePassive) override;
		void disconnect(bool graceless) override;

		int getType() const override { return TYPE_NMDC; }
		void hubMessage(const string& message, bool thirdPerson = false) override;
		bool privateMessage(const OnlineUserPtr& user, const string& message, int flags) override;
		void sendUserCmd(const UserCommand& command, const StringMap& params) override;
		void password(const string& pwd, bool setPassword) override;
		void info(bool forceUpdate) override
		{
			myInfo(forceUpdate);
		}
		size_t getUserCount() const override
		{
			READ_LOCK(*csUsers);
			return users.size();
		}
		string escape(const string& str) const noexcept override
		{
			return validateMessage(str, false);
		}
		bool convertNick(string& nick, bool& suffixAppended) const noexcept override
		{
			if (!nickRule)
			{
				suffixAppended = false;
				return true;
			}
			return nickRule->convertNick(nick, suffixAppended);
		}
		void checkNick(string& nick) const noexcept override;
		static string unescape(const string& str)
		{
			return validateMessage(str, true);
		}
		bool send(const AdcCommand&) override
		{
			dcassert(0);
			return false;
		}
		static string validateMessage(string tmp, bool reverse) noexcept;
		void refreshUserList(bool) override;
		
		void getUserList(OnlineUserList& result) const override;

		static string makeKeyFromLock(const string& lock);
		static const string& getLock();
		static const string& getPk();
		static bool isExtended(const string& lock)
		{
			return strncmp(lock.c_str(), "EXTENDEDPROTOCOL", 16) == 0;
		}

	protected:
		void searchToken(const SearchParam& sp) override;
		void onTimer(uint64_t tick) noexcept override;
		void getUsersToCheck(UserList& res, int64_t tick, int timeDiff) const noexcept override;

	private:
		friend class ClientManager;
		enum SupportFlags
		{
			SUPPORTS_USERCOMMAND = 0x001,
			SUPPORTS_NOGETINFO   = 0x002,
			SUPPORTS_USERIP2     = 0x004,
#ifdef BL_FEATURE_NMDC_EXT_JSON
			SUPPORTS_EXTJSON2    = 0x008,
#endif
			SUPPORTS_NICKRULE    = 0x010,
			SUPPORTS_SEARCH_TTHS = 0x020, // $SA and $SP
			SUPPORTS_SEARCHRULE  = 0x040,
			SUPPORTS_SALT_PASS   = 0x080,
			SUPPORTS_MCTO        = 0x100
		};
		
		enum
		{
			WAITING_FOR_MYINFO,
			MYINFO_LIST,
			MYINFO_LIST_COMPLETED
		};

		NmdcHub(const string& hubURL, const string& address, uint16_t port, bool secure);
		~NmdcHub();

		typedef boost::unordered_map<string, OnlineUserPtr> NickMap;
		
		NickMap users;
		std::unique_ptr<RWLock> csUsers;

		int myInfoState;
		string lastNatUser;
		string lastMyInfo;
		string lastExtJSONInfo;
		string salt;
		uint64_t lastUpdate;
		uint64_t lastNatUserExpires;
		unsigned hubSupportFlags;
		char lastModeChar; // last Mode MyINFO

		HubRequestCounters reqSearch;
		HubRequestCounters reqConnectToMe;

	private:
		void updateMyInfoState(bool isMyInfo);

		struct NickRule
		{
			static const size_t MAX_CHARS = 32;
			static const size_t MAX_PREFIXES = 16;
			unsigned minLen = 0;
			unsigned maxLen = 0;
			vector<char> invalidChars;
			vector<string> prefixes;
			bool convertNick(string& nick, bool& suffixAppended) const noexcept;
		};
		std::unique_ptr<NickRule> nickRule;

		void clearUsers();
		void onLine(const char* buf, size_t len);

		OnlineUserPtr getUser(const string& nick);
		OnlineUserPtr findUser(const string& nick) const override;
		void putUser(const string& nick);
		bool getShareGroup(const string& seeker, CID& shareGroup, bool& hideShare) const;
		bool isMcPmSupported() const override;

		void privateMessage(const string& nick, const string& myNick, const string& message, int flags);
		void version()
		{
			send("$Version 1,0091|");
		}
		void getNickList()
		{
			send("$GetNickList|");
		}
		void connectToMe(const OnlineUser& user, const string& token);

		static void sendUDP(const string& address, uint16_t port, string& sr);
		void handleSearch(const NmdcSearchParam& searchParam);
		bool handlePartialSearch(const NmdcSearchParam& searchParam);
		bool getMyExternalIP(IpAddress& ip) const;
		void getMyUDPAddr(string& ip, uint16_t& port) const;
		void revConnectToMe(const OnlineUser& user);
		bool resendMyINFO(bool alwaysSend, bool forcePassive) override;
		void myInfo(bool alwaysSend, bool forcePassive = false);
		void myInfoParse(const string& param);
#ifdef BL_FEATURE_NMDC_EXT_JSON
		bool extJSONParse(const string& param);
#endif
		void searchParse(const string& param, int type);
		void connectToMeParse(const string& param);
		void revConnectToMeParse(const string& param);
		void hubNameParse(const string& param);
		void supportsParse(const string& param);
		void userCommandParse(const string& param);
		void lockParse(const char* buf, size_t len);
		void helloParse(const string& param);
		void userIPParse(const string& param);
		void botListParse(const string& param);
		void nickListParse(const string& param);
		void opListParse(const string& param);
		void toParse(const string& param);
		void mcToParse(const string& param);
		void chatMessageParse(const string& line);
		void updateFromTag(Identity& id, const string& tag);
		static int getEncodingFromDomain(const string& domain);
		bool checkConnectToMeFlood(const IpAddress& ip, uint16_t port);
		bool checkSearchFlood(const IpAddress& ip, uint16_t port);

		void onConnected() noexcept override;
		void onDataLine(const char* buf, size_t len) noexcept override;
		void onFailed(const string&) noexcept override;
};

#endif // DCPLUSPLUS_DCPP_NMDC_HUB_H
