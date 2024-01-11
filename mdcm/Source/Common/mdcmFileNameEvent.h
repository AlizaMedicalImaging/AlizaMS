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
#ifndef MDCMFILENAMEEVENT_H
#define MDCMFILENAMEEVENT_H

#include "mdcmEvent.h"
#include "mdcmTag.h"

namespace mdcm
{

/**
 * FileNameEvent
 * Special type of event triggered during processing of FileSet
 *
 */
class FileNameEvent : public AnyEvent
{
public:
  typedef FileNameEvent Self;
  typedef AnyEvent      Superclass;
  FileNameEvent(const char * s = "")
    : m_FileName(s)
  {}
  FileNameEvent(const Self & s)
    : AnyEvent(s) {}
  ~FileNameEvent() = default;

  const char *
  GetEventName() const override
  {
    return "FileNameEvent";
  }

  bool
  CheckEvent(const ::mdcm::Event * e) const override
  {
    return dynamic_cast<const Self *>(e) ? true : false;
  }

  ::mdcm::Event *
  MakeObject() const override
  {
    return new Self;
  }

  void
  SetFileName(const char * f)
  {
    m_FileName = f;
  }

  const char *
  GetFileName() const
  {
    return m_FileName.c_str();
  }

private:
  void
  operator=(const Self &);
  std::string m_FileName;
};


} // end namespace mdcm

#endif // MDCMFILENAMEEVENT_H
