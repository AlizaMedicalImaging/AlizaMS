/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef MDCMDECODER_H
#define MDCMDECODER_H

#include "mdcmTypes.h"
#include "mdcmDataElement.h"

namespace mdcm
{

class TransferSyntax;
class DataElement;
/**
 * \brief Decoder
 */
class MDCM_EXPORT Decoder
{
public:
  virtual ~Decoder() {}
  virtual bool CanDecode(TransferSyntax const &) const = 0;
  virtual bool Decode(DataElement const &, DataElement &) { return false; }
protected:
  virtual bool DecodeByStreams(std::istream &, std::ostream &) { return false; }
};

} // end namespace mdcm

#endif //MDCMDECODER_H
