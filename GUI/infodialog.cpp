#include "infodialog.h"

InfoDialog::InfoDialog()
{
	setupUi(this);
}

void InfoDialog::set_text(const QString & t)
{
	textBrowser->setPlainText(t);
}
