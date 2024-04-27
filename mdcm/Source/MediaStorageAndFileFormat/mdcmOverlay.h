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
#ifndef MDCMOVERLAY_H
#define MDCMOVERLAY_H

#include "mdcmTypes.h"
#include "mdcmObject.h"
#include <vector>

namespace mdcm
{

class ByteValue;
class DataSet;
class DataElement;

class OverlayInternal
{
public:
  OverlayInternal() = default;
  bool              InPixelData{};
  unsigned short    Group{};
  unsigned short    Rows{};           // (6000,0010) US
  unsigned short    Columns{};        // (6000,0011) US
  unsigned int      NumberOfFrames{}; // (6000,0015) IS
  std::string       Description;      // (6000,0022) LO
  std::string       Type;             // (6000,0040) CS
  signed short      Origin[2]{};      // (6000,0050) SS
  unsigned short    FrameOrigin{};    // (6000,0051) US
  unsigned short    BitsAllocated{};  // (6000,0100) US
  unsigned short    BitPosition{};    // (6000,0102) US
  std::vector<char> Data{};           // data, no trailing padding '\0'
};

/**
 * Overlay class
 */
class MDCM_EXPORT Overlay : public Object
{
public:
  typedef enum
  {
    Invalid = 0,
    Graphics = 1,
    ROI = 2
  } OverlayType;

  Overlay() = default;

  Overlay(const Overlay & o) : Object(o)
  {
    Internal = o.Internal;
  }

  ~Overlay() = default;

  Overlay &
  operator=(const Overlay & ov)
  {
    Internal = ov.Internal;
    return *this;
  }

  void
  Update(const DataElement &);

  bool
  GrabOverlayFromPixelData(const DataSet &);

  void
  SetGroup(unsigned short);

  unsigned short
  GetGroup() const;

  void
  SetRows(unsigned short);

  unsigned short
  GetRows() const;

  void
  SetColumns(unsigned short);

  unsigned short
  GetColumns() const;

  void
  SetNumberOfFrames(unsigned int);

  unsigned int
  GetNumberOfFrames() const;

  void
  SetDescription(const char *);

  const char *
  GetDescription() const;

  void
  SetType(const char *);

  const char *
  GetType() const;

  static const char * GetOverlayTypeAsString(OverlayType);

  static OverlayType
  GetOverlayTypeFromString(const char *);

  OverlayType
  GetTypeAsEnum() const;

  void
  SetOrigin(const signed short origin[2]);

  const signed short *
  GetOrigin() const;

  void
  SetFrameOrigin(unsigned short);

  unsigned short
  GetFrameOrigin() const;

  void
  SetBitsAllocated(unsigned short);

  unsigned short
  GetBitsAllocated() const;

  void
  SetBitPosition(unsigned short);

  unsigned short
  GetBitPosition() const;

  ByteValue
  GetOverlayData() const;

  bool
  IsEmpty() const;

  bool
  IsZero() const;

  bool
  IsInPixelData() const;

  void
  IsInPixelData(bool b);

  void
  SetOverlay(const char *, size_t);

  size_t
  GetUnpackBufferLength() const;

  bool
  GetUnpackBuffer(char *, size_t) const;

  void
  Decompress(std::ostream &) const;
  
  void
  Print(std::ostream &) const override;

private:
  OverlayInternal Internal{};
};

} // end namespace mdcm

#endif // MDCMOVERLAY_H
