#ifndef A_STRUCTURES_H
#define A_STRUCTURES_H

#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
#include "CG/glew/include/GL/glew.h"
#endif
#include <itkImage.h>
#include <itkRGBPixel.h>
#include <itkRGBAPixel.h>
#include <mdcmTag.h>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QMultiMap>
#include <QVariant>
#include <QList>
#include <QPixmap>
#include <QPainterPath>
#include "dicom/ultrasoundregiondata.h"
#include "dicom/spectroscopydata.h"

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

struct DPoint { float x, y, z, u, v; int t; };
struct ROIcolor { float r, g, b; };
struct Contourcolor { float r, g, b; };
struct Meshcolor { float r, g, b; };
typedef QList<DPoint> ListOfDPoints;

class Contour
{
public:
	Contour() = default;
	~Contour() = default;
	int id{-1};
	int roiid{-1};
	quint32 vaoid{}; // GLuint
	quint32 vboid{}; // GLuint
	bool vao_initialized{};
	// type:
	// 0 - not set
	// 1 - CLOSED_PLANAR
	// 2 - OPEN_PLANAR
	// 3 - OPEN_NONPLANAR
	// 4 - POINT
	// 5 - CLOSEDPLANAR_XOR
	short type{};
	Contourcolor color{};
	ListOfDPoints dpoints;
	QStringList ref_sop_instance_uids;
	QPainterPath path;
};
typedef QMap< int, Contour* > Contours;
typedef QMultiMap<int,int> ContoursMap;

class ROI
{
public:
	ROI() = default;
	~ROI() = default;
	int id{-1};
	bool show{true};
	bool random_color{};
	double max_delta{};
	QString name;
	QString interpreted_type;
	ROIcolor color{};
	QString ref_frame_of_ref;
	Contours contours;
	ContoursMap map;
};
typedef QList<ROI> ROIs;

class TriMesh
{
public:
	TriMesh() = default;
	~TriMesh() = default;
	int id{-1};
	bool visible{true};
	qMeshData * qmesh{};
	bool initialized{};
	double max_delta{};
	double R{0.9};
	double G{0.9};
	double B{0.9};
};
typedef QMap<int, TriMesh*> TriMeshes;

class AnatomyDesc
{
public:
	AnatomyDesc() = default;
	~AnatomyDesc() = default;
	QString laterality;
	QString body_part;
};
typedef QMap<int, AnatomyDesc> AnatomyMap;

class SegmentationInfo
{
public:
	SegmentationInfo() = default;
	~SegmentationInfo() = default;
	int ref_segment_num{-1};
	int R{};
	int G{};
	int B{};
	QString label;
};

class ImageSlice
{
public:
	ImageSlice()
	{
		v       = new float[12]{};
		fv      = new float[12]{};
		tc      = new float[12]{};
		ipp_iop = new double[9]{};
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
	double * ipp_iop;
	QString slice_orientation_string;
};
typedef std::vector<ImageSlice*> SlicesVector;

typedef QMap<unsigned int, QString> Orientations_20_20;

class SpectroscopySlice
{
public:
	SpectroscopySlice()
	{
		fv = new float[12]{};
	}
	~SpectroscopySlice()
	{
		delete [] fv;
	}
	quint32 fvaoid{};
	quint32 fvboid{};
	quint32 lvaoid{};
	quint32 lvboid{};
	quint32 pvaoid{};
	quint32 pvboid{};
	float * fv;
	unsigned long lsize{};
	unsigned long psize{};
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
	~SliceOverlay() = default;
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
	ImageOverlays() = default;
	~ImageOverlays() = default;
	QMap<int,SliceOverlays> all_overlays;
};

typedef QMap<int,QString> SOPInstanceUids;

typedef QList<UltrasoundRegionData> USRegions;

struct SliceInstance
{
	unsigned int id;
	int instance_number;
	long long slice_position;
};

typedef std::vector<double> FrameTimes;

typedef QMap<int, QString> LabelsMap;

class PresentationStateObj
{
public:
	PresentationStateObj() = default;
	virtual ~PresentationStateObj() = default;
	QString id;
	QString layer_id;
};

class PRDisplayArea : public PresentationStateObj
{
public:
	PRDisplayArea() = default;
	~PRDisplayArea() = default;
	int top_left_x{-1};
	int top_left_y{-1};
	int bottom_right_x{-1};
	int bottom_right_y{-1};
};
typedef QMap<int, PRDisplayArea> PRDisplayAreas;

class PRTextAnnotation : public PresentationStateObj
{
public:
	PRTextAnnotation() = default;
	~PRTextAnnotation() = default;
	bool    has_bb{};
	bool    has_anchor{};
	bool    has_textstyle{};
	double  bb_top_left_x{-1};
	double  bb_top_left_y{-1};
	double  bb_bottom_right_x{-1};
	double  bb_bottom_right_y{-1};
	double  anchor_x{-1};
	double  anchor_y{-1};
	double  ShadowOffsetX{};
	double  ShadowOffsetY{};
	double  ShadowOpacity{1.0};
	int     TextColorCIELabValue_L{-1};
	int     TextColorCIELabValue_a{-1};
	int     TextColorCIELabValue_b{-1};
	int     ShadowColorCIELabValue_L{-1};
	int     ShadowColorCIELabValue_a{-1};
	int     ShadowColorCIELabValue_b{-1};
	int     CompoundGraphicInstanceID{-1};
	int     GraphicGroupID{-1};
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
	PRGraphicObject() = default;
	~PRGraphicObject() = default;
	unsigned int NumberofGraphicPoints{};
	int LinePatternOnColorCIELabValue_L{};
	int LinePatternOnColorCIELabValue_a{};
	int LinePatternOnColorCIELabValue_b{};
	int LinePatternOffColorCIELabValue_L{};
	int LinePatternOffColorCIELabValue_a{};
	int LinePatternOffColorCIELabValue_b{};
	double LinePatternOnOpacity{1.0};
	double LinePatternOffOpacity{1.0};
	double LineThickness{};
	unsigned int LinePattern{1};
	double ShadowOffsetX{};
	double ShadowOffsetY{};
	int ShadowColorCIELabValue_L{};
	int ShadowColorCIELabValue_a{};
	int ShadowColorCIELabValue_b{};
	double ShadowOpacity{1.0};
	int FillPatternOnColorCIELabValue_L{};
	int FillPatternOnColorCIELabValue_a{};
	int FillPatternOnColorCIELabValue_b{};
	int FillPatternOffColorCIELabValue_L{};
	int FillPatternOffColorCIELabValue_a{};
	int FillPatternOffColorCIELabValue_b{};
	double FillPatternOnOpacity{1.0};
	double FillPatternOffOpacity{1.0};
	int CompoundGraphicInstanceID{};
	int GraphicGroupID{};
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
	PRDisplayShutter() = default;
	~PRDisplayShutter() = default;
	int ShutterLeftVerticalEdge{-1};
	int ShutterRightVerticalEdge{-1};
	int ShutterUpperHorizontalEdge{-1};
	int ShutterLowerHorizontalEdge{-1};
	int CenterofCircularShutter_x{-1};
	int CenterofCircularShutter_y{-1};
	int RadiusofCircularShutter{-1};
	int ShutterPresentationValue{};
	int ShutterPresentationColorCIELabValue_L{-1};
	int ShutterPresentationColorCIELabValue_a{-1};
	int ShutterPresentationColorCIELabValue_b{-1};
	QString ShutterShape;
	std::vector<int> VerticesofthePolygonalShutter;
	bool operator==(const PRDisplayShutter & o) const
	{
		if (o.ShutterShape != ShutterShape)
		{
			return false;
		}
		if (o.VerticesofthePolygonalShutter.size() != VerticesofthePolygonalShutter.size())
		{
			return false;
		}
		for (unsigned int x = 0;
			x < VerticesofthePolygonalShutter.size();
			++x)
		{
			if (o.VerticesofthePolygonalShutter.at(x) != VerticesofthePolygonalShutter.at(x))
			{
				return false;
			}
		}
		if ((o.ShutterLeftVerticalEdge == ShutterLeftVerticalEdge) &&
			(o.ShutterRightVerticalEdge == ShutterRightVerticalEdge) &&
			(o.ShutterUpperHorizontalEdge == ShutterUpperHorizontalEdge) &&
			(o.ShutterLowerHorizontalEdge == ShutterLowerHorizontalEdge) &&
			(o.CenterofCircularShutter_x == CenterofCircularShutter_x) &&
			(o.CenterofCircularShutter_y == CenterofCircularShutter_y) &&
			(o.RadiusofCircularShutter == RadiusofCircularShutter) &&
			(o.ShutterPresentationValue == ShutterPresentationValue) &&
			(o.ShutterPresentationColorCIELabValue_L == ShutterPresentationColorCIELabValue_L) &&
			(o.ShutterPresentationColorCIELabValue_a == ShutterPresentationColorCIELabValue_a) &&
			(o.ShutterPresentationColorCIELabValue_b == ShutterPresentationColorCIELabValue_b))
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
	~DisplayInterface() = default;
	void set_glwidget(GLWidget*);
	const int id;
	const bool opengl_ok;
	bool skip_texture;
	GLWidget * gl;
	short selected_lut;
	int lookup_id{-1};
	int paint_id{-1};
	bool disable_int_level{};
	bool maxwindow{};
	short filtering{}; // 0 - no, 1 - "bilinear", 2 - "trilinear"
	bool transparency{true};
	bool lock_2Dview{};
	bool lock_single{};
	bool lock_level2D{true};
	quint32 cube_3dtex{};
	float origin[3]{};
	bool origin_ok{};
	short tex_info{-1};
	int idimx{};
	int idimy{};
	int idimz{};
	float ix_origin{};
	float iy_origin{};
	float iz_origin{};
	float dircos[6]{};
	double ix_spacing{};
	double iy_spacing{};
	double iz_spacing{};
	int dimx{};
	int dimy{};
	double x_spacing{};
	double y_spacing{};
	double vmin{};
	double vmax{};
	double rmin{};
	double rmax{};
	float center_x{};
	float center_y{};
	float center_z{};
	float default_center_x{};
	float default_center_y{};
	float default_center_z{};
	float slices_direction_x{};
	float slices_direction_y{};
	float slices_direction_z{1.0f};
	float up_direction_x{};
	float up_direction_y{};
	float up_direction_z{1.0f};
	// 0 - linear_exact
	// 1 - linear
	// 2 - sigmoid
	short lut_function{0};
	short default_lut_function{0};
	double us_window_center{-999999.0};
	double us_window_width{-999999.0};
	double default_us_window_center{-999999.0};
	double default_us_window_width{-999999.0};
	double window_center{0.5};
	double window_width{1.0};
	bool slices_generated{};
	bool slices_from_dicom{};
	bool spectroscopy_generated{};
	bool hide_orientation{true};
	short spectroscopy_ref{};
	int from_slice{};
	int to_slice{};
	int irect_index[2]{};
	int irect_size[2]{};
	int selected_x_slice{};
	int selected_y_slice{};
	int selected_z_slice{};
	double bb_x_min{};
	double bb_x_max{1.0};
	double bb_y_min{};
	double bb_y_max{1.0};
	unsigned short bits_allocated{};
	unsigned short bits_stored{};
	unsigned short high_bit{};
	double shift_tmp{};
	double scale_tmp{1.0};
	int supp_palette_subsciptor;
	float R, G, B;
	SlicesVector image_slices;
	SpectroscopySlicesVector spectroscopy_slices;
	ROIs rois;
	TriMeshes trimeshes;
	void close(bool = true);
};

class ImageVariant
{
public:
	ImageVariant(int, bool, bool, GLWidget*, int);
	~ImageVariant();
	const int id;
	DisplayInterface * di;
	int group_id{-1};
	int instance_number{-1};
	short image_type{-1};
	bool equi{};
	bool one_direction{};
	unsigned int orientation{};
	bool iod_supported{};
	bool rescale_disabled{};
	bool modified{};
	bool ybr{};
	bool dicom_pixel_signed{};
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
	SegmentationInfo seg_info;
	QPixmap icon;
	QPixmap histogram;
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
	ImageVariant2D() = default;
	~ImageVariant2D();
	short image_type{-1};
	unsigned int idimx{};
	unsigned int idimy{};
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
};

class ImageContainer
{
public:
	ImageContainer() = default;
	~ImageContainer()
	{
		if (image2D)
		{
			delete image2D;
			image2D = nullptr;
		}
		image3D = nullptr;
	}
	short axis{-1};
	ImageVariant * image3D{};
	ImageVariant2D * image2D{};
	int selected_x_slice_ext{-1};
	int selected_y_slice_ext{-1};
	int selected_z_slice_ext{-1};
	double us_window_center_ext{};
	double us_window_width_ext{1e-6};
	short selected_lut_ext{};
	short lut_function_ext{};
	bool level_locked_ext{true};
	QString orientation_20_20;
};

#endif

