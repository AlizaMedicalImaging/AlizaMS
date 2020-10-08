/*********************************************************
 *
 * MDCM
 *
 * Modifications github.com/issakomi
 *
 *********************************************************/

/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2016 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "mdcmSystem.h"
#include "mdcmTrace.h"
#include "mdcmFilename.h"
#include "mdcmDirectory.h"
#include "mdcmException.h"
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h> // PATH_MAX

#ifdef MDCM_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#if defined(MDCM_HAVE_SNPRINTF)
#elif defined(MDCM_HAVE__SNPRINTF)
#define snprintf _snprintf
#endif
#ifdef MDCM_USE_COREFOUNDATION_LIBRARY
#include <CoreFoundation/CoreFoundation.h>
#endif

#if defined(_WIN32) && (defined(_MSC_VER) || defined(__WATCOMC__) ||defined(__BORLANDC__) || defined(__MINGW32__))
#include <io.h>
#include <direct.h>
#define _unlink unlink
#else

#include <dlfcn.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h> // gethostname
#include <strings.h> // strncasecmp
#endif

namespace mdcm
{

#if defined(_WIN32) && (defined(_MSC_VER) || defined(__WATCOMC__) || defined(__BORLANDC__) || defined(__MINGW32__))
static int Mkdir(const char * dir)
{
  return _mkdir(dir);
}

static int Rmdir(const char * dir)
{
  return _rmdir(dir);
}

static const char* Getcwd(char * buf, unsigned int len)
{
  const char * ret = _getcwd(buf, len);
  return ret;
}

#else
static int Mkdir(const char * dir)
{
  return mkdir(dir, 00777);
}

static int Rmdir(const char * dir)
{
  return rmdir(dir);
}

static const char * Getcwd(char * buf, unsigned int len)
{
  const char * ret = getcwd(buf, len);
  return ret;
}
#endif

const char * System::GetCWD()
{
  static char buf[2048];
  const char* cwd = Getcwd(buf, 2048);
  return cwd;
}

static int Mkdir2(const char * p)
{
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
  const std::wstring unc = System::ConvertToUNC(p);
  return _wmkdir(unc.c_str());
#else
  return Mkdir(p);
#endif
}

bool System::MakeDirectory(const char * path)
{
  if(!path || !*path)return false;
  if(System::FileExists(path)) return true;
  Filename fn(path);
  std::string dir = fn.ToUnixSlashes();
  std::string::size_type pos = dir.find(':');
  if(pos == std::string::npos)
  {
    pos = 0;
  }
  std::string topdir;
  bool ok = true;
  while(ok && (pos = dir.find('/', pos)) != std::string::npos)
  {
    topdir = dir.substr(0, pos+1);
    ok = ok && (System::FileIsDirectory(topdir.c_str()) || 0 == Mkdir2(topdir.c_str()));
    pos++;
  }
  if(!ok) return false;
  if(dir[dir.size()-1] == '/')
  {
    topdir = dir.substr(0, dir.size());
  }
  else
  {
    topdir = dir;
  }
  if(Mkdir2(topdir.c_str()) != 0)
  {
    // There is a bug in the Borland Run time library which makes MKDIR
    // return EACCES when it should return EEXISTS
    // if it is some other error besides directory exists
    // then return false
    if((errno != EEXIST)
#ifdef __BORLANDC__
       && (errno != EACCES)
#endif
    )
    {
      return false;
    }
  }
  return true;
}

bool System::FileExists(const char * filename)
{
#ifndef R_OK
#define R_OK 04
#endif
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
  const std::wstring unc = System::ConvertToUNC(filename);
  if (_waccess(unc.c_str(), R_OK) != 0)
#else
#ifdef _WIN32
  if (_access(filename, R_OK) != 0)
#else
  if (access(filename, R_OK) != 0)
#endif
#endif
  {
    return false;
  }
  else
  {
    return true;
  }
}

bool System::FileIsDirectory(const char * name)
{
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
  struct _stat64i32 fs;
  const std::wstring wname = System::ConvertToUNC(name);
  if (_wstat(wname.c_str(), &fs) == 0)
#else
  struct stat fs;
  if(stat(name, &fs) == 0)
#endif
  {
#ifdef _WIN32
    return ((fs.st_mode & _S_IFDIR) != 0);
#else
    return S_ISDIR(fs.st_mode);
#endif
  }
  else
  {
    return false;
  }
}

bool System::FileIsSymlink(const char * name)
{
#ifdef _WIN32
  (void)name;
#else
  struct stat fs;
  if(lstat(name, &fs) == 0)
  {
    return S_ISLNK(fs.st_mode);
  }
#endif
  return false;
}

// TODO st_mtimensec
time_t System::FileTime(const char * filename)
{
  struct stat fs;
  if(stat(filename, &fs) == 0)
  {
    // man 2 stat
    // time_t    st_atime;   /* time of last access */
    // time_t    st_mtime;   /* time of last modification */
    // time_t    st_ctime;   /* time of last status change */
    return fs.st_mtime;
    // Since  kernel 2.5.48, the stat structure supports nanosecond resolution
    // for the three file timestamp fields.  Glibc exposes the nanosecond com-
    // ponent of each field using names either of the form st_atim.tv_nsec, if
    // the _BSD_SOURCE or _SVID_SOURCE feature test macro is  defined,  or  of
    // the  form st_atimensec, if neither of these macros is defined.  On file
    // systems that do not support  sub-second  timestamps,  these  nanosecond
    // fields are returned with the value 0.
  }
  return 0;
}

const char * System::GetLastSystemError()
{
  int e = errno;
  return strerror(e);
}

bool System::GetPermissions(const char * file, unsigned short & mode)
{
  if(!file)
  {
    return false;
  }
  struct stat st;
  if(stat(file, &st) < 0)
  {
    return false;
  }
  mode = (short)st.st_mode;
  return true;
}

bool System::SetPermissions(const char * file, unsigned short mode)
{
  if(!file)
  {
    return false;
  }
  if(!System::FileExists(file))
  {
    return false;
  }
  if(chmod(file, mode) < 0)
  {
    return false;
  }
  return true;
}

bool System::RemoveFile(const char * source)
{
#ifdef _WIN32
  unsigned short mode;
  if(!System::GetPermissions(source, mode))
  {
    return false;
  }
  System::SetPermissions(source, S_IWRITE);
#endif
  bool res = unlink(source) != 0 ? false : true;
#ifdef _WIN32
  if (!res)
  {
    System::SetPermissions(source, mode);
  }
#endif
  return res;
}

bool System::DeleteDirectory(const char * source)
{
  unsigned short mode;
  if(System::GetPermissions(source, mode))
  {
#if defined(_WIN32) && !defined(__CYGWIN__)
    mode |= S_IWRITE;
#else
    mode |= S_IWUSR;
#endif
    System::SetPermissions(source, mode);
  }
  Directory dir;
  unsigned int numfiles = dir.Load(source, false);
  (void)numfiles;
  Directory::FilenamesType const & files = dir.GetFilenames();
  for (Directory::FilenamesType::const_iterator it = files.begin();
    it != files.end(); ++it)
  {
    const char *filename = it->c_str();
    if(System::FileIsDirectory(filename) &&
      !System::FileIsSymlink(filename))
    {
      if (!System::DeleteDirectory(filename))
      {
        return false;
      }
    }
    else
    {
      if(!System::RemoveFile(filename))
      {
        return false;
      }
    }
  }
  return Rmdir(source) == 0;
}

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
namespace
{

static std::wstring utf8_decode(std::string const &str)
{
  if(str.empty()) return std::wstring();
  const int len = MultiByteToWideChar(CP_UTF8, 0, &str[0], -1, NULL, 0);
  std::wstring ret(len, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], -1, &ret[0], len);
  return ret;
}

// http://arsenmk.blogspot.com/2015/12/handling-long-paths-on-windows.html
static bool ComputeFullPath(std::wstring const &in, std::wstring &out)
{
  // consider an input fileName of type PCWSTR (const wchar_t*)
  const wchar_t *fileName = in.c_str();
  DWORD requiredBufferLength = GetFullPathNameW(fileName, 0, nullptr, nullptr);
  if (0 == requiredBufferLength) return false;
  out.resize(requiredBufferLength);
  wchar_t *buffer = &out[0];
  DWORD result = GetFullPathNameW(fileName, requiredBufferLength, buffer, nullptr);
  if (0 == result) return false;
  return true;
}

static std::wstring HandleMaxPath(std::wstring const &in)
{
  if (in.size() >= MAX_PATH)
  {
    std::wstring out;
    bool ret = ComputeFullPath(in, out);
    if (!ret) return in;
    if (out.size() < 4) return in;
    if (out[0] == '\\' && out[1] == '\\' && out[2] == '?')
    {
      ;;
    }
    else if (out[0] == '\\' && out[1] == '\\' && out[2] != '?')
    {
      // server path
      const std::wstring prefix = LR"(\\?\UNC\)";
      out = prefix + (out.c_str() + 2);
    }
    else
    {
      // regular C:\ style path:
      assert(out[1] == ':');
      const std::wstring prefix = LR"(\\?\)";
      out = prefix + out.c_str();
    }
    return out;
  }
  return in;
}

}

std::wstring System::ConvertToUNC(const char *utf8path)
{
  const std::wstring uft16path = utf8_decode(utf8path);
  const std::wstring uncpath = HandleMaxPath(uft16path);
  return uncpath;
}
#endif

size_t System::FileSize(const char * filename)
{
/*
stat structure contains following fields:
 dev_t     st_dev;    // ID of device containing file
 ino_t     st_ino;    // inode number
 mode_t    st_mode;   // protection
 nlink_t   st_nlink;  // number of hard links
 uid_t     st_uid;    // user ID of owner
 gid_t     st_gid;    // group ID of owner
 dev_t     st_rdev;   // device ID (if special file)
 off_t     st_size;   // total size, in bytes
 blksize_t st_blksize;// blocksize for filesystem I/O
 blkcnt_t  st_blocks; // number of blocks allocated
 time_t    st_atime;  // time of last access
 time_t    st_mtime;  // time of last modification
 time_t    st_ctime;  // time of last status change
*/
  struct stat fs;
  if (stat(filename, &fs) != 0)
  {
    return 0;
  }
  off_t size = fs.st_size;
  size_t size2 = size;
  // off_t can be larger than size_t
  if(size != (off_t)size2) return 0;
  return size2;
}

const char * System::GetCurrentProcessFileName()
{
#ifdef _WIN32
  static char buf[MAX_PATH];
  if (::GetModuleFileName(0, buf, sizeof(buf)))
  {
    return buf;
  }
#elif defined(MDCM_USE_COREFOUNDATION_LIBRARY)
  static char buf[PATH_MAX];
  Boolean success = false;
  CFURLRef pathURL = CFBundleCopyExecutableURL(CFBundleGetMainBundle());
  if (pathURL)
  {
    success = CFURLGetFileSystemRepresentation(
      pathURL, true /*resolveAgainstBase*/, (unsigned char*) buf, PATH_MAX);
    CFRelease(pathURL);
  }
  if (success)
  {
    return buf;
  }
#elif defined (__SVR4) && defined (__sun)
  // solaris
  const char *ret = getexecname();
  if(ret) return ret;
#elif defined(__DragonFly__) || defined(__OpenBSD__) || defined(__FreeBSD__)
  static char path[PATH_MAX];
  if (readlink ("/proc/curproc/file", path, sizeof(path)) > 0)
  {
    return path;
  }
#elif defined(__linux__)
  static char path[PATH_MAX];
  // Technically 0 is not an error, but that would mean
  // 0 byte were copied ... thus considered it as an error
  if (readlink ("/proc/self/exe", path, sizeof(path)) > 0)
  {
    return path;
  }
#else
  mdcmErrorMacro("Not implementated");
#endif
  return 0;
}

#ifdef __USE_GNU
static void where_am_i() {}
#endif

const char * System::GetCurrentModuleFileName()
{
#ifdef __USE_GNU
  static char path[PATH_MAX];
  Dl_info info;
  if (dladdr((void*)&where_am_i, &info) == 0)
  {
    size_t len = strlen(info.dli_fname);
    if(len >= PATH_MAX) return NULL;
    strcpy(path,info.dli_fname);
    return path;
  }
#elif defined(_WIN32)
  return System::GetCurrentProcessFileName();
#endif
  return NULL;
}

const char * System::GetCurrentResourcesDirectory()
{
#ifdef MDCM_USE_COREFOUNDATION_LIBRARY
  static char path[PATH_MAX];
  Boolean success = false;
  CFURLRef pathURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
  if (pathURL != NULL)
  {
    success = CFURLGetFileSystemRepresentation(
      pathURL, true /*resolveAgainstBase*/, (unsigned char*) path, PATH_MAX);
    CFRelease(pathURL);
  }
  if (success)
  {
    strlcat(path, "/" MDCM_INSTALL_DATA_DIR, PATH_MAX);
    return path;
  }
#endif
  return NULL;
}

/*
 * Encode the mac address on a fixed length string of 15 characters.
 * we save space this way.
 */
static int getlastdigit(unsigned char * data, unsigned long size)
{
  int extended, carry = 0;
  for(unsigned int i=0;i<size;i++)
  {
    extended = (carry << 8) + data[i];
    data[i] = (unsigned char)(extended / 10);
    carry = extended % 10;
  }
  assert(carry >= 0 && carry < 10);
  return carry;
}

size_t System::EncodeBytes(char * out, const unsigned char * data, int size)
{
  bool zero = false;
  int res;
  std::string sres;
  unsigned char buffer[32];
  unsigned char *addr = buffer;
  memcpy(addr, data, size);
  while(!zero)
  {
    res = getlastdigit(addr, size);
    const char v = (char)('0' + res);
    sres.insert(sres.begin(), v);
    zero = true;
    for(int i = 0; i < size; ++i)
    {
      zero = zero && (addr[i] == 0);
    }
  }
  strcpy(out, sres.c_str());
  return sres.size();
}

#if defined(_WIN32) && !defined(MDCM_HAVE_GETTIMEOFDAY)

static int gettimeofday2(struct timeval * tv, struct timezone * tz)
{
  FILETIME ft;
  const uint64_t c1 = 27111902;
  const uint64_t c2 = 3577643008UL;
  const uint64_t OFFSET = (c1 << 32) + c2;
  uint64_t filetime = 0;
  GetSystemTimeAsFileTime(&ft);
  filetime |= ft.dwHighDateTime;
  filetime <<= 32;
  filetime |= ft.dwLowDateTime;
  filetime -= OFFSET;
  tv->tv_sec = (long)(filetime / 10000000); // seconds since epoch
  tv->tv_usec = (uint32_t)((filetime % 10000000) / 10);
  return 0;
}

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
/*
The use of the timezone structure is obsolete; the tz  argument  should
normally  be  specified  as  NULL.  The tz_dsttime field has never been
used under Linux; it has not been and will not be supported by libc  or
glibc.   Each  and  every occurrence of this field in the kernel source
(other than the declaration) is a bug. Thus, the following is purely of
historic interest.
*/
  assert(tz == 0);
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
    //converting file time to unix epoch
    tmpres /= 10;  //convert into microseconds
    tmpres -= DELTA_EPOCH_IN_MICROSECS;
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }
  return 0;
}
#endif

/*
 Implementation note. We internally use mktime which seems to be quite relaxed when it
 comes to invalid date. It handles :
 "17890714172557";
 "19891714172557";
 "19890014172557";
 While the DICOM PS 3.5-2008 would prohibit them.
 I leave it this way so that we correctly read in /almost/ valid date. What we write out is
 always valid anyway which is what is important.
*/
bool System::ParseDateTime(time_t & timep, const char date[22])
{
  long milliseconds;
  return ParseDateTime(timep, milliseconds, date);
}

bool System::ParseDateTime(time_t & timep, long & milliseconds, const char date[22])
{
  if(!date) return false;
  size_t len = strlen(date);
  if(len < 4)  return false;
  if(len > 21) return false;
  struct tm ptm;
#if defined(MDCM_HAVE_STRPTIME) && 0
  char * ptr1 = strptime(date, "%Y%m%d%H%M%S", &ptm);
  if(ptr1 != date + 14)
  {
    return false;
  }
#else
  int year, mon, day, hour, min, sec, n;
  if ((n = sscanf(date, "%4d%2d%2d%2d%2d%2d",
        &year, &mon, &day, &hour, &min, &sec)) >= 1)
  {
    switch (n)
    {
    case 1: mon = 1;
    case 2: day = 1;
    case 3: hour = 0;
    case 4: min = 0;
    case 5: sec = 0;
      break;
    }
    ptm.tm_year = year - 1900;
    if(mon < 1 || mon > 12) return false;
    ptm.tm_mon = mon - 1;
    if(day < 1 || day > 31) return false;
    ptm.tm_mday = day;
    if(hour > 24) return false;
    ptm.tm_hour = hour;
    if(min > 60) return false;
    ptm.tm_min = min;
    if(sec > 60) return false;
    ptm.tm_sec = sec;
    ptm.tm_wday = -1;
    ptm.tm_yday = -1;
    ptm.tm_isdst = -1;
  }
  else
  {
    return false;
  }
#endif
  timep = mktime(&ptm);
  if(timep == (time_t)-1) return false;
  milliseconds = 0;
  if(len > 14)
  {
    const char *ptr = date + 14;
    if(*ptr != '.') return false;
    ++ptr;
    if(!*ptr || sscanf( ptr, "%06ld", &milliseconds) != 1 )
    {
      return false;
    }
  }
  return true;
}

const char * System::GetTimezoneOffsetFromUTC()
{
  static std::string buffer;
  char outstr[10];
  time_t t = time(NULL);
  struct tm *tmp = localtime(&t);
  size_t l = strftime(outstr, sizeof(outstr), "%z", tmp);
  assert(l == 5); (void)l;
  buffer = outstr;
  return buffer.c_str();
}

bool System::FormatDateTime(char date[22], time_t timep, long milliseconds)
{
  if(!(milliseconds >= 0 && milliseconds < 1000000))
  {
    return false;
  }
  // YYYYMMDDHHMMSS.FFFFFF&ZZXX
  if(!date)
  {
    return false;
  }
  const size_t maxsize = 40;
  char tmp[maxsize];
  // Obtain the time of day, and convert it to a tm struct.
  struct tm *ptm = localtime (&timep);
  if(!ptm)
  {
    return false;
  }
  // Format the date and time, down to a single second.
  size_t ret = strftime (tmp, sizeof (tmp), "%Y%m%d%H%M%S", ptm);
  assert(ret == 14);
  if(ret == 0 || ret >= maxsize)
  {
    return false;
  }
  // Add milliseconds
  const size_t maxsizall = 22;
  const int ret2 = snprintf(date,maxsizall,"%s.%06ld",tmp,milliseconds);
  if(ret2 < 0) return false;
  if((size_t)ret2 >= maxsizall)
  {
    return false;
  }
  return true;
}

bool System::GetCurrentDateTime(char date[22])
{
  long milliseconds;
  time_t timep;
/*
The functions gettimeofday() and settimeofday() can  get  and  set  the
time  as  well  as a timezone.  The tv argument is a struct timeval (as
specified  in <sys/time.h>):

struct timeval
{
  time_t      tv_sec;     // seconds
  suseconds_t tv_usec;    // microseconds
};

and gives the number of seconds and microseconds since the  Epoch  (see
time(2)).  The tz argument is a struct timezone:

struct timezone
{
  int tz_minuteswest;     // minutes west of Greenwich
  int tz_dsttime;         // type of DST correction
};

If  either  tv or tz is NULL, the corresponding structure is not set or
returned.

The use of the timezone structure is obsolete; the tz  argument  should
normally  be  specified  as  NULL.  The tz_dsttime field has never been
used under Linux; it has not been and will not be supported by libc  or
glibc.   Each  and  every occurrence of this field in the kernel source
(other than the declaration) is a bug. Thus, the following is purely of
historic interest.
*/
  struct timeval tv;
  gettimeofday (&tv, NULL);
  timep = tv.tv_sec;
  // A concatenated date-time character string in the format:
  // YYYYMMDDHHMMSS.FFFFFF&ZZXX
  // The components of this string, from left to right, are YYYY = Year, MM =
  // Month, DD = Day, HH = Hour (range "00" - "23"), MM = Minute (range "00" -
  // "59"), SS = Second (range "00" - "60").
  // FFFFFF = Fractional Second contains a fractional part of a second as small
  // as 1 millionth of a second (range 000000 - 999999).
  assert(tv.tv_usec >= 0 && tv.tv_usec < 1000000);
  milliseconds = tv.tv_usec;
  return FormatDateTime(date, timep, milliseconds);
}

int System::StrNCaseCmp(const char * s1, const char * s2, size_t n)
{
#if defined(MDCM_HAVE_STRNCASECMP)
  return strncasecmp(s1,s2,n);
#elif defined(MDCM_HAVE__STRNICMP)
  return _strnicmp(s1,s2,n);
#else
#error
  assert(n); // TODO
  while (--n && *s1 && (tolower(*s1) == tolower(*s2)))
  {
    s1++;
    s2++;
  }
  return tolower(*s1) - tolower(*s2);
#endif
}

int System::StrCaseCmp(const char * s1, const char * s2)
{
#if defined(MDCM_HAVE_STRCASECMP)
  return strcasecmp(s1,s2);
#elif defined(MDCM_HAVE__STRNICMP)
  return _stricmp(s1,s2);
#else
#error
  while (*s1 && (tolower(*s1) == tolower(*s2)))
  {
    s1++;
    s2++;
  }
  return tolower(*s1) - tolower(*s2);
#endif
}

bool System::GetHostName(char name[255])
{
// http://msdn.microsoft.com/en-us/library/ms738527.aspx
// WSANOTINITIALISED A successful WSAStartup call must occur before using this function.
#ifdef _WIN32
  WORD wVersionRequested;
  WSADATA wsaData;
  wVersionRequested = MAKEWORD(2,0);
  if (WSAStartup( wVersionRequested, &wsaData) == 0 )
  {
    bool ret = false;
    if(gethostname(name,255) == 0)
    {
      ret = true;
    }
    else
    {
      *name = 0;
    }
    WSACleanup();
    return ret;
  }
#else
  if(gethostname(name, 255) == 0)
  {
    return true;
  }
#endif
  *name = 0;
  return false;
}

char * System::StrTokR(char * str, const char * delim, char ** nextp)
{
#if 1
  char * ret;
  if (str == NULL)
  {
    str = *nextp;
  }
  str += strspn(str, delim);
  if (*str == '\0')
  {
    return NULL;
  }
  ret = str;
  str += strcspn(str, delim);
  if (*str)
  {
    *str++ = '\0';
  }
  *nextp = str;
  return ret;
#else
  return strtok_r(str,delim,nextp);
#endif
}

char *System::StrSep(char ** sp, const char * sep)
{
#if 1
  char *p, *s;
  if (sp == NULL || *sp == NULL || **sp == '\0') return NULL;
  s = *sp;
  p = s + strcspn(s, sep);
  if (*p != '\0') *p++ = '\0';
  *sp = p;
  return s;
#else
  return strsep(sp, sep);
#endif
}

struct CharsetAliasType
{
  const char * alias;
  const char * name;
};

#ifdef _WIN32
static const char * CharsetAliasToName(const char * alias)
{
  assert(alias);
  static CharsetAliasType aliases[] =
  {
    { "CP1252", "ISO-8859-1" },
    { NULL, NULL },
  };
  for(CharsetAliasType *a = aliases; a->alias; a++)
  {
    if (strcmp(a->alias, alias) == 0)
    {
      return a->name;
    }
  }
  mdcmWarningMacro("Could not find Charset");
  return NULL;
}
#endif

} // end namespace mdcm

