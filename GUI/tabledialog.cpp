#include "tabledialog.h"

TableDialog::TableDialog()
{
	setupUi(this);
}

int TableDialog::get_rows() const
{
	return r_spinBox->value();
}

int TableDialog::get_columns() const
{
	return c_spinBox->value();
}

