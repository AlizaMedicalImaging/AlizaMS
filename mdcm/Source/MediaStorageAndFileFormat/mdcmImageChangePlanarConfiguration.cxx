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

#include "mdcmImageChangePlanarConfiguration.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmSequenceOfItems.h"
#include "mdcmFragment.h"

namespace mdcm
{
/*
 * C.7.6.3.1.3 Planar Configuration
 * Note: Planar Configuration (0028,0006) is not meaningful when a compression transfer syntax is
 * used that involves reorganization of sample components in the compressed bit stream. In such
 * cases, since the Attribute is required to be sent, then an appropriate value to use may be
 * specified in the description of the Transfer Syntax in PS 3.5, though in all likelihood the value of
 * the Attribute will be ignored by the receiving implementation.
 */

bool
ImageChangePlanarConfiguration::Change()
{
  Output = Input;
  if (!(PlanarConfiguration == 0 || PlanarConfiguration == 1))
  {
    return false;
  }
  if (Input->GetPixelFormat().GetSamplesPerPixel() != 3)
  {
    return true;
  }
  if (Input->GetPlanarConfiguration() == PlanarConfiguration)
  {
    return true;
  }
  const Bitmap &       image = *Input;
  const unsigned int * dims = image.GetDimensions();
  unsigned long long   len = image.GetBufferLength();
  if (len > 0xffffffff)
  {
    mdcmAlwaysWarnMacro("ImageChangePlanarConfiguration::Change(): can not set length " << len);
    return false;
  }
  if (len % 3 != 0)
  {
    mdcmAlwaysWarnMacro("ImageChangePlanarConfiguration::Change(): wrong length " << len);
    return false;
  }
  //
  char * p;
  try
  {
    p = new char[len];
  }
  catch (const std::bad_alloc &)
  {
    return false;
  }
  image.GetBuffer(p);
  //
  const PixelFormat pf = Input->GetPixelFormat();
  const unsigned short bitsallocated = pf.GetBitsAllocated();
  const size_t ps = pf.GetPixelSize();
  const size_t framesize = dims[0] * dims[1] * ps;
  assert(framesize * dims[2] == len);
  //
  char * copy;
  try
  {
    copy = new char[len];
  }
  catch (const std::bad_alloc &)
  {
    delete[] p;
    return false;
  }
  const size_t size = framesize / 3;
  if (PlanarConfiguration == 0)
  {
    for (unsigned int z = 0; z < dims[2]; ++z)
    {
      const char * frame = p + z * framesize;
      const char * r = frame + 0;
      const char * g = frame + size;
      const char * b = frame + size + size;
      char *       framecopy = copy + z * framesize;
      const void * vr = static_cast<const void*>(r);
      const void * vg = static_cast<const void*>(g);
      const void * vb = static_cast<const void*>(b);
      void *       vframecopy = static_cast<void*>(framecopy);
      if (bitsallocated == 8)
      {
        ImageChangePlanarConfiguration::RGBPlanesToRGBPixels<uint8_t>(
          static_cast<uint8_t *>(vframecopy),
          static_cast<const uint8_t *>(vr),
          static_cast<const uint8_t *>(vg),
          static_cast<const uint8_t *>(vb),
          size);
      }
      else if (bitsallocated == 16)
      {
        if (size % 2 != 0)
        {
          delete[] p;
          delete[] copy;
          return false;
        }
        ImageChangePlanarConfiguration::RGBPlanesToRGBPixels<uint16_t>(
          static_cast<uint16_t *>(vframecopy),
          static_cast<const uint16_t *>(vr),
          static_cast<const uint16_t *>(vg),
          static_cast<const uint16_t *>(vb),
          size / 2);
      }
      else if (bitsallocated == 32)
      {
        if (size % 4 != 0)
        {
          delete[] p;
          delete[] copy;
          return false;
        }
        ImageChangePlanarConfiguration::RGBPlanesToRGBPixels<uint32_t>(
          static_cast<uint32_t *>(vframecopy),
          static_cast<const uint32_t *>(vr),
          static_cast<const uint32_t *>(vg),
          static_cast<const uint32_t *>(vb),
          size / 4);
      }
      else
      {
        delete[] p;
        delete[] copy;
        mdcmAlwaysWarnMacro("ImageChangePlanarConfiguration::Change(): not supported BitsAllocated "
                            << bitsallocated);
        return false;
      }
    }
  }
  else if (PlanarConfiguration == 1)
  {
    for (unsigned int z = 0; z < dims[2]; ++z)
    {
      const char * frame = p + z * framesize;
      char *       framecopy = copy + z * framesize;
      char *       r = framecopy + 0;
      char *       g = framecopy + size;
      char *       b = framecopy + size + size;
      void *       vr = static_cast<void*>(r);
      void *       vg = static_cast<void*>(g);
      void *       vb = static_cast<void*>(b);
      const void * vframe = static_cast<const void*>(frame);
      if (bitsallocated == 8)
      {
        ImageChangePlanarConfiguration::RGBPixelsToRGBPlanes<uint8_t>(
          static_cast<uint8_t *>(vr),
          static_cast<uint8_t *>(vg),
          static_cast<uint8_t *>(vb),
          static_cast<const uint8_t *>(vframe),
          size);
      }
      else if (bitsallocated == 16)
      {
        if (size % 2 != 0)
        {
          delete[] p;
          delete[] copy;
          return false;
        }
        ImageChangePlanarConfiguration::RGBPixelsToRGBPlanes<uint16_t>(
          static_cast<uint16_t *>(vr),
          static_cast<uint16_t *>(vg),
          static_cast<uint16_t *>(vb),
          static_cast<const uint16_t *>(vframe),
          size / 2);
      }
      else if (bitsallocated == 32)
      {
        if (size % 4 != 0)
        {
          delete[] p;
          delete[] copy;
          return false;
        }
        ImageChangePlanarConfiguration::RGBPixelsToRGBPlanes<uint32_t>(
          static_cast<uint32_t *>(vr),
          static_cast<uint32_t *>(vg),
          static_cast<uint32_t *>(vb),
          static_cast<const uint32_t *>(vframe),
          size / 4);
      }
      else
      {
        delete[] p;
        delete[] copy;
        mdcmAlwaysWarnMacro("ImageChangePlanarConfiguration::Change(): not supported BitsAllocated "
                            << bitsallocated);
        return false;
      }
    }
  }
  delete[] p;
  DataElement & de = Output->GetDataElement();
  de.SetByteValue(copy, static_cast<uint32_t>(len));
  delete[] copy;
  Output->SetPlanarConfiguration(PlanarConfiguration);
  if (Input->GetTransferSyntax().IsImplicit())
  {
    assert(Output->GetTransferSyntax().IsImplicit());
  }
  else if (Input->GetTransferSyntax() == TransferSyntax::ExplicitVRBigEndian)
  {
    Output->SetTransferSyntax(TransferSyntax::ExplicitVRBigEndian);
  }
  else
  {
    Output->SetTransferSyntax(TransferSyntax::ExplicitVRLittleEndian);
  }
  return true;
}

} // end namespace mdcm
