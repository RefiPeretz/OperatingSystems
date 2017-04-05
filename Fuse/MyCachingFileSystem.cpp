/*
 * MyCachingFileSystem.cpp
 *
 *  Created on: 18 May 2014
 *      Author: Gil Tal ,Pavel Gak. HUJI, 67808 - Operating Systems 2013/4
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <iostream>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <dirent.h>
#include <stdio.h>
#include <queue>
#include <sys/time.h>
#include <algorithm>
#include <fstream>
#include <errno.h>
#include <math.h>

#define ERROR -1
#define SUCCESS 0
#define DATE_STRING_LENGTH 50

using namespace std;

// struct representing one cache block
struct cacheDataBlock {
	string fileName;
	int numberOfBlock;
	struct timeval lastAccess;
	string data;
	cacheDataBlock(string file, int blockNum, struct timeval lastAccess, char * givenData) :
			fileName(file), numberOfBlock(blockNum), lastAccess(lastAccess), data(givenData) {
	}
	cacheDataBlock(const cacheDataBlock &other) :
			numberOfBlock(other.numberOfBlock) {
		lastAccess.tv_sec = other.lastAccess.tv_sec;
		lastAccess.tv_usec = other.lastAccess.tv_usec;
		data = other.data;
		fileName = other.fileName;
	}

};

struct caching_state {
	// used to save the rootdir path
	char *rootdir = NULL;
	// used to indicate the size of each cache block in memory
	int blockSize;
	// max blocks in cache
	unsigned int numOfBlocks;
	// used to store the cache
	deque<cacheDataBlock> cache;
	// the log file
	ofstream logFile;
};

#define CACHING_DATA ((struct caching_state *) fuse_get_context()->private_data)

class Compareblocks {
public:
	bool operator()(cacheDataBlock& b1, cacheDataBlock& b2)
	// Returns true if t1 access earlier than t2
			{
		struct timeval* a = &b1.lastAccess;
		struct timeval* b = &b2.lastAccess;
		return timercmp(a,b,<);
	}
};
// fuse struct
struct fuse_operations caching_oper;

//ERROR TYPES for ex5
enum errorType {
	SYS_GEN_ERROR, // system error or other (general) error occured
	BAD_PARAMS, // got bad params (in main)
	WRITE_LOG, // cant write to log file
	OPEN_FILE, // cant open log file
	MAIN_ERROR // error allocating memory in main (used by fuse)
};
/**
 * used to handle the various errors in ex5 OS,
 * retVal - the value to be checked, should be 0 if no ERROR otherwise - error
 * error - the type of the error (represented in enum)
 * funcName - the function name (where error occured)
 * toExit - true iff in case of error needs to exit from the process.
 */
int handleError(int retVal, errorType error, std::string funcName = "unknown", bool toExit = false) {
	if (retVal != 0) {
		switch (error) {
		case SYS_GEN_ERROR:
			CACHING_DATA ->logFile << "error in " + funcName + "\n";
			break;
		case BAD_PARAMS:
			CACHING_DATA ->logFile
					<< "usage: MyCachingFileSystem rootdir mountdir numberOfBlocks blockSize\n";
			break;
		case OPEN_FILE:
			cerr << "system error: couldn't open ioctloutput.log file\n";
			break;
		case WRITE_LOG:
			cerr << "system error: couldn't write to ioctloutput.log file\n";
			break;
		case MAIN_ERROR:
			cerr << " main cant allocate memory\n";
			break;

		}
		if (toExit) {
			exit(ERROR);
		}
		return ERROR;

	} else {
		return SUCCESS;
	}
}
/**
 * converts relative path to a fullpath
 */
static void caching_fullpath(char fullPath[PATH_MAX], const char *path) {
	handleError((strlen(fullPath) + strlen(path) > PATH_MAX), SYS_GEN_ERROR, __func__);
	strcpy(fullPath, CACHING_DATA ->rootdir);
	strncat(fullPath, path, PATH_MAX); // long paths will break here
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int caching_getattr(const char *path, struct stat *statbuf) {
	int retstat;
	char fullPath[PATH_MAX];
	fullPath[0] = '\0';
	caching_fullpath(fullPath, path);
	retstat = lstat(fullPath, statbuf);
	if (retstat != SUCCESS) {
		//Has to return -errno in order to make "rename" work.
		//We weren't sure if we need to print this error. logically - should not be printed,
		//else, it will print error every time the program starts.
		return -errno;
	}
	return SUCCESS;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int caching_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi) {
	int retstat = fstat(fi->fh, statbuf);
	return handleError(retstat, SYS_GEN_ERROR, __func__);
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int caching_access(const char *path, int mask) {
	char fpath[PATH_MAX];
	fpath[0] = '\0';
	caching_fullpath(fpath, path);
	int retstat = access(fpath, mask);
	return handleError(retstat, SYS_GEN_ERROR, __func__);
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int caching_open(const char *path, struct fuse_file_info *fi) {
	int retstat = 0;
	int fd;
	char fpath[PATH_MAX];
	fpath[0] = '\0';
	caching_fullpath(fpath, path);
	fd = open(fpath, fi->flags);
	if (fd < 0)
		retstat = handleError(fd, SYS_GEN_ERROR, __func__);
	fi->fh = fd;
	return retstat;
}
/**
 * returns true iff the path and the needed part is in the cache, if so, reutrns the
 * data in buf
 */
bool checkIfInCache(const char *path, int part, char *buf) {
	for (deque<cacheDataBlock>::iterator it = CACHING_DATA ->cache.begin(); it != CACHING_DATA ->cache.end();
			++it) {

		if ((it->fileName.compare(path) == 0) && (it->numberOfBlock == part)) {
			if (buf != NULL) {
				handleError(gettimeofday(&it->lastAccess, NULL), SYS_GEN_ERROR, __func__);
				memcpy(buf, it->data.c_str(), it->data.length());
			} else {
				handleError(ERROR, SYS_GEN_ERROR, __func__);
			}
			return true;
		}
	}
	return false;

}
/**
 * renames the path to newName in the cache
 */
int renameCache(const char *path, const char *newName) {
	for (deque<cacheDataBlock>::iterator it = CACHING_DATA ->cache.begin(); it != CACHING_DATA ->cache.end();
			++it) {
		if ((it->fileName.compare(path) == 0)) {
			it->fileName = newName;
		}
	}
	return SUCCESS;

}
/**
 * reads the file stored in fi->fh from disk, in offset and returns char*
 * of size CACHING_DATA->blockSize of less in buffer.
 */
int readBlockFromDisk(off_t offset, struct fuse_file_info *fi, char * buffer) {
	int retstat = pread(fi->fh, buffer, CACHING_DATA ->blockSize, offset);
	if (retstat < 0) {
		handleError(ERROR, SYS_GEN_ERROR, __func__);
		return ERROR;
	} else {
		buffer[retstat] = '\0';
		return SUCCESS;
	}
}
/**
 * adds new block to cache
 * path - the fileName to be storred
 * blockNum - the number of the block in the file
 * buffer - the data of the block
 */
int updateCache(const char * path, char* buffer, int blockNum) {
	int retVal = SUCCESS;
	timeval tv;
	retVal += handleError(gettimeofday(&tv, NULL), SYS_GEN_ERROR, __func__);
	cacheDataBlock toCache = cacheDataBlock(path, blockNum, tv, buffer);
	int diff = CACHING_DATA ->cache.size() - CACHING_DATA ->numOfBlocks;
	if (diff >= 0) {
		CACHING_DATA ->cache.pop_back();
	}
	CACHING_DATA ->cache.push_front(toCache);
	make_heap(CACHING_DATA ->cache.begin(), CACHING_DATA ->cache.end(), Compareblocks());
	return retVal;
}
/**
 * returns the relevant part of data block
 * offset - the offset specidied in caching_read
 * size - the size to be read from the file
 * current - the number of current block
 * first - the number of first block
 * last - the number of last block
 * block - the input block
 * buf - the output buffer to be returned to user
 */
int getRelevantPartOfBlock(off_t offset, size_t size, int current, int first, int last, char * block,
		char * buf) {

	int blocksRead = CACHING_DATA ->blockSize;
	if (current == first) {
		block += (offset % CACHING_DATA ->blockSize);
		blocksRead -= offset % CACHING_DATA ->blockSize;
	}
	if (current == last) {
		blocksRead = size;
	}
	memcpy(buf + ((current - first) * CACHING_DATA ->blockSize), block, blocksRead);
	return blocksRead;

}
/**
 * returns the real size of file.
 * gets path of file and it fuse_file_info struct
 */
int getRealSizeOfFile(const char *path, struct fuse_file_info *fi) {
	struct stat statBuf;
	caching_fgetattr(path, &statBuf, fi);
	return statBuf.st_size;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int caching_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	// init the bytes read.
	int bytesRead = 0;
	// get the real size of file
	int fileSize = getRealSizeOfFile(path, fi);
	// set the size to be the min
	size = min(fileSize - (int) offset, (int) size);
	// check which is the maxBlockNumber for the file
	int maxBlockNumber = ceil(double(fileSize) / double(CACHING_DATA ->blockSize));
	int firstBlockNeeded = (offset / CACHING_DATA ->blockSize);
	// set the last block needed to not bigger the file size
	int lastBlockNeeded = min(maxBlockNumber,
			firstBlockNeeded + int(ceil(double(size) / double(CACHING_DATA ->blockSize))));
	// since blocks start from 1
	firstBlockNeeded++;
	// to remove the / from the path
	path = path + 1;
	// for each needed block - do iteration
	for (int currentBlock = firstBlockNeeded; currentBlock <= lastBlockNeeded; currentBlock++) {
		// create new dataBlock
		char * dataBlock = new char[CACHING_DATA ->blockSize + 1];
		dataBlock[0] = '\0';
		// check if block in cache:
		if (!checkIfInCache(path, currentBlock, dataBlock)) {
			// if not in cache - get block from disk, and get needed part
			handleError(readBlockFromDisk(CACHING_DATA ->blockSize * (currentBlock - 1), fi, dataBlock),
					SYS_GEN_ERROR, __func__);
			bytesRead += getRelevantPartOfBlock(offset, size - bytesRead, currentBlock, firstBlockNeeded,
					lastBlockNeeded, dataBlock, buf);
			handleError(updateCache(path, dataBlock, currentBlock), SYS_GEN_ERROR, __func__);
		} else {
			// if in cache get the relevant part
			bytesRead += getRelevantPartOfBlock(offset, size - bytesRead, currentBlock, firstBlockNeeded,
					lastBlockNeeded, dataBlock, buf);
		}
		// delete the created data block
		delete[] dataBlock;
	}
	// put terminator at the end of data
	buf[bytesRead] = '\0';
	return bytesRead;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int caching_flush(const char *path, struct fuse_file_info *fi) {
	return SUCCESS;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int caching_release(const char *path, struct fuse_file_info *fi) {
	return handleError(close(fi->fh), SYS_GEN_ERROR, __func__);
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int caching_opendir(const char *path, struct fuse_file_info *fi) {
	DIR *dp;
	int retstat = 0;
	char fpath[PATH_MAX];
	fpath[0] = '\0';
	caching_fullpath(fpath, path);
	dp = opendir(fpath);
	if (dp == NULL)
		retstat = handleError(ERROR, SYS_GEN_ERROR, __func__);
	fi->fh = (intptr_t) dp;
	return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int caching_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi) {
	int retstat = 0;
	DIR *dp;
	struct dirent *de;

	// once again, no need for fullpath -- but note that I need to cast fi->fh
	dp = (DIR *) (uintptr_t) fi->fh;

	// Every directory contains at least two entries: . and ..  If my
	// first call to the system readdir() returns NULL I've got an
	// error; near as I can tell, that's the only condition under
	// which I can get an error from readdir()
	de = readdir(dp);
	if (de == 0) {
		handleError(ERROR, SYS_GEN_ERROR, __func__);
		return ERROR;
	}

	// This will copy the entire directory into the buffer.  The loop exits
	// when either the system readdir() returns NULL, or filler()
	// returns something non-zero.  The first case just means I've
	// read the whole directory; the second means the buffer is full.
	do {
		if (filler(buf, de->d_name, NULL, 0) != 0) {
			return handleError(-1, SYS_GEN_ERROR);
		}
	} while ((de = readdir(dp)) != NULL);

	return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int caching_releasedir(const char *path, struct fuse_file_info *fi) {
	int retstat = 0;
	closedir((DIR *) (uintptr_t) fi->fh);
	return retstat;
}

/** Rename a file */
int caching_rename(const char *path, const char *newpath) {
	int retstat = 0;
	char fullPath[PATH_MAX];
	char fullNewPath[PATH_MAX];
	fullPath[0] = '\0';
	fullNewPath[0] = '\0';
	// create the full path needed
	caching_fullpath(fullPath, path);
	caching_fullpath(fullNewPath, newpath);

	retstat += rename(fullPath, fullNewPath);
	// remove the first / from path
	path++;
	newpath++;
	retstat += renameCache(path, newpath);

	return handleError(retstat, SYS_GEN_ERROR, __func__);

}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *caching_init(struct fuse_conn_info *conn) {
	return CACHING_DATA ;
}

/**
 * Clean up filesystem
 * Called on filesystem exit.
 * Introduced in version 2.3
 */
void caching_destroy(void *userdata) {
	CACHING_DATA ->logFile.close();
	free(CACHING_DATA ->rootdir);
	delete (CACHING_DATA );
	return;
}

/**
 * receives timval struct and returns the needed char array (date) according to format
 * specified in OS ex5.
 */
void formatDate(struct timeval* tv, char * date) {

	time_t nowtime;
	struct tm *nowtm;
	char tmbuf[64];
	nowtime = tv->tv_sec;
	nowtm = localtime(&nowtime);
	handleError(!strftime(tmbuf, sizeof tmbuf, "%m:%d:%H:%M:%S:", nowtm), SYS_GEN_ERROR, __func__);
	strcpy(date, tmbuf);
	char milli[4];
	if (snprintf(milli, sizeof(milli), "%ld", tv->tv_usec) < 0) {
		handleError(ERROR, SYS_GEN_ERROR, __func__);
	}
	strcat(date, milli);

}

/**
 * Ioctl
 *
 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
 * 64bit environment.  The size and direction of data is
 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
 * data will be NULL, for _IOC_WRITE data is out area, for
 * _IOC_READ in area and if both are set in/out area.  In all
 * non-NULL cases, the area is of _IOC_SIZE(cmd) bytes.
 *
 * Introduced in version 2.8
 */
int caching_ioctl(const char *, int cmd, void *arg, struct fuse_file_info *, unsigned int flags, void *data) {
	// create date char array
	char date[DATE_STRING_LENGTH];
	struct timeval now;
	handleError(gettimeofday(&now, NULL), SYS_GEN_ERROR, __func__);
	formatDate(&now, date);
	handleError(!(CACHING_DATA ->logFile << date << "\n"), WRITE_LOG);
	for (deque<cacheDataBlock>::iterator it = CACHING_DATA ->cache.begin(); it != CACHING_DATA ->cache.end();
			++it) {
		handleError(!(CACHING_DATA ->logFile << it->fileName << "\t" << it->numberOfBlock << "\t"),
				WRITE_LOG);
		formatDate(&it->lastAccess, date);
		handleError(!(CACHING_DATA ->logFile << date << "\n"), WRITE_LOG);

	}

	return SUCCESS;
}

int main(int argc, char* argv[]) {
	// init of fuse struct and its components
	struct caching_state *caching_data;
	caching_data = new struct caching_state;
	if (caching_data == NULL) {
		// this one is custom error msg, since we could not find any other ex error msg for this error.
		handleError(ERROR, MAIN_ERROR, __func__, true);
	}
	caching_data->logFile.open("ioctloutput.log", ios::app);
	handleError(!caching_data->logFile.is_open(), OPEN_FILE);

	cout << "in main: " << "num of blocks: " << argv[3] << " block size: :" << argv[4] << endl;
	caching_data->numOfBlocks = atoi(argv[3]);
	caching_data->blockSize = atoi(argv[4]);
	caching_data->rootdir = realpath(argv[1], NULL);
	// check if all params are ok
	if (caching_data->blockSize <= 0|| caching_data->numOfBlocks <= 0
	|| caching_data->rootdir == NULL || realpath(argv[2], NULL) == NULL) {handleError(ERROR, BAD_PARAMS, __func__,true);
	}
	// init the initial (usually empty!) cache
	make_heap(caching_data->cache.begin(), caching_data->cache.end(), Compareblocks());

	// Initialise the operations
	caching_oper.getattr = caching_getattr;
	caching_oper.access = caching_access;
	caching_oper.open = caching_open;
	caching_oper.read = caching_read;
	caching_oper.flush = caching_flush;
	caching_oper.release = caching_release;
	caching_oper.opendir = caching_opendir;
	caching_oper.readdir = caching_readdir;
	caching_oper.releasedir = caching_releasedir;
	caching_oper.rename = caching_rename;
	caching_oper.init = caching_init;
	caching_oper.destroy = caching_destroy;
	caching_oper.ioctl = caching_ioctl;
	caching_oper.fgetattr = caching_fgetattr;

	caching_oper.readlink = NULL;
	caching_oper.getdir = NULL;
	caching_oper.mknod = NULL;
	caching_oper.mkdir = NULL;
	caching_oper.unlink = NULL;
	caching_oper.rmdir = NULL;
	caching_oper.symlink = NULL;
	caching_oper.link = NULL;
	caching_oper.chmod = NULL;
	caching_oper.chown = NULL;
	caching_oper.truncate = NULL;
	caching_oper.utime = NULL;
	caching_oper.write = NULL;
	caching_oper.statfs = NULL;
	caching_oper.fsync = NULL;
	caching_oper.setxattr = NULL;
	caching_oper.getxattr = NULL;
	caching_oper.listxattr = NULL;
	caching_oper.removexattr = NULL;
	caching_oper.fsyncdir = NULL;
	caching_oper.create = NULL;
	caching_oper.ftruncate = NULL;
	argv[1] = argv[2];
	for (int i = 2; i <= (argc - 1); i++) {
		argv[i] = NULL;
	}
	// init the needed flags
	argc = 2;
	int fuse_stat = fuse_main(argc, argv, &caching_oper, caching_data);
	return fuse_stat;
}
