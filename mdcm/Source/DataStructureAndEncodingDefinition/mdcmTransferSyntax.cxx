/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "mdcmTransferSyntax.h"
#include "mdcmTrace.h"
#include <cstring>
#include <string>
#include <iostream>

namespace mdcm
{

static const char * TSStrings[] = {
  "1.2.840.10008.1.2",          // Implicit VR Little Endian
  "1.2.840.113619.5.2",         // Implicit VR Big Endian DLX (G.E Private)
  "1.2.840.10008.1.2.1",        // Explicit VR Little Endian
  "1.2.840.10008.1.2.1.99",     // Deflated Explicit VR Little Endian
  "1.2.840.10008.1.2.2",        // Explicit VR Big Endian
  "1.2.840.10008.1.2.4.50",     // JPEG Baseline (Process 1)
  "1.2.840.10008.1.2.4.51",     // JPEG Extended (Process 2 & 4)
  "1.2.840.10008.1.2.4.52",     // JPEG Extended (Process 3 & 5)
  "1.2.840.10008.1.2.4.53",     // JPEG Spectral Selection, Non-Hierarchical (Process 6 & 8)
  "1.2.840.10008.1.2.4.55",     // JPEG Full Progression, Non-Hierarchical (Process 10 & 12)
  "1.2.840.10008.1.2.4.56",     // JPEG Full Progression, Nonhierarchical (Processes 11 & 13)
  "1.2.840.10008.1.2.4.57",     // JPEG Lossless, Non-Hierarchical (Process 14)
  "1.2.840.10008.1.2.4.58",     // JPEG Lossless, Non-Hierarchical (Process 15)
  "1.2.840.10008.1.2.4.59",     // JPEG Extended, Hierarchical (Process 16 & 18)
  "1.2.840.10008.1.2.4.60",     // JPEG Extended, Hierarchical (Process 17 & 19)
  "1.2.840.10008.1.2.4.61",     // JPEG Spectral Selection, Hierarchical (Process 20 & 22)
  "1.2.840.10008.1.2.4.62",     // JPEG Spectral Selection, Hierarchical (Process 21 & 23)
  "1.2.840.10008.1.2.4.63",     // JPEG Full Progression, Hierarchical (Process 24 & 26)
  "1.2.840.10008.1.2.4.64",     // JPEG Full Progression, Hierarchical (Process 25 & 27)
  "1.2.840.10008.1.2.4.65",     // JPEG Lossless, Hierarchical (Process 28)
  "1.2.840.10008.1.2.4.66",     // JPEG Lossless, Hierarchical (Process 29)
  "1.2.840.10008.1.2.4.70",     // JPEG Lossless, Non-Hierarchical, First-Order Prediction (Process 14, [Selection Value 1])
  "1.2.840.10008.1.2.4.80",     // JPEG-LS Lossless Image Compression
  "1.2.840.10008.1.2.4.81",     // JPEG-LS Lossy (Near-Lossless) Image Compression
  "1.2.840.10008.1.2.4.90",     // JPEG 2000 Lossless
  "1.2.840.10008.1.2.4.91",     // JPEG 2000
  "1.2.840.10008.1.2.4.92",     // JPEG 2000 Part 2 Lossless
  "1.2.840.10008.1.2.4.93",     // JPEG 2000 Part 2
  "1.2.840.10008.1.2.5",        // RLE Lossless
  "1.2.840.10008.1.2.4.100",    // MPEG2 Main Profile @ Main Level
  "ImplicitVRBigEndianACRNEMA", // Fake old ACR NEMA
  "1.2.840.10008.1.20",         // Weird Papyrus
  "1.3.46.670589.33.1.4.1",     // CT_private_ELE
  "1.2.840.10008.1.2.4.94",     // JPIP Referenced
  "1.2.840.10008.1.2.4.95",     // JPIP Referenced Deflate
  "1.2.840.10008.1.2.4.101",    // MPEG2 Main Profile @ High Level
  "1.2.840.10008.1.2.4.102",    // MPEG-4 AVC/H.264 High Profile / Level 4.1
  "1.2.840.10008.1.2.4.103",    // MPEG-4 AVC/H.264 BD-compatible High Profile / Level 4.1
  "1.2.840.10008.1.2.1.98",     // Encapsulated Uncompressed Explicit VR Little Endian
  "1.2.840.10008.1.2.7.1",      // SMPTE ST 2110-20 Uncompressed Progressive Active Video
  "1.2.840.10008.1.2.7.2",      // SMPTE ST 2110-20 Uncompressed Interlaced Active Video
  "1.2.840.10008.1.2.7.3",      // SMPTE ST 2110-30 PCM Digital Audio
  "1.2.840.10008.1.2.4.104",    // MPEG-4 AVC/H.264 High Profile / Level 4.2 For 2D Video
  "1.2.840.10008.1.2.4.105",    // MPEG-4 AVC/H.264 High Profile / Level 4.2 For 3D Video
  "1.2.840.10008.1.2.4.106",    // MPEG-4 AVC/H.264 Stereo High Profile / Level 4.2
  "1.2.840.10008.1.2.4.107",    // HEVC/H.265 Main Profile / Level 5.1
  "1.2.840.10008.1.2.4.108",    // HEVC/H.265 Main 10 Profile / Level 5.1
  "1.2.840.10008.1.2.4.100.1",  // Fragmentable MPEG2 Main Profile / Main Level
  "1.2.840.10008.1.2.4.101.1",  // Fragmentable MPEG2 Main Profile / High Level
  "1.2.840.10008.1.2.4.102.1",  // Fragmentable MPEG-4 AVC/H.264 High Profile / Level 4.1
  "1.2.840.10008.1.2.4.103.1",  // Fragmentable MPEG-4 AVC/H.264 BD-compatible High Profile / Level 4.1
  "1.2.840.10008.1.2.4.104.1",  // Fragmentable MPEG-4 AVC/H.264 High Profile / Level 4.2 For 2D Video
  "1.2.840.10008.1.2.4.105.1",  // Fragmentable MPEG-4 AVC/H.264 High Profile / Level 4.2 For 3D Video
  "1.2.840.10008.1.2.4.106.1",  // Fragmentable MPEG-4 AVC/H.264 Stereo High Profile / Level 4.2
  "1.2.840.10008.1.2.4.201",    // High-Throughput JPEG 2000 Image Compression (Lossless Only)
  "1.2.840.10008.1.2.4.202",    // High-Throughput JPEG 2000 with RPCL Options Image Compression (Lossless Only)
  "1.2.840.10008.1.2.4.203",    // High-Throughput JPEG 2000 Image Compression
  "1.2.840.10008.1.2.4.204",    // JPIP HTJ2K Referenced
  "1.2.840.10008.1.2.4.205",    // JPIP HTJ2K Referenced Deflate
  "Unknown Transfer Syntax",    // Unknown
  nullptr
};

TransferSyntax::TSType
TransferSyntax::GetTSType(const char * cstr)
{
  // trim trailing whitespace
  std::string            str = cstr;
  std::string::size_type notspace = str.find_last_not_of(' ') + 1;
  if (notspace != str.size())
  {
    mdcmDebugMacro("Header (TS) constains " << str.size() - notspace << " whitespace character(s)");
    str.erase(notspace);
  }
  int i = 0;
  while (TSStrings[i] != nullptr)
  {
    if (str == TSStrings[i])
      return static_cast<TSType>(i);
    ++i;
  }
  return TS_END;
}

const char *
TransferSyntax::GetTSString(TSType ts)
{
  assert(ts <= TS_END);
  return TSStrings[static_cast<unsigned int>(ts)];
}

bool
TransferSyntax::IsImplicit(TSType ts) const
{
  assert(ts != TS_END);
  return (ts == ImplicitVRLittleEndian || ts == ImplicitVRBigEndianACRNEMA || ts == ImplicitVRBigEndianPrivateGE
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
          || ts == WeirdPapryus
#endif
  );
}

bool
TransferSyntax::IsImplicit() const
{
  if (TSField == TS_END)
    return false;
  return (TSField == ImplicitVRLittleEndian || TSField == ImplicitVRBigEndianACRNEMA ||
          TSField == ImplicitVRBigEndianPrivateGE
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
          || TSField == WeirdPapryus
#endif
  );
}

bool
TransferSyntax::IsExplicit() const
{
  if (TSField == TS_END)
    return false;
  return !IsImplicit();
}

// clang-format off
bool
TransferSyntax::IsLossy() const
{
  if (TSField == JPEGBaselineProcess1 ||
      TSField == JPEGExtendedProcess2_4 ||
      TSField == JPEGExtendedProcess3_5 ||
      TSField == JPEGSpectralSelectionProcess6_8 ||
      TSField == JPEGFullProgressionProcess10_12 ||
      TSField == JPEGFullProgressionProcess11_13 ||
      TSField == JPEGExtendedProcess16_18 ||
      TSField == JPEGExtendedProcess17_19 ||
      TSField == JPEGSpectralSelectionProcess20_22 ||
      TSField == JPEGSpectralSelectionProcess21_23 ||
      TSField == JPEGFullProgressionProcess24_26 ||
      TSField == JPEGFullProgressionProcess25_27 ||
      TSField == JPEGLSNearLossless ||
      TSField == JPEG2000 ||
      TSField == JPEG2000Part2 ||
      TSField == HTJPEG2000 ||
      TSField == MPEG2MainProfile ||
      TSField == MPEG2MainProfileHighLevel ||
      TSField == MPEG4AVCH264HighProfileLevel4_1 ||
      TSField == MPEG4AVCH264BDcompatibleHighProfileLevel4_1 ||
      TSField == MPEG4AVCH264HighProfileLevel42For2DVideo ||
      TSField == MPEG4AVCH264HighProfileLevel42For3DVideo ||
      TSField == MPEG4AVCH264StereoHighProfileLevel42 ||
      TSField == HEVCH265MainProfileLevel51 ||
      TSField == HEVCH265Main10ProfileLevel51 ||
      TSField == SMPTEST2110_20UncompressedProgressiveActiveVideo ||
      TSField == SMPTEST2110_20UncompressedInterlacedActiveVideo ||
      TSField == SMPTEST2110_30PCMDigitalAudio ||
      TSField == FragmentableMPEG2MainProfileMainLevel ||
      TSField == FragmentableMPEG2MainProfileHighLevel ||
      TSField == FragmentableMPEG4AVCH264HighProfileLevel4_1 ||
      TSField == FragmentableMPEG4AVCH264BDcompatibleHighProfileLevel4_1 ||
      TSField == FragmentableMPEG4AVCH264HighProfileLevel4_2For2DVideo ||
      TSField == FragmentableMPEG4AVCH264HighProfileLevel4_2For3DVideo ||
      TSField == FragmentableMPEG4AVCH264StereoHighProfileevel4_2 ||
      TSField == JPIPReferenced ||
      TSField == JPIPReferencedDeflate ||
      TSField == JPIPHTReferenced ||
      TSField == JPIPHTReferencedDeflate)
  {
    return true;
  }
  return false;
}
// clang-format on

bool
TransferSyntax::IsExplicit(TSType ts) const
{
  assert(ts != TS_END);
  return !IsImplicit(ts);
}

TransferSyntax::NegociatedType
TransferSyntax::GetNegociatedType() const
{
  if (TSField == TS_END)
  {
    return TransferSyntax::Unknown;
  }
  else if (IsImplicit(TSField))
  {
    return TransferSyntax::Implicit;
  }
  return TransferSyntax::Explicit;
}

bool
TransferSyntax::IsLittleEndian(TSType ts) const
{
  assert(ts != TS_END);
  return !IsBigEndian(ts);
}

bool
TransferSyntax::IsBigEndian(TSType ts) const
{
  assert(ts != TS_END);
  return (ts == ExplicitVRBigEndian || ts == ImplicitVRBigEndianACRNEMA);
}

SwapCode
TransferSyntax::GetSwapCode() const
{
  assert(TSField != TS_END);
  if (IsBigEndian(TSField))
  {
    return SwapCode::BigEndian;
  }
  assert(IsLittleEndian(TSField));
  return SwapCode::LittleEndian;
}

bool
TransferSyntax::IsEncoded() const
{
  return TSField == DeflatedExplicitVRLittleEndian;
}

bool
TransferSyntax::IsEncapsulated() const
{
  bool r = false;
  switch (TSField)
  {
    case JPEGBaselineProcess1:
    case JPEGExtendedProcess2_4:
    case JPEGExtendedProcess3_5:
    case JPEGSpectralSelectionProcess6_8:
    case JPEGFullProgressionProcess10_12:
    case JPEGFullProgressionProcess11_13:
    case JPEGLosslessProcess14:
    case JPEGLosslessProcess15:
    case JPEGExtendedProcess16_18:
    case JPEGExtendedProcess17_19:
    case JPEGSpectralSelectionProcess20_22:
    case JPEGSpectralSelectionProcess21_23:
    case JPEGFullProgressionProcess24_26:
    case JPEGFullProgressionProcess25_27:
    case JPEGLosslessProcess28:
    case JPEGLosslessProcess29:
    case JPEGLosslessProcess14_1:
    case JPEGLSLossless:
    case JPEGLSNearLossless:
    case JPEG2000Lossless:
    case JPEG2000:
    case JPEG2000Part2Lossless:
    case JPEG2000Part2:
    case HTJPEG2000Lossless:
    case HTJPEG2000RPCLLossless:
    case HTJPEG2000:
    case RLELossless:
    case MPEG2MainProfile:
    case MPEG2MainProfileHighLevel:
    case MPEG4AVCH264HighProfileLevel4_1:
    case MPEG4AVCH264BDcompatibleHighProfileLevel4_1:
    case EncapsulatedUncompressedExplicitVRLittleEndian:
    case SMPTEST2110_20UncompressedProgressiveActiveVideo:
    case SMPTEST2110_20UncompressedInterlacedActiveVideo:
    case SMPTEST2110_30PCMDigitalAudio:
    case MPEG4AVCH264HighProfileLevel42For2DVideo:
    case MPEG4AVCH264HighProfileLevel42For3DVideo:
    case MPEG4AVCH264StereoHighProfileLevel42:
    case HEVCH265MainProfileLevel51:
    case HEVCH265Main10ProfileLevel51:
    case FragmentableMPEG2MainProfileMainLevel:
    case FragmentableMPEG2MainProfileHighLevel:
    case FragmentableMPEG4AVCH264HighProfileLevel4_1:
    case FragmentableMPEG4AVCH264BDcompatibleHighProfileLevel4_1:
    case FragmentableMPEG4AVCH264HighProfileLevel4_2For2DVideo:
    case FragmentableMPEG4AVCH264HighProfileLevel4_2For3DVideo:
    case FragmentableMPEG4AVCH264StereoHighProfileevel4_2:
    case JPIPReferenced:
    case JPIPReferencedDeflate:
    case JPIPHTReferenced:
    case JPIPHTReferencedDeflate:
      r = true;
      break;
    default:
      break;
  }
  return r;
}

} // end namespace mdcm
