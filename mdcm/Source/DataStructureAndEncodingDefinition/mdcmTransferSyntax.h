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
#ifndef MDCMTRANSFERSYNTAX_H
#define MDCMTRANSFERSYNTAX_H

#include "mdcmSwapCode.h"

namespace mdcm
{

class MDCM_EXPORT TransferSyntax
{
public:
  typedef enum
  {
    Unknown = 0,
    Explicit,
    Implicit
  } NegociatedType;

  typedef enum
  {
    ImplicitVRLittleEndian = 0,
    ImplicitVRBigEndianPrivateGE,
    ExplicitVRLittleEndian,
    DeflatedExplicitVRLittleEndian,
    ExplicitVRBigEndian,
    JPEGBaselineProcess1,
    JPEGExtendedProcess2_4,
    JPEGExtendedProcess3_5,
    JPEGSpectralSelectionProcess6_8,
    JPEGFullProgressionProcess10_12,
    JPEGLosslessProcess14,
    JPEGLosslessProcess14_1,
    JPEGLSLossless,
    JPEGLSNearLossless,
    JPEG2000Lossless,
    JPEG2000,
    JPEG2000Part2Lossless,
    JPEG2000Part2,
    RLELossless,
    MPEG2MainProfile,
    ImplicitVRBigEndianACRNEMA,
    WeirdPapryus,
    CT_private_ELE,
    JPIPReferenced,
    MPEG2MainProfileHighLevel,
    MPEG4AVCH264HighProfileLevel4_1,
    MPEG4AVCH264BDcompatibleHighProfileLevel4_1,
    //EncapsulatedUncompressedExplicitVRLittleEndian,
    TS_END
  } TSType;
  static const char * GetTSString(TSType);
  static TSType
  GetTSType(const char *);
  NegociatedType
  GetNegociatedType() const;
  // Caution with the GE private syntax: the dataset is written
  // little-endian, but the pixel data is big-endian.
  SwapCode
  GetSwapCode() const;
  bool
  IsValid() const
  {
    return TSField != TS_END;
  }
  operator TSType() const { return TSField; }
  TransferSyntax(TSType type = ImplicitVRLittleEndian)
    : TSField(type)
  {}
  bool
  IsEncoded() const;
  bool
  IsImplicit() const;
  bool
  IsExplicit() const;
  bool
  IsEncapsulated() const;
  bool
  IsLossy() const;
  const char *
  GetString() const
  {
    return TransferSyntax::GetTSString(TSField);
  }
  friend std::ostream &
  operator<<(std::ostream &, const TransferSyntax &);

private:
  bool IsImplicit(TSType) const;
  bool IsExplicit(TSType) const;
  bool IsLittleEndian(TSType) const;
  bool IsBigEndian(TSType) const;
  TSType TSField;
};

inline std::ostream &
operator<<(std::ostream & _os, const TransferSyntax & ts)
{
  _os << TransferSyntax::GetTSString(ts);
  return _os;
}

} // end namespace mdcm

#endif // MDCMTRANSFERSYNTAX_H
