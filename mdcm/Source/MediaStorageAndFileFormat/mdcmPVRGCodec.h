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

namespace mdcm
{

class PVRGCodec : public ImageCodec
{
public:
  PVRGCodec();
  ~PVRGCodec();
  bool CanDecode(TransferSyntax const &) const;
  bool CanCode(TransferSyntax const &) const;
  bool Decode(DataElement const &is, DataElement &);
  bool Code(DataElement const &, DataElement &);
  void SetLossyFlag(bool);
  virtual ImageCodec * Clone() const;
};

} // end namespace mdcm

#endif //MDCMPVRGCODEC_H
