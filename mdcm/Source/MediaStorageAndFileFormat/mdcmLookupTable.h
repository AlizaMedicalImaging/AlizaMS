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
 * \brief LookupTable class
 */
class MDCM_EXPORT LookupTable : public Object
{
public:
  typedef enum // don't change
  {
    RED        = 0,
    GREEN      = 1,
    BLUE       = 2,
    GRAY       = 3,
    SUPPLRED   = 4,
    SUPPLGREEN = 5,
    SUPPLBLUE  = 6,
    UNKNOWN
  } LookupTableType;

  LookupTable();
  ~LookupTable();

  void Print(std::ostream &) const {}
  void Allocate( unsigned short bitsample = 8 );
  //TODO: check to see if length should be unsigned short, unsigned int, or whatever
  void InitializeLUT(LookupTableType type, unsigned short length,
    unsigned short subscript, unsigned short bitsize);
  unsigned int GetLUTLength(LookupTableType type) const;
  virtual void SetLUT(LookupTableType type, const unsigned char * array,
    unsigned int length);
  void GetLUT(LookupTableType type, unsigned char * array, unsigned int & length) const;
  void GetLUTDescriptor(LookupTableType type, unsigned short & length,
    unsigned short & subscript, unsigned short & bitsize) const;
  void InitializeRedLUT(unsigned short length, unsigned short subscript,
    unsigned short bitsize);
  void SetRedLUT(const unsigned char * red, unsigned int length);
  void InitializeGreenLUT(unsigned short length, unsigned short subscript,
    unsigned short bitsize);
  void SetGreenLUT(const unsigned char * green, unsigned int length);
  void InitializeBlueLUT(unsigned short length, unsigned short subscript,
    unsigned short bitsize);
  void SetBlueLUT(const unsigned char * blue, unsigned int length);
  void Clear();
  void Decode(std::istream & is, std::ostream & os) const;
  bool Decode(
    char * outputbuffer, size_t outlen,
    const char * inputbuffer, size_t inlen) const;
  int DecodeSupplemental(
    char * outputbuffer, size_t outlen,
    const char * inputbuffer, size_t inlen) const; 
  LookupTable(LookupTable const & lut) : Object(lut)
  {
    assert(0);
  }
  bool GetBufferAsRGBA(unsigned char * rgba) const;
  const unsigned char * GetPointer() const;
  bool WriteBufferAsRGBA(const unsigned char * rgba);
  unsigned short GetBitSample() const { return BitSample; }
  bool Initialized() const;

private:
protected:
  LookupTableInternal *Internal;
  unsigned short BitSample;
  bool IncompleteLUT:1;
};

} // end namespace mdcm

#endif //MDCMLOOKUPTABLE_H
