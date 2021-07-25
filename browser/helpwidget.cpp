#include "helpwidget.h"

HelpWidget::HelpWidget()
{
	setupUi(this);
#ifdef __APPLE__
	minimaze_sc = new QShortcut(QKeySequence("Ctrl+M"), this, SLOT(showMinimized()));
	minimaze_sc->setAutoRepeat(false);
	fullsceen_sc = new QShortcut(QKeySequence("Ctrl+Meta+F"),this,SLOT(showFullScreen()));
	fullsceen_sc->setAutoRepeat(false);
	normal_sc = new QShortcut(QKeySequence("Esc"),this,SLOT(showNormal()));
	normal_sc->setAutoRepeat(false);
#endif
}

HelpWidget::~HelpWidget()
{
}
