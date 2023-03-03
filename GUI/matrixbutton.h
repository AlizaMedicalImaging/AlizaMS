#ifndef A_MATRIXBUTTON_H
#define A_MATRIXBUTTON_H

#include <QWidget>
#include <QToolButton>
#include <QAction>

#define MATRIX_BUTTON_CUSTOM_ACT 0

class QMenu;

class MatrixButton : public QToolButton
{
Q_OBJECT
public:
	MatrixButton(float);
	void emit_matrix_selected(int, int);
#if MATRIX_BUTTON_CUSTOM_ACT == 1
	QAction * p_action;
#endif

signals:
	void matrix_selected(int, int);

private:
	QMenu * p_menu;
};

#endif

