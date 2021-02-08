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
#ifndef MDCMPNMCODEC_H
#define MDCMPNMCODEC_H

#include "mdcmImageCodec.h"

namespace mdcm
{

/**
 * Class for PNM
 * PNM is the Portable anymap file format.
 * http://netpbm.sourceforge.net
 * Only support P5 & P6 PNM file (binary grayscale and binary rgb)
 */
class MDCM_EXPORT PNMCodec : public ImageCodec
{
public:
  PNMCodec();
  ~PNMCodec();
  bool CanDecode(TransferSyntax const &) const;
  bool CanCode(TransferSyntax const &) const;
  unsigned long long GetBufferLength() const { return BufferLength; }
  void SetBufferLength(unsigned long long l) { BufferLength = l; }
  bool GetHeaderInfo(std::istream &, TransferSyntax &);
  virtual ImageCodec * Clone() const;
  bool Read(const char *, DataElement &) const;
  bool Write(const char *, const DataElement &) const;

private:
  unsigned long long BufferLength;
};

} // end namespace mdcm

#endif //MDCMPNMCODEC_H
