#include "infodialog.h"

InfoDialog::InfoDialog()
{
	setupUi(this);
}

InfoDialog::~InfoDialog()
{
}

void InfoDialog::set_text(const QString & t)
{
	textBrowser->setPlainText(t);
}
