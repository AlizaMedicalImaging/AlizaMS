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
// do the magic:
#define MDCM_FORCE_EXPORT
#define MDCM_OVERRIDE_BROKEN_IMPLEMENTATION
#undef mdcm_ns
#define mdcm_ns mdcmstrict
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
#error misconfiguration
#endif
#include "mdcmReader.h"

namespace mdcm
{
  // make it public:
  MDCM_EXPORT
  bool StrictReadUpToTag( const char * filename, Tag const & last, std::set<Tag> const & skiptags )
    {
    mdcmstrict::Reader reader;
    assert( filename );
    reader.SetFileName( filename );
    bool read = false;
    try
      {
      // Start reading all tags, including the 'last' one:
      read = reader.ReadUpToTag(last, skiptags);
      }
    catch(std::exception & ex)
      {
      (void)ex;
      mdcmWarningMacro( "Failed to read:" << filename << " with ex:" << ex.what() );
      }
    catch(...)
      {
      mdcmWarningMacro( "Failed to read:" << filename  << " with unknown error" );
      }
    return read;
    }
} // end namespace mdcm
