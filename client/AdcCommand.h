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

#ifndef DCPLUSPLUS_DCPP_ADC_COMMAND_H
#define DCPLUSPLUS_DCPP_ADC_COMMAND_H

#include "typedefs.h"
#include "CID.h"
#include "BaseUtil.h"
#include "Tag16.h"

class AdcCommand
{
	public:
		template<uint32_t T>
		struct Type
		{
			enum { CMD = T };
		};

		enum Error
		{
			SUCCESS = 0,
			ERROR_GENERIC = 0,
			ERROR_HUB_GENERIC = 10,
			ERROR_HUB_FULL = 11,
			ERROR_HUB_DISABLED = 12,
			ERROR_LOGIN_GENERIC = 20,
			ERROR_NICK_INVALID = 21,
			ERROR_NICK_TAKEN = 22,
			ERROR_BAD_PASSWORD = 23,
			ERROR_CID_TAKEN = 24,
			ERROR_COMMAND_ACCESS = 25,
			ERROR_REGGED_ONLY = 26,
			ERROR_INVALID_PID = 27,
			ERROR_BANNED_GENERIC = 30,
			ERROR_PERM_BANNED = 31,
			ERROR_TEMP_BANNED = 32,
			ERROR_PROTOCOL_GENERIC = 40,
			ERROR_PROTOCOL_UNSUPPORTED = 41,
			ERROR_CONNECT_FAILED = 42,
			ERROR_INF_MISSING = 43,
			ERROR_BAD_STATE = 44,
			ERROR_FEATURE_MISSING = 45,
			ERROR_BAD_IP = 46,
			ERROR_NO_HUB_HASH = 47,
			ERROR_TRANSFER_GENERIC = 50,
			ERROR_FILE_NOT_AVAILABLE = 51,
			ERROR_FILE_PART_NOT_AVAILABLE = 52,
			ERROR_SLOTS_FULL = 53,
			ERROR_NO_CLIENT_HASH = 54
		};

		enum Severity
		{
			SEV_SUCCESS = 0,
			SEV_RECOVERABLE = 1,
			SEV_FATAL = 2
		};

		enum
		{
			PARSE_OK,
			PARSE_ERROR_TOO_SHORT,
			PARSE_ERROR_INVALID_TYPE,
			PARSE_ERROR_ESCAPE_AT_EOL,
			PARSE_ERROR_UNKNOWN_ESCAPE,
			PARSE_ERROR_INVALID_SID_LENGTH,
			PARSE_ERROR_INVALID_FEATURE_LENGTH,
			PARSE_ERROR_MISSING_FROM_SID,
			PARSE_ERROR_MISSING_TO_SID,
			PARSE_ERROR_MISSING_FEATURE
		};

		static const char TYPE_BROADCAST = 'B';
		static const char TYPE_CLIENT = 'C';
		static const char TYPE_DIRECT = 'D';
		static const char TYPE_ECHO = 'E';
		static const char TYPE_FEATURE = 'F';
		static const char TYPE_INFO = 'I';
		static const char TYPE_HUB = 'H';
		static const char TYPE_UDP = 'U';

#define C(n, a, b, c) static const uint32_t CMD_##n = (((uint32_t)a) | (((uint32_t)b)<<8) | (((uint32_t)c)<<16)); typedef Type<CMD_##n> n
		// Base commands
		C(SUP, 'S', 'U', 'P');
		C(STA, 'S', 'T', 'A');
		C(INF, 'I', 'N', 'F');
		C(MSG, 'M', 'S', 'G');
		C(SCH, 'S', 'C', 'H');
		C(RES, 'R', 'E', 'S');
		C(CTM, 'C', 'T', 'M');
		C(RCM, 'R', 'C', 'M');
		C(GPA, 'G', 'P', 'A');
		C(PAS, 'P', 'A', 'S');
		C(QUI, 'Q', 'U', 'I');
		C(GET, 'G', 'E', 'T');
		C(GFI, 'G', 'F', 'I');
		C(SND, 'S', 'N', 'D');
		C(SID, 'S', 'I', 'D');
		// Extensions
		C(CMD, 'C', 'M', 'D');
		C(NAT, 'N', 'A', 'T');
		C(RNT, 'R', 'N', 'T');
		C(PSR, 'P', 'S', 'R');
		C(PUB, 'P', 'U', 'B');
		C(PMI, 'P', 'M', 'I');
		// ZLIF support
		C(ZON, 'Z', 'O', 'N');
		C(ZOF, 'Z', 'O', 'F');
#undef C

		static const uint32_t HUB_SID = UINT32_MAX; // No client will have this sid

		static uint32_t toFourCC(const char* x) noexcept
		{
			return *reinterpret_cast<const uint32_t*>(x);
		}
		static std::string fromFourCC(uint32_t x) noexcept
		{
			return std::string(reinterpret_cast<const char*>(&x), sizeof(x));
		}

		explicit AdcCommand(uint32_t cmd, char type = TYPE_CLIENT);
		explicit AdcCommand(uint32_t cmd, const uint32_t target, char type);
		explicit AdcCommand(Severity sev, Error err, const string& desc, char type = TYPE_CLIENT);

		AdcCommand(const AdcCommand&) = delete;
		AdcCommand& operator= (const AdcCommand&) = delete;

		uint32_t getCommand() const noexcept { return cmdInt; }
		char getType() const noexcept { return type; }
		void setType(char t) noexcept { type = t; }
		string getFourCC() const noexcept
		{
			string tmp(4, 0);
			tmp[0] = type;
			tmp[1] = cmdChar[0];
			tmp[2] = cmdChar[1];
			tmp[3] = cmdChar[2];
			return tmp;
		}

		int parse(const string& line, bool nmdc = false) noexcept;

		const string& getFeatures() const noexcept
		{
			return features;
		}

		AdcCommand& setFeatures(const string& feat) noexcept
		{
			features = feat;
			return *this;
		}

		StringList& getParameters() noexcept { return parameters; }
		const StringList& getParameters() const noexcept { return parameters; }

		string toString(const CID& cid, bool nmdc = false) const noexcept;
		string toString(uint32_t sid, bool nmdc = false) const noexcept;

		AdcCommand& addParam(const string& name, const string& value) noexcept
		{
			parameters.push_back(name);
			parameters.back() += value;
			return *this;
		}
		AdcCommand& addParam(const string& str) noexcept
		{
			parameters.push_back(str);
			return *this;
		}
		const string& getParam(size_t n) const noexcept
		{
			dcassert(parameters.size() > n);
			return parameters.size() > n ? parameters[n] : Util::emptyString;
		}
		// Return a named parameter where the name is a two-letter code
		bool getParam(uint16_t name, size_t start, string& value) const noexcept;
		bool getParam(const char* name, size_t start, string& value) const noexcept;
		bool hasFlag(uint16_t name, size_t start) const noexcept;
		bool hasFlag(const char* name, size_t start) const noexcept;

		static uint16_t toCode(const char* x) noexcept
		{
			return *reinterpret_cast<const uint16_t*>(x);
		}

		static string escape(const string& str, bool old) noexcept;
		uint32_t getTo() const noexcept { return to; }
		AdcCommand& setTo(const uint32_t sid) noexcept { to = sid; return *this; }
		uint32_t getFrom() const noexcept { return from; }
		void setFrom(const uint32_t sid) noexcept { from = sid; }
		string getNick() const noexcept
		{
			string nick;
			if (!getParam(TAG('N', 'I'), 0, nick))
				nick = "[nick unknown]"; // FIXME FIXME
			return nick;
		}
		static uint32_t toSID(const string& sid) noexcept
		{
			if (sid.length() != 4) return 0;
			return *reinterpret_cast<const uint32_t*>(sid.data());
		}
		static string fromSID(const uint32_t sid) noexcept
		{
			return string(reinterpret_cast<const char*>(&sid), sizeof(sid));
		}

		string getParamString(bool nmdc) const noexcept;

	private:
		string getHeaderString(const CID& cid) const noexcept;
		string getHeaderString(uint32_t sid, bool nmdc) const noexcept;
		StringList parameters;
		string features;
		union
		{
			char cmdChar[4];
			uint32_t cmdInt;
		};
		uint32_t from;
		uint32_t to;
		char type;
};

template<class T>
class CommandHandler
{
	public:
		virtual ~CommandHandler() {}
		void dispatch(const string& line, bool nmdc = false)
		{
			AdcCommand cmd(0);
			int parseResult = cmd.parse(line, nmdc);

			if (parseResult != AdcCommand::PARSE_OK)
			{
				dcdebug("Invalid ADC command: %.50s\n", line.c_str());
				return;
			}

#define CALL_CMD(n) case AdcCommand::CMD_##n: ((T*)this)->handle(AdcCommand::n(), cmd); break;
			switch (cmd.getCommand())
			{
					CALL_CMD(SUP);
					CALL_CMD(STA);
					CALL_CMD(INF);
					CALL_CMD(MSG);
					CALL_CMD(SCH);
					CALL_CMD(RES);
					CALL_CMD(CTM);
					CALL_CMD(RCM);
					CALL_CMD(GPA);
					CALL_CMD(PAS);
					CALL_CMD(QUI);
					CALL_CMD(GET);
					CALL_CMD(GFI);
					CALL_CMD(SND);
					CALL_CMD(SID);
					CALL_CMD(CMD);
					CALL_CMD(NAT);
					CALL_CMD(RNT);
					CALL_CMD(PSR);
					CALL_CMD(PMI);
					// ZLIF support
					CALL_CMD(ZON);
					CALL_CMD(ZOF);
				default:
					dcdebug("Unknown ADC command: %.50s\n", line.c_str());
					dcassert(0);
					break;
#undef CALL_CMD
			}
		}
};

#endif // !defined(ADC_COMMAND_H)
