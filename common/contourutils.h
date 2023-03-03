#ifndef A_CONTOURUTILS_H
#define A_CONTOURUTILS_H

#include "structures.h"

class GLWidget;

class ContourUtils
{
public:
	ContourUtils();
	~ContourUtils();
	static float distance_to_plane(
		float, float, float, float, float, float, float, float, float);
	static long long get_next_contour_tmpid();
	static void calculate_rois_center(ImageVariant*);
	static int  get_new_roi_id(const ImageVariant*);
	static void generate_roi_vbos(GLWidget*, ROI&, bool);
	static void copy_roi(ROI&, const ROI&);
	static void calculate_uvt_nonuniform(ImageVariant*);
	static void calculate_contours_uv(ImageVariant*);
	static void map_contours_uniform(ImageVariant*, int);
	static void map_contours_nonuniform(ImageVariant*, int);
	static void map_contours(ImageVariant*, int);
	static void map_contours_all(ImageVariant*);
	static void map_contours_test_refs(ImageVariant*);
	static void contours_build_path(ImageVariant*,int);
	static void contours_build_path_all(ImageVariant*);
	static void copy_imagevariant_rois_no_init(
		ImageVariant*,
		const ImageVariant*);
	static bool phys_space_from_slice(
		const ImageVariant*, unsigned int, ImageTypeUC::Pointer &);
};

#endif

