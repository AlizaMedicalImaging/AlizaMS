/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "mdcmMD5.h"
#include "mdcmSystem.h"
#ifdef MDCM_USE_SYSTEM_OPENSSL
#  include <openssl/md5.h>
#endif
#include <fstream>
#include <vector>

// http://stackoverflow.com/questions/13256446/compute-md5-hash-value-by-c-winapi
namespace mdcm
{

bool
MD5::Compute(const char * buffer, size_t buf_len, char digest_str[33])
{
  if (!buffer || !buf_len)
    return false;
#ifdef MDCM_USE_SYSTEM_OPENSSL
  unsigned char digest[16];
  MD5_CTX       ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, buffer, buf_len);
  MD5_Final(digest, &ctx);
  for (int di = 0; di < 16; ++di)
  {
    sprintf(digest_str + 2 * di, "%02x", digest[di]);
  }
  digest_str[2 * 16] = '\0';
  return true;
#else
  (void)digest_str;
  return false;
#endif
}

#ifdef MDCM_USE_SYSTEM_OPENSSL
static bool
process_file(const char * filename, unsigned char * digest)
{
  if (!filename || !digest)
    return false;
  std::ifstream file(filename, std::ios::binary);
  if (!file)
    return false;
  const size_t      file_size = System::FileSize(filename);
  std::vector<char> v(file_size);
  char *            buffer = &v[0];
  file.read(buffer, file_size);
  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, buffer, file_size);
  MD5_Final(digest, &ctx);
  return true;
}
#else
static inline bool
process_file(const char *, unsigned char *)
{
  return false;
}
#endif

bool
MD5::ComputeFile(const char * filename, char digest_str[33])
{
#ifdef MDCM_USE_SYSTEM_OPENSSL
  unsigned char digest[16];
  if (!process_file(filename, digest))
    return false;
  for (int di = 0; di < 16; ++di)
  {
    sprintf(digest_str + 2 * di, "%02x", digest[di]);
  }
  digest_str[2 * 16] = '\0';
  return true;
#else
  (void)filename;
  (void)digest_str;
  return false;
#endif
}

} // end namespace mdcm
