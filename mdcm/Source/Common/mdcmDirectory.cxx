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
#include "mdcmDirectory.h"
#include "mdcmTrace.h"
#include <iterator>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <cstring>
#ifdef _MSC_VER
#include <windows.h>
#include <direct.h>
#else
#include <dirent.h>
#include <sys/types.h>
#endif

namespace mdcm
{

unsigned int Directory::Load(FilenameType const & name, bool recursive)
{
  Filenames.clear();
  Directories.clear();
  if(System::FileIsDirectory(name.c_str()))
  {
    Toplevel = name;
    return Explore(Toplevel, recursive);
  }
  return false;
}

#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
static std::string utf8_encode(std::wstring const & wstr)
{
  if(wstr.empty()) return std::string();
  const int len = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, NULL, 0, NULL, NULL);
  std::string ret(len, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, &ret[0], len, NULL, NULL);
  return ret;
}
#endif

unsigned int Directory::Explore(FilenameType const & name, bool recursive)
{
  unsigned int nFiles = 0;
#ifdef _WIN32
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
  std::wstring fileName;
  std::wstring dirName = System::ConvertToUNC(name.c_str());
  Directories.push_back(utf8_encode(dirName));
  WIN32_FIND_DATAW fileData;
  if(('\\' != dirName[dirName.size()-1]) && ('/' != dirName[dirName.size()-1]))
  {
    dirName.push_back('/'); // FIXME
  }
  const std::wstring firstfile = dirName+L"*";
  HANDLE hFile = FindFirstFileW(firstfile.c_str(), &fileData);
  for(BOOL b = (hFile != INVALID_HANDLE_VALUE); b; b = FindNextFileW(hFile, &fileData))
  {
    fileName = fileData.cFileName;
    if(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      // Check for . and .. to avoid infinite loop and discard hidden dir
      if(fileName != L"." && fileName != L".." && fileName[0] != '.' && recursive)
      {
        nFiles += Explore(utf8_encode(dirName + fileName), recursive);
      }
    }
    else
    {
      if(fileName[0] != '.') // discard UNIX hidden files
      {
        Filenames.push_back(utf8_encode(dirName+fileName));
        nFiles++;
      }
    }
  }
  DWORD dwError = GetLastError();
  if(hFile != INVALID_HANDLE_VALUE) FindClose(hFile);
  if(dwError != ERROR_NO_MORE_FILES)
  {
    return 0;
  }
#else
  std::string fileName;
  std::string dirName = name;
  Directories.push_back(dirName);
  WIN32_FIND_DATA fileData;
  if ('/' != dirName[dirName.size()-1])
  {
    dirName.push_back('/'); // FIXME
  }
  const FilenameType firstfile = dirName+"*";
  HANDLE hFile = FindFirstFile(firstfile.c_str(), &fileData);
  for(BOOL b = (hFile != INVALID_HANDLE_VALUE); b;
      b = FindNextFile(hFile, &fileData))
  {
    fileName = fileData.cFileName;
    if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      // check for . and .. to avoid infinite loop and discard any hidden dir
      if (fileName != "." && fileName != ".." && fileName[0] != '.' && recursive)
      {
        nFiles += Explore(dirName+fileName,recursive);
      }
    }
    else
    {
      Filenames.push_back(dirName+fileName);
      nFiles++;
    }
  }
  DWORD dwError = GetLastError();
  if (hFile != INVALID_HANDLE_VALUE) FindClose(hFile);
  if (dwError != ERROR_NO_MORE_FILES)
  {
    return 0;
  }
#endif
#else
  std::string fileName;
  std::string dirName = name;
  Directories.push_back(dirName);
  DIR* dir = opendir(dirName.c_str());
  if (!dir)
  {
    const char *str = strerror(errno);
    mdcmErrorMacro("Error: " << str << ", directory " << dirName);
    (void)str;
    return 0;
  }
  struct stat buf;
  dirent *d;
  if('/' != dirName[dirName.size()-1]) dirName.push_back('/');
  for(d = readdir(dir); d; d = readdir(dir))
  {
    fileName = dirName + d->d_name;
    if(stat(fileName.c_str(), &buf) != 0)
    {
      const char *str = strerror(errno);
      mdcmErrorMacro("Last error: " << str << " for file " << fileName);
      (void)str;
      break;
    }
    if(S_ISREG(buf.st_mode)) // is a regular file?
    {
      if(d->d_name[0] != '.')
      {
        Filenames.push_back(fileName);
        nFiles++;
      }
    }
    else if (S_ISDIR(buf.st_mode)) // directory?
    {
      // discard '.', '..' and hidden
      if(strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || d->d_name[0] == '.')
      {
        continue;
      }
      if(recursive)
      {
        nFiles += Explore(fileName, recursive);
      }
    }
    else
    {
      mdcmErrorMacro("Unexpected type: " << fileName);
      break;
    }
  }
  if(closedir(dir) != 0)
  {
    const char *str = strerror(errno);
    mdcmErrorMacro("Last error: " << str << " when closing directory " << fileName);
	(void)str;
  }
#endif
  return nFiles;
}

void Directory::Print(std::ostream & _os) const
{
  _os << "Directories: ";
  if(Directories.empty())
  {
    _os << "(None)" << std::endl;
  }
  else
  {
    std::copy(Directories.begin(), Directories.end(),
      std::ostream_iterator<std::string>(_os << std::endl, "\n"));
  }
  _os << "Filenames: ";
  if(Filenames.empty())
  {
     _os << "(None)" << std::endl;
  }
  else
  {
    std::copy(Filenames.begin(), Filenames.end(),
      std::ostream_iterator<std::string>(_os << std::endl, "\n"));
  }
}

} // end namespace mdcm

