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
#include "mdcmObject.h"
#include <cstdlib>

namespace mdcm
{

class LookupTableInternal;

/**
 * LookupTable class
 */
class MDCM_EXPORT LookupTable : public Object
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

  LookupTable();
  ~LookupTable();
  bool
  Initialized() const;
  void
  Clear();
  void
  Allocate(unsigned short = 8 /*bitsample*/);
  void InitializeLUT(LookupTableType, unsigned short, unsigned short, unsigned short);
  unsigned int GetLUTLength(LookupTableType) const;
  virtual void
  SetLUT(LookupTableType, const unsigned char *, unsigned int);
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
  const unsigned char *
  GetPointer() const;
  bool
  GetBufferAsRGBA(unsigned char *) const;
  bool
  WriteBufferAsRGBA(const unsigned char *);
  unsigned short
  GetBitSample() const;
  void
  Print(std::ostream &) const override;

protected:
  LookupTableInternal * Internal;
  unsigned short        BitSample;
  bool                  IncompleteLUT;
};

} // end namespace mdcm

#endif // MDCMLOOKUPTABLE_H
