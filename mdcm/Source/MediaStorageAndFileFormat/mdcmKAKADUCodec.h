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
#ifndef MDCMKAKADUCODEC_H
#define MDCMKAKADUCODEC_H

#include "mdcmImageCodec.h"

namespace mdcm
{

/**
 * KAKADUCodec
 */
class KAKADUCodec : public ImageCodec
{
public:
  KAKADUCodec();
  ~KAKADUCodec();
  bool CanDecode(TransferSyntax const &) const;
  bool CanCode(TransferSyntax const &) const;
  bool Decode(DataElement const &, DataElement &);
  bool Code(DataElement const &, DataElement &);
  virtual ImageCodec * Clone() const;
private:
};

} // end namespace mdcm

#endif //MDCMKAKADUCODEC_H
