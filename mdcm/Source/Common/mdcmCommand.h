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
#ifndef MDCMCOMMAND_H
#define MDCMCOMMAND_H

#include "mdcmSubject.h"
#include "mdcmMacro.h"

namespace mdcm
{

class Event;

class MDCM_EXPORT Command : public Subject
{
public:
  MDCM_DISALLOW_COPY_AND_MOVE(Command);

  virtual void
  Execute(Subject *, const Event &) = 0;
  virtual void
  Execute(const Subject *, const Event &) = 0;

protected:
  Command() = default;
  ~Command() = default;
};

template <class T>
class MemberCommand : public Command
{
public:
  MDCM_DISALLOW_COPY_AND_MOVE(MemberCommand);

  typedef void (T::*TMemberFunctionPointer)(Subject *, const Event &);
  typedef void (T::*TConstMemberFunctionPointer)(const Subject *, const Event &);
  typedef MemberCommand Self;

  static SmartPointer<MemberCommand>
  New()
  {
    return new MemberCommand;
  }

  void
  SetCallbackFunction(T * object, TMemberFunctionPointer memberFunction)
  {
    m_This = object;
    m_MemberFunction = memberFunction;
  }

  void
  SetCallbackFunction(T * object, TConstMemberFunctionPointer memberFunction)
  {
    m_This = object;
    m_ConstMemberFunction = memberFunction;
  }

  void
  Execute(Subject * caller, const Event & event) override
  {
    if (m_This && m_MemberFunction)
    {
      ((*m_This).*(m_MemberFunction))(caller, event);
    }
  }

  void
  Execute(const Subject * caller, const Event & event) override
  {
    if (m_This && m_MemberFunction)
    {
      ((*m_This).*(m_ConstMemberFunction))(caller, event);
    }
  }

protected:
  T *                         m_This{};
  TMemberFunctionPointer      m_MemberFunction{nullptr};
  TConstMemberFunctionPointer m_ConstMemberFunction{nullptr};
  MemberCommand() = default;
  virtual ~MemberCommand() = default;
};


template <typename T>
class SimpleMemberCommand : public Command
{
public:
  MDCM_DISALLOW_COPY_AND_MOVE(SimpleMemberCommand);

  typedef void (T::*TMemberFunctionPointer)();
  typedef SimpleMemberCommand Self;
  static SmartPointer<SimpleMemberCommand>
  New()
  {
    return new SimpleMemberCommand;
  }

  void
  SetCallbackFunction(T * object, TMemberFunctionPointer memberFunction)
  {
    m_This = object;
    m_MemberFunction = memberFunction;
  }

  void
  Execute(Subject *, const Event &) override
  {
    if (m_This && m_MemberFunction)
    {
      ((*m_This).*(m_MemberFunction))();
    }
  }

  void
  Execute(const Subject *, const Event &) override
  {
    if (m_This && m_MemberFunction)
    {
      ((*m_This).*(m_MemberFunction))();
    }
  }

protected:
  T *                    m_This{};
  TMemberFunctionPointer m_MemberFunction{nullptr};
  SimpleMemberCommand() = default;
  virtual ~SimpleMemberCommand() = default;
};

} // end namespace mdcm

#endif // MDCMCOMMAND_H
