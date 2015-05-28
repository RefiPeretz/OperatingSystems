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
#define RENAME "rename"
#define INIT "init"
#define DESTROY "destroy"
#define IOCTL "ioctl"

#define MANAGER ((CacheManager *) fuse_get_context()->private_data)

struct fuse_operations caching_oper;



// enum errorType {
// 	SYS_GEN_ERROR, // system error or other (general) error occured
// 	BAD_PARAMS, // got bad params (in main)
// 	WRITE_LOG, // cant write to log file
// 	OPEN_FILE, // cant open log file
// 	MAIN_ERROR // error allocating memory in main (used by fuse)
// };


// int handleError(int retVal, errorType error, std::string funcName = "unknown", bool toExit = false) {
// 	if (retVal != 0) {
// 		switch (error) {
// 		case SYS_GEN_ERROR:
// 			CACHING_DATA ->logFile << "error in " + funcName + "\n";
// 			break;
// 		case BAD_PARAMS:
// 			CACHING_DATA ->logFile
// 					<< "usage: MyCachingFileSystem rootdir mountdir numberOfBlocks blockSize\n";
// 			break;
// 		case OPEN_FILE:
// 			cerr << "system error: couldn't open ioctloutput.log file\n";
// 			break;
// 		case WRITE_LOG:
// 			cerr << "system error: couldn't write to ioctloutput.log file\n";
// 			break;
// 		case MAIN_ERROR:
// 			cerr << " main cant allocate memory\n";
// 			break;

// 		}
// 		// if (toExit) {
// 		// 	exit(ERROR);//TODO
// 		// }
// 		return ERROR;

// 	} else {
// 		return SUCCESS;
// 	}
// }

/**
* a function that converts relative path to a fullpath
*/
static void caching_fileFullPath(char fileFullPath[PATH_MAX], const char *path) 
{
	//TODO - error if path is too big // more then path_max?
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
	int retstat = 0;

	cout<<"getattr"<<endl;
	MANAGER->writeToLog(GETATTR); // TODO - check error
	char fileFullPath[PATH_MAX]; 
	fileFullPath[0] = '\0'; // initial full path
	caching_fileFullPath(fileFullPath, path); // change path to full path
	retstat = lstat(fileFullPath, statbuf); // return information about a file.
	if(retstat != 0)
	{
		cout<<-errno<<path<<endl;
		retstat = -errno;
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
	cout<<"fgetattr"<<endl;
    // TODO  call log, including error
    MANAGER->writeToLog(FGETATTR);
	int restat = fstat(fi->fh, statbuf);

	// TODO special case of a "/"??
	if(restat < 0)
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
	int restat = 0;
	cout<<"access"<<endl;
	MANAGER->writeToLog(ACCESS);
	char filePath[PATH_MAX]; 
	filePath[0] = '\0';
	caching_fileFullPath(filePath, path); // converts relative path to fullpath
	restat = access(filePath, mask);
	//TODO return error if needed
	if(restat < 0)
	{
		cout<<"error"<<endl;
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
	cout<<"open"<<endl;
	MANAGER->writeToLog(OPEN);
	int restat = 0;
	int fd;
	char fpath[PATH_MAX];
	
	fpath[0] = '\0';

	caching_fileFullPath(fpath, path);

	fd = open(fpath, fi->flags);


	if(fd < 0 )
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
	cout<<"read"<<endl;//TODO trying access the log return -ENOENT


	MANAGER->writeToLog(READ); // write to the log file

	size = min(getRealSizeOfFile(path, fi) - (int) offset, (int) size);

	char fpath[PATH_MAX];
	//fpath[0] = '\0';
	caching_fileFullPath(fpath, path);
	int readResult = MANAGER->cacheRead(fi->fh, fpath, offset, size, buf); 
	if(readResult < 0)
	{
		readResult = -errno;
	}
	//int bufferWriteResult = MANAGER->bufferWrite(size, offset, path, buf); // TODO - has error?
	cout<<"return value is: "<<readResult<<endl;
	return readResult;//bufferWriteResult;
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
	MANAGER->writeToLog(FLUSH);
	cout<<"flush"<<endl;
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
	cout<<"release"<<endl;
	MANAGER->writeToLog(RELEASE);
	return (close(fi->fh));

}

/** Open directory
*
* This method should check if the open operation is permitted for
* this  directory
*
* Introduced in version 2.3
*/
int caching_opendir(const char *path, struct fuse_file_info *fi){

	cout<<"opendir"<<endl;
	MANAGER->writeToLog(OPENDIR);
	DIR *dp;
	int retstat = 0;

	char fpath[PATH_MAX];

	fpath[0] = '\0';

	caching_fileFullPath(fpath, path);

	dp = opendir(fpath);

	if (dp == NULL)
	{
		retstat = -errno;
	}

	fi->fh = (intptr_t) dp;
	return retstat;


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

	cout<<"readdir"<<endl;
	MANAGER->writeToLog(READDIR);
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
	// if (de == 0) {//TODO
	// 	handleError(ERROR, SYS_GEN_ERROR, __func__);
	// 	return ERROR;
	// }
	if(de == 0)
	{
		return -errno;
	}
	// This will copy the entire directory into the buffer.  The loop exits
	// when either the system readdir() returns NULL, or filler()
	// returns something non-zero.  The first case just means I've
	// read the whole directory; the second means the buffer is full.
	do {
		if (filler(buf, de->d_name, NULL, 0) != 0) {
			//return handleError(-1, SYS_GEN_ERROR);//TODO
		}
	} while ((de = readdir(dp)) != NULL);

	

	return retstat;
}

/**
 * Release directory
 *
 * Introduced in velsrsion 2.3
 */
int caching_releasedir(const char *path, struct fuse_file_info *fi){
	cout<<"realeasedit"<<endl;
	MANAGER->writeToLog(RELEASEDIR);
	int retstat = 0;
	closedir((DIR *) (uintptr_t) fi->fh);
	return retstat;
}

/**
* update the path of the new name in the cache
* go true the block list in the cache and change to the new name
*/
// int renameInCache(const char* path, const char* newName)
// {
// 	for (std::list<Block*>::iterator it = MANAGER ->blockList.begin(); it != MANAGER ->blockList.end();
// 			++it) 
// 	{
// 		if (((*it)->getBlockFileName()).compare(path) == 0) 
// 		{
// 			(*it)->getBlockFileName() = newName;
// 		}
// 	}
// 	return 0;
// }


/**
* Rename a file 
*/
int caching_rename(const char *path, const char *newpath)
{
	cout<<"rename"<<endl;//TODO change names to parameters
	// TODO - write to log + error
	MANAGER->writeToLog(RENAME);

    int retstat = 0;
    char fpath[PATH_MAX];
    char fnewpath[PATH_MAX];
   
    caching_fileFullPath(fpath, path);
    caching_fileFullPath(fnewpath, newpath);

    cout <<"oldName " <<path<<"new "<<newpath<<endl;
   
   
    char* moldpath = realpath(fpath,NULL);
    cout <<"send to rename old " <<fpath<<" and new send is: "<<fnewpath<<endl;
    retstat = rename(fpath,fnewpath);
    char* mnewpath = realpath(fnewpath,NULL);

    if (retstat < 0)
    {
        
        printf("ERROR : caching_rename\n");
        return -errno;
    }


    MANAGER->Rename(moldpath ,mnewpath);
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
	cout<<"init"<<endl;
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
	cout<<"destroy"<<endl;
	// TODO - close log file?
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
	MANAGER->writeToLog(IOCTL);
    string ioctldata = MANAGER->toString();
    

 	return 0;
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
int main(int argc, char* argv[]){
	// reseting caching operators
	init_caching_oper();
	//prepering the log file address

	cout<<"create manager"<<endl;
	CacheManager* manager = new CacheManager(realpath(argv[1], NULL), realpath(argv[2], NULL), atoi(argv[3]), atoi(argv[4]));

	argv[1] = argv[2];

	cout<<"I am here"<<endl;

	for (int i = 2; i< (argc - 1); i++){
		argv[i] = NULL;
	}
        argv[2] = (char*) "-s";
        argv[3] = (char*) "-f";//TODO delete this before sub

	argc = 4;

	
	int fuse_stat = fuse_main(argc, argv, &caching_oper, manager);

	return fuse_stat;
}




//g++ -Wall -g -std=c++11 -o papo Block.cpp CacheManager.cpp CachingFileSystem.cpp -lfuse -D_FILE_OFFSET_BITS=64 `pkg-config fuse --libs` `pkg-config fuse --cflags`

//fusermount -u /tmp/ff/md       //TODO