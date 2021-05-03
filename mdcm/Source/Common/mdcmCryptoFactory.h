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
#ifndef MDCMCRYPTOFACTORY_H
#define MDCMCRYPTOFACTORY_H

#include "mdcmCryptographicMessageSyntax.h"
#include "mdcmTrace.h"
#include <map>

namespace mdcm
{

class MDCM_EXPORT CryptoFactory
{
public:
  enum CryptoLib
  {
    DEFAULT = 0,
    OPENSSL = 1,
    CAPI = 2,
    OPENSSLP7 = 3
  };
  virtual CryptographicMessageSyntax *
                         CreateCMSProvider() = 0;
  static CryptoFactory * GetFactoryInstance(CryptoLib = DEFAULT);

protected:
  CryptoFactory(CryptoLib);
  CryptoFactory();
  ~CryptoFactory();

private:
  static std::map<CryptoLib, CryptoFactory *> &
  getInstanceMap();
  static void
  AddLib(CryptoLib, CryptoFactory *);
};

} // end namespace mdcm

#endif // MDCMCRYPTOFACTORY_H
