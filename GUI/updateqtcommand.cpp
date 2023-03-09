#include "updateqtcommand.h"
#include <QApplication>

void UpdateQtCommand::Execute(
	itk::Object * caller,
	const itk::EventObject & e)
{
	Execute(const_cast<const itk::Object*>(caller), e);
}

void UpdateQtCommand::Execute(
	const itk::Object *,
	const itk::EventObject &)
{
	QApplication::processEvents();
}

