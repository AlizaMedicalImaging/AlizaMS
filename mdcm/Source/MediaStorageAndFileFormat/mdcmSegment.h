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
#ifndef MDCMSEGMENT_H
#define MDCMSEGMENT_H

#include <mdcmObject.h>
#include <mdcmSurface.h>
#include "mdcmSegmentHelper.h"
#include <vector>

namespace mdcm
{

/**
  * \brief  This class defines a segment.
  * \details It mainly contains attributes of group 0x0062.
  * In addition, it can be associated with surface.
  *
  * \see  PS 3.3 C.8.20.2 and C.8.23
  */
class MDCM_EXPORT Segment : public Object
{
public:

  typedef std::vector< SmartPointer< Surface > > SurfaceVector;
  typedef std::vector< SegmentHelper::BasicCodedEntry > BasicCodedEntryVector;

  typedef enum
  {
    AUTOMATIC = 0,
    SEMIAUTOMATIC,
    MANUAL,
    ALGOType_END
  } ALGOType;

  static const char * GetALGOTypeString(ALGOType type);
  static ALGOType GetALGOType(const char * type);
  Segment();
  virtual ~Segment();
  unsigned short GetSegmentNumber() const;
  void SetSegmentNumber(const unsigned short num);
  const char * GetSegmentLabel() const;
  void SetSegmentLabel(const char * label);
  const char * GetSegmentDescription() const;
  void SetSegmentDescription(const char * description);
  SegmentHelper::BasicCodedEntry const & GetAnatomicRegion() const;
  SegmentHelper::BasicCodedEntry & GetAnatomicRegion();
  void SetAnatomicRegion(SegmentHelper::BasicCodedEntry const & BSE);
  BasicCodedEntryVector const & GetAnatomicRegionModifiers() const;
  BasicCodedEntryVector & GetAnatomicRegionModifiers();
  void SetAnatomicRegionModifiers(BasicCodedEntryVector const & BSEV);
  SegmentHelper::BasicCodedEntry const & GetPropertyCategory() const;
  SegmentHelper::BasicCodedEntry & GetPropertyCategory();
  void SetPropertyCategory(SegmentHelper::BasicCodedEntry const & BSE);
  SegmentHelper::BasicCodedEntry const & GetPropertyType() const;
  SegmentHelper::BasicCodedEntry & GetPropertyType();
  void SetPropertyType(SegmentHelper::BasicCodedEntry const & BSE);
  BasicCodedEntryVector const & GetPropertyTypeModifiers() const;
  BasicCodedEntryVector & GetPropertyTypeModifiers();
  void SetPropertyTypeModifiers(BasicCodedEntryVector const & BSEV);
  ALGOType GetSegmentAlgorithmType() const;
  void SetSegmentAlgorithmType(ALGOType type);
  void SetSegmentAlgorithmType(const char * typeStr);
  const char * GetSegmentAlgorithmName() const;
  void SetSegmentAlgorithmName(const char * name);
  unsigned long GetSurfaceCount();
  void SetSurfaceCount(const unsigned long nb);
  SurfaceVector const & GetSurfaces() const;
  SurfaceVector & GetSurfaces();
  SmartPointer< Surface > GetSurface(const unsigned int idx = 0) const;
  void AddSurface(SmartPointer< Surface > surface);

protected :
  //0062 0004 US 1 Segment Number
  unsigned short  SegmentNumber;
  //0062 0005 LO 1 Segment Label
  std::string     SegmentLabel;
  //0062 0006 ST 1 Segment Description
  std::string     SegmentDescription;
  SegmentHelper::BasicCodedEntry AnatomicRegion;
  BasicCodedEntryVector AnatomicRegionModifiers;
  SegmentHelper::BasicCodedEntry PropertyCategory;
  SegmentHelper::BasicCodedEntry PropertyType;
  BasicCodedEntryVector PropertyTypeModifiers;
  //0062 0008 CS 1 Segment Algorithm Type
  ALGOType        SegmentAlgorithmType;
  //0062 0009 LO 1 Segment Algorithm Name
  std::string     SegmentAlgorithmName;
  //0066 002a UL 1 Surface Count
  unsigned long   SurfaceCount;
  SurfaceVector   Surfaces;

private :
  void ComputeSurfaceCount();
};

}

#endif // MDCMSEGMENT_H
