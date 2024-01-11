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
#ifndef MDCMPVRGCODEC_H
#define MDCMPVRGCODEC_H

#include "mdcmImageCodec.h"

// TODO

namespace mdcm
{

class PVRGCodec : public ImageCodec
{
public:
  PVRGCodec();
  ~PVRGCodec() = default;
  bool
  CanDecode(TransferSyntax const &) const override;
  bool
  Decode(DataElement const &, DataElement &) override;
  bool
  Code(DataElement const &, DataElement &) override;
};

} // end namespace mdcm

#endif // MDCMPVRGCODEC_H
