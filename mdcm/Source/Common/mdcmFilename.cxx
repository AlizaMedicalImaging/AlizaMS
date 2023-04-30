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
#include <climits>

namespace mdcm
{

const char *
Filename::GetFileName() const
{
  return FileName.c_str();
}

const char *
Filename::GetPath()
{
  std::string            fn = ToUnixSlashes();
  std::string::size_type slash_pos = fn.rfind('/');
  if (slash_pos != std::string::npos)
  {
    Path = fn.substr(0, slash_pos);
  }
  else
  {
    Path = "";
  }
  return Path.c_str();
}

const char *
Filename::GetName()
{
  std::string filename = FileName;
#if defined(_WIN32)
  std::string::size_type slash_pos = filename.find_last_of("/\\");
#else
  std::string::size_type slash_pos = filename.find_last_of('/');
#endif
  if (slash_pos != std::string::npos)
  {
    return &FileName[0] + slash_pos + 1;
  }
  return &FileName[0];
}

const char *
Filename::GetExtension()
{
  std::string            name = GetName();
  std::string::size_type dot_pos = name.rfind('.');
  if (dot_pos != std::string::npos)
  {
    return GetName() + dot_pos;
  }
  return nullptr;
}

const char *
Filename::ToUnixSlashes()
{
  Conversion = FileName;
  for (std::string::iterator it = Conversion.begin(); it != Conversion.end(); ++it)
  {
    if (*it == '\\')
    {
      *it = '/';
    }
  }
  return Conversion.c_str();
}

const char *
Filename::ToWindowsSlashes()
{
  Conversion = FileName;
  for (std::string::iterator it = Conversion.begin(); it != Conversion.end(); ++it)
  {
    if (*it == '/')
    {
      *it = '\\';
    }
  }
  return Conversion.c_str();
}

std::string
Filename::Join(const char * path, const char * filename)
{
  std::string s = path;
  s += '/';
  s += filename;
  return s;
}

bool
Filename::IsEmpty() const
{
  return FileName.empty();
}

bool
Filename::EndWith(const char ending[]) const
{
  if (!ending)
    return false;
  const char * str = FileName.c_str();
  size_t       str_len = FileName.size();
  size_t       ending_len = strlen(ending);
  if (ending_len > str_len)
    return false;
  return (0 == strncmp(str + str_len - ending_len, ending, ending_len));
}

} // end namespace mdcm
