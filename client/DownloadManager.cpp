/* 
 * Copyright (C) 2001-2005 Jacek Sieka, arnetheduck on gmail point com
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
#include "DCPlusPlus.h"

#include "DownloadManager.h"

#include "ConnectionManager.h"
#include "QueueManager.h"
#include "CryptoManager.h"
#include "HashManager.h"

#include "LogManager.h"
#include "SFVReader.h"
#include "User.h"
#include "File.h"
#include "FilteredFile.h"
#include "MerkleCheckOutputStream.h"

#include <limits>

// some strange mac definition
#ifdef ff
#undef ff
#endif

static const string DOWNLOAD_AREA = "Downloads";
const string Download::ANTI_FRAG_EXT = ".antifrag";

Download::Download() throw() : file(NULL),
crcCalc(NULL), treeValid(false), tth(NULL) { 
}

Download::Download(QueueItem* qi) throw() : source(qi->getCurrent()->getPath()),
	target(qi->getTarget()), tempTarget(qi->getTempTarget()), file(NULL),
	crcCalc(NULL), treeValid(false), tth(qi->getTTH()) { 
	
	setSize(qi->getSize());
	if(qi->isSet(QueueItem::FLAG_USER_LIST))
		setFlag(Download::FLAG_USER_LIST);
	if(qi->isSet(QueueItem::FLAG_RESUME))
		setFlag(Download::FLAG_RESUME);
	if(qi->getCurrent()->isSet(QueueItem::Source::FLAG_UTF8))
		setFlag(Download::FLAG_UTF8);
};

AdcCommand Download::getCommand(bool zlib, bool tthf) {
	AdcCommand cmd(AdcCommand::CMD_GET);
	if(isSet(FLAG_TREE_DOWNLOAD)) {
		cmd.addParam("tthl");
	} else {
		cmd.addParam("file");
	}
	if(tthf && getTTH() != NULL) {
		cmd.addParam("TTH/" + getTTH()->toBase32());
	} else {
		cmd.addParam(Util::toAdcFile(getSource()));
	}
	cmd.addParam(Util::toString(getPos()));
	cmd.addParam(Util::toString(getSize() - getPos()));

	if(zlib && getSize() != -1 && BOOLSETTING(COMPRESS_TRANSFERS)) {
		cmd.addParam("ZL1");
	}

	return cmd;
}

void DownloadManager::on(TimerManagerListener::Second, u_int32_t /*aTick*/) throw() {
	Lock l(cs);

	Download::List tickList;
	// Tick each ongoing download
	for(Download::Iter i = downloads.begin(); i != downloads.end(); ++i) {
		if((*i)->getTotal() > 0) {
			tickList.push_back(*i);
		}
	}

	if(tickList.size() > 0)
		fire(DownloadManagerListener::Tick(), tickList);
}

void DownloadManager::FileMover::moveFile(const string& source, const string& target) {
	Lock l(cs);
	files.push_back(make_pair(source, target));
	if(!active) {
		active = true;
		start();
	}
}

int DownloadManager::FileMover::run() {
	for(;;) {
		FilePair next;
		{
			Lock l(cs);
			if(files.empty()) {
				active = false;
				return 0;
			}
			next = files.back();
			files.pop_back();
		}
		try {
			File::renameFile(next.first, next.second);
		} catch(const FileException&) {
			// Too bad...
		}
	}
}

void DownloadManager::removeConnection(UserConnection::Ptr aConn, bool reuse /* = false */, bool ntd /* = false */) {
	dcassert(aConn->getDownload() == NULL);
	aConn->removeListener(this);
	ConnectionManager::getInstance()->putDownloadConnection(aConn, reuse, ntd);
}

class TreeOutputStream : public OutputStream {
public:
	TreeOutputStream(TigerTree& aTree) : tree(aTree), bufPos(0) {
	}

	virtual size_t write(const void* xbuf, size_t len) throw(Exception) {
		size_t pos = 0;
		u_int8_t* b = (u_int8_t*)xbuf;
		while(pos < len) {
			size_t left = len - pos;
			if(bufPos == 0 && left >= TigerTree::HASH_SIZE) {
				tree.getLeaves().push_back(TTHValue(b + pos));
				pos += TigerTree::HASH_SIZE;
			} else {
				size_t bytes = min(TigerTree::HASH_SIZE - bufPos, left);
				memcpy(buf + bufPos, b + pos, bytes);
				bufPos += bytes;
				pos += bytes;
				if(bufPos == TigerTree::HASH_SIZE) {
					tree.getLeaves().push_back(TTHValue(buf));
					bufPos = 0;
				}
			}
		}
		return len;
	}

	virtual size_t flush() throw(Exception) {
		return 0;
	}
private:
	TigerTree& tree;
	u_int8_t buf[TigerTree::HASH_SIZE];
	size_t bufPos;
};

void DownloadManager::checkDownloads(UserConnection* aConn) {
	dcassert(aConn->getDownload() == NULL);

	bool slotsFull = (SETTING(DOWNLOAD_SLOTS) != 0) && (getDownloadCount() >= (size_t)SETTING(DOWNLOAD_SLOTS));
	bool speedFull = (SETTING(MAX_DOWNLOAD_SPEED) != 0) && (getAverageSpeed() >= (SETTING(MAX_DOWNLOAD_SPEED)*1024));

	if( slotsFull || speedFull ) {
		bool extraFull = (SETTING(DOWNLOAD_SLOTS) != 0) && (getDownloadCount() >= (size_t)(SETTING(DOWNLOAD_SLOTS)+3));
		if(extraFull || !QueueManager::getInstance()->hasDownload(aConn->getUser(), QueueItem::HIGHEST)) {
			removeConnection(aConn);
			return;
		}
	}

	Download* d = QueueManager::getInstance()->getDownload(aConn->getUser(), aConn->isSet(UserConnection::FLAG_SUPPORTS_TTHL));

	if(d == NULL) {
		removeConnection(aConn, true);
		return;
	}

	{
		Lock l(cs);
		downloads.push_back(d);
	}

	d->setUserConnection(aConn);
	aConn->setDownload(d);

	aConn->setState(UserConnection::STATE_FILELENGTH);
	
	if(d->isSet(Download::FLAG_RESUME)) {
		dcassert(d->getSize() != -1);

		const string& target = (d->getTempTarget().empty() ? d->getTarget() : d->getTempTarget());
		int64_t start = File::getSize(target);

		// Only use antifrag if we don't have a previous non-antifrag part
		if( BOOLSETTING(ANTI_FRAG) && (start == -1) && (d->getSize() != -1) ) {
			int64_t aSize = File::getSize(target + Download::ANTI_FRAG_EXT);

			if(aSize == d->getSize())
				start = d->getPos();
			else
				start = 0;

			d->setFlag(Download::FLAG_ANTI_FRAG);
		}

		if(BOOLSETTING(ADVANCED_RESUME) && d->getTreeValid() && d->getStartPos() > 0) {
			d->setStartPos(getResumePos(d->getDownloadTarget(), d->getTigerTree(), d->getStartPos()));
		} else {
			int rollback = SETTING(ROLLBACK);
			if(rollback > start) {
				d->setStartPos(0);
			} else {
				d->setStartPos(start - rollback);
				d->setFlag(Download::FLAG_ROLLBACK);
			}
		}
		
	} else {
		d->setStartPos(0);
	}

	if(d->isSet(Download::FLAG_USER_LIST)) {
		if(!aConn->isSet(UserConnection::FLAG_NMDC) || aConn->isSet(UserConnection::FLAG_SUPPORTS_XML_BZLIST)) {
			d->setSource("files.xml.bz2");
			if(!aConn->isSet(UserConnection::FLAG_NMDC) || aConn->isSet(UserConnection::FLAG_SUPPORTS_ADCGET))
				d->setFlag(Download::FLAG_UTF8);
		}
	}

	// File ok for adcget in nmdc-conns
	bool adcOk = d->isSet(Download::FLAG_UTF8) || (aConn->isSet(UserConnection::FLAG_SUPPORTS_TTHF) && d->getTTH() != NULL);

	if(!aConn->isSet(UserConnection::FLAG_NMDC) || (aConn->isSet(UserConnection::FLAG_SUPPORTS_ADCGET) && adcOk)) {
		aConn->send(d->getCommand(
			aConn->isSet(UserConnection::FLAG_SUPPORTS_ZLIB_GET),
			aConn->isSet(!aConn->isSet(UserConnection::FLAG_NMDC) || UserConnection::FLAG_SUPPORTS_TTHF)
			));
	} else {
		if(BOOLSETTING(COMPRESS_TRANSFERS) && aConn->isSet(UserConnection::FLAG_SUPPORTS_GETZBLOCK) && d->getSize() != -1 ) {
			// This one, we'll download with a zblock download instead...
			d->setFlag(Download::FLAG_ZDOWNLOAD);
			aConn->getZBlock(d->getSource(), d->getPos(), d->getBytesLeft(), d->isSet(Download::FLAG_UTF8));
		} else if(aConn->isSet(UserConnection::FLAG_SUPPORTS_XML_BZLIST) && d->isSet(Download::FLAG_UTF8)) {
			aConn->uGetBlock(d->getSource(), d->getPos(), d->getBytesLeft());
		} else {
			aConn->get(d->getSource(), d->getPos());
		}
	}
}

class DummyOutputStream : public OutputStream {
public:
	virtual size_t write(const void*, size_t n) throw(Exception) { return n; }
	virtual size_t flush() throw(Exception) { return 0; }
};

int64_t DownloadManager::getResumePos(const string& file, const TigerTree& tt, int64_t startPos) {
	// Always discard data until the last block
	startPos = startPos - (startPos % tt.getBlockSize());
	if(startPos < tt.getBlockSize())
		return 0;

	DummyOutputStream dummy;

	vector<u_int8_t> buf((size_t)min((int64_t)1024*1024, tt.getBlockSize()));

	do {
		int64_t blockPos = startPos - tt.getBlockSize();
		MerkleCheckOutputStream<TigerTree, false> check(tt, &dummy, blockPos);

		try {
			File inFile(file, File::READ, File::OPEN);
			inFile.setPos(blockPos);
			int64_t bytesLeft = tt.getBlockSize();
			while(bytesLeft > 0) {
				size_t n = buf.size();
				n = inFile.read(&buf[0], n);
				check.write(&buf[0], n);
				bytesLeft -= n;
			}
			break;
		} catch(const Exception&) {
			dcdebug("Removed bad block at " I64_FMT "\n", blockPos);
		}
		startPos = blockPos;
	} while(startPos > 0);
	return startPos;
}


void DownloadManager::on(UserConnectionListener::Sending, UserConnection* aSource, int64_t aBytes) throw() {
	if(aSource->getState() != UserConnection::STATE_FILELENGTH) {
		dcdebug("DM::onFileLength Bad state, ignoring\n");
		return;
	}

	if(prepareFile(aSource, (aBytes == -1) ? -1 : aSource->getDownload()->getPos() + aBytes, aSource->getDownload()->isSet(Download::FLAG_ZDOWNLOAD))) {
		aSource->setDataMode();
	}
}

void DownloadManager::on(UserConnectionListener::FileLength, UserConnection* aSource, int64_t aFileLength) throw() {

	if(aSource->getState() != UserConnection::STATE_FILELENGTH) {
		dcdebug("DM::onFileLength Bad state, ignoring\n");
		return;
	}

	if(prepareFile(aSource, aFileLength, aSource->getDownload()->isSet(Download::FLAG_ZDOWNLOAD))) {
		aSource->setDataMode();
		aSource->startSend();
	}
}

void DownloadManager::on(AdcCommand::SND, UserConnection* aSource, const AdcCommand& cmd) throw() {
	if(aSource->getState() != UserConnection::STATE_FILELENGTH) {
		dcdebug("DM::onFileLength Bad state, ignoring\n");
		return;
	}

	const string& type = cmd.getParam(0);
	int64_t bytes = Util::toInt64(cmd.getParam(3));

	if(!(type == "file" || (type == "tthl" && aSource->getDownload()->isSet(Download::FLAG_TREE_DOWNLOAD))))
	{
		// Uhh??? We didn't ask for this?
		aSource->disconnect();
		return;
	}

	if(prepareFile(aSource, (bytes == -1) ? -1 : aSource->getDownload()->getPos() + bytes, cmd.hasFlag("ZL", 4))) {
		aSource->setDataMode();
	}
}

class RollbackException : public FileException {
public:
	RollbackException (const string& aError) : FileException(aError) { };
};

template<bool managed>
class RollbackOutputStream : public OutputStream {
public:
	RollbackOutputStream(File* f, OutputStream* aStream, size_t bytes) : s(aStream), pos(0), bufSize(bytes), buf(new u_int8_t[bytes]) {
		size_t n = bytes;
		f->read(buf, n);
		f->movePos(-((int64_t)bytes));
	}
	virtual ~RollbackOutputStream() { delete[] buf; if(managed) delete s; };

	virtual size_t flush() throw(FileException) {
		return s->flush();
	}

	virtual size_t write(const void* b, size_t len) throw(FileException) {
		u_int8_t* wb = (u_int8_t*)b;
		if(buf != NULL) {
			size_t n = len < (bufSize - pos) ? len : bufSize - pos;

			if(memcmp(buf + pos, wb, n) != 0) {
				throw RollbackException(STRING(ROLLBACK_INCONSISTENCY));
			}
			pos += n;
			if(pos == bufSize) {
				delete buf;
				buf = NULL;
			}
		}
		return s->write(wb, len);
	}

private:
	OutputStream* s;
	size_t pos;
	size_t bufSize;
	u_int8_t* buf;
};


bool DownloadManager::prepareFile(UserConnection* aSource, int64_t newSize, bool z) {
	Download* d = aSource->getDownload();
	dcassert(d != NULL);

	if(newSize != -1) {
		d->setSize(newSize);
	}
	if(d->getPos() >= d->getSize()) {
		// Already finished?
		aSource->setDownload(NULL);
		removeDownload(d);
		QueueManager::getInstance()->putDownload(d, true);
		removeConnection(aSource);
		return false;
	}

	dcassert(d->getSize() != -1);

	if(d->isSet(Download::FLAG_TREE_DOWNLOAD)) {
		d->setFile(new TreeOutputStream(d->getTigerTree()));
	} else {
		string target = d->getDownloadTarget();
		File::ensureDirectory(target);
		if(d->isSet(Download::FLAG_USER_LIST)) {
			if(!aSource->isSet(UserConnection::FLAG_NMDC) || aSource->isSet(UserConnection::FLAG_SUPPORTS_XML_BZLIST)) {
				target += ".xml.bz2";
			} else {
				target += ".DcLst";
			}
		}

		File* file = NULL;
		try {
			// Let's check if we can find this file in a any .SFV...
			int trunc = d->isSet(Download::FLAG_RESUME) ? 0 : File::TRUNCATE;
			file = new File(target, File::RW, File::OPEN | File::CREATE | trunc);
			if(d->isSet(Download::FLAG_ANTI_FRAG)) {
				file->setSize(d->getSize());
			}
			file->setPos(d->getPos());
		} catch(const FileException& e) {
			delete file;
			removeDownload(d);
			fire(DownloadManagerListener::Failed(), d, STRING(COULD_NOT_OPEN_TARGET_FILE) + e.getError());
			aSource->setDownload(NULL);
			QueueManager::getInstance()->putDownload(d, false);
			removeConnection(aSource);
			return false;
		} catch(const Exception& e) {
			delete file;
			removeDownload(d);
			fire(DownloadManagerListener::Failed(), d, e.getError());
			aSource->setDownload(NULL);
			QueueManager::getInstance()->putDownload(d, false);
			removeConnection(aSource);
			return false;
		}

		d->setFile(file);

		bool sfvcheck = BOOLSETTING(SFV_CHECK) && (d->getPos() == 0) && (SFVReader(d->getTarget()).hasCRC());

		if(sfvcheck) {
			d->setFlag(Download::FLAG_CALC_CRC32);
			Download::CrcOS* crc = new Download::CrcOS(d->getFile());
			d->setCrcCalc(crc);
			d->setFile(crc);
		}

		if(SETTING(BUFFER_SIZE) > 0 ) {
			d->setFile(new BufferedOutputStream<true>(d->getFile()));
		}

		/** @todo something when resuming... */
		if(d->getTreeValid()) {
			if((d->getPos() % d->getTigerTree().getBlockSize()) == 0) {
				d->setFile(new MerkleCheckOutputStream<TigerTree, true>(d->getTigerTree(), d->getFile(), d->getPos()));
			}
		}
		if(d->isSet(Download::FLAG_ROLLBACK)) {
			d->setFile(new RollbackOutputStream<true>(file, d->getFile(), (size_t)min((int64_t)SETTING(ROLLBACK), d->getSize() - d->getPos())));
		}

	}

	if(z) {
		d->setFlag(Download::FLAG_ZDOWNLOAD);
		d->setFile(new FilteredOutputStream<UnZFilter, true>(d->getFile()));
	}

	dcassert(d->getPos() != -1);
	d->setStart(GET_TICK());
	aSource->setState(UserConnection::STATE_DONE);
	
	fire(DownloadManagerListener::Starting(), d);
	
	return true;
}	

void DownloadManager::on(UserConnectionListener::Data, UserConnection* aSource, const u_int8_t* aData, size_t aLen) throw() {
	Download* d = aSource->getDownload();
	dcassert(d != NULL);

	try {
		d->addPos(d->getFile()->write(aData, aLen), aLen);

		if(d->getPos() > d->getSize()) {
			throw Exception(STRING(TOO_MUCH_DATA));
		} else if(d->getPos() == d->getSize()) {
			handleEndData(aSource);
			aSource->setLineMode();
		}
	} catch(const RollbackException& e) {
		string target = d->getTarget();
		QueueManager::getInstance()->removeSource(target, aSource->getUser(), QueueItem::Source::FLAG_ROLLBACK_INCONSISTENCY);
		removeDownload(d);
		fire(DownloadManagerListener::Failed(), d, e.getError());

		d->resetPos();
		aSource->setDownload(NULL);
		QueueManager::getInstance()->putDownload(d, false);
		removeConnection(aSource);
		return;
	} catch(const FileException& e) {
		removeDownload(d);
		fire(DownloadManagerListener::Failed(), d, e.getError());

		d->resetPos();
		aSource->setDownload(NULL);
		QueueManager::getInstance()->putDownload(d, false);
		removeConnection(aSource);
		return;
	} catch(const Exception& e) {
		removeDownload(d);
		fire(DownloadManagerListener::Failed(), d, e.getError());
		// Nuke the bytes we have written, this is probably a compression error
		d->resetPos();
		aSource->setDownload(NULL);
		QueueManager::getInstance()->putDownload(d, false);
		removeConnection(aSource);
		return;
	}
}

/** Download finished! */
void DownloadManager::handleEndData(UserConnection* aSource) {

	dcassert(aSource->getState() == UserConnection::STATE_DONE);
	Download* d = aSource->getDownload();
	dcassert(d != NULL);

	if(d->isSet(Download::FLAG_TREE_DOWNLOAD)) {
		d->getFile()->flush();
		delete d->getFile();
		d->setFile(NULL);

		int64_t bl = 1024;
		while(bl * (int64_t)d->getTigerTree().getLeaves().size() < d->getTigerTree().getFileSize())
			bl *= 2;
		d->getTigerTree().setBlockSize(bl);
		d->getTigerTree().calcRoot();

		if(!(*d->getTTH() == d->getTigerTree().getRoot())) {
			// This tree is for a different file, remove from queue...
			removeDownload(d);
			fire(DownloadManagerListener::Failed(), d, STRING(INVALID_TREE));

			string target = d->getTarget();

			aSource->setDownload(NULL);
			QueueManager::getInstance()->putDownload(d, false);

			QueueManager::getInstance()->removeSource(target, aSource->getUser(), QueueItem::Source::FLAG_BAD_TREE, false);
			checkDownloads(aSource);
			return;
		}
		d->setTreeValid(true);
	} else {

		// Hm, if the real crc == 0, we'll get a file reread extra, but what the heck...
		u_int32_t crc = 0;

		// First, finish writing the file (flushing the buffers and closing the file...)
		try {
			d->getFile()->flush();
			if(d->getCrcCalc() != NULL)
				crc = d->getCrcCalc()->getFilter().getValue();
			delete d->getFile();
			d->setFile(NULL);
			d->setCrcCalc(NULL);

			// Check if we're anti-fragging...
			if(d->isSet(Download::FLAG_ANTI_FRAG)) {
				// Ok, rename the file to what we expect it to be...
				try {
					const string& tgt = d->getTempTarget().empty() ? d->getTarget() : d->getTempTarget();
					File::renameFile(d->getDownloadTarget(), tgt);
					d->unsetFlag(Download::FLAG_ANTI_FRAG);
				} catch(const FileException& e) {
					dcdebug("AntiFrag: %s\n", e.getError().c_str());
					// Now what?
				}
			}
		} catch(const FileException& e) {
			removeDownload(d);
			fire(DownloadManagerListener::Failed(), d, e.getError());
			
			aSource->setDownload(NULL);
			QueueManager::getInstance()->putDownload(d, false);
			removeConnection(aSource);
			return;
		}
		
		dcassert(d->getPos() == d->getSize());
		dcdebug("Download finished: %s, size " I64_FMT ", downloaded " I64_FMT "\n", d->getTarget().c_str(), d->getSize(), d->getTotal());

		// Check if we have some crc:s...
		if(BOOLSETTING(SFV_CHECK)) {
			if(!checkSfv(aSource, d, crc))
				return;
		}

		if(BOOLSETTING(LOG_DOWNLOADS) && (BOOLSETTING(LOG_FILELIST_TRANSFERS) || !d->isSet(Download::FLAG_USER_LIST))) {
			logDownload(aSource, d);
		}

		// Check if we need to move the file
		if( !d->getTempTarget().empty() && (Util::stricmp(d->getTarget().c_str(), d->getTempTarget().c_str()) != 0) ) {
			moveFile(d->getTempTarget(), d->getTarget());
		}
	}

	removeDownload(d);
	fire(DownloadManagerListener::Complete(), d);
	
	aSource->setDownload(NULL);
	QueueManager::getInstance()->putDownload(d, true);
	checkDownloads(aSource);
}

u_int32_t DownloadManager::calcCrc32(const string& file) throw(FileException) {
	File ff(file, File::READ, File::OPEN);
	CalcInputStream<CRC32Filter, false> f(&ff);

	const size_t BUF_SIZE = 1024*1024;
	AutoArray<u_int8_t> b(BUF_SIZE);
	size_t n = BUF_SIZE;
	while(f.read((u_int8_t*)b, n) > 0)
		;		// Keep on looping...

	return f.getFilter().getValue();
}

bool DownloadManager::checkSfv(UserConnection* aSource, Download* d, u_int32_t crc) {
	SFVReader sfv(d->getTarget());
	if(sfv.hasCRC()) {
		bool crcMatch = (crc == sfv.getCRC());
		if(!crcMatch && crc == 0) {
			// Blah. We have to reread the file...
			try {
				crcMatch = (calcCrc32(d->getDownloadTarget()) == sfv.getCRC());
			} catch(const FileException& ) {
				// Couldn't read the file to get the CRC(!!!)
				crcMatch = false;
			}
		}

		if(!crcMatch) {
			File::deleteFile(d->getDownloadTarget());
			dcdebug("DownloadManager: CRC32 mismatch for %s\n", d->getTarget().c_str());
			LogManager::getInstance()->message(STRING(SFV_INCONSISTENCY) + " (" + STRING(FILE) + ": " + d->getTarget() + ")");
			removeDownload(d);				
			fire(DownloadManagerListener::Failed(), d, STRING(SFV_INCONSISTENCY));

			string target = d->getTarget();

			aSource->setDownload(NULL);
			QueueManager::getInstance()->putDownload(d, false);

			QueueManager::getInstance()->removeSource(target, aSource->getUser(), QueueItem::Source::FLAG_CRC_WARN, false);
			checkDownloads(aSource);
			return false;
		} 

		d->setFlag(Download::FLAG_CRC32_OK);

		dcdebug("DownloadManager: CRC32 match for %s\n", d->getTarget().c_str());
	}
	return true;
}

void DownloadManager::logDownload(UserConnection* aSource, Download* d) {
	StringMap params;
	params["target"] = d->getTarget();
	params["user"] = aSource->getUser()->getNick();
	params["userip"] = aSource->getRemoteIp();
	params["hub"] = aSource->getUser()->getLastHubName();
	params["hubip"] = aSource->getUser()->getLastHubAddress();
	params["size"] = Util::toString(d->getSize());
	params["sizeshort"] = Util::formatBytes(d->getSize());
	params["chunksize"] = Util::toString(d->getTotal());
	params["chunksizeshort"] = Util::formatBytes(d->getTotal());
	params["actualsize"] = Util::toString(d->getActual());
	params["actualsizeshort"] = Util::formatBytes(d->getActual());
	params["speed"] = Util::formatBytes(d->getAverageSpeed()) + "/s";
	params["time"] = Util::formatSeconds((GET_TICK() - d->getStart()) / 1000);
	params["sfv"] = Util::toString(d->isSet(Download::FLAG_CRC32_OK) ? 1 : 0);
	TTHValue *hash = d->getTTH();
	if(hash != NULL) {
		params["tth"] = d->getTTH()->toBase32();
	}
	LOG(LogManager::DOWNLOAD, params);
}

void DownloadManager::moveFile(const string& source, const string& target) {
	try {
		File::ensureDirectory(target);
		if(File::getSize(source) > MOVER_LIMIT) {
			mover.moveFile(source, target);
		} else {
			File::renameFile(source, target);
		}
	} catch(const FileException&) {
		try {
			if(!SETTING(DOWNLOAD_DIRECTORY).empty()) {
				File::renameFile(source, SETTING(DOWNLOAD_DIRECTORY) + Util::getFileName(target));
			} else {
				File::renameFile(source, Util::getFilePath(source) + Util::getFileName(target));
			}
		} catch(const FileException&) {
			try {
				File::renameFile(source, Util::getFilePath(source) + Util::getFileName(target));
			} catch(const FileException&) {
				// Ignore...
			}
		}
	}

}

void DownloadManager::on(UserConnectionListener::MaxedOut, UserConnection* aSource) throw() { 
	noSlots(aSource);
}
void DownloadManager::noSlots(UserConnection* aSource) {
	if(aSource->getState() != UserConnection::STATE_FILELENGTH && aSource->getState() != UserConnection::STATE_TREE) {
		dcdebug("DM::onMaxedOut Bad state, ignoring\n");
		return;
	}

	Download* d = aSource->getDownload();
	dcassert(d != NULL);

	removeDownload(d);
	fire(DownloadManagerListener::Failed(), d, STRING(NO_SLOTS_AVAILABLE));

	aSource->setDownload(NULL);
	QueueManager::getInstance()->putDownload(d, false);
	removeConnection(aSource, false, !aSource->isSet(UserConnection::FLAG_NMDC));
}

void DownloadManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) throw() {
	Download* d = aSource->getDownload();

	if(d == NULL) {
		removeConnection(aSource);
		return;
	}
	
	removeDownload(d);
	fire(DownloadManagerListener::Failed(), d, aError);

	string target = d->getTarget();
	aSource->setDownload(NULL);
	QueueManager::getInstance()->putDownload(d, false);
	removeConnection(aSource);
}

void DownloadManager::removeDownload(Download* d) {
	if(d->getFile()) {
		if(d->getActual() > 0) {
			try {
				d->getFile()->flush();
			} catch(const Exception&) {
			}
		}
		delete d->getFile();
		d->setFile(NULL);
		d->setCrcCalc(NULL);

		if(d->isSet(Download::FLAG_ANTI_FRAG)) {
			// Ok, set the pos to whereever it was last writing and hope for the best...
			d->unsetFlag(Download::FLAG_ANTI_FRAG);
		} 
	}

	{
		Lock l(cs);
		// Either I'm stupid or the msvc7 optimizer is doing something _very_ strange here...
		// STL-port -D_STL_DEBUG complains that .begin() and .end() don't have the same owner (!),
		// but only in release build

		dcassert(find(downloads.begin(), downloads.end(), d) != downloads.end());

		//		downloads.erase(find(downloads.begin(), downloads.end(), d));
		
		for(Download::Iter i = downloads.begin(); i != downloads.end(); ++i) {
			if(*i == d) {
				downloads.erase(i);
				break;
			}
		}
	}
}

void DownloadManager::abortDownload(const string& aTarget) {
	Lock l(cs);
	for(Download::Iter i = downloads.begin(); i != downloads.end(); ++i) {
		Download* d = *i;
		if(d->getTarget() == aTarget) {
			dcassert(d->getUserConnection() != NULL);
			d->getUserConnection()->disconnect();
			break;
		}
	}
}

void DownloadManager::on(UserConnectionListener::FileNotAvailable, UserConnection* aSource) throw() {
	fileNotAvailable(aSource);
}

/** @todo Handle errors better */
void DownloadManager::on(AdcCommand::STA, UserConnection* aSource, const AdcCommand& cmd) throw() {
	if(cmd.getParameters().size() < 2) {
		aSource->disconnect();
		return;
	}

	const string& err = cmd.getParameters()[0];
	if(err.length() < 3) {
		aSource->disconnect();
		return;
	}

	switch(Util::toInt(err.substr(0, 1))) {
	case AdcCommand::SEV_FATAL:
		aSource->disconnect();
		return;
	case AdcCommand::SEV_RECOVERABLE:
		switch(Util::toInt(err.substr(1))) {
		case AdcCommand::ERROR_FILE_NOT_AVAILABLE:
			fileNotAvailable(aSource);
			return;
		case AdcCommand::ERROR_SLOTS_FULL:
			noSlots(aSource);
			return;
		}
	}
	aSource->disconnect();
}

void DownloadManager::fileNotAvailable(UserConnection* aSource) {
	Download* d = aSource->getDownload();
	dcassert(d != NULL);
	dcdebug("File Not Available: %s\n", d->getTarget().c_str());

	if(d->getFile()) {
		delete d->getFile();
		d->setFile(NULL);
		d->setCrcCalc(NULL);
	}

	removeDownload(d);
	fire(DownloadManagerListener::Failed(), d, d->getTargetFileName() + ": " + STRING(FILE_NOT_AVAILABLE));

	aSource->setDownload(NULL);

	QueueManager::getInstance()->removeSource(d->getTarget(), aSource->getUser(), d->isSet(Download::FLAG_TREE_DOWNLOAD) ? QueueItem::Source::FLAG_NO_TREE : QueueItem::Source::FLAG_FILE_NOT_AVAILABLE, false);

	QueueManager::getInstance()->putDownload(d, false);
	checkDownloads(aSource);
}


/**
 * @file
 * $Id: DownloadManager.cpp,v 1.142 2005/01/14 19:55:42 arnetheduck Exp $
 */
