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

#ifndef MDCMMESHPRIMITIVE_H
#define MDCMMESHPRIMITIVE_H

#include <mdcmObject.h>
#include <mdcmDataElement.h>

namespace mdcm
{

/**
 * This class defines surface mesh primitives.
 * PS 3.3 C.27.4
 */
class MDCM_EXPORT MeshPrimitive : public Object
{
public:
  typedef std::vector<DataElement> PrimitivesData;
  /**
   * This enumeration defines primitive types.
   * PS 3.3 C.27.4.1
   */
  enum MPType
  {
    VERTEX = 0,
    EDGE,
    TRIANGLE,
    TRIANGLE_STRIP,
    TRIANGLE_FAN,
    LINE,
    FACET,
    MPType_END
  };

  static const char *
  GetMPTypeString(const MPType);
  static MPType
  GetMPType(const char *);
  MeshPrimitive();
  virtual ~MeshPrimitive() = default;
  MPType
  GetPrimitiveType() const;
  void
  SetPrimitiveType(const MPType);
  const DataElement &
  GetPrimitiveData() const;
  DataElement &
  GetPrimitiveData();
  void
  SetPrimitiveData(const DataElement &);
  const PrimitivesData &
  GetPrimitivesData() const;
  PrimitivesData &
  GetPrimitivesData();
  void
  SetPrimitivesData(const PrimitivesData &);
  const DataElement &
  GetPrimitiveData(const unsigned int) const;
  DataElement &
  GetPrimitiveData(const unsigned int);
  void
  SetPrimitiveData(const unsigned int, const DataElement &);
  void
  AddPrimitiveData(const DataElement &);
  unsigned int
  GetNumberOfPrimitivesData() const;

protected:
  // Use to define tag where PrimitiveData will be put.
  MPType PrimitiveType;
  // PrimitiveData contains point index list.
  // It shall have 1 or 1-n DataElement following PrimitiveType.
  PrimitivesData PrimitiveData;
};

} // namespace mdcm

#endif // MDCMMESHPRIMITIVE_H
