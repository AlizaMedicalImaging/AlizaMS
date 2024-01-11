/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMSEGMENTWRITER_H
#define MDCMSEGMENTWRITER_H

#include <mdcmWriter.h>
#include <mdcmSegment.h>

namespace mdcm
{

/**
 * This class defines a segment writer.
 * It writes attributes of group 0x0062.
 *
 * PS 3.3 C.8.20.2 and C.8.23
 */
class MDCM_EXPORT SegmentWriter : public Writer
{
public:
  typedef std::vector<SmartPointer<Segment>> SegmentVector;
  SegmentWriter() = default;
  virtual ~SegmentWriter() = default;
  bool
  Write() override;
  unsigned int
  GetNumberOfSegments() const;
  void
  SetNumberOfSegments(const unsigned int);
  const SegmentVector &
  GetSegments() const;
  SegmentVector &
  GetSegments();
  SmartPointer<Segment>
  GetSegment(const unsigned int = 0) const;
  void
  AddSegment(SmartPointer<Segment> segment);
  void
  SetSegments(SegmentVector & segments);

protected:
  virtual bool
                PrepareWrite();
  SegmentVector Segments;
};

} // namespace mdcm

#endif // MDCMSEGMENTWRITER_H
