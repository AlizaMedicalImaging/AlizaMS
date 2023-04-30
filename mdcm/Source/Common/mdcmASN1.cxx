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
#include "mdcmASN1.h"
#include "mdcmSystem.h"
#include <cstring>
#ifdef MDCM_USE_SYSTEM_OPENSSL
#  include <openssl/bio.h>
#  include <openssl/err.h>
#  include <openssl/evp.h>
#  include <openssl/hmac.h>
#  include <openssl/pem.h>
#  include <openssl/rand.h>
#  include <openssl/x509.h>
#endif
#include <fstream>

namespace mdcm
{

ASN1::ASN1() {}

ASN1::~ASN1() {}

bool
ASN1::ParseDumpFile(const char * filename)
{
  if (!filename)
    return false;
  std::ifstream is(filename, std::ios::binary);
  if (!is.good())
    return false;
  size_t length = System::FileSize(filename);
  char * str = new char[length];
  is.read(str, length);
  bool b = ParseDump(str, length);
  delete[] str;
  return b;
}

bool
ASN1::ParseDump(const char * array, size_t length)
{
#ifdef MDCM_USE_SYSTEM_OPENSSL
  // If length == 0 return ok. This is an empty element.
  if (!array)
    return !length;
  int   indent = 1; // 0 is not visually nice
  int   dump = 0;   // -1 => will print hex stuff
  BIO * out = nullptr;
  out = BIO_new(BIO_s_file());
  assert(out);
  BIO_set_fp(out, stdout, BIO_NOCLOSE | BIO_FP_TEXT);
  if (!ASN1_parse_dump(out, (const unsigned char *)array, length, indent, dump))
  {
    return false;
  }
  return true;
#else
  (void)array;
  (void)length;
  return false;
#endif
}

#ifdef MDCM_USE_SYSTEM_OPENSSL
static int
print_hex(unsigned char * buf, int len)
{
  int i = 0;
  int n = 0;
  for (; i < len; ++i)
  {
    if (n > 7)
    {
      printf("\n");
      n = 0;
    }
    printf("0x%02x, ", buf[i]);
    ++n;
  }
  printf("\n");
  return 0;
}
#endif


int
ASN1::TestPBKDF2()
{
#ifdef MDCM_USE_SYSTEM_OPENSSL
  const char    pass[] = "password";
  const char    salt[] = "12340000";
  int           ic = 1;
  unsigned char buf[1024];
  PKCS5_PBKDF2_HMAC_SHA1(pass, (int)strlen(pass), (const unsigned char *)salt, (int)strlen(salt), ic, 32 + 16, buf);
  printf("PKCS5_PBKDF2_HMAC_SHA1(\"%s\", \"%s\", %d)=\n", pass, salt, ic);
  print_hex(buf, 32 + 16);
  ic = 1;
  EVP_BytesToKey(EVP_aes_256_cbc(),
                 EVP_sha1(),
                 (const unsigned char *)salt,
                 (unsigned char *)pass,
                 (int)strlen(pass),
                 ic,
                 buf,
                 buf + 32);
  printf("EVP_BytesToKey(\"%s\", \"%s\", %d)=\n", pass, salt, ic);
  print_hex(buf, 32 + 16);
#endif
  return 0;
}

} // end namespace mdcm
