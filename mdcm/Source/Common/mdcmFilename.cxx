/*********************************************************
 *
 * MDCM
 *
 * Modifications github.com/issakomi
 *
 *********************************************************/

/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "mdcmFilename.h"
#include <cstdlib>
#include <cstring>
#include <limits.h>

namespace mdcm
{

const char * Filename::GetPath()
{
  std::string fn = ToUnixSlashes();
  std::string::size_type slash_pos = fn.rfind("/");
  if(slash_pos != std::string::npos)
  {
    Path = fn.substr(0, slash_pos);
  }
  else
  {
    Path = "";
  }
  return Path.c_str();
}

const char * Filename::GetName()
{
  std::string filename = FileName;
#if defined(_WIN32)
  std::string::size_type slash_pos = filename.find_last_of("/\\");
#else
  std::string::size_type slash_pos = filename.find_last_of("/");
#endif
  if(slash_pos != std::string::npos)
  {
    return &FileName[0] + slash_pos + 1;
  }
  return &FileName[0];
}

const char * Filename::ToWindowsSlashes()
{
  Conversion = FileName;
  for (std::string::iterator it = Conversion.begin(); it != Conversion.end(); ++it)
  {
    if(*it == '/')
    {
      *it = '\\';
    }
  }
  return Conversion.c_str();
}

const char * Filename::ToUnixSlashes()
{
  Conversion = FileName;
  for (std::string::iterator it = Conversion.begin(); it != Conversion.end(); ++it)
  {
    if( *it == '\\' )
    {
      //assert(it+1 == Conversion.end() || *(it+1) != ' '); // is it an escaped space ?
      *it = '/';
    }
  }
  return Conversion.c_str();
}

#if defined(_WIN32) && (defined(_MSC_VER) || defined(__WATCOMC__) || defined(__BORLANDC__) || defined(__MINGW32__))
#include <windows.h>

inline void Realpath(const char * path, std::string & resolved_path)
{
  char * ptemp;
  char fullpath[MAX_PATH];
#if 1
  if(GetFullPathName(path, sizeof(fullpath), fullpath, &ptemp))
#else
  if(GetFullPathNameA(path, sizeof(fullpath), fullpath, &ptemp))
#endif
  {
    Filename fn(fullpath);
    resolved_path = fn.ToUnixSlashes();
  }
  else
  {
    resolved_path = "";
  }
}
#else
#if defined(PATH_MAX)
#define MDCM_FILENAME_MAXPATH PATH_MAX
#elif defined(MAXPATHLEN)
#define MDCM_FILENAME_MAXPATH MAXPATHLEN
#else
#define MDCM_FILENAME_MAXPATH 16384
#endif

inline void Realpath(const char * path, std::string & resolved_path)
{
  char resolved_name[MDCM_FILENAME_MAXPATH];

  char * ret = realpath(path, resolved_name);
  if(ret)
  {
    resolved_path = resolved_name;
  }
  else
  {
    resolved_path = "";
  }
}
#endif

const char * Filename::GetExtension()
{
  std::string name = GetName();
  std::string::size_type dot_pos = name.rfind(".");
  if(dot_pos != std::string::npos)
  {
    return GetName() + dot_pos;
  }

  return 0;
}

const char * Filename::Join(const char * path, const char *filename)
{
  static std::string s; // construction of local static object is not thread-safe
  s = path;
  s += '/';
  s += filename;
  return s.c_str();
}

bool Filename::EndWith(const char ending[]) const
{
  if( !ending ) return false;
  const char * str = FileName.c_str();
  size_t str_len = FileName.size();
  size_t ending_len = strlen(ending);
  if(ending_len > str_len) return false;
  return 0 == strncmp(str + str_len - ending_len, ending, ending_len);
}

} // end namespace mdcm
