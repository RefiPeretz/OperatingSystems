/*
 * myCachingFileSystem.cpp
 *
 *  Created on: 18 May 2014
 *      Author: Noga H. Rotman, HUJI, 67808 - Operating Systems 2013/4
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include "MyLog.hpp"
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include "BlockAndCache.hpp"
#include <iostream>
#include <errno.h>
#define ARGS_NUM 5
#define ROOT_INDEX 1
#define MOUNT_INDEX 2
#define SUCCESS 0
#define FAILURE -1

using namespace std;

typedef struct GlobalData
{
	Cache* myCache;
	int numberOfBlocks;
	int sizeOfBlock;
	char *rootdir;
	FILE* ioctlFile;

} GlobalData;
#define GDATA ((GlobalData *) fuse_get_context()->private_data)
struct fuse_operations caching_oper;

void pathCopy(char fpath[], const char * path)
{
	strcpy(fpath, GDATA ->rootdir);
	strncat(fpath, path, PATH_MAX);

}
/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int caching_getattr(const char *path, struct stat *statbuf)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	pathCopy(fpath, path);
	retstat = lstat(fpath, statbuf);
	if (retstat != 0)
	{
		int res = fputs("error in caching_getattr\n", GDATA ->ioctlFile);
		{
			if (res <= 0)
			{
				cerr << "system error: couldn't write to ioctloutput.log file\n" << endl;
			}
		}
		fflush(GDATA ->ioctlFile);
		return -errno;
	}
	return retstat;
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
int caching_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{

	int retstat = 0;
	retstat = fstat(fi->fh, statbuf);
	if (retstat < 0)
	{
		int res = fputs("error in caching_fgetattr\n", GDATA ->ioctlFile);
		if (res <= 0)
		{
			cerr << "system error: couldn't write to ioctloutput.log file\n" << endl;
		}
		fflush(GDATA ->ioctlFile);
	}
	return retstat;
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
int caching_access(const char *path, int mask)
{
	int retstat = 0;
	char fpath[PATH_MAX];

	pathCopy(fpath, path);

	retstat = access(fpath, mask);

	if (retstat < 0)
	{
		int res = fputs("error in caching_access\n", GDATA ->ioctlFile);
		if (res <= 0)
		{
			cerr << "system error: couldn't write to ioctloutput.log file\n" << endl;
		}
		fflush(GDATA ->ioctlFile);
		return -errno;
	}
	return retstat;
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
int caching_open(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	int fd;
	char fpath[PATH_MAX];
	pathCopy(fpath, path);
	fd = open(fpath, fi->flags);
	if (fd < 0)
	{
		int res = fputs("error in caching_open\n", GDATA ->ioctlFile);
		if (res <= 0)
		{
			cerr << "system error: couldn't write to ioctloutput.log file\n" << endl;
		}
		fflush(GDATA ->ioctlFile);
		return -errno;
	}
	fi->fh = fd;
	return retstat;
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
int caching_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int ret = GDATA ->myCache->readToCache(fi->fh, path, offset, size);
	if (ret < 0)
	{
		int res = fputs("error in caching_read\n", GDATA ->ioctlFile);
		if (res <= 0)
		{
			cerr << "system error: couldn't write to ioctloutput.log file\n" << endl;
		}
		fflush(GDATA ->ioctlFile);
		return -errno;
	}
	int read = GDATA ->myCache->writeToBuffer(size, offset, buf, path);
	return read;

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
int caching_flush(const char *path, struct fuse_file_info *fi)
{
	return 0;
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
int caching_release(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
// We need to close the file.  Had we allocated any resources
// (buffers etc) we'd need to free them here as well.
	retstat = close(fi->fh);
	if (retstat < 0)
	{
		int res = fputs("error in caching_release\n", GDATA ->ioctlFile);
		if (res <= 0)
		{
			cerr << "system error: couldn't write to ioctloutput.log file\n" << endl;
		}
		fflush(GDATA ->ioctlFile);
		return -errno;

	}
	return retstat;
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int caching_opendir(const char *path, struct fuse_file_info *fi)
{
	DIR *dp;
	int retstat = 0;
	char fpath[PATH_MAX];
	pathCopy(fpath, path);
	dp = opendir(fpath);
	if (dp == NULL)
	{
		int res = fputs("error in caching_opendir\n", GDATA ->ioctlFile);

		if (res <= 0)
		{
			cerr << "system error: couldn't write to ioctloutput.log file\n" << endl;
		}
		fflush(GDATA ->ioctlFile);
		return -errno;
	}
	fi->fh = (intptr_t)dp;

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
					struct fuse_file_info *fi)
{
	int retstat = 0;
	DIR *dp;
	struct dirent *de;
// once again, no need for fullpath -- but note that I need to cast fi->fh
	dp = (DIR *)(uintptr_t)fi->fh;

	de = readdir(dp);
	if (de == 0)
	{
		int res = fputs("error in caching_readdir\n", GDATA ->ioctlFile);
		if (res <= 0)
		{
			cerr << "system error: couldn't write to ioctloutput.log file\n" << endl;
		}
		fflush(GDATA ->ioctlFile);
		return -errno;
	}
	do
	{
		if (filler(buf, de->d_name, NULL, 0) != 0)
		{
			int res = fputs("error in caching_readdir\n", GDATA ->ioctlFile);
			if (res <= 0)
			{
				cerr << "system error: couldn't write to ioctloutput.log file\n" << endl;
			}
			fflush(GDATA ->ioctlFile);
			return -errno;
		}

	} while ((de = readdir(dp)) != NULL);

	return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int caching_releasedir(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	retstat = closedir((DIR *)(uintptr_t)fi->fh);
	if (retstat < 0)
	{
		int res = fputs("error in caching_releasedir\n", GDATA ->ioctlFile);
		if (res <= 0)
		{
			cerr << "system error: couldn't write to ioctloutput.log file\n" << endl;
		}
		fflush(GDATA ->ioctlFile);
		return -errno;

	}
	return retstat;
}

/** Rename a file */
int caching_rename(const char *path, const char *newpath)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	char fnewpath[PATH_MAX];
	pathCopy(fpath, path);
	pathCopy(fnewpath, newpath);
	retstat = rename(fpath, fnewpath);
	if (retstat < 0)
	{
		int res = fputs("error in caching_rename\n", GDATA ->ioctlFile);
		if (res <= 0)
		{
			cerr << "system error: couldn't write to ioctloutput.log file\n" << endl;
		}
		return -errno;
	}
	GDATA ->myCache->rename(path, newpath);
	return retstat;
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
void *caching_init(struct fuse_conn_info *conn)
{
	GDATA ->myCache = new Cache(GDATA ->numberOfBlocks, GDATA ->sizeOfBlock);
	return GDATA ;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void caching_destroy(void *userdata)
{
	delete (GDATA ->myCache);
	fclose(GDATA ->ioctlFile);
	free(GDATA ->rootdir);
	free(GDATA );
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
int caching_ioctl(const char *, int cmd, void *arg, struct fuse_file_info *, unsigned int flags,
				  void *data)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	struct tm* nowtime = NULL;
	nowtime = localtime(&tv.tv_sec);
	if (nowtime == NULL) {
		return -1;
	}
	std::stringstream curTime;
	curTime << (nowtime->tm_mon + 1) << ":";
	curTime << (nowtime->tm_mday) << ":";
	curTime << (nowtime->tm_hour) << ":";
	curTime << (nowtime->tm_min) << ":";
	curTime << nowtime->tm_sec << ":";
	curTime << (tv.tv_usec / 1000) << "\n";
	if (GDATA ->ioctlFile == NULL)
	{
		std::cerr << "ioctl null" << std::endl;
	}
	fputs(curTime.str().c_str(), GDATA ->ioctlFile);
	fflush(GDATA ->ioctlFile);
	std::string yadid = GDATA->myCache->toString();
	int ret = fputs((GDATA->myCache->toString()).c_str(), GDATA ->ioctlFile);
	fflush(GDATA ->ioctlFile);
	if (ret <= 0)
	{
		cerr << "system error: couldn't write to ioctloutput.log file\n" << endl;
	}
	return 0;
}

int main(int argc, char* argv[])
{
	GlobalData* gdata = (GlobalData*)malloc(sizeof(GlobalData));
	gdata->rootdir = realpath(argv[1], NULL);
	gdata->numberOfBlocks = atoi(argv[3]);
	gdata->sizeOfBlock = atoi(argv[4]);
	bool isDir = true;
	//if the dirs don't exist
	for (int i = ROOT_INDEX; i <= MOUNT_INDEX; i++)
	{
		if (SUCCESS != access(argv[i], F_OK))
		{
			if (ENOENT == errno || ENOTDIR == errno)
			{
				isDir = false;
			}
		}
	}
	//if invalid arguments
	if (argc != ARGS_NUM || gdata->numberOfBlocks <= 0 || gdata->sizeOfBlock <= 0 || !isDir)
	{
		cout << "usage:MyCachingFileSystem rootdir mountdir numberOfBlocks blockSize\n" << endl;
		exit(FAILURE);
	}
	gdata->ioctlFile = fopen("ioctloutput.log", "a+");
	if (gdata->ioctlFile == NULL)
	{
		cerr << "system error:couldn't ioctloutput.log file\n" << endl;
		exit(FAILURE);
	}
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
	for (int i = 2; i < (argc - 1); i++)
	{
		argv[i] = NULL;
	}
	argc = 3;
	argv[2] = (char*)"-s";
	int fuse_stat = fuse_main(argc, argv, &caching_oper, gdata);
	return fuse_stat;
}
