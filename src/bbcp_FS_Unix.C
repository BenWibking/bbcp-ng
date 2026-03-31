/******************************************************************************/
/*                                                                            */
/*                        b b c p _ F S _ U n i x . C                         */
/*                                                                            */
/*(c) 2002-14 by the Board of Trustees of the Leland Stanford, Jr., University*//*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC02-76-SFO0515 with the Department of Energy              */
/*                                                                            */
/* bbcp is free software: you can redistribute it and/or modify it under      */
/* the terms of the GNU Lesser General Public License as published by the     */
/* Free Software Foundation, either version 3 of the License, or (at your     */
/* option) any later version.                                                 */
/*                                                                            */
/* bbcp is distributed in the hope that it will be useful, but WITHOUT        */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or      */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public       */
/* License for more details.                                                  */
/*                                                                            */
/* You should have received a copy of the GNU Lesser General Public License   */
/* along with bbcp in a file called COPYING.LESSER (LGPL license) and file    */
/* COPYING (GPL license).  If not, see <http://www.gnu.org/licenses/>.        */
/*                                                                            */
/* The copyright holder's institutional names and contributor's names may not */
/* be used to endorse or promote products derived from this software without  */
/* specific prior written permission of the institution or contributor.       */
/******************************************************************************/
  
#ifdef LINUX
#define _XOPEN_SOURCE 600
#endif

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <string.h>
#include <unistd.h>
#include <utime.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef FREEBSD
#include <sys/statvfs.h>
#endif
#include "bbcp_File.h"
#include "bbcp_FS_Unix.h"
#include "bbcp_Platform.h"
#include "bbcp_Pthread.h"
#include "bbcp_System.h"
#include "bbcp_Debug.h"

/******************************************************************************/
/*                         L o c a l   D e f i n e s                          */
/******************************************************************************/
  
#if defined(MACOS) || defined (AIX)
#define S_IAMB      0x1FF
#endif

#if defined(SUN) && RELEASE < 511
#define READLINK(fd, dent, path, buff, blen) readlink(path,buff,blen)
#else
#define READLINK(fd, dent, path, buff, blen) readlinkat(fd,dent,buff,blen)
#endif

/******************************************************************************/
/*                         L o c a l   H e l p e r s                          */
/******************************************************************************/

namespace
{
int bbcp_SplitPathLocal(const char *path, char *dirbuf, size_t dlen,
                        char *filebuf, size_t flen)
{
   const char *slash;
   size_t dirlen, filelen;

   if (!path || !*path) return -(errno = EINVAL);
   slash = strrchr(path, '/');
   if (!slash)
      {dirlen  = 1;
       filelen = strlen(path);
       if (dirlen+1 > dlen || filelen+1 > flen) return -(errno = ENAMETOOLONG);
       memcpy(dirbuf,  ".", dirlen+1);
       memcpy(filebuf, path, filelen+1);
       return 0;
      }

   filelen = strlen(slash+1);
   dirlen  = slash-path;
   if (!dirlen) dirlen = 1;
   if (dirlen+1 > dlen || filelen+1 > flen) return -(errno = ENAMETOOLONG);
   if (slash == path) memcpy(dirbuf, "/", dirlen);
      else memcpy(dirbuf, path, dirlen);
   dirbuf[dirlen] = '\0';
   memcpy(filebuf, slash+1, filelen+1);
   return 0;
}

int bbcp_OpenMetaTarget(const char *path)
{
   int fd, opts = O_RDONLY;

#ifdef O_NONBLOCK
   opts |= O_NONBLOCK;
#endif
#ifdef O_NOFOLLOW
   opts |= O_NOFOLLOW;
#endif
   do {fd = open(path, opts);}
      while(fd < 0 && errno == EINTR);
   return fd;
}

int bbcp_OpenParentDir(const char *path, char *leaf, size_t llen)
{
   char dirbuf[MAXPATHLEN+1];
   int fd, opts = O_RDONLY, rc;

   if ((rc = bbcp_SplitPathLocal(path, dirbuf, sizeof(dirbuf), leaf, llen))) return rc;
#ifdef O_DIRECTORY
   opts |= O_DIRECTORY;
#endif
#ifdef O_NOFOLLOW
   opts |= O_NOFOLLOW;
#endif
   do {fd = open(dirbuf, opts);}
      while(fd < 0 && errno == EINTR);
   if (fd < 0) return -errno;
   return fd;
}

int bbcp_OpenDirLocal(const char *path)
{
   int fd, opts = O_RDONLY;

#ifdef O_DIRECTORY
   opts |= O_DIRECTORY;
#endif
#ifdef O_NOFOLLOW
   opts |= O_NOFOLLOW;
#endif
   do {fd = open(path, opts);}
      while(fd < 0 && errno == EINTR);
   if (fd < 0) return -errno;
   return fd;
}

int bbcp_VerifyDirFD(int fd)
{
   struct stat sbuf;

   if (fstat(fd, &sbuf)) return -errno;
   if (!S_ISDIR(sbuf.st_mode)) return -(errno = ENOTDIR);
   return 0;
}
}

/******************************************************************************/
/*                        G l o b a l   O b j e c t s                         */
/******************************************************************************/

extern bbcp_System bbcp_OS;
  
/******************************************************************************/
/*                            A p p l i c a b l e                             */
/******************************************************************************/
  
int bbcp_FS_Unix::Applicable(const char *path)
{
#ifdef FREEBSD
   if (!fs_path) fs_path = strdup(path);
#else
   struct statvfs buf;

// To find out whether or not we are applicable, simply do a statvfs on the
// incomming path. If we can do it, then we are a unix filesystem.
//
   if (statvfs(path, &buf)) return 0;

// Set the sector size
//
   secSize = buf.f_frsize;

// Save the path to this filesystem if we don't have one. This is a real
// kludgy short-cut since in bbcp we only have a single output destination.
//
   if (!fs_path) 
      {fs_path = strdup(path); 
       memcpy((void *)&fs_id, (const void *)&buf.f_fsid, sizeof(fs_id));
      }
#endif
   return 1;
}

/******************************************************************************/
/*                                E n o u g h                                 */
/******************************************************************************/
  
int bbcp_FS_Unix::Enough(long long bytes, int numfiles)
{
#ifndef FREEBSD
    struct statvfs buf;
    long long free_space;

// Perform a stat call on the filesystem
//
   if (statvfs(fs_path, &buf)) return 0;

// Calculate free space
//
   free_space = (long long)buf.f_bsize * (long long)buf.f_bavail;

// Indicate whether there is enough space here
//
#ifdef LINUX
   if (!(buf.f_files | buf.f_ffree | buf.f_favail)) numfiles = 0;
#endif
   return (free_space > bytes) && buf.f_favail > numfiles;
#else
   return 1;
#endif
}

/******************************************************************************/
/*                                 F s y n c                                  */
/******************************************************************************/
  
int bbcp_FS_Unix::Fsync(const char *fn, int fd)
{
   int rc = 0;

// First do an fsync on the file
//
   if (fsync(fd)) return -errno;

// If a filename was passed, do an fsync on the directory as well
//
   if (fn)
      {const char *Slash = rindex(fn, '/');
       char dBuff[MAXPATHLEN+8];
       if (Slash)
          {int n = Slash - fn;
           strncpy(dBuff, fn, n); dBuff[n] = 0;
           if ((n = bbcp_OpenDirLocal(dBuff)) < 0) return n;
           if (fsync(n)) rc = -errno;
           close(n);
          }
      }

// All done
//
   return rc;
}

/******************************************************************************/
/*                               g e t S i z e                                */
/******************************************************************************/
  
long long bbcp_FS_Unix::getSize(int fd, long long *bsz)
{
   struct stat Stat;

// Get the size of the file
//
   if (fstat(fd, &Stat)) return -errno;
   if (bsz) *bsz = (secSize > 8192 ? 8192 : secSize);
   return Stat.st_size;
}

/******************************************************************************/
/*                                 M K D i r                                  */
/******************************************************************************/

int bbcp_FS_Unix::MKDir(const char *path, mode_t mode)
{
    char leaf[MAXPATHLEN+1];
    int dfd, fd, rc;

    if ((dfd = bbcp_OpenParentDir(path, leaf, sizeof(leaf))) < 0) return dfd;
#if defined(AT_FDCWD)
    do {rc = mkdirat(dfd, leaf, mode);}
       while(rc && errno == EINTR);
#else
    rc = mkdir(path, mode);
#endif
    if (rc)
       {rc = -errno;
        close(dfd);
        return rc;
       }
#if defined(AT_FDCWD)
#ifdef O_NOFOLLOW
    do {fd = openat(dfd, leaf, O_RDONLY|O_NOFOLLOW
#ifdef O_DIRECTORY
                    |O_DIRECTORY
#endif
                   );}
#else
    do {fd = openat(dfd, leaf, O_RDONLY
#ifdef O_DIRECTORY
                    |O_DIRECTORY
#endif
                   );}
#endif
       while(fd < 0 && errno == EINTR);
    if (fd < 0)
       {rc = -errno;
        close(dfd);
        return rc;
       }
    if ((rc = bbcp_VerifyDirFD(fd)))
       {close(fd);
        close(dfd);
        return rc;
       }
    if (fchmod(fd, mode)) rc = -errno;
    if (close(fd) && !rc) rc = -errno;
#else
    if ((fd = bbcp_OpenDirLocal(path)) < 0)
       {close(dfd);
        return fd;
       }
    if (fchmod(fd, mode)) rc = -errno;
    if (close(fd) && !rc) rc = -errno;
#endif
    if (close(dfd) && !rc) rc = -errno;
    if (rc) return rc;

    return 0;
}

/******************************************************************************/
/*                                 M K L n k                                  */
/******************************************************************************/

int bbcp_FS_Unix::MKLnk(const char *ldata, const char *path)
{
    char leaf[MAXPATHLEN+1];
    int dfd, rc;

    if ((dfd = bbcp_OpenParentDir(path, leaf, sizeof(leaf))) < 0) return dfd;
#if defined(AT_FDCWD)
    do {rc = symlinkat(ldata, dfd, leaf);}
       while(rc && errno == EINTR);
#else
    rc = symlink(ldata, path);
#endif
    if (rc) rc = -errno;
    if (close(dfd) && !rc) rc = -errno;
    if (rc) return rc;

    return 0;
}
  
/******************************************************************************/
/*                                  O p e n                                   */
/******************************************************************************/

bbcp_File *bbcp_FS_Unix::Open(const char *fn, int opts, int mode, const char *fa)
{
    static const int rwxMask = S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO;
    int FD;
    bbcp_IO *iob;

// Check for direct I/O
//
#ifdef O_DIRECT
   if (dIO) opts |= O_DIRECT;
#endif
#ifdef O_NOFOLLOW
   opts |= O_NOFOLLOW;
#endif

// Open the file
//
   mode &= rwxMask;
   if ((FD = (mode ? open(fn, opts, mode) : open(fn, opts))) < 0) 
      return (bbcp_File *)0;

// Advise about file access in Linux
//
#ifdef LINUX
   posix_fadvise(FD,0,0,POSIX_FADV_SEQUENTIAL|POSIX_FADV_NOREUSE);
#endif

// Do direct I/O for Solaris
//
#ifdef SUN
   if (dIO && directio(FD, DIRECTIO_ON))
      {DEBUG(strerror(errno) <<" requesting direct i/o for "
             <<bbcp_DebugMask(fn, "path", DEBUGON)); dIO = 0;}
#endif

// Allocate a file object and return that
//
   iob =  new bbcp_IO(FD);
   return new bbcp_File(fn, iob, (bbcp_FileSystem *)this, (dIO ? secSize : 0));
}
  
/******************************************************************************/
/*                                    R M                                     */
/******************************************************************************/
  
int bbcp_FS_Unix::RM(const char *path)
{
    char leaf[MAXPATHLEN+1];
    int dfd, rc, ecode = 0;

    if ((dfd = bbcp_OpenParentDir(path, leaf, sizeof(leaf))) < 0) return dfd;
    do {rc = unlinkat(dfd, leaf, 0);}
       while(rc && errno == EINTR);
    if (rc && (errno == EISDIR || errno == EPERM))
       do {rc = unlinkat(dfd, leaf, AT_REMOVEDIR);}
          while(rc && errno == EINTR);
    if (rc) ecode = errno;
    if (close(dfd) && !ecode) ecode = errno;
    return (ecode ? -ecode : 0);
}

/******************************************************************************/
/*                              s e t G r o u p                               */
/******************************************************************************/
  
int bbcp_FS_Unix::setGroup(const char *path, const char *Group)
{
    int fd, rc;

   if ((fd = bbcp_OpenMetaTarget(path)) < 0) return -errno;
   rc = setGroupFD(fd, Group);
   if (close(fd) && !rc) rc = -errno;
   return rc;
}

int bbcp_FS_Unix::setGroupFD(int fd, const char *Group)
{
    gid_t gid;

   if (!Group || !Group[0]) return 0;
   gid = bbcp_OS.getGID(Group);
   if (fchown(fd, (uid_t)-1, gid)) return -errno;
   return 0;
}

/******************************************************************************/
/*                               s e t M o d e                                */
/******************************************************************************/
  
int bbcp_FS_Unix::setMode(const char *path, mode_t mode)
{
    int fd, rc;

    if ((fd = bbcp_OpenMetaTarget(path)) < 0) return -errno;
    rc = setModeFD(fd, mode);
    if (close(fd) && !rc) rc = -errno;
    return rc;
}

int bbcp_FS_Unix::setModeFD(int fd, mode_t mode)
{
    if (fchmod(fd, mode)) return -errno;
    return 0;
}

/******************************************************************************/
/*                              s e t T i m e s                               */
/******************************************************************************/
  
int bbcp_FS_Unix::setTimes(const char *path, time_t atime, time_t mtime)
{
    int fd, rc;

    if ((fd = bbcp_OpenMetaTarget(path)) < 0) return -errno;
    rc = setTimesFD(fd, atime, mtime);
    if (close(fd) && !rc) rc = -errno;
    return rc;
}

int bbcp_FS_Unix::setTimesFD(int fd, time_t atime, time_t mtime)
{
#if defined(_POSIX_VERSION)
    struct timeval ftimes[2];

    ftimes[0].tv_sec  = atime;
    ftimes[0].tv_usec = 0;
    ftimes[1].tv_sec  = mtime;
    ftimes[1].tv_usec = 0;
    if (futimes(fd, ftimes)) return -errno;
    return 0;
#else
    return -ENOTSUP;
#endif
}
 
/******************************************************************************/
/*                                  S t a t                                   */
/******************************************************************************/
  
int bbcp_FS_Unix::Stat(const char *path, bbcp_FileInfo *sbuff)
{
   struct stat xbuff;

// Perform the stat function
//
   if (stat(path, &xbuff)) return -errno;
   if (!sbuff) return 0;
   return Stat(xbuff, sbuff);
}

/******************************************************************************/
  
int bbcp_FS_Unix::Stat(const char *path, const char *dent, int fd,
                       int chklnks, bbcp_FileInfo *sbuff)
{
   struct stat xbuff;
   char lbuff[2048];
   int n;

// Perform the stat function
//
#ifdef AT_SYMLINK_NOFOLLOW
   if (fstatat(fd, dent, &xbuff, AT_SYMLINK_NOFOLLOW)) return -errno;
   if ((xbuff.st_mode & S_IFMT) != S_IFLNK)
      return (sbuff ? Stat(xbuff, sbuff) : 0);
   if (chklnks > 0) return -ENOENT;
   if (!sbuff) return 0;
   if ((n = READLINK(fd,dent,path,lbuff,sizeof(lbuff)-1)) < 0) return -errno;
// if ((n = readlinkat(fd, dent, lbuff, sizeof(lbuff)-1)) < 0) return -errno;
   lbuff[n] = 0;
   if(sbuff->SLink) free(sbuff->SLink);
   sbuff->SLink = strdup(lbuff);
   if (!chklnks && fstatat(fd, dent, &xbuff, 0)) return -errno;
   return Stat(xbuff, sbuff);
#else
   if (lstat(path, &xbuff)) return -errno;
   if ((xbuff.st_mode & S_IFMT) != S_IFLNK)
      return (sbuff ? Stat(xbuff, sbuff) : 0);
   if (chklnks > 0) return -ENOENT;
   if (!sbuff) return 0;
   if ((n = readlink(path, lbuff, sizeof(lbuff)-1)) < 0) return -errno;
   lbuff[n] = 0;
   if(sbuff->SLink) free(sbuff->SLink);
   sbuff->SLink = strdup(lbuff);
   if (!chklnks && stat(path, &xbuff)) return -errno;
   return Stat(xbuff, sbuff);
#endif
}

/******************************************************************************/
  
int bbcp_FS_Unix::Stat(struct stat &xbuff, bbcp_FileInfo *sbuff)
{
   static const int isXeq = S_IXUSR|S_IXGRP|S_IXOTH;

// Copy the stat info into our own structure
//
   sbuff->fileid = ((long long)xbuff.st_dev)<<32 | (long long)xbuff.st_ino;
   sbuff->mode   = xbuff.st_mode & (S_IAMB | 0xF00);
   sbuff->size   = xbuff.st_size;
   sbuff->atime  = xbuff.st_atime;
   sbuff->ctime  = xbuff.st_ctime;
   sbuff->mtime  = xbuff.st_mtime;

// Get type of object
//
//
        if (S_ISREG( xbuff.st_mode)){sbuff->Otype = 'f';
        if (isXeq &  xbuff.st_mode)  sbuff->Xtype = 'x';
                                    }
   else if (S_ISFIFO(xbuff.st_mode)) sbuff->Otype = 'p';
   else if (S_ISDIR( xbuff.st_mode)) sbuff->Otype = 'd';
   else if ((xbuff.st_mode & S_IFMT) == S_IFLNK)
                                     sbuff->Otype = 'l';
   else                              sbuff->Otype = '?';

// Convert gid to a group name
//
   if (sbuff->Group) free(sbuff->Group);
   sbuff->Group = bbcp_OS.getGNM(xbuff.st_gid);

// All done
//
   return 0;
}
