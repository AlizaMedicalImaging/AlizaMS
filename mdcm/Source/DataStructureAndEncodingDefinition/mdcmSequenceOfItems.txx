/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMSEQUENCEOFITEMS_TXX
#define MDCMSEQUENCEOFITEMS_TXX

namespace mdcm_ns
{

template <typename TDE>
VL SequenceOfItems::ComputeLength() const
{
  typename ItemVector::const_iterator it = Items.begin();
  VL length = 0;
  for(;it != Items.end(); ++it)
  {
    length += it->template GetLength<TDE>();
  }
  if(SequenceLengthField.IsUndefined())
  {
    length += 8; // item end delimitor (tag + vl)
  }
  return length;
}

}

#endif // MDCMSEQUENCEOFITEMS_TXX
