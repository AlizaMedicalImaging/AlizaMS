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

#ifndef MDCMSUBJECT_H
#define MDCMSUBJECT_H

#include "mdcmObject.h"

namespace mdcm
{

class Event;
class Command;
class SubjectInternals;

/**
 * Subject
 */
class MDCM_EXPORT Subject : public Object
{
public:
  Subject();
  ~Subject();
  /* Allow to add/remove/invoke observers (callbacks) to any MDCM
   * object. This is an implementation of the subject/observer design
   * pattern. An observer is added by specifying an event to respond to
   * and an mdcm::Command to execute. It returns an unsigned long tag
   * which can be used later to remove the event or retrieve the
   * command.  The memory for the Command becomes the responsibility of
   * this object, so don't pass the same instance of a command to two
   * different objects. */
  unsigned long
  AddObserver(const Event &, Command *);
  unsigned long
  AddObserver(const Event &, Command *) const;
  /* Get the command associated with the given tag.  Note: This returns
   * a pointer to a Command, but it is safe to asign this to a
   * Command::Pointer.  Since Command inherits from LightObject, at this
   * point in the code, only a pointer or a reference to the Command can
   * be used. */
  Command *
  GetCommand(unsigned long);
  /* Call Execute on all the Commands observing this event id. */
  void
  InvokeEvent(const Event &);
  /* Call Execute on all the Commands observing this event id.
   * The actions triggered by this call doesn't modify this object. */
  void
  InvokeEvent(const Event &) const;
  /* Remove the observer with this tag value. */
  void
  RemoveObserver(unsigned long);
  /* Remove all observers. */
  void
  RemoveAllObservers();
  /* Return true if an observer is registered for this event. */
  bool
  HasObserver(const Event &) const;

private:
  SubjectInternals * Internals;
};

} // end namespace mdcm

#endif // MDCMSUBJECT_H
