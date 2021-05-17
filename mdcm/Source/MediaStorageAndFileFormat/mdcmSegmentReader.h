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
#ifndef MDCMSEGMENTREADER_H
#define MDCMSEGMENTREADER_H

#include <mdcmReader.h>
#include <mdcmSegment.h>
#include <map>

namespace mdcm
{

/**
 * This class defines a segment reader.
 * It reads attributes of group 0x0062
 *
 * PS 3.3 C.8.20.2 and C.8.23
 */
class MDCM_EXPORT SegmentReader : public Reader
{
public:
  typedef std::vector<SmartPointer<Segment>> SegmentVector;
  SegmentReader();
  virtual ~SegmentReader();
  virtual bool
  Read();
  const SegmentVector
  GetSegments() const;
  SegmentVector
  GetSegments();

protected:
  typedef std::map<unsigned long, SmartPointer<Segment>> SegmentMap;
  bool
  ReadSegments();
  bool
  ReadSegment(const Item &, const unsigned int);
  SegmentMap Segments;
};

} // namespace mdcm

#endif // MDCMSEGMENTREADER_H
