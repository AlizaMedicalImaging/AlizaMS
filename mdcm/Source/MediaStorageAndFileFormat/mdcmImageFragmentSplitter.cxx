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

#include "mdcmImageFragmentSplitter.h"
#include "mdcmSequenceOfFragments.h"

namespace mdcm
{

bool ImageFragmentSplitter::Split()
{
  Output = Input;
  const Bitmap & image = *Input;
  const unsigned int *dims = image.GetDimensions();
  if(dims[2] != 1)
  {
    mdcmDebugMacro("Cannot split a 3D image");
    return false;
  }
  const DataElement& pixeldata = image.GetDataElement();
  const SequenceOfFragments * sqf = pixeldata.GetSequenceOfFragments();
  if(!sqf)
  {
    mdcmDebugMacro("Cannot split a non-encapsulated syntax");
    return false;
  }
  if (sqf->GetNumberOfFragments() != 1)
  {
    mdcmDebugMacro("Case not handled (for now)");
    return false;
  }
  const Fragment& frag = sqf->GetFragment(0);
  const ByteValue * bv = frag.GetByteValue();
  const char * p = bv->GetPointer();
  unsigned long len = bv->GetLength();
  if(FragmentSizeMax > len && !Force)
  {
    return true;
  }
  if(FragmentSizeMax == 0)
  {
    mdcmDebugMacro("Need to set a real value for fragment size");
    return false;
  }
  unsigned long nfrags = len / FragmentSizeMax;
  unsigned long lastfrag = len % FragmentSizeMax;
  SmartPointer<SequenceOfFragments> sq = new SequenceOfFragments;
  for(unsigned long i = 0; i < nfrags; ++i)
  {
    Fragment splitfrag;
    splitfrag.SetByteValue(p + i * FragmentSizeMax, FragmentSizeMax);
    sq->AddFragment(splitfrag);
  }
  if(lastfrag)
  {
    Fragment splitfrag;
    splitfrag.SetByteValue(p + nfrags * FragmentSizeMax, (uint32_t)lastfrag);
    assert(nfrags * FragmentSizeMax + lastfrag == len);
    sq->AddFragment(splitfrag);
  }
  Output->GetDataElement().SetValue(*sq);
  bool success = true;
  return success;
}

void ImageFragmentSplitter::SetFragmentSizeMax(unsigned int fragsize)
{
/*
 * A.4 TRANSFER SYNTAXES FOR ENCAPSULATION OF ENCODED PIXEL DATA
 *
 * All items containing an encoded fragment shall be made of an even number of bytes
 * greater or equal to two. The last fragment of a frame may be padded, if necessary,
 * to meet the sequence item format requirements of the DICOM Standard.
 */
  FragmentSizeMax = fragsize;
  if(fragsize % 2)
  {
    FragmentSizeMax--;
  }
  if(fragsize < 2)
  {
    FragmentSizeMax = 2;
  }
  assert(FragmentSizeMax >= 2 && (FragmentSizeMax % 2) == 0);
}

} // end namespace mdcm
