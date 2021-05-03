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
#ifndef MDCMOPENSSLP7CRYPTOGRAPHICMESSAGESYNTAX_H
#define MDCMOPENSSLP7CRYPTOGRAPHICMESSAGESYNTAX_H

#include "mdcmCryptographicMessageSyntax.h"
#include "mdcmTypes.h"

namespace mdcm
{

class CryptographicMessageSyntaxInternals;

/**
 * Class for CryptographicMessageSyntax encryption. This is just a simple
 * wrapper around openssl PKCS7_encrypt functionalities
 *
 * See online documentation
 * http://www.openssl.org/docs/crypto/PKCS7_encrypt.html
 *
 */
class MDCM_EXPORT OpenSSLP7CryptographicMessageSyntax : public CryptographicMessageSyntax
{
public:
  OpenSSLP7CryptographicMessageSyntax();
  ~OpenSSLP7CryptographicMessageSyntax();
  // X.509
  bool
  ParseCertificateFile(const char *) override;
  bool
  ParseKeyFile(const char *) override;
  // PBE
  bool
  SetPassword(const char *, size_t) override;
  // Set Cipher Type.
  // Default is: AES256_CIPHER
  void
  SetCipherType(CipherTypes type) override;
  CipherTypes
  GetCipherType() const override;
  // create a PKCS#7 envelopedData structure
  bool
  Encrypt(char *, size_t &, const char *, size_t) const override;
  // decrypt content from a PKCS#7 envelopedData structure
  bool
  Decrypt(char *, size_t &, const char *, size_t) const override;

private:
  CryptographicMessageSyntaxInternals * Internals;
  OpenSSLP7CryptographicMessageSyntax(const OpenSSLP7CryptographicMessageSyntax &); // Not implemented
  void
  operator=(const OpenSSLP7CryptographicMessageSyntax &); // Not implemented
};

} // end namespace mdcm

#endif // MDCMOPENSSLP7CRYPTOGRAPHICMESSAGESYNTAX_H
