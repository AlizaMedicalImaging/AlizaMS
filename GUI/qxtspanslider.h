/****************************************************************************
 **
 ** Copyright (C) Qxt Foundation. Some rights reserved.
 **
 ** This file is part of the QxtGui module of the Qxt library.
 **
 ** This library is free software; you can redistribute it and/or modify it
 ** under the terms of the Common Public License, version 1.0, as published
 ** by IBM, and/or under the terms of the GNU Lesser General Public License,
 ** version 2.1, as published by the Free Software Foundation.
 **
 ** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
 ** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
 ** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
 ** FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** You should have received a copy of the CPL and the LGPL along with this
 ** file. See the LICENSE file and the cpl1.0.txt/lgpl-2.1.txt files
 ** included with the source distribution for more information.
 ** If you did not receive a copy of the licenses, contact the Qxt Foundation.
 **
 ** <http://libqxt.org>  <foundation@libqxt.org>
 **
 ****************************************************************************/
#ifndef A_QXTSPANSLIDER_H
#define A_QXTSPANSLIDER_H

#include <QSlider>

template <typename PUB>
class QxtPrivate
{
public:
	virtual ~QxtPrivate() {}
	inline void QXT_setPublic(PUB* pub)
	{
		qxt_p_ptr = pub;
	}

protected:
	inline PUB& qxt_p()
	{
		return *qxt_p_ptr;
	}
	inline const PUB& qxt_p() const
	{
		return *qxt_p_ptr;
	}

private:
	PUB* qxt_p_ptr;
};

template <typename PUB, typename PVT>
class QxtPrivateInterface
{
	friend class QxtPrivate<PUB>;
public:
	QxtPrivateInterface()
	{
		pvt = new PVT;
	}
	~QxtPrivateInterface()
	{
		delete pvt;
	}

	inline void setPublic(PUB* pub)
	{
		pvt->QXT_setPublic(pub);
	}
	inline PVT& operator()()
	{
		return *static_cast<PVT*>(pvt);
	}
	inline const PVT& operator()() const
	{
		return *static_cast<PVT*>(pvt);
	}
private:
	QxtPrivateInterface(const QxtPrivateInterface&) {}
	// cppcheck-suppress operatorEqVarError
	void operator=(const QxtPrivateInterface&) {}
	QxtPrivate<PUB>* pvt;
};

class QxtSpanSliderPrivate;

class QxtSpanSlider : public QSlider
{
	Q_OBJECT
	friend class QxtSpanSliderPrivate;
	QxtPrivateInterface<QxtSpanSlider, QxtSpanSliderPrivate> qxt_d;
	Q_PROPERTY(int lowerValue READ lowerValue WRITE setLowerValue)
	Q_PROPERTY(int upperValue READ upperValue WRITE setUpperValue)
	Q_PROPERTY(int lowerPosition READ lowerPosition WRITE setLowerPosition)
	Q_PROPERTY(int upperPosition READ upperPosition WRITE setUpperPosition)
	Q_PROPERTY(HandleMovementMode handleMovementMode READ handleMovementMode WRITE setHandleMovementMode)
	Q_ENUMS(HandleMovementMode)

public:
	explicit QxtSpanSlider(QWidget* parent = nullptr);
	explicit QxtSpanSlider(Qt::Orientation orientation, QWidget* parent = nullptr);
	virtual ~QxtSpanSlider();

	enum HandleMovementMode
	{
		FreeMovement,
		NoCrossing,
		NoOverlapping
	};

	HandleMovementMode handleMovementMode() const;
	void setHandleMovementMode(HandleMovementMode mode);

	int lowerValue() const;
	int upperValue() const;

	int lowerPosition() const;
	int upperPosition() const;

public Q_SLOTS:
	void setLowerValue(int lower);
	void setUpperValue(int upper);
	void setSpan(int lower, int upper);

	void setLowerPosition(int lower);
	void setUpperPosition(int upper);

Q_SIGNALS:
	void spanChanged(int lower, int upper);
	void lowerValueChanged(int lower);
	void upperValueChanged(int upper);

	void lowerPositionChanged(int lower);
	void upperPositionChanged(int upper);

protected:
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	virtual void paintEvent(QPaintEvent* event);
};

#endif

