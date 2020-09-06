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
#ifndef MDCMOPENSSLCRYPTOFACTORY_H
#define MDCMOPENSSLCRYPTOFACTORY_H

#include "mdcmCryptoFactory.h"
#include "mdcmOpenSSLCryptographicMessageSyntax.h"

namespace mdcm
{

class MDCM_EXPORT OpenSSLCryptoFactory : public CryptoFactory
{
public:
  OpenSSLCryptoFactory(CryptoLib id) : CryptoFactory(id)
  {
    mdcmDebugMacro( "OpenSSL Factory registered." );
  }
    
public:
  CryptographicMessageSyntax* CreateCMSProvider()
  {
    InitOpenSSL();
    return new OpenSSLCryptographicMessageSyntax();
  }

protected:
  void InitOpenSSL();

private:
  OpenSSLCryptoFactory(){}
};

} // end namespace mdcm

#endif //MDCMOPENSSLCRYPTOFACTORY_H
