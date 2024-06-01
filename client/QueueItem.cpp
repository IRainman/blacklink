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

#include "stdinc.h"
#include "QueueItem.h"
#include "LogManager.h"
#include "Download.h"
#include "File.h"
#include "UserConnection.h"
#include "SettingsManager.h"
#include "TimeUtil.h"
#include "Util.h"
#include "Random.h"
#include "version.h"

#ifdef USE_QUEUE_RWLOCK
std::unique_ptr<RWLock> QueueItem::g_cs = std::unique_ptr<RWLock>(RWLock::create());
#else
std::unique_ptr<CriticalSection> QueueItem::g_cs = std::unique_ptr<CriticalSection>(new CriticalSection);
#endif
std::atomic_bool QueueItem::checkTempDir(true);

const string dctmpExtension = ".dctmp";
const string xmlExtension = ".xml";
const string xmlBz2Extension = ".xml.bz2";

static const uint64_t DEFAULT_BLOCK_SIZE = 64 * 1024;

QueueItem::QueueItem(const string& target, int64_t size, Priority priority, MaskType flags, MaskType extraFlags,
                     time_t added, const TTHValue& tth, uint8_t maxSegments, const string& tempTarget) :
	target(target),
	size(size),
	added(added),
	flags(flags),
	extraFlags(extraFlags),
	tthRoot(tth),
	blockSize(size >= 0 ? TigerTree::getMaxBlockSize(size) : DEFAULT_BLOCK_SIZE),
	priority(priority),
	maxSegments(std::max(uint8_t(1), maxSegments)),
	doneSegmentsSize(0),
	downloadedBytes(0),
	timeFileBegin(0),
	lastSize(0),
	averageSpeed(0),
	sourcesVersion(0),
	prioQueue(QueueItem::DEFAULT),
	tempTarget(tempTarget)
{
}

QueueItem::QueueItem(const string& target, int64_t size, Priority priority, MaskType flags, MaskType extraFlags,
                     time_t added, uint8_t maxSegments, const string& tempTarget) :
	target(target),
	size(size),
	added(added),
	flags(flags),
	extraFlags(extraFlags),
	blockSize(size >= 0 ? TigerTree::getMaxBlockSize(size) : DEFAULT_BLOCK_SIZE),
	priority(priority),
	maxSegments(std::max(uint8_t(1), maxSegments)),
	doneSegmentsSize(0),
	downloadedBytes(0),
	timeFileBegin(0),
	lastSize(0),
	averageSpeed(0),
	sourcesVersion(0),
	prioQueue(QueueItem::DEFAULT),
	tempTarget(tempTarget)
{
}

QueueItem::QueueItem(const string& newTarget, QueueItem& src) :
	target(newTarget),
	size(src.size),
	added(src.added),
	flags(src.flags),
	extraFlags(src.extraFlags),
	tthRoot(src.tthRoot),
	blockSize(src.blockSize),
	priority(src.priority),
	maxSegments(src.maxSegments),
	downloads(std::move(src.downloads)),
	doneSegments(std::move(src.doneSegments)),
	doneSegmentsSize(src.doneSegmentsSize),
	downloadedBytes(src.downloadedBytes),
	timeFileBegin(src.timeFileBegin),
	lastSize(src.lastSize),
#ifdef DEBUG_TRANSFERS
	sourcePath(std::move(src.sourcePath)),
#endif
	averageSpeed(src.averageSpeed),
	sourcesVersion(0),
	prioQueue(src.prioQueue),
	sources(std::move(src.sources)),
	badSources(std::move(src.badSources))
{
	if (downloadedBytes)
		tempTarget = std::move(src.tempTarget);
	src.downloads.clear();
	src.doneSegments.clear();
	src.sources.clear();
	src.badSources.clear();
}

int16_t QueueItem::getTransferFlags(int& flags) const
{
	flags = TRANSFER_FLAG_DOWNLOAD;
	int16_t segs = 0;
	LOCK(csSegments);
	for (auto i = downloads.cbegin(); i != downloads.cend(); ++i)
	{
		const Download* d = i->d.get();
		if (d && d->getStartTime() > 0)
		{
			segs++;

			if (d->isSet(Download::FLAG_DOWNLOAD_PARTIAL))
				flags |= TRANSFER_FLAG_PARTIAL;
			if (d->isSecure)
			{
				flags |= TRANSFER_FLAG_SECURE;
				if (d->isTrusted)
					flags |= TRANSFER_FLAG_TRUSTED;
			}
			if (d->isSet(Download::FLAG_TTH_CHECK))
				flags |= TRANSFER_FLAG_TTH_CHECK;
			if (d->isSet(Download::FLAG_ZDOWNLOAD))
				flags |= TRANSFER_FLAG_COMPRESSED;
			if (d->isSet(Download::FLAG_CHUNKED))
				flags |= TRANSFER_FLAG_CHUNKED;
		}
	}
	return segs;
}

QueueItem::Priority QueueItem::calculateAutoPriorityL() const
{
	if (extraFlags & XFLAG_AUTO_PRIORITY)
	{
		QueueItem::Priority p;
		const int percent = static_cast<int>(getDownloadedBytes() * 10 / getSize());
		switch (percent)
		{
			case 0:
			case 1:
			case 2:
				p = QueueItem::LOW;
				break;
			case 3:
			case 4:
			case 5:
			default:
				p = QueueItem::NORMAL;
				break;
			case 6:
			case 7:
			case 8:
				p = QueueItem::HIGH;
				break;
			case 9:
			case 10:
				p = QueueItem::HIGHER;
				break;
		}
		return p;
	}
	return priority;
}

string QueueItem::getDCTempName(const string& fileName, const TTHValue* tth)
{
	string result = fileName;
	if (tth)
	{
		result += '.';
		result += tth->toBase32();
	}
	result += dctmpExtension;
	return result;
}

bool QueueItem::isBadSourceExceptL(const UserPtr& user, MaskType exceptions) const
{
	const auto i = badSources.find(user);
	if (i != badSources.end())
	{
		return i->second.isAnySet((MaskType) (exceptions ^ Source::FLAG_MASK));
	}
	return false;
}

bool QueueItem::hasOnlineSourcesL(size_t count) const
{
	if (sources.size() < count)
		return false;
	size_t onlineCount = 0;
	for (auto i = sources.cbegin(); i != sources.cend(); ++i)
		if (i->first->isOnline())
		{
			if (++onlineCount == count)
				return true;
		}
	return false;
}

size_t QueueItem::getOnlineSourceCountL() const
{
	size_t count = 0;
	for (auto i = sources.cbegin(); i != sources.cend(); ++i)
		if (i->first->isOnline())
			++count;
	return count;
}

void QueueItem::getOnlineUsers(UserList& list) const
{
	QueueRLock(*QueueItem::g_cs);
	for (auto i = sources.cbegin(); i != sources.cend(); ++i)
		if (i->first->isOnline())
			list.push_back(i->first);
}

UserPtr QueueItem::getFirstSource() const
{
	UserPtr user;
	QueueRLock(*QueueItem::g_cs);
	auto i = sources.cbegin();
	if (i != sources.end()) user = i->first;
	return user;
}

QueueItem::SourceIter QueueItem::addSourceL(const UserPtr& user)
{
	SourceIter it;
	dcassert(!isSourceL(user));
	SourceIter i = findBadSourceL(user);
	if (i != badSources.end())
	{
		it = sources.insert(*i).first;
		badSources.erase(i->first);
	}
	else
		it = sources.insert(std::make_pair(user, Source())).first;
	++sourcesVersion;
	return it;
}

void QueueItem::getPFSSourcesL(const QueueItemPtr& qi, SourceList& sourceList, uint64_t now, size_t maxCount)
{
	auto addToList = [&](const bool isBadSources) -> void
	{
		const auto& sources = isBadSources ? qi->getBadSourcesL() : qi->getSourcesL();
		for (auto j = sources.cbegin(); j != sources.cend(); ++j)
		{
			const auto &ps = j->second.partialSource;
			if (j->second.isCandidate(isBadSources) && ps->isCandidate(now))
			{
				sourceList.emplace(SourceListItem{ps->getNextQueryTime(), qi, j});
				if (sourceList.size() > maxCount)
					sourceList.erase(sourceList.begin() + sourceList.size()-1);
			}
		}
	};

	addToList(false);
	addToList(true);
}

void QueueItem::resetDownloaded()
{
	LOCK(csSegments);
	resetDownloadedL();
}

void QueueItem::resetDownloadedL()
{
	doneSegments.clear();
	doneSegmentsSize = downloadedBytes = 0;
}

bool QueueItem::isFinished() const
{
	LOCK(csSegments);
	return doneSegments.size() == 1 &&
		doneSegments.begin()->getStart() == 0 &&
		doneSegments.begin()->getSize() == getSize();
}

bool QueueItem::isChunkDownloaded(int64_t startPos, int64_t& len) const
{
	if (len <= 0)
		return false;
	LOCK(csSegments);
	for (auto i = doneSegments.cbegin(); i != doneSegments.cend(); ++i)
	{
		const int64_t start = i->getStart();
		const int64_t end   = i->getEnd();
		if (start <= startPos && startPos < end)
		{
			len = min(len, end - startPos);
			return true;
		}
	}
	return false;
}

void QueueItem::removeSourceL(const UserPtr& user, MaskType reason)
{
	SourceIter i = findSourceL(user);
	if (i != sources.end())
	{
		i->second.setFlag(reason);
		badSources.insert(*i);
		sources.erase(i);
		++sourcesVersion;
		if (sources.empty()) prioQueue = DEFAULT;
	}
#ifdef _DEBUG
	else
	{
		string error = "Error QueueItem::removeSourceL, user = [" + user->getLastNick() + "]";
		LogManager::message(error);
		dcassert(0);
	}
#endif
}

const string& QueueItem::getListExt() const
{
	dcassert(flags & (FLAG_USER_LIST | FLAG_DCLST_LIST));
	if (getExtraFlags() & XFLAG_XML_BZLIST)
		return xmlBz2Extension;
	if (flags & FLAG_DCLST_LIST)
		return Util::emptyString;
	return xmlExtension;
}

void QueueItem::addDownload(const DownloadPtr& download)
{
	LOCK(csSegments);
	dcassert(download->getUser());
	RunningSegment rs;
	rs.d = download;
	rs.seg = download->getSegment();
	downloads.push_back(rs);
}

void QueueItem::addDownload(const Segment& seg)
{
	LOCK(csSegments);
	RunningSegment rs;
	rs.seg = seg;
	downloads.push_back(rs);
}

bool QueueItem::setDownloadForSegment(const Segment& seg, const DownloadPtr& download)
{
	LOCK(csSegments);
	for (auto i = downloads.begin(); i != downloads.end(); ++i)
	{
		if (i->seg == seg)
		{
			dcassert(!i->d);
			i->d = download;
			return true;
		}
	}
	return false;
}

bool QueueItem::removeDownload(const UserPtr& user)
{
	LOCK(csSegments);
	for (auto i = downloads.begin(); i != downloads.end(); ++i)
	{
		const Download* d = i->d.get();
		if (d && d->getUser() == user)
		{
			downloads.erase(i);
			return true;
		}
	}
	return false;
}

bool QueueItem::removeDownload(const Segment& seg)
{
	LOCK(csSegments);
	for (auto i = downloads.begin(); i != downloads.end(); ++i)
		if (i->seg == seg)
		{
			downloads.erase(i);
			return true;
		}
	return false;
}

Segment QueueItem::getNextSegmentForward(const int64_t blockSize, const int64_t targetSize, vector<Segment>* neededParts, const vector<int64_t>& posArray) const
{
	int64_t start = 0;
	int64_t curSize = targetSize;
	if (!doneSegments.empty())
	{
		const Segment& segBegin = *doneSegments.begin();
		if (segBegin.getStart() == 0) start = segBegin.getEnd();
	}
	while (start < getSize())
	{
		int64_t end = std::min(getSize(), start + curSize);
		Segment block(start, end - start);
		bool overlaps = false;
		for (auto i = doneSegments.cbegin(); !overlaps && i != doneSegments.cend(); ++i)
		{
			if (curSize <= blockSize)
			{
				int64_t dstart = i->getStart();
				int64_t dend = i->getEnd();
				// We accept partial overlaps, only consider the block done if it is fully consumed by the done block
				if (dstart <= start && dend >= end)
					overlaps = true;
			}
			else
				overlaps = block.overlaps(*i);
		}
		if (!overlaps)
		{
			for (auto i = downloads.cbegin(); !overlaps && i != downloads.cend(); ++i)
				overlaps = block.overlaps(i->seg);
		}

		if (!overlaps)
		{
			if (!neededParts) return block;
			// store all chunks we could need
			for (vector<int64_t>::size_type j = 0; j < posArray.size(); j += 2)
			{
				if ((posArray[j] <= start && start < posArray[j + 1]) || (start <= posArray[j] && posArray[j] < end))
				{
					int64_t b = max(start, posArray[j]);
					int64_t e = min(end, posArray[j + 1]);

					// segment must be blockSize aligned
					dcassert(b % blockSize == 0);
					dcassert(e % blockSize == 0 || e == getSize());

					neededParts->push_back(Segment(b, e - b));
				}
			}
		}

		if (overlaps && curSize > blockSize)
		{
			curSize -= blockSize;
		}
		else
		{
			start = end;
			curSize = targetSize;
		}
	}
	return Segment(0, 0);
}

Segment QueueItem::getNextSegmentBackward(const int64_t blockSize, const int64_t targetSize, vector<Segment>* neededParts, const vector<int64_t>& posArray) const
{
	int64_t end = 0;
	int64_t curSize = targetSize;
	if (!doneSegments.empty())
	{
		const Segment& segEnd = *doneSegments.crbegin();
		if (segEnd.getEnd() == getSize()) end = segEnd.getStart();
	}
	if (!end) end = Util::roundUp(getSize(), blockSize);
	while (end > 0)
	{
		int64_t start = std::max<int64_t>(0, end - curSize);
		Segment block(start, std::min(end, getSize()) - start);
		bool overlaps = false;
		for (auto i = doneSegments.crbegin(); !overlaps && i != doneSegments.crend(); ++i)
		{
			if (curSize <= blockSize)
			{
				int64_t dstart = i->getStart();
				int64_t dend = i->getEnd();
				// We accept partial overlaps, only consider the block done if it is fully consumed by the done block
				if (dstart <= start && dend >= end)
					overlaps = true;
			}
			else
				overlaps = i->overlaps(block);
		}
		if (!overlaps)
		{
			for (auto i = downloads.crbegin(); !overlaps && i != downloads.crend(); ++i)
				overlaps = i->seg.overlaps(block);
		}

		if (!overlaps)
		{
			if (!neededParts) return block;
			// store all chunks we could need
			for (vector<int64_t>::size_type j = 0; j < posArray.size(); j += 2)
			{
				if ((posArray[j] <= start && start < posArray[j + 1]) || (start <= posArray[j] && posArray[j] < end))
				{
					int64_t b = max(start, posArray[j]);
					int64_t e = min(end, posArray[j + 1]);

					// segment must be blockSize aligned
					dcassert(b % blockSize == 0);
					dcassert(e % blockSize == 0 || e == getSize());

					neededParts->push_back(Segment(b, e - b));
				}
			}
		}

		if (overlaps && curSize > blockSize)
		{
			curSize -= blockSize;
		}
		else
		{
			end = start;
			curSize = targetSize;
		}
	}
	return Segment(0, 0);
}

bool QueueItem::shouldSearchBackward() const
{
	if (!(flags & FLAG_WANT_END) || doneSegments.empty()) return false;
	const Segment& segBegin = *doneSegments.begin();
	if (segBegin.getStart() != 0 || segBegin.getSize() < 1024*1204) return false;
	const Segment& segEnd = *doneSegments.rbegin();
	if (segEnd.getEnd() == getSize())
	{
		int64_t requiredSize = getSize()*3/100; // 3% of file
		if (segEnd.getSize() > requiredSize) return false;
	}
	return true;
}

Segment QueueItem::getNextSegmentL(const GetSegmentParams& gsp, const PartialSource::Ptr& partialSource, int* error) const
{
	const int64_t blockSize = getBlockSize();
	if (getSize() == -1 || blockSize == 0)
	{
		if (error) *error = SUCCESS;
		return Segment(0, -1);
	}

	csAttribs.lock();
	uint8_t savedMaxSegments = maxSegments;
	csAttribs.unlock();

	LOCK(csSegments);
	if (!gsp.enableMultiChunk)
	{
		if (!downloads.empty())
		{
			if (error) *error = ERROR_DOWNLOAD_SLOTS_TAKEN;
			return Segment(-1, 0);
		}

		int64_t start = 0;
		int64_t end = getSize();

		if (!doneSegments.empty())
		{
			const Segment& first = *doneSegments.begin();
			if (first.getStart() > 0)
			{
				end = Util::roundUp(first.getStart(), blockSize);
			}
			else
			{
				start = Util::roundDown(first.getEnd(), blockSize);
				if (doneSegments.size() > 1)
				{
					const Segment& second = *(++doneSegments.begin());
					end = Util::roundUp(second.getStart(), blockSize);
				}
			}
		}
		if (error) *error = SUCCESS;
		return Segment(start, std::min(getSize(), end) - start);
	}

	if (downloads.size() >= savedMaxSegments ||
	    (gsp.dontBeginSegment && static_cast<int64_t>(gsp.dontBeginSegSpeed) * 1024 < getAverageSpeed()))
	{
		// no other segments if we have reached the speed or segment limit
		if (error) *error = ERROR_NO_FREE_BLOCK;
		return Segment(-1, 0);
	}

	// Added for PFS
	vector<int64_t> posArray;
	vector<Segment> neededParts;

	if (partialSource && partialSource->getBlockSize() == blockSize)
	{
		posArray.reserve(partialSource->getParts().size());
		// Convert block index to file position
		for (auto i = partialSource->getParts().cbegin(); i != partialSource->getParts().cend(); ++i)
		{
			int64_t pos = (int64_t) *i * blockSize;
			posArray.push_back(min(getSize(), pos));
		}
	}

	double donePart = static_cast<double>(doneSegmentsSize) / getSize();

	// We want smaller blocks at the end of the transfer, squaring gives a nice curve...
	int64_t targetSize = static_cast<int64_t>(static_cast<double>(gsp.wantedSize) * std::max(0.25, 1 - donePart * donePart));
	if (gsp.maxChunkSize && targetSize > gsp.maxChunkSize)
		targetSize = gsp.maxChunkSize;

	// Round to nearest block size
	if (targetSize > blockSize)
		targetSize = Util::roundDown(targetSize, blockSize);
	else
		targetSize = blockSize;

	Segment block = shouldSearchBackward()?
		getNextSegmentBackward(blockSize, targetSize, partialSource? &neededParts : nullptr, posArray) :
		getNextSegmentForward(blockSize, targetSize, partialSource? &neededParts : nullptr, posArray);
	if (block.getSize())
	{
		if (error) *error = SUCCESS;
		return block;
	}

	if (!neededParts.empty())
	{
		// select random chunk for download
		dcdebug("Found partial chunks: %d\n", int(neededParts.size()));
		Segment& selected = neededParts[Util::rand(0, static_cast<uint32_t>(neededParts.size()))];
		selected.setSize(std::min(selected.getSize(), targetSize)); // request only wanted size
		if (error) *error = SUCCESS;
		return selected;
	}

#if 0 // Disabled
	if (!partialSource && gsp.overlapChunks && gsp.lastSpeed > 10 * 1024)
	{
		// overlap slow running chunk
		const uint64_t currentTick = GET_TICK();
		for (auto i = downloads.cbegin(); i != downloads.cend(); ++i)
		{
			const Download* d = i->d.get();
			if (!d)
				continue;

			// current chunk mustn't be already overlapped
			if (d->getOverlapped())
				continue;

			// current chunk must be running at least for 2 seconds
			if (d->getStartTime() == 0 || currentTick - d->getStartTime() < 2000)
				continue;

			// current chunk mustn't be finished in next 10 seconds
			int64_t secondsLeft = d->getSecondsLeft();
			if (secondsLeft < 10)
				continue;

			// overlap current chunk at last block boundary
			int64_t pos = d->getPos() - (d->getPos() % blockSize);
			int64_t size = d->getSize() - pos;

			// new user should finish this chunk more than 2x faster
			int64_t newChunkLeft = size / gsp.lastSpeed;
			if (2 * newChunkLeft < secondsLeft)
			{
				dcdebug("Overlapping... old user: " I64_FMT " s, new user: " I64_FMT " s\n", d->getSecondsLeft(), newChunkLeft);
				if (error) *error = SUCCESS;
				return Segment(d->getStartPos() + pos, size, true);
			}
		}
	}
#endif

	if (error) *error = ERROR_NO_NEEDED_PART;
	return Segment(-1, 0);
}

void QueueItem::setOverlapped(const Segment& segment, bool isOverlapped)
{
	// set overlapped flag to original segment
	LOCK(csSegments);
	for (auto i = downloads.cbegin(); i != downloads.cend(); ++i)
	{
		Download* d = i->d.get();
		if (d && d->getSegment().contains(segment))
		{
			d->setOverlapped(isOverlapped);
			break;
		}
	}
}

void QueueItem::updateDownloadedBytesAndSpeed()
{
	int64_t totalSpeed = 0;
	LOCK(csSegments);
	downloadedBytes = doneSegmentsSize;
	for (auto i = downloads.cbegin(); i != downloads.cend(); ++i)
	{
		const Download* d = i->d.get();
		if (d)
		{
			downloadedBytes += d->getPos();
			totalSpeed += d->getRunningAverage();
		}
	}
	averageSpeed = totalSpeed;
}

void QueueItem::updateDownloadedBytes()
{
	LOCK(csSegments);
	downloadedBytes = doneSegmentsSize;
}

void QueueItem::addSegment(const Segment& segment)
{
	LOCK(csSegments);
	addSegmentL(segment);
}

void QueueItem::addSegmentL(const Segment& segment)
{
	dcassert(!segment.getOverlapped());
	doneSegments.insert(segment);
	doneSegmentsSize += segment.getSize();
	// Consolidate segments
	if (doneSegments.size() == 1)
		return;
	for (auto current = ++doneSegments.cbegin(); current != doneSegments.cend();)
	{
		auto prev = current;
		--prev;
		const Segment& segCurrent = *current;
		const Segment& segPrev = *prev;
		if (segPrev.getEnd() >= segCurrent.getStart())
		{
			int64_t bigEnd = std::max(segCurrent.getEnd(), segPrev.getEnd());
			Segment big(segPrev.getStart(), bigEnd - segPrev.getStart());
			int64_t removedSize = segPrev.getSize() + segCurrent.getSize();
			doneSegments.erase(current);
			doneSegments.erase(prev);
			current = ++doneSegments.insert(big).first;
			doneSegmentsSize -= removedSize;
			doneSegmentsSize += big.getSize();
		} else ++current;
	}
}

bool QueueItem::isNeededPart(const PartsInfo& theirParts, const PartsInfo& ourParts)
{
	if ((theirParts.size() & 1) || (ourParts.size() & 1))
	{
		dcassert(0);
		return false;
	}
	if (theirParts.empty()) return false;
	uint16_t start = theirParts[0];
	uint16_t end = theirParts[1];
	size_t i = 0, j = 0;
	while (j + 2 <= ourParts.size())
	{
		uint16_t myStart = ourParts[j];
		uint16_t myEnd = ourParts[j + 1];
		if (start < myStart) return true;
		if (start >= myEnd)
		{
			j += 2;
			continue;
		}
		if (end <= myEnd)
		{
			i += 2;
			if (i + 2 > theirParts.size()) return false;
			start = theirParts[i];
			end = theirParts[i + 1];
		}
		else
		{
			start = myEnd;
			j += 2;
		}
	}
	return true;
}

void QueueItem::getParts(PartsInfo& partialInfo, uint64_t blockSize) const
{
	dcassert(blockSize);
	if (blockSize == 0)
		return;

	LOCK(csSegments);
	const size_t maxSize = min(doneSegments.size() * 2, (size_t) 510);
	partialInfo.reserve(maxSize);

	for (auto i = doneSegments.cbegin(); i != doneSegments.cend() && partialInfo.size() < maxSize; ++i)
	{
		uint16_t s = (uint16_t)((i->getStart() + blockSize - 1) / blockSize); // round up
		int64_t end = i->getEnd();
		if (end >= getSize()) end += blockSize - 1;
		uint16_t e = (uint16_t)(end / blockSize); // round down for all chunks but last
		partialInfo.push_back(s);
		partialInfo.push_back(e);
	}
}

int QueueItem::countParts(const QueueItem::PartsInfo& pi)
{
	int res = 0;
	for (size_t i = 0; i + 2 <= pi.size(); i += 2)
		res += pi[i+1] - pi[i];
	return res;
}

bool QueueItem::compareParts(const QueueItem::PartsInfo& a, const QueueItem::PartsInfo& b)
{
	if (a.size() != b.size()) return false;
	return memcmp(&a[0], &b[0], a.size() * sizeof(a[0])) == 0;
}

void QueueItem::getDoneSegments(vector<Segment>& done) const
{
	done.clear();
	LOCK(csSegments);
	done.reserve(doneSegments.size());
	for (auto i = doneSegments.cbegin(); i != doneSegments.cend(); ++i)
		done.push_back(*i);
}

void QueueItem::getChunksVisualisation(vector<SegmentEx>& running, vector<Segment>& done) const
{
	running.clear();
	done.clear();
	LOCK(csSegments);
	running.reserve(downloads.size());
	SegmentEx rs;
	for (auto i = downloads.cbegin(); i != downloads.cend(); ++i)
	{
		const Segment& segment = i->seg;
		const Download* d = i->d.get();
		rs.start = segment.getStart();
		rs.end = segment.getEnd();
		rs.pos = d ? d->getPos() : 0;
		running.push_back(rs);
	}
	done.reserve(doneSegments.size());
	for (auto i = doneSegments.cbegin(); i != doneSegments.cend(); ++i)
		done.push_back(*i);
}

bool QueueItem::isMultipleSegments() const
{
	int activeSegments = 0;
	LOCK(csSegments);
	for (auto i = downloads.cbegin(); i != downloads.cend(); ++i)
	{
		const Download* d = i->d.get();
		if (d && d->getStartTime() > 0)
		{
			activeSegments++;
			// more segments won't change anything
			if (activeSegments > 1)
				return true;
		}
	}
	return false;
}

void QueueItem::getUsers(UserList& users) const
{
	LOCK(csSegments);
	users.reserve(downloads.size());
	for (auto i = downloads.cbegin(); i != downloads.cend(); ++i)
	{
		const Download* d = i->d.get();
		if (d) users.push_back(d->getUser());
	}
}

void QueueItem::disconnectOthers(const DownloadPtr& download)
{
	LOCK(csSegments);
	// Disconnect all possible overlapped downloads
	for (auto i = downloads.cbegin(); i != downloads.cend(); ++i)
	{
		const DownloadPtr& d = i->d;
		if (d && d != download)
			d->disconnect();
	}
}

bool QueueItem::disconnectSlow(const DownloadPtr& download)
{
	bool found = false;
	// ok, we got a fast slot, so it's possible to disconnect original user now
	LOCK(csSegments);
	for (auto i = downloads.cbegin(); i != downloads.cend(); ++i)
	{
		const DownloadPtr& d = i->d;
		if (d && d != download && d->getSegment().contains(download->getSegment()))
		{
			// overlapping has no sense if segment is going to finish
			if (d->getSecondsLeft() < 10)
				break;

			found = true;

			// disconnect slow chunk
			d->disconnect();
			break;
		}
	}
	return found;
}

bool QueueItem::updateBlockSize(uint64_t treeBlockSize)
{
	if (treeBlockSize > blockSize)
	{
		dcassert(!(treeBlockSize % blockSize));
		blockSize = treeBlockSize;
		return true;
	}
	return false;
}

void QueueItem::GetSegmentParams::readSettings()
{
	enableMultiChunk = BOOLSETTING(ENABLE_MULTI_CHUNK);
	dontBeginSegment = BOOLSETTING(DONT_BEGIN_SEGMENT);
	overlapChunks = BOOLSETTING(OVERLAP_CHUNKS);
	dontBeginSegSpeed = SETTING(DONT_BEGIN_SEGMENT_SPEED);
	maxChunkSize = SETTING(MAX_CHUNK_SIZE);
}
