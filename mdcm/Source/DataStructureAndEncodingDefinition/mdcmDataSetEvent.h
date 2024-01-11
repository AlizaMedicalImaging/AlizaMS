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
#ifndef MDCMDATASETEVENT_H
#define MDCMDATASETEVENT_H

#include "mdcmEvent.h"
#include "mdcmDataSet.h"

namespace mdcm
{

/**
 * DataSetEvent
 * Special type of event triggered during the DataSet store/move process
 *
 */
class DataSetEvent : public AnyEvent
{
public:
  typedef DataSetEvent Self;
  typedef AnyEvent     Superclass;
  DataSetEvent(DataSet const * ds = nullptr)
    : m_DataSet(ds)
  {}
  virtual ~DataSetEvent() {}
  virtual const char *
  GetEventName() const override
  {
    return "DataSetEvent";
  }
  virtual bool
  CheckEvent(const ::mdcm::Event * e) const override
  {
    return (dynamic_cast<const Self *>(e) == nullptr ? false : true);
  }
  virtual ::mdcm::Event *
  MakeObject() const override
  {
    return new Self;
  }
  DataSetEvent(const Self & s)
    : AnyEvent(s) {}
  DataSet const &
  GetDataSet() const
  {
    return *m_DataSet;
  }

private:
  void
  operator=(const Self &);
  const DataSet * m_DataSet;
};

} // end namespace mdcm

#endif
