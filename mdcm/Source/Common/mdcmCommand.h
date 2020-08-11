/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef MDCMCOMMAND_H
#define MDCMCOMMAND_H

#include "mdcmSubject.h"

namespace mdcm
{
class Event;

class MDCM_EXPORT Command : public Subject
{
public :
  virtual void Execute(Subject *, const Event &) = 0;
  virtual void Execute(const Subject *, const Event &) = 0;

protected:
  Command()  {};
  ~Command() {};

private:
  Command(const Command &);  // Not implemented.
  void operator=(const Command &);  // Not implemented.
};

template <class T>
class MemberCommand : public Command
{
public:
  typedef  void (T::*TMemberFunctionPointer)(Subject *, const Event &);
  typedef  void (T::*TConstMemberFunctionPointer)(
    const Subject *,
    const Event &);
  typedef MemberCommand Self;

  static SmartPointer<MemberCommand> New()
  {
    return new MemberCommand;
  }

  void SetCallbackFunction(T * object,
    TMemberFunctionPointer memberFunction)
  {
    m_This = object;
    m_MemberFunction = memberFunction;
  }

  void SetCallbackFunction(T * object,
    TConstMemberFunctionPointer memberFunction)
  {
    m_This = object;
    m_ConstMemberFunction = memberFunction;
  }

  virtual void Execute(Subject * caller, const Event & event)
  {
    if(m_MemberFunction)
    {
      ((*m_This).*(m_MemberFunction))(caller, event);
    }
  }

  virtual void Execute(const Subject * caller, const Event & event)
  {
    if(m_ConstMemberFunction)
    {
      ((*m_This).*(m_ConstMemberFunction))(caller, event);
    }
  }

protected:
  T * m_This;
  TMemberFunctionPointer m_MemberFunction;
  TConstMemberFunctionPointer m_ConstMemberFunction;
  MemberCommand() : m_MemberFunction(0),m_ConstMemberFunction(0) {}
  virtual ~MemberCommand(){}

private:
  MemberCommand(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

};


template <typename T>
class SimpleMemberCommand : public Command
{
public:
  typedef  void (T::*TMemberFunctionPointer)();

  typedef SimpleMemberCommand Self;

  static SmartPointer<SimpleMemberCommand> New()
  {
    return new SimpleMemberCommand;
  }

  void SetCallbackFunction(T * object,
    TMemberFunctionPointer memberFunction)
  {
    m_This = object;
    m_MemberFunction = memberFunction;
  }

  virtual void Execute(Subject *,const Event &)
  {
    if(m_MemberFunction)
    {
      ((*m_This).*(m_MemberFunction))();
    }
  }

  virtual void Execute(const Subject *,const Event &)
  {
    if(m_MemberFunction)
    {
      ((*m_This).*(m_MemberFunction))();
    }
  }

protected:
  T * m_This;
  TMemberFunctionPointer m_MemberFunction;
  SimpleMemberCommand() : m_This(0),m_MemberFunction(0) {}
  virtual ~SimpleMemberCommand() {}

private:
  SimpleMemberCommand(const Self &); //purposely not implemented
  void operator=(const Self &); //purposely not implemented
};

} // end namespace mdcm

#endif //MDCMCOMMAND_H
