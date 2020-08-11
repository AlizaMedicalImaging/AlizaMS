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
  * \brief  This class defines a segment writer.
  * \details It writes attributes of group 0x0062.
  *
  * \see  PS 3.3 C.8.20.2 and C.8.23
  */
class MDCM_EXPORT SegmentWriter : public Writer
{
public:
  typedef std::vector< SmartPointer< Segment > > SegmentVector;
  SegmentWriter();
  virtual ~SegmentWriter();
  bool Write();
  unsigned int GetNumberOfSegments() const;
  void SetNumberOfSegments(const unsigned int size);
  const SegmentVector & GetSegments() const;
  SegmentVector & GetSegments();
  SmartPointer< Segment > GetSegment(const unsigned int idx = 0) const;
  void AddSegment(SmartPointer< Segment > segment);
  void SetSegments(SegmentVector & segments);
protected:
  bool PrepareWrite();
  SegmentVector Segments;
};

}

#endif // MDCMSEGMENTWRITER_H
