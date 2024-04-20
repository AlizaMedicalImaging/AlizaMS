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

#ifndef MDCMLOOKUPTABLE_H
#define MDCMLOOKUPTABLE_H

#include "mdcmTypes.h"
#include <vector>
#include <istream>

namespace mdcm
{

class LookupTableInternal
{
public:
  unsigned int               Length[3]{};
  unsigned short             Subscript[3]{};
  unsigned short             BitSize[3]{};
  std::vector<unsigned char> RGB;
};

/**
 * LookupTable class
 */
class MDCM_EXPORT LookupTable
{
public:
  typedef enum
  {
    RED = 0,
    GREEN = 1,
    BLUE = 2,
    GRAY = 3,
    SUPPLRED = 4,
    SUPPLGREEN = 5,
    SUPPLBLUE = 6,
    UNKNOWN
  } LookupTableType;

  bool
  Initialized() const;
  void
  Clear();
  void
  Allocate(unsigned short = 8 /*bitsample*/);
  void InitializeLUT(LookupTableType, unsigned short, unsigned short, unsigned short);
  unsigned int GetLUTLength(LookupTableType) const;
  void
  SetLUT(LookupTableType, const unsigned char *, unsigned int);
  void
  SetSegmentedLUT(LookupTableType, const unsigned char *, unsigned int);
  void
  GetLUT(LookupTableType, unsigned char *, unsigned int &) const;
  void
  GetLUTDescriptor(LookupTableType, unsigned short &, unsigned short &, unsigned short &) const;
  void
  InitializeRedLUT(unsigned short, unsigned short, unsigned short);
  void
  InitializeGreenLUT(unsigned short, unsigned short, unsigned short);
  void
  InitializeBlueLUT(unsigned short, unsigned short, unsigned short);
  void
  SetRedLUT(const unsigned char *, unsigned int);
  void
  SetGreenLUT(const unsigned char *, unsigned int);
  void
  SetBlueLUT(const unsigned char *, unsigned int);
  void
  Decode(std::istream & is, std::ostream & os) const;
  bool
  Decode(char *, size_t, const char *, size_t) const;
  int
  DecodeSupplemental(char *, size_t, const char *, size_t) const;
#if 0
  bool
  GetBufferAsRGBA(unsigned char *) const;
  bool
  WriteBufferAsRGBA(const unsigned char *);
#endif
  unsigned short
  GetBitSample() const;

private:
  LookupTableInternal Internal{};
  unsigned short      BitSample{};
  bool                IncompleteLUT{};
};

} // end namespace mdcm

#endif // MDCMLOOKUPTABLE_H
