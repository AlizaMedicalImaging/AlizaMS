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
#ifndef MDCMSIMPLESUBJECTWATCHER_H
#define MDCMSIMPLESUBJECTWATCHER_H

#include "mdcmSubject.h"
#include "mdcmCommand.h"
#include "mdcmSmartPointer.h"
#include "mdcmDataEvent.h"

namespace mdcm
{

class Event;
/**
 * SimpleSubjectWatcher
 * This is a typical Subject Watcher class. It will observe all events.
 */
class MDCM_EXPORT SimpleSubjectWatcher
{
public:
  SimpleSubjectWatcher(Subject * s, const char * comment = "");
  virtual ~SimpleSubjectWatcher();
protected:
  virtual void StartFilter();
  virtual void EndFilter();
  virtual void ShowProgress(Subject * caller, const Event & evt);
  virtual void ShowFileName(Subject * caller, const Event & evt);
  virtual void ShowIteration();
  virtual void ShowDataSet(Subject * caller, const Event & evt);
  virtual void ShowData(Subject * caller, const Event & evt);
  virtual void ShowAbort();
protected:
  void TestAbortOn();
  void TestAbortOff();
private:
  SmartPointer<Subject> m_Subject;
  std::string m_Comment;
  typedef SimpleMemberCommand<SimpleSubjectWatcher> SimpleCommandType;
  typedef MemberCommand<SimpleSubjectWatcher> CommandType;
  SmartPointer<SimpleCommandType> m_StartFilterCommand;
  SmartPointer<SimpleCommandType> m_EndFilterCommand;
  SmartPointer<CommandType> m_ProgressFilterCommand;
  SmartPointer<CommandType> m_FileNameFilterCommand;
  SmartPointer<SimpleCommandType> m_IterationFilterCommand;
  SmartPointer<SimpleCommandType> m_AbortFilterCommand;
  SmartPointer<CommandType> m_DataFilterCommand;
  SmartPointer<CommandType> m_DataSetFilterCommand;
  unsigned long m_StartTag;
  unsigned long m_EndTag;
  unsigned long m_ProgressTag;
  unsigned long m_FileNameTag;
  unsigned long m_IterationTag;
  unsigned long m_AbortTag;
  unsigned long m_DataTag;
  unsigned long m_DataSetTag;
  bool m_TestAbort;
  SimpleSubjectWatcher(const SimpleSubjectWatcher &);  // Not implemented
  void operator=(const SimpleSubjectWatcher &);  // Not implemented
};
} // end namespace mdcm

#endif //MDCMSIMPLESUBJECTWATCHER_H
