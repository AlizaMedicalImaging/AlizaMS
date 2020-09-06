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

/*
 * PHILIPS_Gyroscan-12-Jpeg_Extended_Process_2_4.dcm
 */
class PVRGCodec : public ImageCodec
{
public:
  PVRGCodec();
  ~PVRGCodec();
  bool CanDecode(TransferSyntax const &ts) const;
  bool CanCode(TransferSyntax const &ts) const;

  bool Decode(DataElement const &is, DataElement &os);
  bool Code(DataElement const &in, DataElement &out);
  void SetLossyFlag( bool l );

  virtual ImageCodec * Clone() const;
private:
};

} // end namespace mdcm

#endif //MDCMPVRGCODEC_H
