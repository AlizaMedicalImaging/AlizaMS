#include "structures.h"
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt5-core.h"
#else
#include "CG/glwidget-qt5.h"
#endif
#else
#ifdef ALIZA_GL_3_2_CORE
#include "CG/glwidget-qt4-core.h"
#else
#include "CG/glwidget-qt4.h"
#endif
#endif
#include "commonutils.h"
#include <climits>

DisplayInterface::DisplayInterface(
	const int id_,
	const bool opengl_ok_,
	bool skip_texture_,
	GLWidget * gl_,
	int default_lut_)
	:
	id(id_),
	opengl_ok(opengl_ok_),
	skip_texture(skip_texture_),
	gl(gl_)
{
	lookup_id = -1;
	paint_id  = -1;
	disable_int_level = false;
	maxwindow = false;
	filtering = 0; // 0 - no, 1 - "bilinear", 2 - "trilinear"
	transparency = true; // 3D phys. space view
	lock_2Dview = false;
	lock_single = false;
	lock_level2D = true;
	cube_3dtex = 0;
	for (int x = 0;x < 3; ++x) { origin[x] = 0.0f; }
	origin_ok = false;
	tex_info = -1;
	idimx = idimy = idimz = 0;
	ix_origin = iy_origin = iz_origin = 0.0f;
	for (int x = 0; x < 6; ++x) { dircos[x] = 0.0f; }
	ix_spacing = iy_spacing = iz_spacing = 0.0;
	dimx = dimy = 0;
	x_spacing = y_spacing = 0.0;
	vmin = vmax = 0.0f;
	rmin = rmax = 0.0f;
	supp_palette_subsciptor = INT_MIN;
	center_x = center_y = center_z = 0.0f;
	default_center_x = default_center_y = default_center_z = 0.0f;
	slices_direction_x = slices_direction_y = 0.0f;
	slices_direction_z = 1.0;
	up_direction_x = up_direction_z = 0.0f;
	up_direction_y = 1.0f;
	// -1 - not set,
	// 0 - linear,
	// 1 - linear_exact,
	// 2 - sigmoid
	lut_function = -1;
	default_lut_function = -1;
	us_window_center = us_window_width = -999999.0;
	default_us_window_center = default_us_window_width = -999999.0;
	window_center = 0.5;
	window_width = 1.0;
	slices_generated = false;
	slices_from_dicom = false;
	spectroscopy_generated = false;
	hide_orientation = true;
	spectroscopy_ref = 0;
	selected_lut  = default_lut_;
	from_slice = to_slice = 0;
	irect_index[0] = irect_index[1] = 0;
	irect_size[0]  = irect_size[1]  = 0;
	selected_x_slice = selected_y_slice = selected_z_slice = 0;
	bb_x_min = 0.0;
	bb_x_max = 1.0;
	bb_y_min = 0.0;
	bb_y_max = 1.0;
	bits_allocated = 0;
	bits_stored = 0;
	high_bit = 0;
	shift_tmp = 0.0;
	scale_tmp = 1.0;
	CommonUtils::random_RGB(&R,&G,&B);
}

DisplayInterface::~DisplayInterface()
{
}

void DisplayInterface::close(bool clear_geometry)
{
	if (opengl_ok && gl) gl->makeCurrent();
	if (cube_3dtex > 0)
	{
		if (opengl_ok && gl)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			gl->glDeleteTextures(1, &cube_3dtex);
#else
			glDeleteTextures(1, &cube_3dtex);
#endif
		}
		cube_3dtex =  0;
	}
	tex_info = -1;
	x_spacing = y_spacing = 0.0;
	dimx = dimy = 0;
	TriMeshes::iterator mi;
	if(!clear_geometry) goto quit__;
	//
	//
	//
	for (unsigned int x = 0; x < image_slices.size(); ++x)
	{
		delete image_slices[x];
	}
	image_slices.clear();
	slices_generated = false;
	for (unsigned int x = 0; x < spectroscopy_slices.size(); ++x)
	{
		if (spectroscopy_slices.at(x))
		{
			if (opengl_ok && gl)
			{
				if (spectroscopy_slices.at(x)->fvaoid > 0)
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
					gl->glDeleteVertexArrays(
						1, &(spectroscopy_slices[x]->fvaoid));
					gl->glDeleteBuffers(
						1, &(spectroscopy_slices[x]->fvboid));
#else
					glDeleteVertexArrays(
						1, &(spectroscopy_slices[x]->fvaoid));
					glDeleteBuffers(
						1, &(spectroscopy_slices[x]->fvboid));
#endif
					GLWidget::increment_count_vbos(-1);
				}
				if (spectroscopy_slices.at(x)->lvaoid > 0)
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
					gl->glDeleteVertexArrays(1, &(spectroscopy_slices[x]->lvaoid));
					gl->glDeleteBuffers(1, &(spectroscopy_slices[x]->lvboid));
#else
					glDeleteVertexArrays(1, &(spectroscopy_slices[x]->lvaoid));
					glDeleteBuffers(1, &(spectroscopy_slices[x]->lvboid));
#endif
					GLWidget::increment_count_vbos(-1);
				}
				if (spectroscopy_slices.at(x)->pvboid > 0)
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
					gl->glDeleteVertexArrays(1, &(spectroscopy_slices[x]->pvaoid));
					gl->glDeleteBuffers(1, &(spectroscopy_slices[x]->pvboid));
#else
					glDeleteVertexArrays(1, &(spectroscopy_slices[x]->pvaoid));
					glDeleteBuffers(1, &(spectroscopy_slices[x]->pvboid));
#endif
					GLWidget::increment_count_vbos(-1);
				}
			}
			delete spectroscopy_slices[x];
		}
	}
	spectroscopy_slices.clear();
	spectroscopy_generated = false;
	for (int x = 0; x < 6; ++x) dircos[x] = 0.0f;
	for (int x = 0; x < 3; ++x) origin[x] = 0.0f;
	origin_ok = false;
	for (int k = 0; k < rois.size(); ++k)
	{
		QList<int> keys;
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			QMap< int,Contour* >::const_iterator it2 =
				rois.at(k).contours.cbegin();
			while (it2 != rois.at(k).contours.cend())
#else
			QMap< int,Contour* >::const_iterator it2 =
				rois.at(k).contours.constBegin();
			while (it2 != rois.at(k).contours.constEnd())
#endif
			{
				keys.push_back(it2.key());
				++it2;
			}
		}
		for (int x = 0; x < keys.size(); ++x)
		{
			Contour * c = rois[k].contours.value(keys.at(x));
			if (c)
			{
				c->dpoints.clear();
				c->path = QPainterPath();
				c->ref_sop_instance_uids.clear();
				if (opengl_ok && gl && c->vao_initialized)
				{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
					gl->glDeleteVertexArrays(1, &(c->vaoid));
					gl->glDeleteBuffers(1, &(c->vboid));
#else
					glDeleteVertexArrays(1, &(c->vaoid));
					glDeleteBuffers(1, &(c->vboid));
#endif
					GLWidget::increment_count_vbos(-1);
				}
				delete c;
			}
		}
		keys.clear();
		rois[k].contours.clear();
		rois[k].map.clear();
	}
	rois.clear();
	mi = trimeshes.begin();
	while (mi != trimeshes.end())
	{
		TriMesh * trimesh = mi.value();
		++mi;
		if (trimesh &&
			opengl_ok && gl &&
			trimesh->initialized &&
			trimesh->qmesh)
		{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			gl->glDeleteVertexArrays(1, &(trimesh->qmesh->vaoid));
			gl->glDeleteBuffers(2, trimesh->qmesh->vboid);
#else
			glDeleteVertexArrays(1, &(trimesh->qmesh->vaoid));
			glDeleteBuffers(2, trimesh->qmesh->vboid);
#endif
			GLWidget::increment_count_vbos(-2);
			delete trimesh->qmesh;
		}
		delete trimesh;
	}
	trimeshes.clear();
	center_x = center_y = center_z = 0.0f;
	slices_direction_x = slices_direction_y = 0.0f;
	slices_direction_z = 1.0;
	up_direction_x = up_direction_z = 0.0f;
	up_direction_y = 1.0f;
	from_slice = 0; to_slice = (idimz > 0) ? idimz-1 : 0;
quit__:
#if 0
	if (opengl_ok && gl)
		GLWidget::checkGLerror(" DisplayInterface::close()\n");
#endif
	return;
}

ImageVariant::ImageVariant(
	int  id_,
	bool opengl_,
	bool skip_texture_,
	GLWidget * gl_,
	int  default_lut_)
	:
	id(id_)
{
	di = new DisplayInterface(
		id_,
		opengl_,
		skip_texture_,
		gl_,
		default_lut_);
	group_id = -1;
	instance_number = -1;
	image_type = -1;
	equi = false;
	one_direction = false;
	orientation = 0; 
	orientation_string = QString("");
	iod_supported = false;
	rescale_disabled = false;
	modified = false;
	ybr = false;
	dicom_pixel_signed = false;
}

ImageVariant::~ImageVariant()
{
	// highly likely not required
	if(pSS.IsNotNull())     {pSS->DisconnectPipeline();     };pSS     =NULL;
	if(pUS.IsNotNull())     {pUS->DisconnectPipeline();     };pUS     =NULL;
	if(pSI.IsNotNull())     {pSI->DisconnectPipeline();     };pSI     =NULL;
	if(pUI.IsNotNull())     {pUI->DisconnectPipeline();     };pUI     =NULL;
	if(pUC.IsNotNull())     {pUC->DisconnectPipeline();     };pUC     =NULL;
	if(pF.IsNotNull())      {pF->DisconnectPipeline();      };pF      =NULL;
	if(pD.IsNotNull())      {pD->DisconnectPipeline();      };pD      =NULL;
	if(pSLL.IsNotNull())    {pSLL->DisconnectPipeline();    };pSLL    =NULL;
	if(pULL.IsNotNull())    {pULL->DisconnectPipeline();    };pULL    =NULL;
	if(pSS_rgb.IsNotNull()) {pSS_rgb->DisconnectPipeline(); };pSS_rgb =NULL;
	if(pUS_rgb.IsNotNull()) {pUS_rgb->DisconnectPipeline(); };pUS_rgb =NULL;
	if(pSI_rgb.IsNotNull()) {pSI_rgb->DisconnectPipeline(); };pSI_rgb =NULL;
	if(pUI_rgb.IsNotNull()) {pUI_rgb->DisconnectPipeline(); };pUI_rgb =NULL;
	if(pUC_rgb.IsNotNull()) {pUC_rgb->DisconnectPipeline(); };pUC_rgb =NULL;
	if(pF_rgb.IsNotNull())  {pF_rgb->DisconnectPipeline();  };pF_rgb  =NULL;
	if(pD_rgb.IsNotNull())  {pD_rgb->DisconnectPipeline();  };pD_rgb  =NULL;
	if(pSS_rgba.IsNotNull()){pSS_rgba->DisconnectPipeline();};pSS_rgba=NULL;
	if(pUS_rgba.IsNotNull()){pUS_rgba->DisconnectPipeline();};pUS_rgba=NULL;
	if(pSI_rgba.IsNotNull()){pSI_rgba->DisconnectPipeline();};pSI_rgba=NULL;
	if(pUI_rgba.IsNotNull()){pUI_rgba->DisconnectPipeline();};pUI_rgba=NULL;
	if(pUC_rgba.IsNotNull()){pUC_rgba->DisconnectPipeline();};pUC_rgba=NULL;
	if(pF_rgba.IsNotNull()) {pF_rgba->DisconnectPipeline(); };pF_rgba =NULL;
	if(pD_rgba.IsNotNull()) {pD_rgba->DisconnectPipeline(); };pD_rgba =NULL;
	//
	di->close();
	delete di;
}

ImageVariant2D::ImageVariant2D()
{
	image_type = -1;
	orientation_string = QString("");
	idimx = 0;
	idimy = 0;
}

ImageVariant2D::~ImageVariant2D()
{
	if(pSS.IsNotNull())     {pSS->DisconnectPipeline();     };pSS     =NULL;
	if(pUS.IsNotNull())     {pUS->DisconnectPipeline();     };pUS     =NULL;
	if(pSI.IsNotNull())     {pSI->DisconnectPipeline();     };pSI     =NULL;
	if(pUI.IsNotNull())     {pUI->DisconnectPipeline();     };pUI     =NULL;
	if(pUC.IsNotNull())     {pUC->DisconnectPipeline();     };pUC     =NULL;
	if(pF.IsNotNull())      {pF->DisconnectPipeline();      };pF      =NULL;
	if(pD.IsNotNull())      {pD->DisconnectPipeline();      };pD      =NULL;
	if(pSLL.IsNotNull())    {pSLL->DisconnectPipeline();    };pSLL    =NULL;
	if(pULL.IsNotNull())    {pULL->DisconnectPipeline();    };pULL    =NULL;
	if(pSS_rgb.IsNotNull()) {pSS_rgb->DisconnectPipeline(); };pSS_rgb =NULL;
	if(pUS_rgb.IsNotNull()) {pUS_rgb->DisconnectPipeline(); };pUS_rgb =NULL;
	if(pSI_rgb.IsNotNull()) {pSI_rgb->DisconnectPipeline(); };pSI_rgb =NULL;
	if(pUI_rgb.IsNotNull()) {pUI_rgb->DisconnectPipeline(); };pUI_rgb =NULL;
	if(pUC_rgb.IsNotNull()) {pUC_rgb->DisconnectPipeline(); };pUC_rgb =NULL;
	if(pF_rgb.IsNotNull())  {pF_rgb->DisconnectPipeline();  };pF_rgb  =NULL;
	if(pD_rgb.IsNotNull())  {pD_rgb->DisconnectPipeline();  };pD_rgb  =NULL;
	if(pSS_rgba.IsNotNull()){pSS_rgba->DisconnectPipeline();};pSS_rgba=NULL;
	if(pUS_rgba.IsNotNull()){pUS_rgba->DisconnectPipeline();};pUS_rgba=NULL;
	if(pSI_rgba.IsNotNull()){pSI_rgba->DisconnectPipeline();};pSI_rgba=NULL;
	if(pUI_rgba.IsNotNull()){pUI_rgba->DisconnectPipeline();};pUI_rgba=NULL;
	if(pUC_rgba.IsNotNull()){pUC_rgba->DisconnectPipeline();};pUC_rgba=NULL;
	if(pF_rgba.IsNotNull()) {pF_rgba->DisconnectPipeline(); };pF_rgba =NULL;
	if(pD_rgba.IsNotNull()) {pD_rgba->DisconnectPipeline(); };pD_rgba =NULL;
}

void ProcessImageThread_::run()
{
	Image2DTypeUC::SizeType size;
	size[0] = size_0;
	size[1] = size_1;
	Image2DTypeUC::IndexType index;
	index[0] = index_0;
	index[1] = index_1;
 	Image2DTypeUC::RegionType region;
	region.SetSize(size);
	region.SetIndex(index);
	itk::ImageRegionConstIterator<Image2DTypeUC>
		iterator(image, region);
	iterator.GoToBegin();
	unsigned int j_ = j;
	while(!iterator.IsAtEnd())
	{
		p[j_+2] = p[j_+1] = p[j_+0] =
			static_cast<unsigned char>(iterator.Get());
		j_ += 3;
 		++iterator;
	}
}

