#ifndef MATRIXBUTTON__H
#define MATRIXBUTTON__H

#include <QWidget>
#include <QToolButton>
#include <QAction>

class QMenu;

class MatrixButton : public QToolButton
{
Q_OBJECT
public:
	MatrixButton(float);
	void emit_matrix_selected(int, int);
	QAction * p_action;

signals:
	void matrix_selected(int, int);

private:
	QMenu * p_menu;
};

#endif // MATRIXBUTTON__H
