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
#include "mdcmSHA1.h"
#include "mdcmSystem.h"
#ifdef MDCM_USE_SYSTEM_OPENSSL
#include <openssl/sha.h>
#endif
#include <cstring>
#include <cstdlib>
#include <cstdio>

namespace mdcm
{

class SHA1Internals
{
public:
#ifdef MDCM_USE_SYSTEM_OPENSSL
  SHA_CTX ctx;
#endif
};

SHA1::SHA1()
{
  Internals = new SHA1Internals;
}

SHA1::~SHA1()
{
  delete Internals;
}

bool SHA1::Compute(const char * buffer, unsigned long buf_len, char digest[])
{
#ifdef MDCM_USE_SYSTEM_OPENSSL
  if(!buffer || !buf_len) return false;
  unsigned char output[20];
  SHA_CTX ctx;
  SHA1_Init(&ctx);
  SHA1_Update(&ctx, buffer, buf_len);
  SHA1_Final(output, &ctx);
  for (int di = 0; di < 20; ++di)
  {
    sprintf(digest+2*di, "%02x", output[di]);
  }
  digest[2*20] = '\0';
  return true;
#else
  (void)buffer;
  (void)buf_len;
  (void)digest;
  return false;
#endif
}

#ifdef MDCM_USE_SYSTEM_OPENSSL
static bool process_file(const char * filename, unsigned char * digest)
{
  if(!filename || !digest) return false;
  FILE * file = fopen(filename, "rb");
  if(!file) return false;
  size_t file_size = System::FileSize(filename);
  void * buffer = malloc(file_size);
  if(!buffer)
  {
    fclose(file);
    return false;
  }
  size_t read = fread(buffer, 1, file_size, file);
  if(read != file_size) return false;
  SHA_CTX ctx;
  int ret = SHA1_Init(&ctx);
  if(!ret) return false;
  ret = SHA1_Update(&ctx, buffer, file_size);
  if(!ret) return false;
  ret = SHA1_Final(digest, &ctx);
  if(!ret) return false;
  free(buffer);
  fclose(file);
  return true;
}
#endif

bool SHA1::ComputeFile(const char * filename, char digest_str[20*2+1])
{
#ifdef MDCM_USE_SYSTEM_OPENSSL
  unsigned char digest[20];
  if(!process_file(filename, digest)) return false;
  for (int di = 0; di < 20; ++di)
  {
    sprintf(digest_str+2*di, "%02x", digest[di]);
  }
  digest_str[2*20] = '\0';
  return true;
#else
  (void)filename;
  (void)digest_str;
  return false;
#endif
}


} // end namespace mdcm
