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
  enum NegociatedType
  {
    Unknown = 0,
    Explicit,
    Implicit
  };

  enum TSType
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
    JPEGFullProgressionProcess11_13,
    JPEGLosslessProcess14,
    JPEGLosslessProcess15,
    JPEGExtendedProcess16_18,
    JPEGExtendedProcess17_19,
    JPEGSpectralSelectionProcess20_22,
    JPEGSpectralSelectionProcess21_23,
    JPEGFullProgressionProcess24_26,
    JPEGFullProgressionProcess25_27,
    JPEGLosslessProcess28,
    JPEGLosslessProcess29,
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
    JPIPReferencedDeflate,
    MPEG2MainProfileHighLevel,
    MPEG4AVCH264HighProfileLevel4_1,
    MPEG4AVCH264BDcompatibleHighProfileLevel4_1,
    EncapsulatedUncompressedExplicitVRLittleEndian,
    SMPTEST2110_20UncompressedProgressiveActiveVideo,
    SMPTEST2110_20UncompressedInterlacedActiveVideo,
    SMPTEST2110_30PCMDigitalAudio,
    MPEG4AVCH264HighProfileLevel42For2DVideo,
    MPEG4AVCH264HighProfileLevel42For3DVideo,
    MPEG4AVCH264StereoHighProfileLevel42,
    HEVCH265MainProfileLevel51,
    HEVCH265Main10ProfileLevel51,
    FragmentableMPEG2MainProfileMainLevel,
    FragmentableMPEG2MainProfileHighLevel,
    FragmentableMPEG4AVCH264HighProfileLevel4_1,
    FragmentableMPEG4AVCH264BDcompatibleHighProfileLevel4_1,
    FragmentableMPEG4AVCH264HighProfileLevel4_2For2DVideo,
    FragmentableMPEG4AVCH264HighProfileLevel4_2For3DVideo,
    FragmentableMPEG4AVCH264StereoHighProfileevel4_2,
    HTJ2KLossless,
    HTJ2KLosslessRPCL,
    HTJ2K,
    JPIPHTJ2KReferenced,
    JPIPHTJ2KReferencedDeflate,
    TS_END
  };

  TransferSyntax() = default;

  TransferSyntax(TSType type) : TSField(type)
  {}

  static const char * GetTSString(TSType);

  static TSType
  GetTSType(const char *);

  NegociatedType
  GetNegociatedType() const;

  // Caution with the GE private syntax, the dataset is written
  // little-endian, but the pixel data is big-endian.
  SwapCode
  GetSwapCode() const;

  bool
  IsValid() const
  {
    return TSField != TS_END;
  }

  operator TSType() const
  {
    return TSField;
  }


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
  TSType TSField{ImplicitVRLittleEndian};
};

inline std::ostream &
operator<<(std::ostream & _os, const TransferSyntax & ts)
{
  _os << TransferSyntax::GetTSString(ts);
  return _os;
}

} // end namespace mdcm

#endif // MDCMTRANSFERSYNTAX_H
