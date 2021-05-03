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
#ifndef MDCMCAPICRYPTOGRAPHICMESSAGESYNTAX_H
#define MDCMCAPICRYPTOGRAPHICMESSAGESYNTAX_H

#include "mdcmCryptographicMessageSyntax.h"
#include <windows.h>
#include <wincrypt.h>
#include <vector>

namespace mdcm
{

class MDCM_EXPORT CAPICryptographicMessageSyntax : public CryptographicMessageSyntax
{
public:
  CAPICryptographicMessageSyntax();
  ~CAPICryptographicMessageSyntax();
  // X.509
  bool
  ParseCertificateFile(const char *) override;
  bool
  ParseKeyFile(const char *) override;
  // PBE
  bool
       SetPassword(const char *, size_t) override;
  void SetCipherType(CipherTypes) override;
  CipherTypes
  GetCipherType() const override;
  // Create a CMS envelopedData structure
  bool
  Encrypt(char *, size_t &, const char *, size_t) const override;
  // Decrypt content from a CMS envelopedData structure
  bool
  Decrypt(char *, size_t &, const char *, size_t) const override;
  bool
  GetInitialized() const
  {
    return initialized;
  }

private:
  bool
  Initialize();
  static ALG_ID
  GetAlgIdByObjId(const char *);
  const char *
  GetCipherObjId() const;
  static void
  ReverseBytes(BYTE * data, DWORD len);
  static bool
                              LoadFile(const char *, BYTE *&, DWORD &);
  bool                        initialized;
  HCRYPTPROV                  hProv;
  std::vector<PCCERT_CONTEXT> certifList;
  HCRYPTKEY                   hRsaPrivK;
  CipherTypes                 cipherType;
};

} // end namespace mdcm

#endif // MDCMCAPICRYPTOGRAPHICMESSAGESYNTAX_H
