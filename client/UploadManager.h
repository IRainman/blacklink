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


#ifndef DCPLUSPLUS_DCPP_UPLOAD_MANAGER_H
#define DCPLUSPLUS_DCPP_UPLOAD_MANAGER_H

#include <set>
#include "Singleton.h"
#include "UploadManagerListener.h"
#include "ClientManagerListener.h"
#include "UserConnection.h"
#include "Client.h"
#include "LocationUtil.h"
#include <regex>

#ifdef _DEBUG
#include <atomic>
#endif

typedef std::list<UploadPtr> UploadList;

class UploadQueueFile
{
	public:
		static const int16_t FLAG_PARTIAL_FILE_LIST = 1;

		UploadQueueFile(const string& file, int64_t pos, int64_t size, uint16_t flags) :
			file(file), flags(flags), pos(pos), size(size), time(GET_TIME())
		{
#ifdef _DEBUG
			++g_upload_queue_item_count;
#endif
		}
#ifdef _DEBUG
		~UploadQueueFile()
		{
			--g_upload_queue_item_count;
		}
#endif
		GETC(string, file, File);
		GETSET(uint16_t, flags, Flags);
		GETSET(int64_t, pos, Pos);
		GETC(int64_t, size, Size);
		GETC(uint64_t, time, Time);
#ifdef _DEBUG
		static std::atomic_int g_upload_queue_item_count;
#endif
};

class WaitingUser
{
	public:
		static const size_t MAX_WAITING_FILES = 50;

		WaitingUser(const HintedUser& hintedUser, const std::string& token, const UploadQueueFilePtr& uqi) : hintedUser(hintedUser), token(token)
		{
			waitingFiles.push_back(uqi);
		}
		operator const UserPtr&() const { return hintedUser.user; }
		const UserPtr& getUser() const { return hintedUser.user; }
		const HintedUser& getHintedUser() const { return hintedUser; }
		void addWaitingFile(const UploadQueueFilePtr& uqi);
		UploadQueueFilePtr findWaitingFile(const string& file) const;
		const vector<UploadQueueFilePtr>& getWaitingFiles() const { return waitingFiles; }

	private:
		vector<UploadQueueFilePtr> waitingFiles;
		const HintedUser hintedUser;

		GETSET(string, token, Token);
};

class UploadManager : private ClientManagerListener, public Speaker<UploadManagerListener>, private TimerManagerListener, public Singleton<UploadManager>
{
		friend class Singleton<UploadManager>;

	public:
		static uint32_t g_count_WaitingUsersFrame;
		typedef std::list<WaitingUser> SlotQueue;

		/** @return Number of uploads. */
		size_t getUploadCount() const
		{
			// READ_LOCK(*g_csUploadsDelay);
			return uploads.size();
		}
		void clearUploads() noexcept;

		static int getRunningCount() { return g_running; }
		static int64_t getRunningAverage() { return g_runningAverage; }
		static void setRunningAverage(int64_t avg) { g_runningAverage = avg; }
		
		static int getSlots();
		static int getFreeSlots() { return std::max((getSlots() - g_running), 0); }

		int getFreeExtraSlots() const;

		/** @param user Reserve an upload slot for this user and connect. */
		void reserveSlot(const HintedUser& hintedUser, uint64_t seconds);
		void unreserveSlot(const HintedUser& hintedUser);
		void clearUserFilesL(const UserPtr&);

		class LockInstanceQueue
		{
			public:
				LockInstanceQueue()
				{
					UploadManager::getInstance()->csQueue.lock();
				}
				~LockInstanceQueue()
				{
					UploadManager::getInstance()->csQueue.unlock();
				}
				UploadManager* operator->()
				{
					return UploadManager::getInstance();
				}
		};
		
		const SlotQueue& getUploadQueueL() const { return slotQueue; }
		uint64_t getSlotQueueIdL() const { return slotQueueId; }
		bool getIsFireballStatus() const { return isFireball; }
		bool getIsFileServerStatus() const { return isFileServer; }

		void processGet(UserConnection* source, const string& fileName, int64_t resume) noexcept;
		void processGetBlock(UserConnection* source, const string& cmd, const string& param) noexcept;
		void processSend(UserConnection* source) noexcept;
		void processGET(UserConnection*, const AdcCommand&) noexcept;
		void processGFI(UserConnection*, const AdcCommand&) noexcept;

		void failed(UserConnection* source, const string& error) noexcept;
		void transmitDone(UserConnection* source) noexcept;

		/** @internal */
		void addConnection(UserConnection* conn);
		void removeFinishedUpload(const UserPtr& user);
		void abortUpload(const string& file, bool waiting = true);
		
		GETSET(int, extraPartial, ExtraPartial);
		GETSET(int, extra, Extra);
		GETSET(uint64_t, lastGrant, LastGrant);
		
		void load();
		void save();

		uint64_t getReservedSlotTick(const UserPtr& user) const;

		struct ReservedSlotInfo
		{
			UserPtr user;
			uint64_t timeout;
		};
		void getReservedSlots(vector<ReservedSlotInfo>& out) const;

		void shutdown();
		
	private:
		enum
		{
			COMPRESSION_DISABLED,
			COMPRESSION_ENABLED,
			COMPRESSION_CHECK_FILE_TYPE
		};

		bool isFireball;
		bool isFileServer;
		static int g_running;
		static int64_t g_runningAverage;
		uint64_t fireballStartTick;
		uint64_t fileServerCheckTick;

		UploadList uploads;
		UploadList finishedUploads;
		std::unique_ptr<RWLock> csFinishedUploads;
		
		void processSlot(UserConnection::SlotTypes slotType, int delta);
		
		int lastFreeSlots; /// amount of free slots at the previous minute
		
		typedef boost::unordered_map<UserPtr, uint64_t> SlotMap;
		
		SlotMap reservedSlots;
		mutable std::unique_ptr<RWLock> csReservedSlots;
		
		SlotMap notifiedUsers;
		SlotQueue slotQueue;
		mutable CriticalSection csQueue;
		uint64_t slotQueueId;

		std::regex reCompressedFiles;
		string compressedFilesPattern;
		FastCriticalSection csCompressedFiles;

		size_t addFailedUpload(const UserConnection* source, const string& file, int64_t pos, int64_t size, uint16_t flags);
		void notifyQueuedUsers(int64_t tick);
		
		bool getAutoSlot() const;
		void removeConnectionSlot(UserConnection* conn);
		void removeUpload(UploadPtr& upload, bool delay = false);
		void logUpload(const UploadPtr& u);
		
		void testSlotTimeout(uint64_t tick = GET_TICK());
		
		// ClientManagerListener
		void on(ClientManagerListener::UserDisconnected, const UserPtr& user) noexcept override;
		
		// TimerManagerListener
		void on(Second, uint64_t tick) noexcept override;
		void on(Minute, uint64_t tick) noexcept override;
		
		bool prepareFile(UserConnection* source, const string& type, const string& file, bool hideShare, const CID& shareGroup, int64_t resume, int64_t& bytes, bool listRecursive, int& compressionType, string& errorText);
		bool isCompressedFile(const string& target);
		bool hasUpload(const UserConnection* newLeecher) const;
		static void initTransferData(TransferData& td, const Upload* u);

		UploadManager() noexcept;
		~UploadManager();
};

#endif // !defined(UPLOAD_MANAGER_H)
