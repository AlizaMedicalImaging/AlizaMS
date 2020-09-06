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
#ifndef MDCMSURFACEREADER_H
#define MDCMSURFACEREADER_H

#include <mdcmSegmentReader.h>
#include <mdcmSurface.h>

namespace mdcm
{

/**
  * \brief  This class defines a SURFACE IE reader.
  * \details It reads surface mesh module attributes.
  *
  * \see  PS 3.3 A.1.2.18 , A.57 and C.27
  */
class MDCM_EXPORT SurfaceReader : public SegmentReader
{
public:
    SurfaceReader();
    virtual ~SurfaceReader();
    virtual bool Read();
    unsigned long GetNumberOfSurfaces() const;
  protected:
    bool ReadSurfaces();
    bool ReadSurface(const Item & surfaceItem, const unsigned long idx);
    bool ReadPointMacro(SmartPointer< Surface > surface, const DataSet & surfaceDS);
};

}

#endif // MDCMSURFACEREADER_H
