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
  * \brief  This class defines a SURFACE IE writer.
  * \details It writes surface mesh module attributes.
  *
  * \see  PS 3.3 A.1.2.18 , A.57 and C.27
  */
class MDCM_EXPORT SurfaceWriter : public SegmentWriter
{
public:
    SurfaceWriter();

    virtual ~SurfaceWriter();
    bool Write();
    unsigned long GetNumberOfSurfaces();
    void SetNumberOfSurfaces(const unsigned long nb);

protected:

    bool PrepareWrite();
    void ComputeNumberOfSurfaces();
    bool PrepareWritePointMacro(SmartPointer< Surface > surface,
                                DataSet & surfaceDS,
                                const TransferSyntax & ts);
    unsigned long NumberOfSurfaces;
};

}

#endif // MDCMSURFACEWRITER_H
