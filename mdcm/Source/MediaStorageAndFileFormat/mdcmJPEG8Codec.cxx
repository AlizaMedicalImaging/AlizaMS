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
#include "mdcmJPEG8Codec.h"
#include "mdcm_ljpeg8.h"

#define JPEGBITSCodec JPEG8Codec
#define my_error_mgr my_error_mgr_8BIT
#define JPEGInternals JPEGInternals_8BIT
#define my_source_mgr my_source_mgr_8BIT
#define my_destination_mgr my_destination_mgr_8BIT
#include "mdcmJPEGBITSCodec.hxx"
