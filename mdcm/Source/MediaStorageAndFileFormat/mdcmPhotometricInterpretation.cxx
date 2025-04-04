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

// clang-format off
const char * const PIStrings[] =
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
  "XYB ",
  nullptr
};
constexpr unsigned int PIStrings_size = 15U;
// clang-format on

const char *
PhotometricInterpretation::GetPIString(PIType pi)
{
  return PIStrings[pi];
}

PhotometricInterpretation::PIType
PhotometricInterpretation::GetPIType(const char * inputpi)
{
  if (!inputpi)
    return PI_END;
  const CodeString codestring = inputpi;
  const CSComp     cs = codestring.GetAsString();
  const char *     pi = cs.c_str();
  for (unsigned int i = 1; i < PIStrings_size; ++i)
  {
    if (strcmp(pi, PIStrings[i]) == 0)
    {
      return PIType(i);
    }
  }
  // We did not find anything.
  // Warning: this piece of code will do MONOCHROME -> MONOCHROME1
  size_t len = strlen(pi);
  if (pi[len - 1] == ' ')
    len--;
  for (unsigned int i = 1; i < PIStrings_size; ++i)
  {
    if (strncmp(pi, PIStrings[i], len) == 0)
    {
      mdcmDebugMacro("PhotometricInterpretation [" << pi << "] may be not valid.");
      return PIType(i);
    }
  }
  return PI_END;
}

const char *
PhotometricInterpretation::GetString() const
{
  return PhotometricInterpretation::GetPIString(PIField);
}

unsigned short
PhotometricInterpretation::GetSamplesPerPixel() const
{
  if (PIField == UNKNOWN)
  {
    return 0;
  }
  else if (PIField == MONOCHROME1 || PIField == MONOCHROME2 || PIField == PALETTE_COLOR)
  {
    return 1;
  }
  else if (PIField == ARGB || PIField == CMYK)
  {
    return 4;
  }
  else
  {
    return 3;
  }
}

bool
PhotometricInterpretation::IsRetired(PIType pi)
{
  return (pi == HSV || pi == ARGB || pi == CMYK || pi == YBR_PARTIAL_422);
}

} // namespace mdcm
