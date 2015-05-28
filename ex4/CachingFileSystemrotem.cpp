/*
 * CachingFileSystem.cpp
 *
 *  Created on: 15 April 2015
 *  Author: Netanel Zakay, HUJI, 67808  (Operating Systems 2014-2015).
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include "CacheManager.h"
#include <fstream>
#include <limits.h> // for PATH_MAX
#include <unistd.h>
#include "Block.h"

using namespace std;

static CacheManager* manager;

struct fuse_operations caching_oper;



//---------------------------------------------------
/**
* a function that converts relative path to a fullpath
*/
static void caching_fileFullPath(char fileFullPath[PATH_MAX], const char *path) 
{
	//TODO - error if path is too big
	strcpy(fileFullPath, MANAGER->getRootDir()); // using static 
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
	// TODO  call log file. include error
	char fileFullPath[PATH_MAX]; 
	fileFullPath[0] = '\0'; // initial full path
	caching_fileFullPath(fileFullPath,path); // change path to full path
	int retstat = lstat(fileFullPath,statbuf); // return information about a file.
	if(retstat != 0)
	{
		//TODO - ERROR
	}
	return 0;
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
    // TODO  call log, including error
	int restat = fstat(fi->fh, statbuf);

	// TODO special case of a "/"??
	
	if(restat != 0)
	{
		// TODO - error
	}
    return 0;
}

/**
* Check file access permissions
*/
int caching_access(const char *path, int mask)
{
	char filePath[PATH_MAX]; 
	filePath[0] = '\0';
	caching_fileFullPath(filePath,path); // converts relative path to fullpath
	int restat = access(filePath, mask);
	//TODO return error if needed
	return 0;
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
int caching_open(const char *path, struct fuse_file_info *fi){
	return 0;
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
		struct fuse_file_info *fi){
	return 0;
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
	//TODO - error
	return 0;
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int caching_opendir(const char *path, struct fuse_file_info *fi){
	return 0;
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
		struct fuse_file_info *fi){
	return 0;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int caching_releasedir(const char *path, struct fuse_file_info *fi){
	return 0;
}
//----------------------------------------------------------------------

/**
* update the path of the new name in the cache
* go true the block list in the cache and change to the new name
*/
int renameInCache(const char *path, const *newName)
{
	for (std::list<Block*>::iterator it = MANAGER ->blockList.begin(); it != MANAGER ->blockList.end();
			++it) 
	{
		if ((strcmp(it->getBlockFileName(),path) == 0)) 
		{
			it->setBlockFileName(newName);
		}
	}
	return 0;
}
//-----------------------------------------------------------------------
/**
* Rename a file 
*/
int caching_rename(const char *path, const char *newpath)
{
	// TODO - write to log + error

	// TODO - error if path does not exist in list 
	char oldFullPath[PATH_MAX];
	char newFullPath[PATH_MAX];
	oldFullPath[0] = '\0';
	newFullPath = '\0';
	caching_fileFullPath(oldFullPath, path);
	caching_fileFullPath(newFullPath, newpath);
	int retstat = rename(oldFullPath, newFullPath);
	// skiping the first '/' from the path
	path++;
	newpath++;
	// change to the new name in the cache
	retstat = retstat + renameInCache(path, newpath);
	// TODO check error for restat
	return 0;
}
//----------------------------------------------------------------------
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
void *caching_init(struct fuse_conn_info *conn){
	return MANAGER;
}

//-----------------------------------------------------------------------
/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void caching_destroy(void *userdata)
{
	// TODO - close log file?
	free(MANAGER-> getRootDir());
	Block corBlock;
	// clearing list of blocks
	while(!MANAGER->blockList().empty())
	{
		corBlock = MANAGER->blockList().front();
		MANAGER->blockList().pop_front();
		delete(corBlock);
	}
	delete(MANAGER->blockList());
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
		struct fuse_file_info *, unsigned int flags, void *data){
	return 0;
}


// Initialise the operations. 
// You are not supposed to change this function.
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

//basic main. You need to complete it.
int main(int argc, char* argv[]){
	// reseting caching operators
	init_caching_oper();
	//prepering the log file address
	std::fstream filesystem;
	std::string logFile = "/.filesystem.log";
	std::string stringArgv(argv[1]);
	std::string newLogName = stringArgv+logFile;
	//TODO check if open didnt gave an error
	filesystem.open(newLogName,std::fstream::app);

	manager = new CacheManager(realpath(argv[1], NULL),realpath(argv[2], NULL), atoi(argv[3]), atoi(argv[4]));

	argv[1] = argv[2];

	cout<<"I am here"<<endl;
	getchar();
	for (int i = 2; i< (argc - 1); i++){
		argv[i] = NULL;
	}
        argv[2] = (char*) "-s";
	argc = 3;

	filesystem.close(); // TODO - check if need to be close here
	int fuse_stat = fuse_main(argc, argv, &caching_oper, NULL);
	return fuse_stat;
}
