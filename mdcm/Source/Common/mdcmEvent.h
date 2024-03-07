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
#ifndef MDCMEVENT_H
#define MDCMEVENT_H

#include "mdcmTypes.h"
#include <iostream>

namespace mdcm
{

class MDCM_EXPORT Event
{
public:
  Event() = default;
  Event(const Event &) = default;
  virtual ~Event() = default;
  virtual Event *
  MakeObject() const = 0;
  virtual void
  Print(std::ostream &) const;
  virtual const char *
  GetEventName() const = 0;
  virtual bool
  CheckEvent(const Event *) const = 0;

private:
  void
  operator=(const Event &); // Not implemented.
};

inline std::ostream &
operator<<(std::ostream & os, const Event & e)
{
  e.Print(os);
  return os;
}

#define mdcmEventMacro(classname, super)                            \
  class classname : public super                                    \
  {                                                                 \
  public:                                                           \
    typedef classname Self;                                         \
    typedef super Superclass;                                       \
    classname() = default;                                          \
    virtual ~classname() = default;                                 \
    virtual const char * GetEventName() const override              \
    {                                                               \
      return #classname;                                            \
    }                                                               \
    virtual bool CheckEvent(const ::mdcm::Event * e) const override \
    {                                                               \
      return dynamic_cast<const Self *>(e) ? true : false;          \
    }                                                               \
    virtual ::mdcm::Event * MakeObject() const override             \
    {                                                               \
      return new Self;                                              \
    }                                                               \
    classname(const Self & s) : super(s) {}                         \
  private:                                                          \
    void operator=(const Self &);                                   \
  }

mdcmEventMacro(NoEvent, Event);
mdcmEventMacro(AnyEvent, Event);
mdcmEventMacro(StartEvent, AnyEvent);
mdcmEventMacro(EndEvent, AnyEvent);
//mdcmEventMacro(ExitEvent, AnyEvent);
mdcmEventMacro(AbortEvent, AnyEvent);
//mdcmEventMacro(ModifiedEvent, AnyEvent);
//mdcmEventMacro(InitializeEvent, AnyEvent);
mdcmEventMacro(IterationEvent, AnyEvent);
//mdcmEventMacro(UserEvent, AnyEvent);

} // end namespace mdcm

#endif // MDCMEVENT_H
