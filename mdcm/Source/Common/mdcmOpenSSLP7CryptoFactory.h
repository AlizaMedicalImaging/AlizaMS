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
#ifndef MDCMOPENSSLP7CRYPTOFACTORY_H
#define MDCMOPENSSLP7CRYPTOFACTORY_H

#include "mdcmCryptoFactory.h"
#include "mdcmOpenSSLP7CryptographicMessageSyntax.h"

namespace mdcm
{
class MDCM_EXPORT OpenSSLP7CryptoFactory : public CryptoFactory
{
public:
  OpenSSLP7CryptoFactory(CryptoLib id) : CryptoFactory(id)
  {
    mdcmDebugMacro( "OpenSSL (PKCS7) Factory registered." );
  }
    
public:
  CryptographicMessageSyntax* CreateCMSProvider()
  {
    return new OpenSSLP7CryptographicMessageSyntax();
  }

private:
  OpenSSLP7CryptoFactory(){}
};
}

#endif //MDCMOPENSSLP7CRYPTOFACTORY_H
