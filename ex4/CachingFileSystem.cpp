/*
* CachingFileSystem.cpp
*
*  Created on: 15 April 2015
*  Author: Netanel Zakay, HUJI, 67808  (Operating Systems 2014-2015).
*/

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include "CacheManager.h"
#include <unistd.h>
#include "Block.h"
using namespace std;
#define GETATTR "getattr"
#define ACCESS "access"
#define OPEN "open"
#define READ "read"
#define FGETATTR "fgetattr"
#define FLUSH "flush"
#define RELEASE "release"
#define OPENDIR "opendir"
#define READDIR "readdir"
#define RELEASEDIR "releasedir"
#define READ_ONLY 3
#define RENAME "rename"
#define INIT "init"
#define DESTROY "destroy"
#define FAIL 1
#define IOCTL "ioctl"
#define LOG_NAME ".filesystem.log"
#define MANAGER ((CacheManager *) fuse_get_context()->private_data)
#define TRY_TO_ACCESS_LOG_FILE if (endsWithLogString(path, LOG_NAME)){return -ENOENT; } 

struct fuse_operations caching_oper;

/**
* chech if a given string ends with a given ending string.
* Returns true if str ends with ending, false otherwise.
*/
bool endsWithLogString(std::string const &path, std::string const &ends) 
{
	if (path.length() >= ends.length())
	{
		return 0 == path.compare(path.length() - ends.length(), ends.length(), ends);
	}
	return false;
}

/**
* a function that converts relative path to a fullpath
*/
static void caching_fileFullPath(char fileFullPath[PATH_MAX], const char\
								 *path)
{
	strcpy(fileFullPath, MANAGER->getRootDir());
	strncat(fileFullPath, path, PATH_MAX); // if the path is long, it will break here
}

/** Get file attributes.
*
* Similar to stat().  The 'st_dev' and 'st_blksize' fields are
* ignored.  The 'st_ino' field is ignored except if the 'use_ino'
* mount option is given.
*/
int caching_getattr(const char *path, struct stat *statbuf)
{ 
	int restat = MANAGER->writeToLog(GETATTR);
	if(restat < SUCCESS)
	{
		return -errno;
	}
	TRY_TO_ACCESS_LOG_FILE;
	restat = SUCCESS;
	char fileFullPath[PATH_MAX]; 
	caching_fileFullPath(fileFullPath, path); // change path to full path
	restat = lstat(fileFullPath, statbuf); // return information about a file.
	if(restat != SUCCESS)
	{
		restat = -errno;
	}
	return restat;
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
	int restat = MANAGER->writeToLog(FGETATTR);
	if(restat < SUCCESS)
	{
		restat = -errno;
	}
	TRY_TO_ACCESS_LOG_FILE;
	restat = fstat(fi->fh, statbuf);
	if(restat < SUCCESS)
	{
		restat = -errno;
	}
	return restat;
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
	int restat  = MANAGER->writeToLog(ACCESS);
	if(restat < SUCCESS)
	{
		return -errno;
	}
	TRY_TO_ACCESS_LOG_FILE;
	char filePath[PATH_MAX]; 
	filePath[0] = '\0';
	caching_fileFullPath(filePath, path); // converts relative path to fullpath
	restat = access(filePath, mask);
	if(restat < SUCCESS)
	{
		restat = -errno;
	}
	return restat;
}


/** File open operation
*
* No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
* will be passed to open().  Open should check if the operation
* is permitted for the given flags.  Optionally open may also
* return an arbitrary filehandle in the fuse_file_info structure,
* which will be passed to all file operations.
* pay attention that the max allowed path is PATH_MAX (in limits.h).
* if the path is longer, return error.
* Changed in version 2.2
*/
int caching_open(const char *path, struct fuse_file_info *fi)
{
	int restat = MANAGER->writeToLog(OPEN);
	if(restat < SUCCESS)
	{
		return -errno;
	}
	TRY_TO_ACCESS_LOG_FILE;
	if ((fi->flags & READ_ONLY) != O_RDONLY)
	{
		return -EACCES;
	}
	restat = SUCCESS;
	int fd;
	char fpath[PATH_MAX];	
	fpath[0] = '\0';
	caching_fileFullPath(fpath, path);
	fd = open(fpath, fi->flags);
	if(fd < SUCCESS)
	{
		restat = -errno;
	}
	fi->fh = fd;
	return restat;

}

/**
* calculate the real size of the path 
*/
int getRealSizeOfFile(const char *path, struct fuse_file_info *fi) 
{
	struct stat statBuf;
	caching_fgetattr(path, &statBuf, fi);
	return statBuf.st_size;
}


/** Read data from an open file
*
* Read should return exactly the number of bytes requested except
* on EOF or error, otherwise the rest of the data will be
* substituted with zeroes. 
*
* Changed in version 2.2
*/
int caching_read(const char *path, char *buf, size_t size, off_t offset,
				 struct fuse_file_info *fi)
{

	int readResult = MANAGER->writeToLog(READ); // write to the log file
	if(readResult < SUCCESS)
	{
		return -errno;
	}
	TRY_TO_ACCESS_LOG_FILE;
	size = min(getRealSizeOfFile(path, fi) - (int) offset, (int) size);
	char fpath[PATH_MAX];
	caching_fileFullPath(fpath, path);
	if (access(fpath, F_OK)) 
	{
		return -ENOENT;
	}
	readResult = MANAGER->cacheRead(fi->fh, fpath, offset, size, buf, fi); 
	if(readResult < SUCCESS)
	{
		readResult = -errno;
	}

	return readResult;
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
	int restat = MANAGER->writeToLog(FLUSH);
	if(restat < SUCCESS)
	{
		return -errno;
	}
	TRY_TO_ACCESS_LOG_FILE;
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
int caching_release(const char *path, struct fuse_file_info *fi)
{
	int restat = MANAGER->writeToLog(RELEASE);
	if(restat < SUCCESS)
	{
		return -errno;
	}
	TRY_TO_ACCESS_LOG_FILE;
	return (close(fi->fh));

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
	int restat = MANAGER->writeToLog(OPENDIR);
	if(restat < SUCCESS)
	{
		return -errno;
	}
	TRY_TO_ACCESS_LOG_FILE;
	DIR *dp;
	restat = SUCCESS;
	char fpath[PATH_MAX];
	fpath[0] = '\0';
	caching_fileFullPath(fpath, path);
	dp = opendir(fpath);
	if (dp == NULL)
	{
		restat = -errno;
	}
	fi->fh = (intptr_t) dp;
	return restat;


}

/** Read directory
*
* This supersedes the old getdir() interface.  New applications
* should use this.
*
* The readdir implementation ignores the offset parameter, and
* passes zero to the filler function's offset.  The filler
* function will not return '1' (unless an error happens), so the
* whole directory is read in a single readdir operation.  This
* works just like the old getdir() method.
*
* Introduced in version 2.3
*/
int caching_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
					struct fuse_file_info *fi)
{

	int restat = MANAGER->writeToLog(READDIR);
	if(restat < SUCCESS)
	{
		return -errno;
	}
	TRY_TO_ACCESS_LOG_FILE;
	restat = 0;
	DIR *dp;
	struct dirent *de;
	// once again, no need for fullpath -- but note that I need to cast fi->fh
	dp = (DIR *) (uintptr_t) fi->fh;
	de = readdir(dp);

	if(de == SUCCESS)
	{
		return -errno;
	}
	do 
	{
		if (endsWithLogString(de->d_name, LOG_NAME))
		{
			continue;
		}
		if (filler(buf, de->d_name, NULL, 0) != 0)
		{
			return -ENOMEM;
		}
	}
	while ((de = readdir(dp)) != NULL);
	return restat;
}

/**
 * Release directory
 *
 * Introduced in velsrsion 2.3
 */
int caching_releasedir(const char *path, struct fuse_file_info *fi)
{
	int restat = MANAGER->writeToLog(RELEASEDIR);
	if(restat < SUCCESS)
	{
		return -errno;
	}
	TRY_TO_ACCESS_LOG_FILE;
	closedir((DIR *) (uintptr_t) fi->fh);
	return SUCCESS;
}

/**
* Rename a file 
*/
int caching_rename(const char *path, const char *newpath)
{
	int restat = MANAGER->writeToLog(RENAME);
	if(restat < SUCCESS)
	{
		return -errno;
	}
	TRY_TO_ACCESS_LOG_FILE;
	restat = 0;
	char fpath[PATH_MAX];
	char fnewpath[PATH_MAX];

	caching_fileFullPath(fpath, path);
	caching_fileFullPath(fnewpath, newpath);
	char* oldP = realpath(fpath, NULL);
	restat = rename(fpath, fnewpath);
	char* newP = realpath(fnewpath, NULL);
	if (restat < SUCCESS)
	{
		free(oldP);
		free(newP);
		return -errno;
	}
	MANAGER->rename(oldP , newP);
	free(oldP);
	free(newP);
	return restat;



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
	MANAGER->writeToLog(INIT);
	return MANAGER;
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
	MANAGER->writeToLog(DESTROY);
	free(MANAGER-> getRootDir()); 
	delete(MANAGER);	
}


/**
* Ioctl from the FUSE sepc:
* flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
* 64bit environment.  The size and direction of data is
* determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
* data will be NULL, for _IOC_WRITE data is out area, for
* _IOC_READ in area and if both are set in/out area.  In all
* non-NULL cases, the area is of _IOC_SIZE(cmd) bytes.
*
* However, in our case, this function only needs to print cache table to the log file .
* 
* Introduced in version 2.8
*/
int caching_ioctl (const char *, int cmd, void *arg,
				   struct fuse_file_info *, unsigned int flags, void *data)
{
	int restat = MANAGER->writeToLog(IOCTL);
	if(restat < SUCCESS)
	{
		return -errno;
	}
	string ioctldata = MANAGER->toString();
	return SUCCESS;
}


/**
* Initialise the operations. 
* You are not supposed to change this function.
*/
void init_caching_oper()
{

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
}


/**
* main function
*/
int main(int argc, char* argv[])
{
	if (argc < 5)
	{
		cout << "usage: CachingFileSystem rootdir mountdir numberOfBlocks blockSize\n" << endl;
		exit(FAIL);
	}
	// reseting caching operators
	init_caching_oper();
	//prepering the log file address
	if (atoi(argv[3]) <= 0 || atoi(argv[4]) <= 0)
	{
		cout << "usage: CachingFileSystem rootdir mountdir numberOfBlocks blockSize\n" << endl;
		exit(FAIL);
	}
	/**
	* stract stat
	*/
	struct stat s = {0};
	char* rootDir = realpath(argv[1], NULL);
	if (rootDir == NULL || stat(rootDir	, &s) || stat(argv[2], &s))
	{
		cout << "usage: CachingFileSystem rootdir mountdir numberOfBlocks blockSize\n" << endl;

		if(rootDir != NULL)
		{
			free(rootDir);
		}
		exit(FAIL);
	}
	CacheManager* manager = new CacheManager(rootDir, atoi(argv[3]), atoi(argv[4]));
	if(manager->initLog() < 0)
	{
		cerr<<"System Error: while openning log file"<<endl;
		exit(FAIL);
	}

	argv[1] = argv[2];
	for (int i = 2; i< (argc - 1); i++)
	{
		argv[i] = NULL;
	}
	argv[2] = (char*) "-s";
	argv[3] = (char*) "-f"; //TODO delete this before sub
	argc = 4;

	
	int fuse_stat = fuse_main(argc, argv, &caching_oper, manager);
	if (fuse_stat)
	{
		free(MANAGER-> getRootDir());
		delete(MANAGER);
		cerr<<"System Error Error: while initializing FUSE"<<endl;
		exit(FAIL);
	}
	return fuse_stat;
}




//g++ -Wall -g -std=c++11 -o papo Block.cpp CacheManager.cpp CachingFileSystem.cpp -lfuse -D_FILE_OFFSET_BITS=64 `pkg-config fuse --libs` `pkg-config fuse --cflags`

//fusermount -u /tmp/ff/md       //TODO

//valgrind --tool=memcheck --leak-check=full papo /tmp/rr/rd /tmp/rr/md 5 1024

//python -c "import os,fcntl; fd = os.open('/tmp/rr/md/.filesystem.log', os.O_RDONLY); fcntl.ioctl(fd, 0); os.close(fd)"
