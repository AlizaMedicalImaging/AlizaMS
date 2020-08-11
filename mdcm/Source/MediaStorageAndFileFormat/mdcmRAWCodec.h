/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMRAWCODEC_H
#define MDCMRAWCODEC_H

#include "mdcmImageCodec.h"

namespace mdcm
{

class RAWInternals;

class MDCM_EXPORT RAWCodec : public ImageCodec
{
public:
  RAWCodec();
  ~RAWCodec();
  bool CanCode(TransferSyntax const &) const;
  bool CanDecode(TransferSyntax const &) const;
  bool Decode(DataElement const & is, DataElement & os);
  bool Code(DataElement const & in, DataElement & out);
  bool GetHeaderInfo(std::istream &, TransferSyntax &);
  virtual ImageCodec * Clone() const;
  // Used by the ImageStreamReader-- converts a read in 
  // buffer into one with the proper encodings.
  bool DecodeBytes(
    const char * inBytes, size_t,
    char *      outBytes, size_t);

protected:
  bool DecodeByStreams(std::istream &, std::ostream &);

private:
  RAWInternals * Internals;
};

} // end namespace mdcm

#endif // MDCMRAWCODEC_H
