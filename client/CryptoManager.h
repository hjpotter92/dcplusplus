/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#if !defined(AFX_CRYPTO_H__28F66860_0AD5_44AD_989C_BA4326C42F46__INCLUDED_)
#define AFX_CRYPTO_H__28F66860_0AD5_44AD_989C_BA4326C42F46__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SettingsManager.h"
#include "ResourceManager.h"

#include "Exception.h"
#include "Singleton.h"

#ifdef WIN32
#include "../zlib/zlib.h"
#else
#include <zlib.h>
#endif

STANDARD_EXCEPTION(CryptoException);

class Node : public FastAlloc<Node> {
public:
	// What's this? The only way (I've found out) to avoid a Internal Compiler Error! If this class is moved into
	// CryptoManager along with the greater specialization, it generates a ICE on the greater class. The typedefs
	// had to be removed in order to avoid template instatiation.
//	typedef Node* Ptr;
//	typedef list<Ptr> List;
//	typedef List::iterator Iter;
	int chr;
	int weight;
	
	Node* left;
	Node* right;
	
	Node(int aChr, int aWeight) : chr(aChr), weight(aWeight), left(NULL), right(NULL) { };
	Node(Node* aLeft, Node* aRight) :  chr(-1), weight(aLeft->weight + aRight->weight), left(aLeft), right(aRight) { };
	~Node() {
		delete left;
		delete right;
	}
	bool operator <(const Node& rhs) const {
		return weight<rhs.weight;
	}
	bool operator >(const Node& rhs) const {
		return weight>rhs.weight;
	}
	bool operator <=(const Node& rhs) const {
		return weight<=rhs.weight;
	}
	bool operator >=(const Node& rhs) const {
		return weight>rhs.weight;
	}
};

class File;

class ZDecompressor {
public:
	
	ZDecompressor() throw(CryptoException);
	~ZDecompressor() { 	
		dcdebug("Decompression ending, %ld/%ld = %.04f", zs.total_out, zs.total_in, (float)zs.total_out / max((float)zs.total_in, (float)1)); 
		inflateEnd(&zs); 
		delete[] outbuf;
	};

	/**
	 * To decompress some given bytes, keep calling this until inbytes
	 * reaches 0.
	 * @return The number of bytes available in the out buffer.
	 */
	u_int32_t decompress(const void* inbuf, int& inbytes) throw(CryptoException);
	GETSET(u_int8_t*, outbuf, Outbuf);
private:
	enum { OUTBUF_SIZE = 64*1024 };
	ZDecompressor(const ZDecompressor&);
	ZDecompressor& operator=(const ZDecompressor&);

	z_stream zs;
};

class FileException;

class ZCompressor : public FastAlloc<ZCompressor> {
public:
	/**
	 * @param maxBytes The maximum number of bytes to read from f, -1 = until EOF.
	 * @param strength Zip compression strength, more = better, slower (0 <= strength <= 9)
	 */
	ZCompressor(File& file, int64_t maxBytes = -1, int aStrength = SETTING(MAX_COMPRESSION)) throw(CryptoException);
	~ZCompressor() throw() {
		dcdebug("Compression ending, %ld/%ld = %.04f", zs.total_out, zs.total_in, (float)zs.total_out / max((float)zs.total_in, (float)1)); 
		deflateEnd(&zs);
		delete[] inbuf;
	};

	/**
	 * Compress data from the file until the buffer is full or no more data is
	 * available. Call this method until it returns 0 to get all bytes necessary
	 * for decompression.
	 * @return The final number of bytes used for the compression. When this equals 0,
	 * the compression is finished.
	 */
	u_int32_t compress(void* buf, u_int32_t bufLen, u_int32_t& bytesRead) throw(FileException, CryptoException);

private:

	enum { INBUF_SIZE = 64*1024 };

	z_stream zs;
	u_int8_t* inbuf;
	
	File& f;
	int64_t maxBytes;

	int level;

	ZCompressor(const ZCompressor&);
	ZCompressor& operator=(const ZCompressor&);

	void setStrength(int str) throw(CryptoException);
};

class CryptoManager : public Singleton<CryptoManager>
{
public:
	string makeKey(const string& aLock);
	const string& getLock() { return lock; };
	const string& getPk() { return pk; };
	bool isExtended(const string& aLock) { return strncmp(aLock.c_str(), "EXTENDEDPROTOCOL", 16) == 0; };

	void decodeHuffman(const u_int8_t* is, string& os) throw(CryptoException);
	void encodeHuffman(const string& is, string& os);
	void decodeBZ2(const u_int8_t* is, size_t sz, string& os) throw(CryptoException);
	void encodeBZ2(const string& is, string& os, int strength = 9);
	
private:

	friend class Singleton<CryptoManager>;
	
	CryptoManager() : lock("EXTENDEDPROTOCOLABCABCABCABCABCABC"), pk("DCPLUSPLUS" VERSIONSTRING "ABCABC") { };
	virtual ~CryptoManager() { };

	class Leaf : public FastAlloc<DecNode> {
	public:
		int chr;
		int len;
		Leaf(int aChr, int aLen) : chr(aChr), len(aLen) { };
		Leaf() : chr(-1), len(-1) { };
	};
	
	class DecNode : public FastAlloc<DecNode> {
	public:
		int chr;
		DecNode* left;
		DecNode* right;
		DecNode(int aChr) : chr(aChr), left(NULL), right(NULL) { };
		DecNode(DecNode* aLeft, DecNode* aRight) : chr(-1), left(aLeft), right(aRight) { };
		DecNode() : chr(-1), left(NULL), right(NULL) { };
		~DecNode() {
			delete left;
			delete right;
		}
	};
	

	const string lock;
	const string pk;

	int countChars(const string& aString, int* c, u_int8_t& csum);
	void walkTree(list<Node*>& aTree);
	void recurseLookup(vector<u_int8_t>* b, Node* node, vector<u_int8_t>& bytes);
	void buildLookup(vector<u_int8_t>* b, Node* root);
	
	string keySubst(const u_int8_t* aKey, int len, int n);
	bool isExtra(u_int8_t b) {
		return (b == 0 || b==5 || b==124 || b==96 || b==126 || b==36);
	}
	
	
};

#endif // !defined(AFX_CRYPTO_H__28F66860_0AD5_44AD_989C_BA4326C42F46__INCLUDED_)

/**
 * @file
 * $Id: CryptoManager.h,v 1.30 2004/01/04 16:34:37 arnetheduck Exp $
 */
