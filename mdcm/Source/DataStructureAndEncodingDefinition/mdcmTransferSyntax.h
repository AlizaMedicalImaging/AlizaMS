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

/**
 * Class to manipulate Transfer Syntax
 * TRANSFER SYNTAX (Standard and Private): A set of encoding rules that allow
 * Application Entities to unambiguously negotiate the encoding techniques
 * (e.g., Data Element structure, byte ordering, compression) they are able to
 * support, thereby allowing these Application Entities to communicate.
 *
 * Todo: The implementation is completely retarded -> see mdcm::UIDs for a replacement
 * We need: IsSupported
 * We need preprocess of raw/xml file
 * We need GetFullName()
 *
 * Need a notion of Private Syntax. As defined in PS 3.5. Section 9.2
 *
 */
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
    TS_END
  } TSType;
  static const char * GetTSString(TSType);
  static TSType GetTSType(const char *);
  NegociatedType GetNegociatedType() const;
  // Return the SwapCode associated with the Transfer Syntax. Be careful with
  // the special GE private syntax the DataSet is written in little endian but
  // the Pixel Data is in Big Endian.
  SwapCode GetSwapCode() const;
  bool IsValid() const { return TSField != TS_END; }
  operator TSType () const { return TSField; }
  TransferSyntax(TSType type = ImplicitVRLittleEndian) : TSField(type) {}
  bool IsEncoded() const;
  bool IsImplicit() const;
  bool IsExplicit() const;
  bool IsEncapsulated() const;
  bool IsLossy() const;
  bool IsLossless() const;
  bool CanStoreLossy() const;
  const char * GetString() const { return TransferSyntax::GetTSString(TSField); }
  friend std::ostream &operator<<(std::ostream &, const TransferSyntax &);

private:
  bool IsImplicit(TSType) const;
  bool IsExplicit(TSType) const;
  bool IsLittleEndian(TSType) const;
  bool IsBigEndian(TSType) const;
  TSType TSField;
};

inline std::ostream &operator<<(std::ostream & _os, const TransferSyntax & ts)
{
  _os << TransferSyntax::GetTSString(ts);
  return _os;
}

} // end namespace mdcm

#endif //MDCMTRANSFERSYNTAX_H
