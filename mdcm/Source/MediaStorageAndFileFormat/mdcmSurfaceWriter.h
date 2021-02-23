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
#ifndef MDCMSURFACEWRITER_H
#define MDCMSURFACEWRITER_H

#include <mdcmSegmentWriter.h>
#include <mdcmSurface.h>

namespace mdcm
{

/**
  * This class defines a SURFACE IE writer.
  * It writes surface mesh module attributes.
  *
  * PS 3.3 A.1.2.18 , A.57 and C.27
  */
class MDCM_EXPORT SurfaceWriter : public SegmentWriter
{
public:
    SurfaceWriter();
    virtual ~SurfaceWriter() override;
    bool Write() override;
    unsigned long GetNumberOfSurfaces();
    void SetNumberOfSurfaces(const unsigned long);
protected:
    bool PrepareWrite() override;
    void ComputeNumberOfSurfaces();
    bool PrepareWritePointMacro(
      SmartPointer< Surface >, DataSet &, const TransferSyntax &);
    unsigned long NumberOfSurfaces;
};

}

#endif // MDCMSURFACEWRITER_H
