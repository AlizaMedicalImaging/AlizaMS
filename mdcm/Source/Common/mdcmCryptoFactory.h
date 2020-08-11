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
#include <map>

namespace mdcm
{

class MDCM_EXPORT CryptoFactory
{
public:
  enum CryptoLib {DEFAULT = 0, OPENSSL = 1, CAPI = 2, OPENSSLP7 = 3};

  virtual CryptographicMessageSyntax* CreateCMSProvider() = 0;
  static CryptoFactory* GetFactoryInstance(CryptoLib id = DEFAULT);

protected:
  CryptoFactory(CryptoLib id)
  {
    AddLib(id, this);
  }

private:
  static std::map<CryptoLib, CryptoFactory*>& getInstanceMap()
  {
    static std::map<CryptoLib, CryptoFactory*> libs;
    return libs;
  }

  static void AddLib(CryptoLib id, CryptoFactory* f)
  {
    if (getInstanceMap().insert(std::pair<CryptoLib, CryptoFactory*>(id, f)).second == false)
      {
      mdcmErrorMacro( "Library already registered under id " << (int)id );
      }
  }

protected:
  CryptoFactory(){}
  ~CryptoFactory(){}
};

} // end namespace mdcm

#endif // MDCMCRYPTOFACTORY_H
