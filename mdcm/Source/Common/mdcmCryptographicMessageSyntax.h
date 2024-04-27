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
#ifndef MDCMCRYPTOGRAPHICMESSAGESYNTAX_H
#define MDCMCRYPTOGRAPHICMESSAGESYNTAX_H

#include "mdcmTypes.h"

namespace mdcm
{

class MDCM_EXPORT CryptographicMessageSyntax
{
public:
  CryptographicMessageSyntax() {}
  virtual ~CryptographicMessageSyntax() {}
  enum CipherTypes
  {
    DES3_CIPHER,   // Triple DES
    AES128_CIPHER, // CBC AES
    AES192_CIPHER, // '   '
    AES256_CIPHER  // '   '
  };
  // X.509
  virtual bool
  ParseCertificateFile(const char *) = 0;
  virtual bool
  ParseKeyFile(const char *) = 0;
  // PBE
  virtual bool
  SetPassword(const char *, size_t) = 0;
  // Create a CMS envelopedData structure
  virtual bool
  Encrypt(char *, size_t &, const char *, size_t) const = 0;
  // Decrypt content from a CMS envelopedData structure
  virtual bool
               Decrypt(char *, size_t &, const char *, size_t) const = 0;
  virtual void SetCipherType(CipherTypes) = 0;
  virtual CipherTypes
  GetCipherType() const = 0;

private:
  CryptographicMessageSyntax(const CryptographicMessageSyntax &); // Not implemented
  void
  operator=(const CryptographicMessageSyntax &); // Not implemented
};

} // end namespace mdcm

#endif // MDCMCRYPTOGRAPHICMESSAGESYNTAX_H
