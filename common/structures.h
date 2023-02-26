#ifndef A_STRUCTURES_H
#define A_STRUCTURES_H

#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
#include "CG/glew/include/GL/glew.h"
#endif
#include <itkImage.h>
#include <itkSpatialOrientation.h>
#include <itkSpatialObjectPoint.h>
#include <itkSpatialOrientationAdapter.h>
#include <itkRGBPixel.h>
#include <itkRGBAPixel.h>
#include <itkImageRegionConstIterator.h>
#include <mdcmTag.h>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QMultiMap>
#include <QVariant>
#include <QList>
#include <QThread>
#include <QPixmap>
#include <QPainterPath>
#include "dicom/ultrasoundregiondata.h"
#include "dicom/spectroscopydata.h"

// Assumed is size of 'int' is 32 bit.
// Not tested on big endian platroms (specially MDCM).

class GLWidget;
class qMeshData;

typedef itk::Image<signed short,       3> ImageTypeSS;
typedef itk::Image<unsigned short,     3> ImageTypeUS;
typedef itk::Image<signed int,         3> ImageTypeSI;
typedef itk::Image<unsigned int,       3> ImageTypeUI;
typedef itk::Image<signed char,        3> ImageTypeSC;
typedef itk::Image<unsigned char,      3> ImageTypeUC;
typedef itk::Image<float,              3> ImageTypeF;
typedef itk::Image<double,             3> ImageTypeD;
typedef itk::Image<signed long,        3> ImageTypeSL;
typedef itk::Image<unsigned long,      3> ImageTypeUL;
typedef itk::Image<signed long long,   3> ImageTypeSLL;
typedef itk::Image<unsigned long long, 3> ImageTypeULL;

typedef itk::Image<signed short,       2> Image2DTypeSS;
typedef itk::Image<unsigned short,     2> Image2DTypeUS;
typedef itk::Image<signed int,         2> Image2DTypeSI;
typedef itk::Image<unsigned int,       2> Image2DTypeUI;
typedef itk::Image<signed char,        2> Image2DTypeSC;
typedef itk::Image<unsigned char,      2> Image2DTypeUC;
typedef itk::Image<float,              2> Image2DTypeF;
typedef itk::Image<double,             2> Image2DTypeD;
typedef itk::Image<signed long,        2> Image2DTypeSL;
typedef itk::Image<unsigned long,      2> Image2DTypeUL;
typedef itk::Image<signed long long,   2> Image2DTypeSLL;
typedef itk::Image<unsigned long long, 2> Image2DTypeULL;

typedef itk::Image<signed short,       4> Image4DTypeSS;
typedef itk::Image<unsigned short,     4> Image4DTypeUS;
typedef itk::Image<signed int,         4> Image4DTypeSI;
typedef itk::Image<unsigned int,       4> Image4DTypeUI;
typedef itk::Image<signed char,        4> Image4DTypeSC;
typedef itk::Image<unsigned char,      4> Image4DTypeUC;
typedef itk::Image<float,              4> Image4DTypeF;
typedef itk::Image<double,             4> Image4DTypeD;
typedef itk::Image<signed long,        4> Image4DTypeSL;
typedef itk::Image<unsigned long,      4> Image4DTypeUL;
typedef itk::Image<signed long long,   4> Image4DTypeSLL;
typedef itk::Image<unsigned long long, 4> Image4DTypeULL;

typedef itk::RGBPixel<signed short>   RGBPixelSS;
typedef itk::RGBPixel<unsigned short> RGBPixelUS;
typedef itk::RGBPixel<signed int>     RGBPixelSI;
typedef itk::RGBPixel<unsigned int>   RGBPixelUI;
typedef itk::RGBPixel<signed char>    RGBPixelSC;
typedef itk::RGBPixel<unsigned char>  RGBPixelUC;
typedef itk::RGBPixel<float>          RGBPixelF;
typedef itk::RGBPixel<double>         RGBPixelD;

typedef itk::RGBAPixel<signed short>   RGBAPixelSS;
typedef itk::RGBAPixel<unsigned short> RGBAPixelUS;
typedef itk::RGBAPixel<signed int>     RGBAPixelSI;
typedef itk::RGBAPixel<unsigned int>   RGBAPixelUI;
typedef itk::RGBAPixel<signed char>    RGBAPixelSC;
typedef itk::RGBAPixel<unsigned char>  RGBAPixelUC;
typedef itk::RGBAPixel<float>          RGBAPixelF;
typedef itk::RGBAPixel<double>         RGBAPixelD;

typedef itk::Image<RGBPixelSS, 3> RGBImageTypeSS;
typedef itk::Image<RGBPixelUS, 3> RGBImageTypeUS;
typedef itk::Image<RGBPixelSI, 3> RGBImageTypeSI;
typedef itk::Image<RGBPixelUI, 3> RGBImageTypeUI;
typedef itk::Image<RGBPixelSC, 3> RGBImageTypeSC;
typedef itk::Image<RGBPixelUC, 3> RGBImageTypeUC;
typedef itk::Image<RGBPixelF,  3> RGBImageTypeF;
typedef itk::Image<RGBPixelD,  3> RGBImageTypeD;

typedef itk::Image<RGBPixelSS, 2> RGBImage2DTypeSS;
typedef itk::Image<RGBPixelUS, 2> RGBImage2DTypeUS;
typedef itk::Image<RGBPixelSI, 2> RGBImage2DTypeSI;
typedef itk::Image<RGBPixelUI, 2> RGBImage2DTypeUI;
typedef itk::Image<RGBPixelSC, 2> RGBImage2DTypeSC;
typedef itk::Image<RGBPixelUC, 2> RGBImage2DTypeUC;
typedef itk::Image<RGBPixelF,  2> RGBImage2DTypeF;
typedef itk::Image<RGBPixelD,  2> RGBImage2DTypeD;

typedef itk::Image<RGBAPixelSS, 3> RGBAImageTypeSS;
typedef itk::Image<RGBAPixelUS, 3> RGBAImageTypeUS;
typedef itk::Image<RGBAPixelSI, 3> RGBAImageTypeSI;
typedef itk::Image<RGBAPixelUI, 3> RGBAImageTypeUI;
typedef itk::Image<RGBAPixelSC, 3> RGBAImageTypeSC;
typedef itk::Image<RGBAPixelUC, 3> RGBAImageTypeUC;
typedef itk::Image<RGBAPixelF,  3> RGBAImageTypeF;
typedef itk::Image<RGBAPixelD,  3> RGBAImageTypeD;

typedef itk::Image<RGBAPixelSS, 2> RGBAImage2DTypeSS;
typedef itk::Image<RGBAPixelUS, 2> RGBAImage2DTypeUS;
typedef itk::Image<RGBAPixelSI, 2> RGBAImage2DTypeSI;
typedef itk::Image<RGBAPixelUI, 2> RGBAImage2DTypeUI;
typedef itk::Image<RGBAPixelSC, 2> RGBAImage2DTypeSC;
typedef itk::Image<RGBAPixelUC, 2> RGBAImage2DTypeUC;
typedef itk::Image<RGBAPixelF,  2> RGBAImage2DTypeF;
typedef itk::Image<RGBAPixelD,  2> RGBAImage2DTypeD;

typedef std::vector<std::string>  FileNamesContainer_;
typedef struct { float x, y, z, u, v; int t; } DPoint;
typedef struct { float r, g, b; } ROIcolor;
typedef struct { float r, g, b; } Contourcolor;
typedef struct { float r, g, b; } Meshcolor;
typedef QList<DPoint> ListOfDPoints;

typedef struct
{
	std::string uid;
	mdcm::Tag index_pointer;
	mdcm::Tag group_pointer;
} DimIndex;

typedef std::vector<DimIndex> DimIndexSq;

typedef struct
{
	unsigned int id;
	std::vector<unsigned int> idx;
} DimIndexValue;

typedef std::vector<DimIndexValue> DimIndexValues;

class FrameGroup
{
public:
	FrameGroup() :
		id(-1),
		stack_id(-1),
		in_stack_pos_num(-1),
		temp_pos_idx(-1),
		temp_pos_off(0.0),
		us_temp_pos_unknown(0.0),
		rescale_intercept(0.0),
		rescale_slope(1.0),
		stack_id_ok(false),
		in_stack_pos_num_ok(false),
		temp_pos_idx_ok(false),
		vol_orient_ok(false),
		vol_pos_ok(false),
		temp_pos_off_ok(false),
		us_temp_pos_unknown_ok(false),
		rescale_ok(false)
	{
		vol_pos[0] = 0.0;
		vol_pos[1] = 0.0;
		vol_pos[2] = 0.0;
		vol_orient[0] = 0.0;
		vol_orient[1] = 0.0;
		vol_orient[2] = 0.0;
		vol_orient[3] = 0.0;
		vol_orient[4] = 0.0;
		vol_orient[5] = 0.0;
	}
	~FrameGroup()
	{
	}
	int    id;
	int    stack_id;
	int    in_stack_pos_num;
	int    temp_pos_idx;
	double temp_pos_off;
	double us_temp_pos_unknown;
	double rescale_intercept;
	double rescale_slope;
	QString pat_pos;
	QString pat_orient;
	QString pix_spacing;
	QString slice_thick;
	QString window_center;
	QString window_width;
	QString lut_function;
	QString data_type;
	QString frame_body_part;
	QString frame_laterality;
	QString rescale_type;
	QString frame_acquisition_datetime;
	QString frame_reference_datetime;
	bool stack_id_ok;
	bool in_stack_pos_num_ok;
	bool temp_pos_idx_ok;
	bool vol_orient_ok;
	bool vol_pos_ok;
	bool temp_pos_off_ok;
	bool us_temp_pos_unknown_ok;
	bool rescale_ok;
	double vol_pos[3];
	double vol_orient[6];
};

typedef std::vector<FrameGroup> FrameGroupValues;

typedef struct
{
	unsigned int type;
	QList<QVariant> values;
} GEMSParam;

class Contour
{
public:
	Contour()
	:
	id(-1), roiid(-1), vaoid(0), vboid(0), vao_initialized(false), type(0)
	{
		color.r = 0; color.g = 0; color.b = 0;
	}
	~Contour() {}
	int id;
	int roiid;
	quint32 vaoid; // have to be 32 bit (GLuint)
	quint32 vboid; // have to be 32 bit (GLuint)
	bool vao_initialized;
	// type:
	// 0 - not set
	// 1 - CLOSED_PLANAR
	// 2 - OPEN_PLANAR
	// 3 - OPEN_NONPLANAR
	// 4 - POINT
	// 5 - CLOSEDPLANAR_XOR
	short type;
	// axis:
	// -1 - not set
	//  0 - x
	//  1 - y
	//  2 - z
	Contourcolor color;
	ListOfDPoints dpoints;
	QStringList ref_sop_instance_uids;
	QPainterPath path;
};
typedef QMap< int, Contour* > Contours;
typedef QMultiMap<int,int> ContoursMap;

class ROI
{
public:
	ROI()
	:
	id(-1),
	show(true),
	random_color(false),
	max_delta(0)
	{
		color.r = 0; color.g = 0; color.b = 0;
	}
	~ROI() {}
	int id;
	bool show;
	bool random_color;
	double max_delta;
	QString name;
	QString interpreted_type;
	ROIcolor color;
	QString ref_frame_of_ref;
	Contours contours;
	ContoursMap map;
};
typedef QList<ROI> ROIs;

class TriMesh
{
public:
	TriMesh()
	:
	id(-1),
	visible(true),
	qmesh(NULL),
	initialized(false),
	max_delta(0),
	R(0.9),
	G(0.9),
	B(0.9) {}
	~TriMesh() {}
	int id;
	bool visible;
	qMeshData * qmesh;
	bool initialized;
	double max_delta;
	double R;
	double G;
	double B;
};
typedef QMap<int, TriMesh*> TriMeshes;

class AnatomyDesc
{
public:
	AnatomyDesc()  {}
	~AnatomyDesc() {}
	QString laterality;
	QString body_part;
};
typedef QMap<int, AnatomyDesc> AnatomyMap;

class ImageSlice
{
public:
	ImageSlice()
	{
		v       = new float[12];
		fv      = new float[12];
		tc      = new float[12];
		ipp_iop = new double[9];
		for (int x = 0; x < 12; ++x)
		{
			v[x]  = 0.0f;
			fv[x] = 0.0f;
			tc[x] = 0.0f;
		}
		for (int x = 0; x <  9; ++x)
		{
			ipp_iop[x] = 0.0;
		}
	}
	~ImageSlice()
	{
		delete [] v;
		delete [] fv;
		delete [] tc;
		delete [] ipp_iop;
	}
	float * v;
	float * fv;
	float * tc;
	double  * ipp_iop;
	QString slice_orientation_string;
};
typedef std::vector<ImageSlice*> SlicesVector;

typedef QMap<unsigned int, QString> Orientations_20_20;

class SpectroscopySlice
{
public:
	SpectroscopySlice()
	{
		fvaoid  = 0;
		fvboid  = 0;
		lvaoid  = 0;
		lvboid  = 0;
		pvaoid  = 0;
		pvboid  = 0;
		fv      = new float[12];
		for (int x = 0; x < 12; ++x) { fv[x] = 0.0f; }
		lsize = 0;
		psize = 0;
	}
	~SpectroscopySlice()
	{
		delete [] fv;
	}
	quint32 fvaoid;
	quint32 fvboid;
	quint32 lvaoid;
	quint32 lvboid;
	quint32 pvaoid;
	quint32 pvboid;
	float * fv;
	unsigned long lsize;
	unsigned long psize;
	QString slice_orientation_string;
};
typedef std::vector<SpectroscopySlice*> SpectroscopySlicesVector;

class SliceOverlay
{
public:
	SliceOverlay()
		:
		dimx(0), dimy(0),
		x(0), y(0) {}
	SliceOverlay(const SliceOverlay & j)
		:
		dimx(j.dimx), dimy(j.dimy),
		x(j.x), y(j.y)
	{
		for (size_t k = 0; k < j.data.size(); ++k)
		{
			data.push_back(j.data.at(k));
		}
	}
	~SliceOverlay() {}
	int dimx;
	int dimy;
	int x;
	int y;
	std::vector<char> data;
	SliceOverlay & operator=(const SliceOverlay & j)
	{
		dimx = j.dimx;
		dimy = j.dimy;
		x = j.x;
		y = j.y;
		for (size_t k = 0; k < j.data.size(); ++k)
		{
			data.push_back(j.data.at(k));
		}
		return *this;
	}
};
typedef QList<SliceOverlay> SliceOverlays;

class ImageOverlays
{
public:
	ImageOverlays(){}
	~ImageOverlays(){}
	QMap<int,SliceOverlays> all_overlays;
};

typedef QMap<int,QString> SOPInstanceUids;

typedef QList<UltrasoundRegionData> USRegions;

typedef struct
{
	unsigned int id;
	int instance_number;
	long long slice_position;
} SliceInstance;

class PrConfig
{
public:
	PrConfig() : id(-1), desc(QString("")) {}
	~PrConfig() {}
	int id;
	QString desc;
	QList<QVariant> values;
};

class PrRefImage
{
public:
	PrRefImage()  {}
	~PrRefImage() {}
	QString uid;
	QString file;
	QList<unsigned int> frames;
};

class PrRefSeries
{
public:
	PrRefSeries()  {}
	~PrRefSeries() {}
	QString uid;
	QList<PrRefImage> images;
	QList<PrConfig> prconfig;
	ImageOverlays image_overlays;
};

typedef std::vector<double> FrameTimes;

typedef QMap<int, QString> LabelsMap;

class PresentationStateObj
{
public:
	PresentationStateObj() {}
	~PresentationStateObj() {}
	QString id;
	QString layer_id;
};

class PRDisplayArea : public PresentationStateObj
{
public:
	PRDisplayArea() :
		top_left_x(-1),
		top_left_y(-1),
		bottom_right_x(-1),
		bottom_right_y(-1) {}
	virtual ~PRDisplayArea() {}
	int top_left_x;
	int top_left_y;
	int bottom_right_x;
	int bottom_right_y;
};
typedef QMap<int, PRDisplayArea> PRDisplayAreas;

class PRTextAnnotation : public PresentationStateObj
{
public:
	PRTextAnnotation() :
		has_bb(false),
		has_anchor(false),
		has_textstyle(false),
		bb_top_left_x(-1),
		bb_top_left_y(-1),
		bb_bottom_right_x(-1),
		bb_bottom_right_y(-1),
		anchor_x(-1),
		anchor_y(-1),
		ShadowOffsetX(0.0),
		ShadowOffsetY(0.0),
		ShadowOpacity(1.0),
		TextColorCIELabValue_L(-1),
		TextColorCIELabValue_a(-1),
		TextColorCIELabValue_b(-1),
		ShadowColorCIELabValue_L(-1),
		ShadowColorCIELabValue_a(-1),
		ShadowColorCIELabValue_b(-1),
		CompoundGraphicInstanceID(-1),
		GraphicGroupID(-1) {}
	virtual ~PRTextAnnotation() {}
	bool    has_bb;
	bool    has_anchor;
	bool    has_textstyle;
	double  bb_top_left_x;
	double  bb_top_left_y;
	double  bb_bottom_right_x;
	double  bb_bottom_right_y;
	double  anchor_x;
	double  anchor_y;
	double  ShadowOffsetX;
	double  ShadowOffsetY;
	double  ShadowOpacity;
	int     TextColorCIELabValue_L;
	int     TextColorCIELabValue_a;
	int     TextColorCIELabValue_b;
	int     ShadowColorCIELabValue_L;
	int     ShadowColorCIELabValue_a;
	int     ShadowColorCIELabValue_b;
	int     CompoundGraphicInstanceID;
	int     GraphicGroupID;
	QString BoundingBoxAnnotationUnits;
	QString AnchorPointAnnotationUnits;
	QString UnformattedTextValue;
	QString BoundingBoxTextHorizontalJustification;
	QString AnchorPointVisibility;
	QString FontName;
	QString FontNameType;
	QString CSSFontName;
	QString HorizontalAlignment;
	QString VerticalAlignment;
	QString ShadowStyle;
	QString Underlined;
	QString Bold;
	QString Italic;
	QString TrackingID;
	QString TrackingUID;
};
typedef QMap< int, QList<PRTextAnnotation > > PRTextAnnotations;

class PRGraphicObject : public PresentationStateObj
{
public:
	PRGraphicObject() :
		NumberofGraphicPoints(0),
		LinePatternOnColorCIELabValue_L(0),
		LinePatternOnColorCIELabValue_a(0),
		LinePatternOnColorCIELabValue_b(0),
		LinePatternOffColorCIELabValue_L(0),
		LinePatternOffColorCIELabValue_a(0),
		LinePatternOffColorCIELabValue_b(0),
		LinePatternOnOpacity(1.0),
		LinePatternOffOpacity(1.0),
		LineThickness(0.0),
		LinePattern(1),
		ShadowOffsetX(0.0),
		ShadowOffsetY(0.0),
		ShadowColorCIELabValue_L(0),
		ShadowColorCIELabValue_a(0),
		ShadowColorCIELabValue_b(0),
		ShadowOpacity(1.0),
		FillPatternOnColorCIELabValue_L(0),
		FillPatternOnColorCIELabValue_a(0),
		FillPatternOnColorCIELabValue_b(0),
		FillPatternOffColorCIELabValue_L(0),
		FillPatternOffColorCIELabValue_a(0),
		FillPatternOffColorCIELabValue_b(0),
		FillPatternOnOpacity(1.0),
		FillPatternOffOpacity(1.0),
		CompoundGraphicInstanceID(0),
		GraphicGroupID(0) {}
	virtual ~PRGraphicObject() {}
	unsigned int NumberofGraphicPoints;
	int LinePatternOnColorCIELabValue_L;
	int LinePatternOnColorCIELabValue_a;
	int LinePatternOnColorCIELabValue_b;
	int LinePatternOffColorCIELabValue_L;
	int LinePatternOffColorCIELabValue_a;
	int LinePatternOffColorCIELabValue_b;
	double LinePatternOnOpacity;
	double LinePatternOffOpacity;
	double LineThickness;
	unsigned int LinePattern;
	double ShadowOffsetX;
	double ShadowOffsetY;
	int ShadowColorCIELabValue_L;
	int ShadowColorCIELabValue_a;
	int ShadowColorCIELabValue_b;
	double ShadowOpacity;
	int FillPatternOnColorCIELabValue_L;
	int FillPatternOnColorCIELabValue_a;
	int FillPatternOnColorCIELabValue_b;
	int FillPatternOffColorCIELabValue_L;
	int FillPatternOffColorCIELabValue_a;
	int FillPatternOffColorCIELabValue_b;
	double FillPatternOnOpacity;
	double FillPatternOffOpacity;
	int CompoundGraphicInstanceID;
	int GraphicGroupID;
	QString GraphicType;
	QString GraphicAnnotationUnits;
	QString LineDashingStyle;
	QString ShadowStyle;
	QString GraphicFilled;
	QString FillMode;
	QString TrackingID;
	QString TrackingUID;
	std::vector<float> GraphicData;
	std::vector<unsigned char> FillPattern;
	QPainterPath path;
};
typedef QMap< int, QList<PRGraphicObject > > PRGraphicObjects;

class PRDisplayShutter : public PresentationStateObj
{
public:
	PRDisplayShutter() :
		ShutterLeftVerticalEdge(-1),
		ShutterRightVerticalEdge(-1),
		ShutterUpperHorizontalEdge(-1),
		ShutterLowerHorizontalEdge(-1),
		CenterofCircularShutter_x(-1),
		CenterofCircularShutter_y(-1),
		RadiusofCircularShutter(-1),
		ShutterPresentationValue(0),
		ShutterPresentationColorCIELabValue_L(-1),
		ShutterPresentationColorCIELabValue_a(-1),
		ShutterPresentationColorCIELabValue_b(-1) {}
	virtual ~PRDisplayShutter() {}
	int ShutterLeftVerticalEdge;
	int ShutterRightVerticalEdge;
	int ShutterUpperHorizontalEdge;
	int ShutterLowerHorizontalEdge;
	int CenterofCircularShutter_x;
	int CenterofCircularShutter_y;
	int RadiusofCircularShutter;
	int ShutterPresentationValue;
	int ShutterPresentationColorCIELabValue_L;
	int ShutterPresentationColorCIELabValue_a;
	int ShutterPresentationColorCIELabValue_b;
	QString ShutterShape;
	std::vector<int> VerticesofthePolygonalShutter;
	bool operator==(const PRDisplayShutter & o) const
	{
		if (o.ShutterShape != ShutterShape)
		{
			return false;
		}
		if (o.VerticesofthePolygonalShutter.size() !=
			VerticesofthePolygonalShutter.size())
		{
			return false;
		}
		for (unsigned int x = 0;
			x < VerticesofthePolygonalShutter.size();
			++x)
		{
			if (o.VerticesofthePolygonalShutter.at(x) !=
				VerticesofthePolygonalShutter.at(x))
			{
				return false;
			}
		}
		if (
			(o.ShutterLeftVerticalEdge
				== ShutterLeftVerticalEdge) &&
			(o.ShutterRightVerticalEdge
				== ShutterRightVerticalEdge) &&
			(o.ShutterUpperHorizontalEdge
				== ShutterUpperHorizontalEdge) &&
			(o.ShutterLowerHorizontalEdge
				== ShutterLowerHorizontalEdge) &&
			(o.CenterofCircularShutter_x
				== CenterofCircularShutter_x) &&
			(o.CenterofCircularShutter_y
				== CenterofCircularShutter_y) &&
			(o.RadiusofCircularShutter
				== RadiusofCircularShutter) &&
			(o.ShutterPresentationValue
				== ShutterPresentationValue) &&
			(o.ShutterPresentationColorCIELabValue_L
				== ShutterPresentationColorCIELabValue_L) &&
			(o.ShutterPresentationColorCIELabValue_a
				== ShutterPresentationColorCIELabValue_a) &&
			(o.ShutterPresentationColorCIELabValue_b
				== ShutterPresentationColorCIELabValue_b))
		{
			return true;
		}
		return false;
	}
	bool operator!=(const PRDisplayShutter & o) const
	{
		return !(*this == o);
	}
};
typedef QMap<int, PRDisplayShutter> PRDisplayShutters;

struct FrameLevel
{
	short lut_function;
	double us_window_center, us_window_width;
};
typedef QMap<int, FrameLevel> FrameLevels;

class DisplayInterface
{
public:
	DisplayInterface(const int, const bool, bool, GLWidget*, int);
	~DisplayInterface();
	const int id;
	int lookup_id;
	int paint_id;
	const bool opengl_ok;
	bool skip_texture;
	GLWidget * gl;
	bool disable_int_level;
	bool maxwindow;
	short filtering;
	bool transparency;
	bool lock_2Dview;
	bool lock_single;
	bool lock_level2D;
	quint32 cube_3dtex;
	float origin[3];
	bool  origin_ok;
	short tex_info;
	int idimx, idimy, idimz;
	float ix_origin, iy_origin, iz_origin;
	float dircos[6];
	double ix_spacing, iy_spacing, iz_spacing;
	int dimx, dimy;
	double x_spacing, y_spacing;
	double vmin, vmax;
	double rmin, rmax;
	int   supp_palette_subsciptor;
	float center_x, center_y, center_z;
	float default_center_x, default_center_y, default_center_z;
	float slices_direction_x, slices_direction_y, slices_direction_z;
	float up_direction_x, up_direction_y, up_direction_z;
	short lut_function;
	short default_lut_function;
	double us_window_center, us_window_width;
	double default_us_window_center, default_us_window_width;
	double window_center, window_width;
	bool slices_generated;
	bool slices_from_dicom;
	bool spectroscopy_generated;
	bool hide_orientation;
	short spectroscopy_ref;
	short selected_lut;
	int from_slice;
	int to_slice;
	int irect_index[2];
	int irect_size[2];
	int selected_x_slice;
	int selected_y_slice;
	int selected_z_slice;
	double bb_x_min, bb_x_max, bb_y_min, bb_y_max;
	unsigned short bits_allocated, bits_stored, high_bit;
	double shift_tmp, scale_tmp;
	float R, G, B;
	SlicesVector image_slices;
	SpectroscopySlicesVector spectroscopy_slices;
	ROIs rois;
	TriMeshes trimeshes;
	void close(bool=true);
};

class SRImage
{
public:
	SRImage() : sx(1.0), sy(1.0), p(NULL) {}
	double sx;
	double sy;
	unsigned char * p;
	QImage i;
	SRImage & operator=(const SRImage & j)
	{
		sx = j.sx;
		sy = j.sy;
		p  = j.p;
		i  = j.i;
		return *this;
	}
	SRImage & operator=(SRImage & j)
	{
		sx = j.sx;
		sy = j.sy;
		p  = j.p;
		i  = j.i;
		return *this;
	}
};

class ImageVariant
{
public:
	ImageVariant(int, bool, bool, GLWidget*, int);
	~ImageVariant();
	const int id;
	int group_id;
	int instance_number;
	short image_type;
	bool equi;
	bool one_direction;
	unsigned int orientation;
	bool iod_supported;
	FrameTimes frame_times; // ms
	QString imagetype;
	QString study_uid;
	QString series_uid;
	QString study_date;
	QString study_time;
	QString study_id;
	QString frame_of_ref_uid;
	QString interpretation;
	QString modality;
	QString iod;
	QString sop;
	QString orientation_string;
	QString spect_orientation_string;
	QString series_description;
	QString study_description;
	QString spectroscopy_info;
	QString pat_name;
	QString pat_id;
	QString pat_birthdate;
	QString pat_sex;
	QString pat_weight;
	QString series_date;
	QString series_time;
	QString acquisition_date;
	QString acquisition_time;
	QString hardware;
	QString institution;
	QString hardware_info;
	QString iinfo;
	QString mrtimes;
	QString unit_str;
	QString comment;
	QString ioinfo;
	DisplayInterface * di;
	ImageOverlays image_overlays;
	SOPInstanceUids image_instance_uids;
	USRegions usregions;
	AnatomyMap anatomy;
	PRDisplayAreas    pr_display_areas;
	PRTextAnnotations pr_text_annotations;
	PRGraphicObjects  pr_graphicobjects;
	PRDisplayShutters pr_display_shutters;
	QStringList filenames;
	FrameLevels frame_levels;
	Orientations_20_20 orientations_20_20;
	QPixmap icon;
	QPixmap histogram;
	bool rescale_disabled;
	bool modified;
	bool ybr;
	bool dicom_pixel_signed;
	//
	ImageTypeSS ::Pointer pSS; //0
	ImageTypeUS ::Pointer pUS; //1
	ImageTypeSI ::Pointer pSI; //2
	ImageTypeUI ::Pointer pUI; //3
	ImageTypeUC ::Pointer pUC; //4
	ImageTypeF  ::Pointer pF;  //5
	ImageTypeD  ::Pointer pD;  //6
	ImageTypeSLL::Pointer pSLL;//7
	ImageTypeULL::Pointer pULL;//8
	//
	RGBImageTypeSS::Pointer pSS_rgb;//10
	RGBImageTypeUS::Pointer pUS_rgb;//11
	RGBImageTypeSI::Pointer pSI_rgb;//12
	RGBImageTypeUI::Pointer pUI_rgb;//13
	RGBImageTypeUC::Pointer pUC_rgb;//14
	RGBImageTypeF ::Pointer pF_rgb; //15
	RGBImageTypeD ::Pointer pD_rgb; //16
	//
	RGBAImageTypeSS::Pointer pSS_rgba;// 20
	RGBAImageTypeUS::Pointer pUS_rgba;// 21
	RGBAImageTypeSI::Pointer pSI_rgba;// 22
	RGBAImageTypeUI::Pointer pUI_rgba;// 23
	RGBAImageTypeUC::Pointer pUC_rgba;// 24
	RGBAImageTypeF ::Pointer pF_rgba; // 25
	RGBAImageTypeD ::Pointer pD_rgba; // 26
	//
	// ROI  100
	// Mesh 200
	// Spectroscopy 300
};

class ImageVariant2D
{
public:
	ImageVariant2D();
	~ImageVariant2D();
	short image_type;
	unsigned int idimx, idimy;
	QString orientation_string;
	QString laterality;
	QString body_part;
	//
	Image2DTypeSS ::Pointer pSS; //0
	Image2DTypeUS ::Pointer pUS; //1
	Image2DTypeSI ::Pointer pSI; //2
	Image2DTypeUI ::Pointer pUI; //3
	Image2DTypeUC ::Pointer pUC; //4
	Image2DTypeF  ::Pointer pF;  //5
	Image2DTypeD  ::Pointer pD;  //6
	Image2DTypeSLL::Pointer pSLL;//7
	Image2DTypeULL::Pointer pULL;//8
	//
	RGBImage2DTypeSS::Pointer pSS_rgb;//10
	RGBImage2DTypeUS::Pointer pUS_rgb;//11
	RGBImage2DTypeSI::Pointer pSI_rgb;//12
	RGBImage2DTypeUI::Pointer pUI_rgb;//13
	RGBImage2DTypeUC::Pointer pUC_rgb;//14
	RGBImage2DTypeF ::Pointer pF_rgb; //15
	RGBImage2DTypeD ::Pointer pD_rgb; //16
	//
	RGBAImage2DTypeSS::Pointer pSS_rgba;//20
	RGBAImage2DTypeUS::Pointer pUS_rgba;//21
	RGBAImage2DTypeSI::Pointer pSI_rgba;//22
	RGBAImage2DTypeUI::Pointer pUI_rgba;//23
	RGBAImage2DTypeUC::Pointer pUC_rgba;//24
	RGBAImage2DTypeF ::Pointer pF_rgba; //25
	RGBAImage2DTypeD ::Pointer pD_rgba; //26
	//
};

class ImageContainer
{
public:
	ImageContainer() :
		axis(-1),
		image3D(NULL),
		image2D(NULL),
		selected_x_slice_ext(-1),
		selected_y_slice_ext(-1),
		selected_z_slice_ext(-1),
		us_window_center_ext(0.0),
		us_window_width_ext(1e-6),
		selected_lut_ext(0),
		lut_function_ext(0),
		level_locked_ext(true)
	{}
	~ImageContainer()
	{
		if (image2D)
		{
			delete image2D;
			image2D = NULL;
		}
		image3D = NULL;
	}
	short            axis;
	ImageVariant   * image3D;
	ImageVariant2D * image2D;
	int selected_x_slice_ext;
	int selected_y_slice_ext;
	int selected_z_slice_ext;
	double us_window_center_ext;
	double us_window_width_ext;
	short selected_lut_ext;
	short lut_function_ext;
	bool  level_locked_ext;
	QString orientation_20_20;
};

class ProcessImageThread_ : public QThread
{
public:
	ProcessImageThread_(
		Image2DTypeUC::Pointer image_,
		unsigned char * p_,
		const int size_0_,
		const int size_1_,
		const int index_0_,
		const int index_1_,
		const unsigned int j_)
		:
		image(image_),
		p(p_),
		size_0(size_0_),   size_1(size_1_),
		index_0(index_0_), index_1(index_1_),
		j(j_)
	{
	}
	~ProcessImageThread_(){}
	void run();
private:
	Image2DTypeUC::Pointer image;
	unsigned char * p;
	const int size_0;
	const int size_1;
	const int index_0;
	const int index_1;
	const unsigned int j;
};

#endif
