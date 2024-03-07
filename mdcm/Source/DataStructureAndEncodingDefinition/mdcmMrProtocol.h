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

#ifndef MDCMMRPROTOCOL_H
#define MDCMMRPROTOCOL_H

#include "mdcmTypes.h"
#include <vector>
#include <map>
#include <string>

namespace mdcm
{

class ByteValue;

/**
 * Class for MrProtocol
 *
 * Everything done in this code is for the sole purpose of writing interoperable
 * software under Sect. 1201 (f) Reverse Engineering exception of the DMCA.
 */
class MDCM_EXPORT MrProtocol
{
  friend std::ostream &
  operator<<(std::ostream &, const MrProtocol &);

public:
  typedef std::map<std::string, std::string> MyMapType;

  struct Vector3
  {
    double dSag;
    double dCor;
    double dTra;
  };

  struct Slice
  {
    Vector3 Normal;
    Vector3 Position;
  };

  struct SliceArray
  {
    std::vector<Slice> Slices;
  };

  struct Element{};

  struct Internals
  {
    MyMapType   mymap;
    std::string csastr;
    int         version;
  };

  MrProtocol();

  ~MrProtocol();

  bool
  Load(const ByteValue *, const char *, int);

  void
  Print(std::ostream &) const;

  int
  GetVersion() const;

  const char *
  GetMrProtocolByName(const char *) const;

  bool
  FindMrProtocolByName(const char *) const;

  bool
  GetSliceArray(SliceArray &) const;

private:
  Internals * Pimpl;
};

inline std::ostream &
operator<<(std::ostream & os, const MrProtocol & d)
{
  d.Print(os);
  return os;
}

} // end namespace mdcm

#endif // MDCMMRPROTOCOL_H
