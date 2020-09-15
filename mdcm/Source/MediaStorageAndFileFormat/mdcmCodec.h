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
#ifndef MDCMCODEC_H
#define MDCMCODEC_H

#include "mdcmCoder.h"
#include "mdcmDecoder.h"

namespace mdcm
{

class MDCM_EXPORT Codec : public Coder, public Decoder
{
};

} // end namespace mdcm

#endif //MDCMCODEC_H
