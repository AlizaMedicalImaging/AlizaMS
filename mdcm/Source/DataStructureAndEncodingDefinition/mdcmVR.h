/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMVR_H
#define MDCMVR_H

#include "mdcmTag.h"
#include "mdcmTrace.h"
#include "mdcmString.h"
#include <iostream>
#include <fstream>
#include <assert.h>

// to ensure compilation on sunos gcc
#ifdef CS
#undef CS
#endif
#ifdef DS
#undef DS
#endif
#ifdef SS
#undef SS
#endif

namespace mdcm
{

/**
 * \brief VR class
 * \details This is adapted from DICOM standard
 * The biggest difference is the INVALID VR
 * and the composite one that differ from standard (more like an addition)
 * This allow us to represent all the possible case express in the DICOMV3 dict
 * \note
 * VALUE REPRESENTATION (VR)
 * Specifies the data type and format of the Value(s) contained in the
 * Value Field of a Data Element.
 * VALUE REPRESENTATION FIELD:
 * The field where the Value Representation of a Data Element is
 * stored in the encoding of a Data Element structure with explicit VR.
 */

class MDCM_EXPORT VR
{
public:
  typedef enum:long long
  {
    INVALID =     0,
    AE =          1, // 2^0
    AS =          2, // 2^1
    AT =          4, // 2^2
    CS =          8, // 2^3
    DA =         16, // 2^4
    DS =         32, // 2^5
    DT =         64, // 2^6
    FD =        128, // 2^7
    FL =        256, // 2^8
    IS =        512, // 2^9
    LO =       1024, // 2^10
    LT =       2048, // 2^11
    OB =       4096, // 2^12
    OF =       8192, // 2^13
    OW =      16384, // 2^14
    PN =      32768, // 2^15
    SH =      65536, // 2^16
    SL =     131072, // 2^17
    SQ =     262144, // 2^18
    SS =     524288, // 2^19
    ST =    1048576, // 2^20
    TM =    2097152, // 2^21
    UI =    4194304, // 2^22
    UL =    8388608, // 2^23
    UN =   16777216, // 2^24
    US =   33554432, // 2^25
    UT =   67108864, // 2^26
    OD =  134217728, // 2^27
    OL =  268435456, // 2^28
    UC =  536870912, // 2^29
    UR = 1073741824, // 2^30
    OV = 2147483648, // 2^31
    SV = 4294967296, // 2^32
    UV = 8589934592, // 2^33
    //
    VR_END = UV+1,
    //
    OB_OW    = OB|OW,
    US_SS    = US|SS,
    US_SS_OW = US|SS|OW,
    US_OW    = US|OW,
    //
    VL16     = AE|AS|AT|CS|DA|DS|DT|FD|FL|IS|LO|LT|PN|SH|SL|SS|ST|TM|UI|UL|US,
    VL32     = OB|OW|OD|OF|OL|OV|SQ|SV|UC|UN|UR|UT|UV,
    VRASCII  = AE|AS|CS|DA|DS|DT|IS|LO|LT|PN|SH|ST|TM|UC|UI|UR|UT,
    VRBINARY = AT|FL|FD|OB|OD|OF|OL|OV|OW|SL|SQ|SS|SV|UL|UN|US|UV,
    VR_VM1   = AS|LT|ST|UT|SQ|OF|OL|OV|OD|OW|OB|UN,
    VRALL    = VRASCII|VRBINARY
  } VRType;

  static const char * GetVRString(VRType vr);
  // This function will only look at the very first two chars nothing else
  static VRType GetVRTypeFromFile(const char *vr);
  static VRType GetVRType(const char * vr);
  static const char * GetVRStringFromFile(VRType vr);
  static bool IsValid(const char * vr);
  static bool IsValid(const char * vr1, VRType vr2);
  static bool IsSwap (const char * vr);
  unsigned int GetLength() const
  {
    return GetLength(VRField);
  }
  unsigned int GetSizeof() const;
  static unsigned int GetLength(VRType vr)
  {
    if (vr & VL32) return 4;
    return 2;
  }
  static bool IsBinary(VRType vr);
  static bool IsASCII(VRType vr);
  static bool CanDisplay(VRType vr);
  static bool IsBinary2(VRType vr);
  static bool IsASCII2(VRType vr);
  VR(VRType vr = INVALID):VRField(vr) {}
  std::istream &Read(std::istream &is)
  {
    char vr[2];
    is.read(vr, 2);
    VRField = GetVRTypeFromFile(vr);
    assert(VRField != VR_END);
    if (VRField == INVALID)
    {
#ifndef MDCM_DONT_THROW
      throw Exception("INVALID VR");
#endif
    }
    if (VRField & VL32)
    {
      char dum[2];
      is.read(dum, 2);
      if(!(dum[0] == 0 && dum[1] == 0))
      {
        mdcmDebugMacro("32 bits VR contains non zero bytes. Skipped");
      }
    }
    return is;
  }

  const std::ostream &Write(std::ostream &os) const
  {
    VRType vrfield = VRField;
    mdcmAssertAlwaysMacro( !IsDual() );
    const char *vr = GetVRString(vrfield);
    assert( vr[0] && vr[1] && vr[2] == 0 );
    os.write(vr, 2);
    // See PS 3.5, Data Element Structure With Explicit VR
    if (vrfield & VL32)
    {
      const char dum[2] = {0, 0};
      os.write(dum,2);
    }
    return os;
  }
  friend std::ostream &operator<<(std::ostream &os, const VR &vr);
  operator VRType () const { return VRField; }
  unsigned int GetSize() const;
  bool Compatible(VR const &vr) const;
  bool IsVRFile() const;
  bool IsDual() const;

private:
  static unsigned int GetIndex(VRType vr);
  VRType VRField;
};

inline std::ostream &operator<<(std::ostream &_os, const VR &val)
{
  _os << VR::GetVRString(val.VRField);
  return _os;
}

template<long long T> struct VRToEncoding;
template<long long T> struct VRToType;
#define TYPETOENCODING(type,rep, rtype)    \
  template<> struct VRToEncoding<VR::type> \
  { enum:long long { Mode = VR::rep }; };  \
  template<> struct VRToType<VR::type>     \
  { typedef rtype Type; };


// Do not use
struct UI
{
  char Internal[64+1];
  friend std::ostream& operator<<(std::ostream &_os, const UI &_val);
};
inline std::ostream& operator<<(std::ostream &_os, const UI &_val)
{
  _os << _val.Internal;
  return _os;
}

typedef String<'\\',16>         AEComp;
typedef String<'\\',64>         ASComp;
typedef String<'\\',16>         CSComp;
typedef String<'\\',64>         DAComp;
typedef String<'\\',64>         DTComp;
typedef String<'\\',64>         LOComp;
typedef String<'\\',64>         LTComp;
typedef String<'\\',64>         PNComp;
typedef String<'\\',64>         SHComp;
typedef String<'\\',64>         STComp;
typedef String<'\\',4294967294> UCComp;
typedef String<'\\',4294967294> URComp;
typedef String<'\\',16>         TMComp;
typedef String<'\\',64,0>       UIComp;
typedef String<'\\',64>         UTComp;

TYPETOENCODING(AE, VRASCII,  AEComp)
TYPETOENCODING(AS, VRASCII,  ASComp)
TYPETOENCODING(AT, VRBINARY, Tag)
TYPETOENCODING(CS, VRASCII,  CSComp)
TYPETOENCODING(DA, VRASCII,  DAComp)
TYPETOENCODING(DS, VRASCII,  double)
TYPETOENCODING(DT, VRASCII,  DTComp)
TYPETOENCODING(FL, VRBINARY, float)
TYPETOENCODING(FD, VRBINARY, double)
TYPETOENCODING(IS, VRASCII,  signed int)
TYPETOENCODING(LO, VRASCII,  LOComp)
TYPETOENCODING(LT, VRASCII,  LTComp)
TYPETOENCODING(OB, VRBINARY, unsigned char)
TYPETOENCODING(OD, VRBINARY, double)
TYPETOENCODING(OF, VRBINARY, float)
TYPETOENCODING(OL, VRBINARY, unsigned int)
TYPETOENCODING(OV, VRBINARY, unsigned long long)
TYPETOENCODING(OW, VRBINARY, unsigned short)
TYPETOENCODING(PN, VRASCII,  PNComp)
TYPETOENCODING(SH, VRASCII,  SHComp)
TYPETOENCODING(SL, VRBINARY, signed int)
TYPETOENCODING(SQ, VRBINARY, unsigned char)
TYPETOENCODING(SS, VRBINARY, signed short)
TYPETOENCODING(ST, VRASCII,  STComp)
TYPETOENCODING(SV, VRBINARY, signed long long)
TYPETOENCODING(TM, VRASCII,  TMComp)
TYPETOENCODING(UC, VRASCII,  UCComp)
TYPETOENCODING(UI, VRASCII,  UIComp)
TYPETOENCODING(UL, VRBINARY, unsigned int)
TYPETOENCODING(UN, VRBINARY, unsigned char)
TYPETOENCODING(UR, VRASCII,  URComp)
TYPETOENCODING(US, VRBINARY, unsigned short)
TYPETOENCODING(UT, VRASCII,  UTComp)
TYPETOENCODING(UV, VRBINARY, unsigned long long)

#define VRTypeTemplateCase(type) \
  case VR::type: \
    return sizeof (VRToType<VR::type>::Type);


inline unsigned int VR::GetSize() const
{
  switch(VRField)
  {
    VRTypeTemplateCase(AE)
    VRTypeTemplateCase(AS)
    VRTypeTemplateCase(AT)
    VRTypeTemplateCase(CS)
    VRTypeTemplateCase(DA)
    VRTypeTemplateCase(DS)
    VRTypeTemplateCase(DT)
    VRTypeTemplateCase(FL)
    VRTypeTemplateCase(FD)
    VRTypeTemplateCase(IS)
    VRTypeTemplateCase(LO)
    VRTypeTemplateCase(LT)
    VRTypeTemplateCase(OB)
    VRTypeTemplateCase(OD)
    VRTypeTemplateCase(OF)
    VRTypeTemplateCase(OL)
    VRTypeTemplateCase(OV)
    VRTypeTemplateCase(OW)
    VRTypeTemplateCase(PN)
    VRTypeTemplateCase(SH)
    VRTypeTemplateCase(SL)
    VRTypeTemplateCase(SQ)
    VRTypeTemplateCase(SS)
    VRTypeTemplateCase(ST)
    VRTypeTemplateCase(SV)
    VRTypeTemplateCase(TM)
    VRTypeTemplateCase(UC)
    VRTypeTemplateCase(UI)
    VRTypeTemplateCase(UL)
    VRTypeTemplateCase(UN)
    VRTypeTemplateCase(UR)
    VRTypeTemplateCase(US)
    VRTypeTemplateCase(UT)
    VRTypeTemplateCase(UV)
    case VR::US_SS:
      return 2;
    case VR::INVALID:
    case VR::OB_OW:
    case VR::US_SS_OW:
    case VR::US_OW:
    case VR::VL16:
    case VR::VL32:
    case VR::VRASCII:
    case VR::VRBINARY:
    case VR::VR_VM1:
    case VR::VRALL:
    case VR::VR_END:
    default:
      break;
  }
  return 0;
}

} // end namespace mdcm

#endif //MDCMVR_H
