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

#include "mdcmPhotometricInterpretation.h"
#include "mdcmTransferSyntax.h"
#include "mdcmTrace.h"
#include "mdcmCodeString.h"
#include "mdcmVR.h"
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace mdcm
{

/*
 *
 * HSV = Pixel data represent a color image described by hue, saturation, and value image planes.
 * The minimum sample value for each HSV plane represents a minimum value of each vector. This
 * value may be used only when Samples per Pixel (0028,0002) has a value of 3.
 *

 * This value may be used only when Samples per Pixel (0028,0002) has a value of 4.
 *
 * CMYK = Pixel data represent a color image described by cyan, magenta, yellow, and black image
 * planes. The minimum sample value for each CMYK plane represents a minimum intensity of the
 * color. This value may be used only when Samples per Pixel (0028,0002) has a value of 4.
 *
 */

const char * PIStrings[] =
{
  "UNKNOWN",
  "MONOCHROME1 ",
  "MONOCHROME2 ",
  "PALETTE COLOR ",
  "RGB ",
  "HSV ",
  "ARGB",
  "CMYK",
  "YBR_FULL",
  "YBR_FULL_422",
  "YBR_PARTIAL_422 ",
  "YBR_PARTIAL_420 ",
  "YBR_ICT ",
  "YBR_RCT ",
  NULL
};

const char * PhotometricInterpretation::GetPIString(PIType pi)
{
  return PIStrings[pi];
}

PhotometricInterpretation::PIType PhotometricInterpretation::GetPIType(const char *inputpi)
{
  if(!inputpi) return PI_END;
  // Handle whitespace and invalid padding
  CodeString codestring = inputpi;
  CSComp cs = codestring.GetAsString();
  const char * pi = cs.c_str();
  for(unsigned int i = 1; PIStrings[i] != 0; ++i)
  {
    if(strcmp(pi, PIStrings[i]) == 0)
    {
      return PIType(i);
    }
  }
  // We did not find anything,
  // warning this piece of code will do MONOCHROME -> MONOCHROME1
  static const unsigned int n = sizeof(PIStrings) / sizeof(*PIStrings) - 1;
  size_t len = strlen(pi);
  if(pi[len-1] == ' ') len--;
  for(unsigned int i = 1; i < n; ++i)
  {
    if(strncmp(pi, PIStrings[i], len) == 0)
    {
      mdcmDebugMacro("PhotometricInterpretation was found: [" << pi
        << "], but is invalid. It should be padded with a space");
      return PIType(i);
    }
  }
  return PI_END;
}

bool PhotometricInterpretation::IsRetired(PIType pi)
{
  return pi == HSV || pi == ARGB || pi == CMYK;
}

unsigned short PhotometricInterpretation::GetSamplesPerPixel() const
{
  if (PIField == UNKNOWN)
  {
    return 0;
  }
  else if(PIField == MONOCHROME1 || PIField == MONOCHROME2 || PIField == PALETTE_COLOR)
  {
    return 1;
  }
  else if(PIField == ARGB || PIField == CMYK)
  {
    return 4;
  }
  else
  {
    return 3;
  }
}

bool PhotometricInterpretation::IsLossy() const
{
  return !IsLossless();
}

bool PhotometricInterpretation::IsLossless() const
{
  switch (PIField)
  {
  case MONOCHROME1:
  case MONOCHROME2:
  case PALETTE_COLOR:
  case RGB:
  case HSV:
  case ARGB:
  case CMYK:
  case YBR_FULL: // FIXME 
  case YBR_RCT:
    return true;
  case YBR_FULL_422:
  case YBR_PARTIAL_422:
  case YBR_PARTIAL_420:
  case YBR_ICT:
    return false;
  default:
    return false;
  }
  return false;
}

const char *PhotometricInterpretation::GetString() const
{
  return PhotometricInterpretation::GetPIString(PIField);
}

/*
// FIXME style
bool PhotometricInterpretation::IsSameColorSpace(PhotometricInterpretation const &pi) const
{
  if(PIField == pi) return true;
  if(PIField == RGB || PIField == YBR_RCT || PIField == YBR_ICT)
  {
    if(pi == RGB || pi == YBR_RCT || pi == YBR_ICT) return true;
  }
  if(PIField == YBR_FULL || PIField == YBR_FULL_422)
  {
    if(pi == YBR_FULL || pi == YBR_FULL_422) return true;
  }
  return false;
}
*/

}
