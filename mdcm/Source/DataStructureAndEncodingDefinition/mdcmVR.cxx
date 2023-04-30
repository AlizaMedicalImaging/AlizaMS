/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// clang-format off

#include "mdcmVR.h"
#include <algorithm>
#include <cstring>

namespace mdcm
{

static const char * VRStrings[] = { "??",             //  0
                                    "AE",             //  1
                                    "AS",             //  2
                                    "AT",             //  3
                                    "CS",             //  4
                                    "DA",             //  5
                                    "DS",             //  6
                                    "DT",             //  7
                                    "FD",             //  8
                                    "FL",             //  9
                                    "IS",             // 10
                                    "LO",             // 11
                                    "LT",             // 12
                                    "OB",             // 13
                                    "OF",             // 14
                                    "OW",             // 15
                                    "PN",             // 16
                                    "SH",             // 17
                                    "SL",             // 18
                                    "SQ",             // 19
                                    "SS",             // 20
                                    "ST",             // 21
                                    "TM",             // 22
                                    "UI",             // 23
                                    "UL",             // 24
                                    "UN",             // 25
                                    "US",             // 26
                                    "UT",             // 27
                                    "OD",             // 28
                                    "OL",             // 29
                                    "UC",             // 30
                                    "UR",             // 31
                                    "OV",             // 32
                                    "SV",             // 33
                                    "UV",             // 34
                                    "OB or OW",       // 35
                                    "US or SS",       // 36
                                    "US or SS or OW", // 37
                                    "US or OW",       // 38
                                    nullptr };

static VR::VRType VRValue[] = { VR::INVALID, VR::AE, VR::AS, VR::AT, VR::CS, VR::DA, VR::DS, VR::DT, VR::FD,
                                VR::FL,      VR::IS, VR::LO, VR::LT, VR::OB, VR::OF, VR::OW, VR::PN, VR::SH,
                                VR::SL,      VR::SQ, VR::SS, VR::ST, VR::TM, VR::UI, VR::UL, VR::UN, VR::US,
                                VR::UT,      VR::OD, VR::OL, VR::UC, VR::UR, VR::OV, VR::SV, VR::UV };

bool
VR::IsVRFile() const
{
  switch (VRField)
  {
    case VR::INVALID:
    case VR::AE:
    case VR::AS:
    case VR::AT:
    case VR::CS:
    case VR::DA:
    case VR::DS:
    case VR::DT:
    case VR::FD:
    case VR::FL:
    case VR::IS:
    case VR::LO:
    case VR::LT:
    case VR::OB:
    case VR::OD:
    case VR::OF:
    case VR::OL:
    case VR::OV:
    case VR::OW:
    case VR::PN:
    case VR::SH:
    case VR::SL:
    case VR::SQ:
    case VR::SS:
    case VR::ST:
    case VR::SV:
    case VR::TM:
    case VR::UC:
    case VR::UI:
    case VR::UL:
    case VR::UN:
    case VR::UR:
    case VR::US:
    case VR::UT:
    case VR::UV:
      return true;
    default:
      break;
  }
  return false;
}

unsigned int
VR::GetSizeof() const
{
  unsigned int size;
  switch(VRField)
  {
  case VR::AE:
    size = sizeof(VRToType<VR::AE>::Type);
    break;
  case VR::AS:
    size = sizeof(VRToType<VR::AS>::Type);
    break;
  case VR::AT:
    size = sizeof(VRToType<VR::AT>::Type);
    break;
  case VR::CS:
    size = sizeof(VRToType<VR::CS>::Type);
    break;
  case VR::DA:
    size = sizeof(VRToType<VR::DA>::Type);
    break;
  case VR::DS:
    size = sizeof(VRToType<VR::DS>::Type);
    break;
  case VR::DT:
    size = sizeof(VRToType<VR::DT>::Type);
    break;
  case VR::FD:
    size = sizeof(VRToType<VR::FD>::Type);
    break;
  case VR::FL:
    size = sizeof(VRToType<VR::FL>::Type);
    break;
  case VR::IS:
    size = sizeof(VRToType<VR::IS>::Type);
    break;
  case VR::LO:
    size = sizeof(VRToType<VR::LO>::Type);
    break;
  case VR::LT:
    size = sizeof(VRToType<VR::LT>::Type);
    break;
  case VR::OB:
    size = sizeof(VRToType<VR::OB>::Type);
    break;
  case VR::OD:
    size = sizeof(VRToType<VR::OD>::Type);
    break;
  case VR::OF:
    size = sizeof(VRToType<VR::OF>::Type);
    break;
  case VR::OL:
    size = sizeof(VRToType<VR::OL>::Type);
    break;
  case VR::OV:
    size = sizeof(VRToType<VR::OV>::Type);
    break;
  case VR::OW:
    size = sizeof(VRToType<VR::OW>::Type);
    break;
  case VR::PN:
    size = sizeof(VRToType<VR::PN>::Type);
    break;
  case VR::SH:
    size = sizeof(VRToType<VR::SH>::Type);
    break;
  case VR::SL:
    size = sizeof(VRToType<VR::SL>::Type);
    break;
  case VR::SQ:
    size = sizeof(VRToType<VR::SQ>::Type);
    break;
  case VR::SS:
    size = sizeof(VRToType<VR::SS>::Type);
    break;
  case VR::ST:
    size = sizeof(VRToType<VR::ST>::Type);
    break;
  case VR::SV:
    size = sizeof(VRToType<VR::SV>::Type);
    break;
  case VR::TM:
    size = sizeof(VRToType<VR::TM>::Type);
    break;
  case VR::UC:
    size = sizeof(VRToType<VR::UC>::Type);
    break;
  case VR::UI:
    size = sizeof(VRToType<VR::UI>::Type);
    break;
  case VR::UL:
    size = sizeof(VRToType<VR::UL>::Type);
    break;
  case VR::UN:
    size = sizeof(VRToType<VR::UN>::Type);
    break;
  case VR::UR:
    size = sizeof(VRToType<VR::UR>::Type);
    break;
  case VR::US:
    size = sizeof(VRToType<VR::US>::Type);
    break;
  case VR::UT:
    size = sizeof(VRToType<VR::UT>::Type);
    break;
  case VR::UV:
    size = sizeof(VRToType<VR::UV>::Type);
    break;
  case VR::US_SS:
  case VR::US_SS_OW:
  case VR::US_OW:
    size = sizeof(short); //
    break;
  default:
    size = 0;
  }
  assert(size);
  return size;
}

unsigned int
VR::GetIndex(VRType vr)
{
  if (vr == VR::VL32)
    return 0;
  unsigned int l = 0;
  assert(vr <= VR::VR_END);
  switch (vr)
  {
    case INVALID:
      break;
    case OB_OW:
      l = 35;
      break;
    case US_SS:
      l = 36;
      break;
    case US_SS_OW:
      l = 37;
      break;
    case US_OW:
      l = 38;
      break;
    case VR_END:
      l = 39;
      break;
    default:
    {
      long long a = static_cast<long long>(vr);
      for (; a > 1; ++l)
        a >>= 1LL;
      l++;
    }
    break;
  }
  return l;
}

const char *
VR::GetVRString(VRType vr)
{
  const unsigned int idx = GetIndex(vr);
  return VRStrings[idx];
}

const char *
VR::GetVRStringFromFile(VRType vr)
{
  static const int N = sizeof(VRValue) / sizeof(VRType);
  static VRType *  start = VRValue;
  static VRType *  end = VRValue + N;
  const VRType *   p = std::lower_bound(start, end, vr);
  assert(*p == vr);
  assert((p - start) == static_cast<long long>(GetIndex(vr)));
  return VRStrings[p - start];
}

// Optimized version for transforming a read VR from DICOM file
// into a VRType (does not support OB_OW for instance)
VR::VRType
VR::GetVRTypeFromFile(const char * vr)
{
  VRType r = VR::VR_END;
  for (int i = 1; VRStrings[i] != nullptr; ++i)
  {
    const char * ref = VRStrings[i];
    if (ref[0] == vr[0] && ref[1] == vr[1])
    {
      r = static_cast<VRType>(1LL << (i - 1));
      break;
    }
  }
  if (r == VR::VR_END)
  {
    // FIXME Control Char LF/FF/CR TAB and ESC should be accepted
    if (vr[0] >= ' ' && vr[0] <= '~' && vr[1] >= ' ' && vr[1] <= '~')
    {
      return VR::UN;
    }
    return VR::INVALID;
  }
  assert(r != VR::INVALID && r != VR::OB_OW && r != VR::US_SS && r != VR::US_SS_OW && r != VR::US_OW &&
         r != VR::VR_END);
  return r;
}

VR::VRType
VR::GetVRType(const char * vr)
{
  VRType r = VR::VR_END;
  if (!vr)
    return r;
  for (int i = 0; VRStrings[i] != nullptr; ++i)
  {
    if (strcmp(VRStrings[i], vr) == 0)
    {
      switch (i)
      {
        case 0:
          r = VR::INVALID;
          break;
        case 35:
          r = VR::OB_OW;
          break;
        case 36:
          r = VR::US_SS;
          break;
        case 37:
          r = VR::US_SS_OW;
          break;
        case 38:
          r = VR::US_OW;
          break;
        case 39:
          r = VR::VR_END;
          break;
        default:
          assert(vr[2] == 0);
          r = static_cast<VRType>(1LL << (i - 1));
      }
      break;
    }
  }
  return r;
}

bool
VR::IsValid(const char * vr)
{
  for (int i = 1; VRStrings[i] != nullptr; ++i)
  {
    const char * ref = VRStrings[i];
    if (ref[0] == vr[0] && ref[1] == vr[1])
    {
      return true;
    }
  }
  return false;
}

bool
VR::IsValid(const char * vr1, VRType vr2)
{
  assert(strlen(vr1) == 2);
  VRType vr = GetVRType(vr1);
  return ((vr & vr2) != 0 ? true : false);
}

bool
VR::IsSwap(const char * vr)
{
  assert(vr[2] == '\0');
  char vr_swap[3];
  vr_swap[0] = vr[1];
  vr_swap[1] = vr[0];
  vr_swap[2] = '\0';
  assert(GetVRType(vr_swap) != VR::SS);
  return (GetVRType(vr_swap) != VR::VR_END);
}

#define VRTemplateCase(type, rep) \
  case VR::type: \
    return static_cast<VR::VRType>(VRToEncoding<VR::type>::Mode) \
       == VR::rep;

#define VRTemplate(rep) \
VRTemplateCase(UI,rep) \
VRTemplateCase(AE,rep) \
VRTemplateCase(AS,rep) \
VRTemplateCase(AT,rep) \
VRTemplateCase(CS,rep) \
VRTemplateCase(DA,rep) \
VRTemplateCase(DS,rep) \
VRTemplateCase(DT,rep) \
VRTemplateCase(FD,rep) \
VRTemplateCase(FL,rep) \
VRTemplateCase(IS,rep) \
VRTemplateCase(LO,rep) \
VRTemplateCase(LT,rep) \
VRTemplateCase(OB,rep) \
VRTemplateCase(OD,rep) \
VRTemplateCase(OF,rep) \
VRTemplateCase(OL,rep) \
VRTemplateCase(OV,rep) \
VRTemplateCase(OW,rep) \
VRTemplateCase(PN,rep) \
VRTemplateCase(SH,rep) \
VRTemplateCase(SL,rep) \
VRTemplateCase(SQ,rep) \
VRTemplateCase(SS,rep) \
VRTemplateCase(ST,rep) \
VRTemplateCase(SV,rep) \
VRTemplateCase(TM,rep) \
VRTemplateCase(UC,rep) \
VRTemplateCase(UL,rep) \
VRTemplateCase(UN,rep) \
VRTemplateCase(UR,rep) \
VRTemplateCase(US,rep) \
VRTemplateCase(UT,rep) \
VRTemplateCase(UV,rep)

bool
VR::IsASCII(VRType vr)
{
  switch (vr)
  {
    VRTemplate(VRASCII)
    default:
      return false;
  }
}

bool
VR::IsASCII2(VRType vr)
{
  return (vr == VR::AE ||
          vr == VR::AS ||
          vr == VR::CS ||
          vr == VR::DA ||
          vr == VR::DS ||
          vr == VR::DT ||
          vr == VR::IS ||
          vr == VR::LO ||
          vr == VR::LT ||
          vr == VR::PN ||
          vr == VR::SH ||
          vr == VR::ST ||
          vr == VR::TM ||
          vr == VR::UC ||
          vr == VR::UI ||
          vr == VR::UR ||
          vr == VR::UT);
}

bool
VR::IsBinary(VRType vr)
{
  switch (vr)
  {
    VRTemplate(VRBINARY)
    case VR::OB_OW:
    case VR::US_SS:
    case VR::US_SS_OW:
    case VR::US_OW:
      return true;
    default:
      return false;
  }
}

bool
VR::IsBinary2(VRType vr)
{
  return (!IsASCII2(vr));
}

bool
VR::Compatible(VR const & vr) const
{
  if (vr.VRField == VR::INVALID)
    return true;
  else if (vr.VRField == VR::UN)
    return true;
  return ((VRField & vr.VRField) > 0 ? true : false);
}

bool
VR::IsDual() const
{
  switch (VRField)
  {
    case VR::OB_OW:
    case VR::US_SS:
    case VR::US_SS_OW:
    case VR::US_OW:
      return true;
    default:
      return false;
  }
}

} // end of namespace mdcm

// clang-format on
