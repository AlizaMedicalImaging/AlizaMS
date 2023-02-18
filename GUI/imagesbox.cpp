#include "imagesbox.h"
#include <QVariant>
#include <QPixmap>
#include <QBrush>
#include <QColor>
#include <QByteArray>
#include <QStringList>
#include <QTextDocument>
#include <QToolButton>
#include <QDate>
#include "structures.h"
#include "commonutils.h"
#include "dicomutils.h"

ListWidgetItem2::ListWidgetItem2(QListWidget * p, int t)
	: QListWidgetItem(p, t)
{
	id = -1;
	ivariant = NULL;
	setFlags(flags()|Qt::ItemIsUserCheckable);
	setCheckState(Qt::Unchecked);
}

ListWidgetItem2::ListWidgetItem2(
	const QString & text,
	QListWidget * p,
	int t)
	: QListWidgetItem(p, t)
{
	id = -1;
	ivariant = NULL;
	setFlags(flags()|Qt::ItemIsUserCheckable);
	setCheckState(Qt::Unchecked);
}

ListWidgetItem2::ListWidgetItem2(
	const QIcon & icon,
	const QString & text,
	QListWidget * p,
	int t)
	: QListWidgetItem(p, t)
{
	id = -1;
	ivariant = NULL;
	setFlags(flags()|Qt::ItemIsUserCheckable);
	setCheckState(Qt::Unchecked);
}

ListWidgetItem2::ListWidgetItem2(
	int id_,
	ImageVariant * v,
	const QIcon & icon,
	const QString & text,
	QListWidget * p, int t)
	:
	QListWidgetItem(p, t), id(id_), ivariant(v)
{
	setFlags(flags()|Qt::ItemIsUserCheckable);
	setCheckState(Qt::Unchecked);
}

ListWidgetItem2::ListWidgetItem2(
	int id_,
	ImageVariant * v,
	QListWidget * p,
	int t)
	:
	QListWidgetItem(p, t), id(id_), ivariant(v)
{
	setFlags(flags()|Qt::ItemIsUserCheckable);
	setCheckState(Qt::Unchecked);
}

ListWidgetItem2::~ListWidgetItem2()
{
}

int ListWidgetItem2::get_id() const
{
	return id;
}

ImageVariant * ListWidgetItem2::get_image_from_item()
{
	return ivariant;
}

const ImageVariant * ListWidgetItem2::get_image_from_item_const() const
{
	return const_cast<const ImageVariant*>(ivariant);
}

void TableWidgetItem2::set_id(int i)
{
	id = i;
}

int TableWidgetItem2::get_id() const
{
	return id;
}

namespace
{

const QString css1(
	"span.y     { color:#050505; font-size: medium; }\n"
	"span.ybl   { color:#050505; font-size: large; font-weight: bold; }\n"
	"span.ybm   { color:#050505; font-size: medium; font-weight: bold; }\n"
	"span.ybs   { color:#050505; font-size: small; font-weight: bold; }\n"
	"span.yl    { color:#050505; font-size: large; }\n"
	"span.y2    { color:#050505; font-size: medium; font-style: italic; }\n"
	"span.y4    { color:#050505; font-size: small; font-style: italic; }\n"
	"span.y6    { color:#050505; font-size: small; }\n"
	"span.yl1   { color:#068a06; font-size: large; }\n"
	"span.y3    { color:#068a06; font-size: small; font-style: italic; }\n"
	"span.yl2   { color:#9a0e0e; font-size: large; }\n"
	"span.ys2   { color:#9a0e0e; font-size: small; }\n"
	"span.red   { color:#ff0000; font-size: medium; }\n"
	"span.green { color:#00ff00; font-size: medium; }\n"
	"span.blue  { color:#0000ff; font-size: medium; }\n"
	"span.red2  { color:#aa0505; font-size: small; font-weight: bold; }\n"
	"span.y6    { color:#050505; font-size: small; }\n"
	"span.r2    { color:#550505; font-size: small; }\n"
	"span.g2    { color:#055505; font-size: small; }\n"
	"span.b2    { color:#050555; font-size: small; }\n"
	"span.y7    { color:#050505; font-size: small; font-weight: bold; font-style: italic; }\n"
	"span.y5    { color:#0d0d76; font-size: medium; }\n"
	"span.y8    { color:#0d0d76; font-size: small;}\n"
	"span.y9    { color:#0d0d76; font-size: small; font-weight: bold; font-style: italic; }\n"
	);
const QString head(
	"<html><head><link rel='stylesheet'"
	" type='text/css' href='format.css'></head><body>");
const QString foot("</body></html>");

}

ImagesBox::ImagesBox(float si)
{
	setupUi(this);
	//
	contours_tableWidget->setColumnWidth(0, 24);
	contours_tableWidget->setColumnWidth(1, 24);
	contours_tableWidget->setColumnWidth(2, 140);
	contours_frame->hide();
	//
	listWidget->setIconSize(QSize(96,96));
	listWidget->setMovement(QListView::Static);
	listWidget->setFlow(QListView::TopToBottom);
	//
	actionNone         = new QAction(QString(""),this);
	actionClear        = new QAction(QIcon(QString(":/bitmaps/delete.svg")),QString("Close selected"),this);
	actionClearChecked = new QAction(QIcon(QString(":/bitmaps/delete.svg")),QString("Close checked"),this);
	actionClearUnChek  = new QAction(QIcon(QString(":/bitmaps/delete.svg")),QString("Close un-checked"),this);
	actionClearAll     = new QAction(QIcon(QString(":/bitmaps/delete2.svg")),QString("Close all"),this);
	actionCheck        = new QAction(QIcon(QString(":/bitmaps/checked.svg")),QString("Set all to checked (list)"),this);
	actionUncheck      = new QAction(QIcon(QString(":/bitmaps/unchecked.svg")),QString("Set all to un-checked (list)"),this);
	actionReloadHistogram = new QAction(QIcon(QString(":/bitmaps/chart0.svg")),QString("Calculate histogram"),this);
	actionColor        = new QAction(QIcon(QString(":/bitmaps/rgb.svg")),QString("Image color in UI"),this);
	actionDICOMMeta    = new QAction(QIcon(QString(":/bitmaps/meta.svg")),QString("Image DICOM Metadata"),this);
	actionContours     = new QAction(QIcon(QString(":/bitmaps/circle1.svg")),QString("Toggle contours window"), this);
	actionContours->setCheckable(true);
	actionContours->setChecked(false);
	actionROIInfo      = new QAction(QString("ROI Info"), this);
	actionStudyMenu    = new QAction(QIcon(QString(":/bitmaps/user.svg")),QString("Multi View"),this);
	actionStudy        = new QAction(QIcon(QString(":/bitmaps/user.svg")),QString("Open study"), this);
	actionStudyChecked = new QAction(QIcon(QString(":/bitmaps/user.svg")),QString("Open sel. and checked"), this);
#if 0
	actionStudyAll     = new QAction(QIcon(QString(":/bitmaps/user.svg")),QString("Open all"), this);
#endif
	//
	actionTmp          = new QAction(QString("TMP"),this);
	//
	actionInfo         = new QAction(QIcon(QString(":/bitmaps/info2.svg")),QString("Toggle info window"),this);
	actionInfo->setCheckable(true);
	actionInfo->setChecked(true);
	//
	listWidget->addAction(actionNone);
	listWidget->addAction(actionCheck);
	listWidget->addAction(actionUncheck);
	listWidget->addAction(actionClearChecked);
	listWidget->addAction(actionClearUnChek);
	listWidget->addAction(actionColor);
	listWidget->addAction(actionDICOMMeta);
#if 0
	listWidget->addAction(actionTmp);
#endif
	//
	QToolBar * toolbar = new QToolBar(this);
	toolbar->setOrientation(Qt::Horizontal);
	toolbar->setIconSize(QSize(static_cast<int>(18*si),static_cast<int>(18*si)));
	toolbar->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
	if (toolbar->layout())
	{
		toolbar->layout()->setContentsMargins(0,0,0,0);
		toolbar->layout()->setSpacing(0);
	}
	QVBoxLayout * l = new QVBoxLayout(toolbar_frame);
	l->setContentsMargins(0,0,0,0);
	l->setSpacing(0);
	l->addWidget(toolbar);
	studyMenu = new QMenu(this);
	studyMenu->addAction(actionStudy);
	studyMenu->addAction(actionStudyChecked);
#if 0
	studyMenu->addAction(actionStudyAll);
#endif
	actionStudyMenu->setMenu(studyMenu);
	toolbar->addAction(actionStudyMenu);
	toolbar->addAction(actionReloadHistogram);
	toolbar->addAction(actionClear);
	toolbar->addAction(actionClearAll);
	QWidget * spacer = new QWidget(this);
	spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	spacer->setMinimumSize(96,0);
#endif
	toolbar->addWidget(spacer);
	//
	contours_tableWidget->addAction(actionNone);
	contours_tableWidget->addAction(actionROIInfo);
	//
	QToolBar * toolbar2 = new QToolBar(this);
	toolbar2->setOrientation(Qt::Horizontal);
	toolbar2->setIconSize(QSize(static_cast<int>(18*si),static_cast<int>(18*si)));
	toolbar2->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
	toolbar2->setLayoutDirection(Qt::RightToLeft);
	if (toolbar2->layout())
	{
		toolbar2->layout()->setContentsMargins(0,0,0,0);
		toolbar2->layout()->setSpacing(0);
	}
	QVBoxLayout * l2 = new QVBoxLayout(toolbar2_frame);
	l2->setContentsMargins(0,0,0,0);
	l2->setSpacing(0);
	l2->addWidget(toolbar2);
	QWidget * spacer2 = new QWidget(this);
	spacer2->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	toolbar2->addWidget(spacer2);
	toolbar2->addAction(actionContours);
	toolbar2->addAction(actionInfo);
	//
	map['R'] = QString("right to left");
	map['L'] = QString("left to right");
	map['A'] = QString("anterior to posterior");
	map['P'] = QString("posterior to anterior");
	map['I'] = QString("inferior to superior");
	map['S'] = QString("superior to inferior");
	textBrowser->document()->addResource(QTextDocument::StyleSheetResource, QUrl("format.css"), css1);
	//
	connect(actionInfo, SIGNAL(toggled(bool)), this, SLOT(toggle_info(bool)));
	connect(actionContours, SIGNAL(toggled(bool)), this, SLOT(toggle_contours(bool)));
}

ImagesBox::~ImagesBox()
{
}

void ImagesBox::add_image(int id, ImageVariant * v, QPixmap * pixmap)
{
	if (!v) return;
	ListWidgetItem2 * i = new ListWidgetItem2(id, v);
	if (pixmap && !pixmap->isNull()) i->setIcon(*pixmap);
	i->setText(
		QVariant(id).toString()+QString(". ")+
		v->modality+QString("\n")+v->series_description);
	listWidget->addItem(static_cast<QListWidgetItem*>(i));
}

void ImagesBox::set_html(const ImageVariant * v)
{
	textBrowser->clearHistory();
	if (!v)
	{
		textBrowser->document()->setHtml(
			QString("<html><body></body></html>"));
		return;
	}
	QString html = QString(head);
//////////////////////////////////////////////////////////////////
//
// DICOM
//
//
	if (!v->modality.isEmpty())
		html.append(
			QString("<span class='ybl'>") +
			v->modality +
			QString("</span> "));

	if (!v->series_description.isEmpty())
		html.append(QString("<span class='y'>") +
			v->series_description +
			QString("</span><br />"));
	else
		html.append(QString("<br />"));
	if (!v->iod.isEmpty())
	{
		if (v->iod_supported)
			html.append(QString("<span class='y3'>") +
				v->iod + QString("</span><br /><br />"));
		else
			html.append(QString("<span class='y4'>") +
				v->iod + QString("</span><br /><br />"));
	}
	if (!v->hardware.isEmpty())
		html.append(QString("<span class='y5'>") +
				v->hardware + QString("</span><br />"));
	if (!v->hardware_info.isEmpty())
	{
		html.append(
			QString("<span class='y8'>") +
			v->hardware_info.trimmed() +
			QString("</span><br /><br />"));
	}
	else { html.append(QString("<br />")); }
	if (!v->study_description.isEmpty())
		html.append(QString("<span class='y'>") +
			v->study_description + QString("</span><br /><br />"));
	if (!v->pat_name.isEmpty()||!v->pat_birthdate.isEmpty())
	{
		QString name_s =
			(!v->pat_name.isEmpty())
			?
			DicomUtils::convert_pn_value(v->pat_name)
			:
			QString("");
		QString birthdate_s("");
		if (!v->pat_birthdate.isEmpty())
		{
			const QDate qd =
				QDate::fromString(
					v->pat_birthdate, QString("yyyyMMdd"));
			birthdate_s =
				QString(" (") +
				qd.toString(QString("d MMM yyyy")) +
				QString(")");
		}
		html.append(
			QString("<span class='y'>") + name_s +
			birthdate_s + QString("</span><br />"));
	}
	if (!v->institution.isEmpty())
	{
		html.append(
			QString("<span class='y'>") +
			v->institution +
			QString("</span><br />"));
	}
	if (!v->study_date.isEmpty())
	{
		const QDate qd =
			QDate::fromString(
				v->study_date,
				QString("yyyyMMdd"));
		html.append(
			QString("<span class='y6'>") +
			qd.toString(QString("d MMM yyyy")) +
			QString("</span><br /><br />"));
	}
	else
	{
		if (!v->institution.isEmpty())
			html.append(QString("<br />"));
	}
//
//
//
//
//////////////////////////////////////////////////////////////////
	if (v->image_type==100)
	{
//////////////////////////////////////////////////////////////////
//
// Contours
//
//
		html.append(QString(
			"<span class='y'>"
			"Contours"
			"</span><br />"));
//
//
//
//
//////////////////////////////////////////////////////////////////
	}
	else if (v->image_type==200)
	{
//////////////////////////////////////////////////////////////////
//
// Mesh
//
//
		html.append(QString(
			"<span class='y'>"
			"Mesh"
			"</span><br />"));
//
//
//
//
//////////////////////////////////////////////////////////////////
	}
	else if (v->image_type==300)
	{
//////////////////////////////////////////////////////////////////
//
// Spectroscopy
//
//
		html.append(
			QString(
				"<span class='ybm'>"
				"Spectroscopy"
				"</span><br />") +
			v->spectroscopy_info +
			QString("<br />"));
//
//
//
//
//////////////////////////////////////////////////////////////////
	}
	else if (v->image_type>=0 && v->image_type<=26)
	{
//////////////////////////////////////////////////////////////////
//
// Image pixel
//
//
		QString t("");
		if (!v->interpretation.isEmpty())
			t.append(v->interpretation+QString("<br />"));
		if (v->di->idimx>0 && v->di->idimy>0 && v->di->idimz>0)
			t.append(QVariant(v->di->idimx).toString() + QString(" x ") +
					QVariant(v->di->idimy).toString()  + QString(" x ") +
					QVariant(v->di->idimz).toString()  + QString("<br />"));
		const QString cs = v->ybr ? QString("Y'CbCr") : QString("RGB"); // TODO
		switch(v->image_type)
		{
		case  0: t.append(QString("signed short"));       break;
		case  1: t.append(QString("unsigned short"));     break;
		case  2: t.append(QString("signed int"));         break;
		case  3: t.append(QString("unsigned int"));       break;
		case  4: t.append(QString("unsigned char"));      break;
		case  5: t.append(QString("float"));              break;
		case  6: t.append(QString("double"));             break;
		case  7: t.append(QString("signed long long"));   break;
		case  8: t.append(QString("unsigned long long")); break;
		case 10: t.append(QString("RGB signed short"));   break;
		case 11: t.append(QString("RGB unsigned short")); break;
		case 12: t.append(QString("RGB signed int"));     break;
		case 13: t.append(QString("RGB unsigned int"));   break;
		case 14: t.append(cs+QString(" unsigned char"));  break;
		case 15: t.append(QString("RGB float"));          break;
		case 16: t.append(QString("RGB double"));         break;
		case 20: t.append(QString("RGBA signed short"));  break;
		case 21: t.append(QString("RGBA unsigned short"));break;
		case 22: t.append(QString("RGBA signed int"));    break;
		case 23: t.append(QString("RGBA unsigned int"));  break;
		case 24: t.append(QString("RGBA unsigned char")); break;
		case 25: t.append(QString("RGBA float"));         break;
		case 26: t.append(QString("RGBA double"));        break;
		default: break;
		}
		if (v->di->bits_allocated>0 && v->di->bits_stored>0 &&
			(v->di->bits_stored<v->di->bits_allocated) &&
			(v->di->high_bit==v->di->bits_stored-1))
		{
			t.append(
				QString("&#160;(stored&#160;") +
				QVariant(static_cast<int>(v->di->bits_stored)).toString() +
				QString("&#160;bit)"));
		}
		if (v->image_type >= 0 && v->image_type < 10)
		{
			t.append(
				QString("<br />[") +
				QVariant(v->di->vmin).toString() +
				QString(", ") +
				QVariant(v->di->vmax).toString() +
				QString("]"));
		}
		html.append(
			QString("<span class='y6'>") +
			t +
			QString("</span><br />"));
		if (v->rescale_disabled)
		{
			html.append(QString(
				"<span class='red2'>"
				"Rescale was disabled in settings"
				"</span><br />"));
		}
//
//
//
//
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
//
// Orientation
//
//
		if (
			v->equi &&
			!v->di->hide_orientation &&
			!v->orientation_string.isEmpty())
		{
			html.append(QString("<span class='y2'>RAI-Code: </span>"));
			if (v->iod_supported && v->di->slices_from_dicom)
				html.append(
					QString("<span class='yl1'>") +
					v->orientation_string+QString("</span>"));
			else
				html.append(
					QString("<span class='yl2'>") +
					v->orientation_string+QString("</span>"));
		}
		else
		{
			if (
				v->di->slices_from_dicom &&
				!v->equi &&
				!v->di->hide_orientation)
			{
				if (v->one_direction)
				{
					html.append(QString(
						"<span class='y6'>"
						"Non-uniform, parallel"
						"</span>"));
				}
				else
				{
					html.append(QString(
						"<span class='y6'>"
						"Non-uniform"
						"</span>"));
				}
			}
			else
			{
				html.append(
					QString("<span class='y'>") +
					QString(QChar(0xD8)) +
					QString("</span>"));
			}
		}
#if 1
		if (
			v->equi &&
			!v->di->hide_orientation &&
			!v->orientation_string.isEmpty())
		{
			html.append(get_orientation_image(v->orientation_string));
		}
#if 0
		if (v->equi && !v->di->image_slices.empty())
		{
			html.append(
				QString(
					"<span class='ybs'>") +
				QVariant(CommonUtils::set_digits(
					v->di->image_slices.at(0)->ipp_iop[3],3))
						.toString() +
				QString("\\") +
				QVariant(CommonUtils::set_digits(
					v->di->image_slices.at(0)->ipp_iop[4],3))
						.toString() +
				QString("\\") +
				QVariant(CommonUtils::set_digits(
					v->di->image_slices.at(0)->ipp_iop[5],3))
						.toString() +
				QString("\\") +
				QVariant(CommonUtils::set_digits(
					v->di->image_slices.at(0)->ipp_iop[6],3))
						.toString() +
				QString("\\") +
				QVariant(CommonUtils::set_digits(
					v->di->image_slices.at(0)->ipp_iop[7],3))
						.toString() +
				QString("\\") +
				QVariant(CommonUtils::set_digits(
					v->di->image_slices.at(0)->ipp_iop[8],3))
						.toString() +
				QString("</span><br />"));
		}
#endif
#else
		html.append(QString("<br />"));
#endif
//
//
//
//
//////////////////////////////////////////////////////////////////

	}
//
//
//
//
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
//
// DICOM Image Type
//
//
	if (!v->imagetype.isEmpty()) html.append(v->imagetype);
	else                         html.append(QString("<br />"));
//
//
//
//
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
//
// DICOM MR
//
//
	if (!v->iinfo.isEmpty()) html.append(v->iinfo);
//
//
//
//
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
//
// OpenGL
//
//
	if (v->di->opengl_ok && v->image_type >= 0 && v->image_type <= 26)
	{
		if (!v->di->skip_texture)
		{
			QString texfilt, texinfo;
			if      (v->di->filtering==0) texfilt = QString(" (no filtering)");
			else if (v->di->filtering==1) texfilt = QString(" (bilinear)");
			else                          texfilt = QString(" (?)");
			if      (v->di->tex_info==0)  texinfo = QString(" R16F ");
			else if (v->di->tex_info==1)  texinfo = QString(" R16 ");
			else if (v->di->tex_info==2)  texinfo = QString(" R8 ");
			else                          texinfo = QString(" ? ");
			html.append(
				QString("<span class='y4'>3D GPU views:<br />") +
				texinfo +
				QVariant(v->di->dimx).toString()  + QString("x") +
				QVariant(v->di->dimy).toString()  + QString("x") +
				QVariant(v->di->idimz).toString() + texfilt +
				QString("<br /></span>"));
		}
		else
		{
			if (v->image_type>=0 && v->image_type<10)
			{
				html.append(QString(
					"<span class='y4'>3D GPU views"
					":</span><br />"
					"<span class='y7'>Texture "
					"is disabled</span><br />"));
			}
		}
	}
//
//
//
//
//////////////////////////////////////////////////////////////////
	html.append(foot);
	textBrowser->document()->setHtml(html);
	QTextCursor cursor = textBrowser->textCursor();
	cursor.setPosition(0);
	textBrowser->setTextCursor(cursor);
	textBrowser->ensureCursorVisible();
}

QString ImagesBox::get_orientation_image(const QString & rai)
{
	if (rai.isEmpty()) return QString("");
	QString s("");
	const QByteArray ba = rai.toLatin1().constData();
	if (ba.size() >= 3)
	{
		s.append(QString("<br />"));
		s.append(
			QString("<span class='ybs'>X&#160;</span><span class='y4'>") +
				map.value(ba.at(0))+QString("</span><br />"));
		s.append(
			QString("<span class='ybs'>Y&#160;</span><span class='y4'>") +
				map.value(ba.at(1))+QString("</span><br />"));
		s.append(
			QString("<span class='ybs'>Z&#160;</span><span class='y4'>") +
				map.value(ba.at(2))+QString("</span><br />"));
	}
	return s;
}

void ImagesBox::toggle_info(bool t)
{
	textBrowser->setHidden(!t);
}

void ImagesBox::toggle_contours(bool t)
{
	contours_tableWidget->clearSelection();
	contours_tableWidget->setCurrentIndex(QModelIndex());
	if (t) contours_frame->show();
	else   contours_frame->hide();
}

void ImagesBox::check_all()
{
	for (int x = 0; x < listWidget->count(); ++x)
	{
		if (listWidget->item(x) &&
			listWidget->item(x)->checkState()==Qt::Unchecked)
			listWidget->item(x)->setCheckState(Qt::Checked);
	}
}

void ImagesBox::uncheck_all()
{
	for (int x = 0; x < listWidget->count(); ++x)
	{
		if (listWidget->item(x) &&
			listWidget->item(x)->checkState()==Qt::Checked)
			listWidget->item(x)->setCheckState(Qt::Unchecked);
	}
}

void ImagesBox::set_contours(const ImageVariant * v)
{
	contours_tableWidget->clearContents();
	contours_tableWidget->setRowCount(0);
	contours_tableWidget->setCurrentIndex(QModelIndex());
	if (!v) return;
	if (v->di->rois.empty()) return;
	for (int x = 0; x < v->di->rois.size(); ++x)
	{
		const QColor c(
			static_cast<int>(v->di->rois.at(x).color.r*255.0f),
			static_cast<int>(v->di->rois.at(x).color.g*255.0f),
			static_cast<int>(v->di->rois.at(x).color.b*255.0f));
		const int idx = contours_tableWidget->rowCount();
		TableWidgetItem2 * i0 = new TableWidgetItem2();
		i0->setFlags(i0->flags()|Qt::ItemIsUserCheckable);
		if (v->di->rois.at(x).show)
		{
			i0->setCheckState(Qt::Checked);
		}
		else
		{
			i0->setCheckState(Qt::Unchecked);
		}
		i0->set_id(v->di->rois.at(x).id);
		contours_tableWidget->setRowCount(idx+1);
		contours_tableWidget->setItem(
			idx,0,static_cast<QTableWidgetItem*>(i0));
		QTableWidgetItem * i1 = new QTableWidgetItem();
		i1->setFlags(i1->flags()^Qt::ItemIsSelectable);
		i1->setBackground(QBrush(c));
		contours_tableWidget->setItem(
			idx,1,i1);
		contours_tableWidget->setItem(
			idx,2,new QTableWidgetItem(v->di->rois.at(x).name));
	}
}

int ImagesBox::get_selected_roi_id() const
{
	if (contours_tableWidget->rowCount() < 1)
	{
		return -1;
	}
	const int row = contours_tableWidget->row(
		contours_tableWidget->currentItem());
	if (row < 0)
	{
		return -1;
	}
	const TableWidgetItem2 * i =
		static_cast<const TableWidgetItem2*>(
			contours_tableWidget->item(row, 0));
	if (i)
	{
		return i->get_id();
	}
	return -1;
}

void ImagesBox::update_background_color(bool b)
{
	if (b)
	{
		textBrowser->setStyleSheet("QTextEdit { background-color: rgb(83, 89, 96); }");
	}
	else
	{
		textBrowser->setStyleSheet("QTextEdit { background-color: rgb(210, 210, 210); }");
	}
}
