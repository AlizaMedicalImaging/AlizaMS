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

#include "mdcmFileDecompressLookupTable.h"
#include "mdcmDataElement.h"
#include "mdcmAttribute.h"
#include <cstring>
#include <limits>

namespace mdcm
{

bool FileDecompressLookupTable::Change()
{
  DataSet & ds = F->GetDataSet();
  PixelFormat pf = PixelData->GetPixelFormat();
  PhotometricInterpretation pi = PixelData->GetPhotometricInterpretation();
  if (pi == PhotometricInterpretation::PALETTE_COLOR)
  {
    const LookupTable & lut = PixelData->GetLUT();
    assert(lut.Initialized());
    unsigned short length, subscript, bitsize;
    unsigned short rawlut8[256];
    unsigned short rawlut16[65536];
    unsigned short * rawlut = rawlut8;
    unsigned int lutlen = 256;
    if(pf.GetBitsAllocated() == 16)
    {
      rawlut = rawlut16;
      lutlen = 65536;
    }
    unsigned int l;
    // RED
    memset(rawlut, 0, lutlen*2);
    lut.GetLUT(LookupTable::RED, (unsigned char*)rawlut, l);
    DataElement redde(Tag(0x0028, 0x1201));
    redde.SetVR(VR::OW);
    redde.SetByteValue((char*)rawlut, l);
    ds.Replace(redde);
    // Descriptor
    Attribute<0x0028, 0x1101, VR::US, VM::VM3> reddesc;
    lut.GetLUTDescriptor(LookupTable::RED, length, subscript, bitsize);
    reddesc.SetValue(length, 0);
    reddesc.SetValue(subscript, 1);
    reddesc.SetValue(bitsize, 2);
    ds.Replace(reddesc.GetAsDataElement());
    // GREEN
    memset(rawlut, 0, lutlen*2);
    lut.GetLUT(LookupTable::GREEN, (unsigned char*)rawlut, l);
    DataElement greende(Tag(0x0028, 0x1202));
    greende.SetVR(VR::OW);
    greende.SetByteValue((char*)rawlut, l);
    ds.Replace(greende);
    // Descriptor
    Attribute<0x0028, 0x1102, VR::US, VM::VM3> greendesc;
    lut.GetLUTDescriptor(LookupTable::GREEN, length, subscript, bitsize);
    greendesc.SetValue(length, 0);
    greendesc.SetValue(subscript, 1);
    greendesc.SetValue(bitsize,2);
    ds.Replace(greendesc.GetAsDataElement());
    // BLUE
    memset(rawlut, 0, lutlen*2);
    lut.GetLUT(LookupTable::BLUE, (unsigned char*)rawlut, l);
    DataElement bluede(Tag(0x0028, 0x1203));
    bluede.SetVR(VR::OW);
    bluede.SetByteValue((char*)rawlut, l);
    ds.Replace(bluede);
    // Descriptor
    Attribute<0x0028, 0x1103, VR::US, VM::VM3> bluedesc;
    lut.GetLUTDescriptor(LookupTable::BLUE, length, subscript, bitsize);
    bluedesc.SetValue(length, 0);
    bluedesc.SetValue(subscript, 1);
    bluedesc.SetValue(bitsize, 2);
    ds.Replace(bluedesc.GetAsDataElement());
    ds.Remove(Tag(0x0028, 0x1221));
    ds.Remove(Tag(0x0028, 0x1222));
    ds.Remove(Tag(0x0028, 0x1223));
    return true;
  }
  return false;
}

void FileDecompressLookupTable::SetFile(const File & f)
{
  F = f;
}

File & FileDecompressLookupTable::GetFile()
{
  return *F;
}

const Pixmap & FileDecompressLookupTable::GetPixmap() const
{
  return *PixelData;
}

Pixmap & FileDecompressLookupTable::GetPixmap()
{
  return *PixelData;
}

void FileDecompressLookupTable::SetPixmap(Pixmap const & img)
{
  PixelData = img;
}

} // end namespace mdcm
