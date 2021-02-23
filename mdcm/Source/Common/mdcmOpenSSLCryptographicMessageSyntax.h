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
#ifndef MDCMOPENSSLCRYPTOGRAPHICMESSAGESYNTAX_H
#define MDCMOPENSSLCRYPTOGRAPHICMESSAGESYNTAX_H

#include "mdcmCryptographicMessageSyntax.h"
#include <openssl/cms.h>
#include <openssl/evp.h>

namespace mdcm
{

class MDCM_EXPORT OpenSSLCryptographicMessageSyntax : public CryptographicMessageSyntax
{
public:
  OpenSSLCryptographicMessageSyntax();
  ~OpenSSLCryptographicMessageSyntax();
  // X.509
  bool ParseCertificateFile(const char *) override;
  bool ParseKeyFile(const char *) override;
  // PBE
  bool SetPassword(const char *, size_t) override;
  // Set Cipher Type.
  // Default is: AES256_CIPHER
  void SetCipherType(CipherTypes) override;
  CipherTypes GetCipherType() const override;
  // create a CMS envelopedData structure
  bool Encrypt(char *, size_t &, const char *, size_t) const override;
  // decrypt content from a PKCS#7 envelopedData structure
  bool Decrypt(char *t, size_t &, const char *, size_t) const override;
private:
//#ifdef MDCM_HAVE_CMS_RECIPIENT_PASSWORD
//  ::stack_st_X509 *recips;
//#else
  STACK_OF(X509) * recips;
//#endif
  ::EVP_PKEY * pkey;
  const EVP_CIPHER * internalCipherType;
  char * password;
  size_t passwordLength;
  CipherTypes cipherType;
  OpenSSLCryptographicMessageSyntax(const OpenSSLCryptographicMessageSyntax&);  // Not implemented.
  void operator=(const OpenSSLCryptographicMessageSyntax&);  // Not implemented.
  const EVP_CIPHER *CreateCipher( CryptographicMessageSyntax::CipherTypes ciphertype);
};

} // end namespace mdcm

#endif //MDCMOPENSSLCRYPTOGRAPHICMESSAGESYNTAX_H
