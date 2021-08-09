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

#include "mdcmImageApplyLookupTable.h"
#include <limits>

namespace mdcm
{

bool
ImageApplyLookupTable::Apply()
{
  Output = Input;
  const Bitmap &            image = *Input;
  PhotometricInterpretation pi = image.GetPhotometricInterpretation();
  if (pi != PhotometricInterpretation::PALETTE_COLOR)
  {
    mdcmDebugMacro("Image is not palettized");
    return false;
  }
  const LookupTable &  lut = image.GetLUT();
  const unsigned short bitsample = lut.GetBitSample();
  if (bitsample == 0)
    return false;
  const unsigned long long len = image.GetBufferLength();
  if ((len * 3) > 0xffffffff)
  {
    mdcmAlwaysWarnMacro("ImageApplyLookupTable::Apply(): len = " << len);
    return false;
  }
  std::vector<char> v;
  v.resize(len);
  char * p = &v[0];
  image.GetBuffer(p);
  std::stringstream is;
  if (!is.write(p, len))
  {
    mdcmErrorMacro("Could not write to stringstream");
    return false;
  }
  DataElement &     de = Output->GetDataElement();
  std::vector<char> v2;
  v2.resize(len * 3);
  lut.Decode(&v2[0], v2.size(), &v[0], v.size());
  de.SetByteValue(&v2[0], (uint32_t)v2.size());
  Output->GetLUT().Clear();
  Output->SetPhotometricInterpretation(PhotometricInterpretation::RGB);
  Output->GetPixelFormat().SetSamplesPerPixel(3);
  // OT-PAL-8-face.dcm has a PlanarConfiguration while being PALETTE COLOR
  Output->SetPlanarConfiguration(0);
  const TransferSyntax & ts = image.GetTransferSyntax();
  if (ts.IsExplicit())
  {
    Output->SetTransferSyntax(TransferSyntax::ExplicitVRLittleEndian);
  }
  else
  {
    assert(ts.IsImplicit());
    Output->SetTransferSyntax(TransferSyntax::ImplicitVRLittleEndian);
  }
  return true;
}

} // end namespace mdcm
