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

namespace mdcm
{

class OverlayInternal;
class ByteValue;
class DataSet;
class DataElement;
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

  Overlay();
  ~Overlay() override;
  Overlay(const Overlay &);
  Overlay &
  operator=(const Overlay &);
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
  const ByteValue &
  GetOverlayData() const; // Not thread safe
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
  OverlayInternal * Internal;
};

} // end namespace mdcm

#endif // MDCMOVERLAY_H
