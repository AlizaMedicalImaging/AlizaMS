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
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <chrono>
#include <sys/stat.h>

#ifdef _WIN32
#  include <windows.h>
#endif

#if defined(_WIN32) && (defined(_MSC_VER) || defined(__WATCOMC__) || defined(__BORLANDC__) || defined(__MINGW32__))
#  include <io.h>
#  include <direct.h>
#else
#  include <dlfcn.h>
#  include <sys/types.h>
#  include <fcntl.h>
#  include <unistd.h>
#  include <strings.h>
#endif

#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
namespace
{

std::wstring
utf8_decode(const std::string & str)
{
  if (str.empty())
    return std::wstring();
  const int    len = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
  std::wstring ret(len, 0);
  MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, &ret[0], len);
  return ret;
}

std::string
utf8_encode(const std::wstring & wstr)
{
  if (wstr.empty())
    return std::string();
  const int   len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
  std::string ret(len, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &ret[0], len, nullptr, nullptr);
  return ret;
}

// http://arsenmk.blogspot.com/2015/12/handling-long-paths-on-windows.html
bool
ComputeFullPath(const std::wstring & in, std::wstring & out)
{
  // consider an input fileName of type PCWSTR (const wchar_t*)
  const wchar_t * fileName = in.c_str();
  DWORD           requiredBufferLength = GetFullPathNameW(fileName, 0, nullptr, nullptr);
  if (0 == requiredBufferLength)
    return false;
  out.resize(requiredBufferLength);
  wchar_t * buffer = &out[0];
  DWORD     result = GetFullPathNameW(fileName, requiredBufferLength, buffer, nullptr);
  if (0 == result)
    return false;
  return true;
}

std::wstring
HandleMaxPath(const std::wstring & in)
{
  if (in.size() >= MAX_PATH)
  {
    std::wstring out;
    bool         ret = ComputeFullPath(in, out);
    if (!ret)
      return in;
    if (out.size() < 4)
      return in;
    if (out[0] == '\\' && out[1] == '\\' && out[2] == '?')
    {
      ;
    }
    else if (out[0] == '\\' && out[1] == '\\' && out[2] != '?')
    {
      // server path
      std::wstring prefix = LR"(\\?\UNC\)";
      out = prefix + (out.c_str() + 2);
    }
    else
    {
      // regular C:\ style path:
      assert(out[1] == ':');
      std::wstring prefix = LR"(\\?\)";
      out = prefix + out.c_str();
    }
    return out;
  }
  return in;
}

}
#endif

namespace mdcm
{

bool
System::FileExists(const char * filename)
{
#ifndef R_OK
#  define R_OK 04
#endif
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
  const std::wstring unc = System::ConvertToUtf16(filename);
  if (_waccess(unc.c_str(), R_OK) != 0)
#else
#  ifdef _WIN32
  if (_access(filename, R_OK) != 0)
#  else
  if (access(filename, R_OK) != 0)
#  endif
#endif
  {
    return false;
  }
  else
  {
    return true;
  }
}

bool
System::FileIsDirectory(const char * name)
{
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
  struct _stat64i32  fs;
  const std::wstring wname = System::ConvertToUtf16(name);
  if (_wstat(wname.c_str(), &fs) == 0)
#else
  struct stat fs;
  if (stat(name, &fs) == 0)
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

bool
System::FileIsSymlink(const char * name)
{
#ifdef _WIN32
  (void)name;
#else
  struct stat fs;
  if (lstat(name, &fs) == 0)
  {
    return S_ISLNK(fs.st_mode);
  }
#endif
  return false;
}

std::wstring
System::ConvertToUtf16(const char * utf8path)
{
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
  const std::wstring uft16path = utf8_decode(utf8path);
  std::wstring       uncpath = HandleMaxPath(uft16path);
  return uncpath;
#else
  (void)utf8path;
  return std::wstring();
#endif
}

// Return the filesize. 0 if file does not exist,
// use FileExists to differentiate between empty file and missing file.
// For very large size file and on system where size_t is not appropriate to store
// off_t value the function will return 0.
size_t
System::FileSize(const char * filename)
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
  const off_t  size = fs.st_size;
  const size_t size2 = size;
  // off_t can be larger than size_t
  if (size != static_cast<off_t>(size2))
    return 0;
  return size2;
}

/*
 Implementation note. We internally use mktime which seems to be quite relaxed when it
 comes to invalid date. It handles :
 "17890714172557";
 "19891714172557";
 "19890014172557";
 While the DICOM PS 3.5-2008 would prohibit them.
*/
bool
System::ParseDateTime(time_t & timep, const char date[22])
{
  long microseconds{};
  return ParseDateTime(timep, microseconds, date);
}

bool
System::ParseDateTime(time_t & timep, long & microseconds, const char date[22])
{
  if (!date)
    return false;
  size_t len = strlen(date);
  if (len < 4)
    return false;
  if (len > 21)
    return false;
  struct tm ptm;
  int year, mon, day, hour, min, sec, n;
  if ((n = sscanf(date, "%4d%2d%2d%2d%2d%2d", &year, &mon, &day, &hour, &min, &sec)) >= 1)
  {
    if (n == 1)
    {
      mon = 1;
      day = 1;
      hour = 0;
      min = 0;
      sec = 0;
    }
    else if (n == 2)
    {
      day = 1;
      hour = 0;
      min = 0;
      sec = 0;
    }
    else if (n == 3)
    {
      hour = 0;
      min = 0;
      sec = 0;
    }
    else if (n == 4)
    {
      min = 0;
      sec = 0;
    }
    else if (n == 5)
    {
      sec = 0;
    }
    ptm.tm_year = year - 1900;
    if (mon < 1 || mon > 12)
      return false;
    ptm.tm_mon = mon - 1;
    if (day < 1 || day > 31)
      return false;
    ptm.tm_mday = day;
    if (hour > 24)
      return false;
    ptm.tm_hour = hour;
    if (min > 60)
      return false;
    ptm.tm_min = min;
    if (sec > 60)
      return false;
    ptm.tm_sec = sec;
    ptm.tm_wday = -1;
    ptm.tm_yday = -1;
    ptm.tm_isdst = -1;
  }
  else
  {
    return false;
  }
  timep = mktime(&ptm);
  if (timep == static_cast<time_t>(-1))
    return false;
  if (len > 14)
  {
    const char * ptr = date + 14;
    if (*ptr != '.')
      return false;
    ++ptr;
    if (!*ptr || sscanf(ptr, "%06ld", &microseconds) != 1)
    {
      return false;
    }
  }
  return true;
}

bool
System::FormatDateTime(char date[22], time_t timep, long microseconds)
{
  // YYYYMMDDHHMMSS.FFFFFF&ZZXX
  if (!date)
  {
    return false;
  }
  const size_t maxsize = 40;
  char         tmp[maxsize];
  // Obtain the time of day, and convert it to a tm struct.
  struct tm * ptm = localtime(&timep);
  if (!ptm)
  {
    return false;
  }
  // Format the date and time, down to a single second.
  const size_t ret = strftime(tmp, sizeof(tmp), "%Y%m%d%H%M%S", ptm);
  assert(ret == 14);
  if (ret == 0 || ret >= maxsize)
  {
    return false;
  }
  // Add microseconds
  const size_t maxsizall = 22;
  const int    ret2 = snprintf(date, maxsizall, "%s.%06ld", tmp, microseconds);
  if (ret2 < 0)
  {
    return false;
  }
  if (static_cast<size_t>(ret2) >= maxsizall)
  {
    return false;
  }
  return true;
}

bool
System::GetCurrentDateTime(char date[22])
{
  const auto tp = std::chrono::system_clock::now();
  const auto s = std::chrono::time_point_cast<std::chrono::seconds>(tp);
  const auto us = std::chrono::time_point_cast<std::chrono::microseconds>(tp) -
                  std::chrono::time_point_cast<std::chrono::microseconds>(s);
  return FormatDateTime(date, std::chrono::system_clock::to_time_t(s), static_cast<long>(us.count()));
}

} // end namespace mdcm
