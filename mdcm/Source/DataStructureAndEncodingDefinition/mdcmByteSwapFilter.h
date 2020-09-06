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
#ifndef MDCMBYTESWAPFILTER_H
#define MDCMBYTESWAPFILTER_H

#include "mdcmDataSet.h"

namespace mdcm
{

/**
 * \brief ByteSwapFilter
 * \details In place byte-swapping of a dataset
 */
class MDCM_EXPORT ByteSwapFilter
{
public:
  ByteSwapFilter(DataSet& ds) : DS(ds), ByteSwapTag(false) {}
  ~ByteSwapFilter();
  bool ByteSwap();
  void SetByteSwapTag(bool b) { ByteSwapTag = b; }

private:
  DataSet &DS;
  bool ByteSwapTag;
  ByteSwapFilter& operator=(const ByteSwapFilter &);
};

} // end namespace mdcm

#endif //MDCMBYTESWAPFILTER_H
