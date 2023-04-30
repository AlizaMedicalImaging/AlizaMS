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
	gl(gl_),
	selected_lut(default_lut_)
{
	supp_palette_subsciptor = INT_MIN;
	CommonUtils::random_RGB(&R, &G, &B);
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
}

ImageVariant::~ImageVariant()
{
	// highly likely not required
	if(pSS.IsNotNull())     {pSS->DisconnectPipeline();     };pSS     =nullptr;
	if(pUS.IsNotNull())     {pUS->DisconnectPipeline();     };pUS     =nullptr;
	if(pSI.IsNotNull())     {pSI->DisconnectPipeline();     };pSI     =nullptr;
	if(pUI.IsNotNull())     {pUI->DisconnectPipeline();     };pUI     =nullptr;
	if(pUC.IsNotNull())     {pUC->DisconnectPipeline();     };pUC     =nullptr;
	if(pF.IsNotNull())      {pF->DisconnectPipeline();      };pF      =nullptr;
	if(pD.IsNotNull())      {pD->DisconnectPipeline();      };pD      =nullptr;
	if(pSLL.IsNotNull())    {pSLL->DisconnectPipeline();    };pSLL    =nullptr;
	if(pULL.IsNotNull())    {pULL->DisconnectPipeline();    };pULL    =nullptr;
	if(pSS_rgb.IsNotNull()) {pSS_rgb->DisconnectPipeline(); };pSS_rgb =nullptr;
	if(pUS_rgb.IsNotNull()) {pUS_rgb->DisconnectPipeline(); };pUS_rgb =nullptr;
	if(pSI_rgb.IsNotNull()) {pSI_rgb->DisconnectPipeline(); };pSI_rgb =nullptr;
	if(pUI_rgb.IsNotNull()) {pUI_rgb->DisconnectPipeline(); };pUI_rgb =nullptr;
	if(pUC_rgb.IsNotNull()) {pUC_rgb->DisconnectPipeline(); };pUC_rgb =nullptr;
	if(pF_rgb.IsNotNull())  {pF_rgb->DisconnectPipeline();  };pF_rgb  =nullptr;
	if(pD_rgb.IsNotNull())  {pD_rgb->DisconnectPipeline();  };pD_rgb  =nullptr;
	if(pSS_rgba.IsNotNull()){pSS_rgba->DisconnectPipeline();};pSS_rgba=nullptr;
	if(pUS_rgba.IsNotNull()){pUS_rgba->DisconnectPipeline();};pUS_rgba=nullptr;
	if(pSI_rgba.IsNotNull()){pSI_rgba->DisconnectPipeline();};pSI_rgba=nullptr;
	if(pUI_rgba.IsNotNull()){pUI_rgba->DisconnectPipeline();};pUI_rgba=nullptr;
	if(pUC_rgba.IsNotNull()){pUC_rgba->DisconnectPipeline();};pUC_rgba=nullptr;
	if(pF_rgba.IsNotNull()) {pF_rgba->DisconnectPipeline(); };pF_rgba =nullptr;
	if(pD_rgba.IsNotNull()) {pD_rgba->DisconnectPipeline(); };pD_rgba =nullptr;
	//
	di->close();
	delete di;
}

ImageVariant2D::~ImageVariant2D()
{
	if(pSS.IsNotNull())     {pSS->DisconnectPipeline();     };pSS     =nullptr;
	if(pUS.IsNotNull())     {pUS->DisconnectPipeline();     };pUS     =nullptr;
	if(pSI.IsNotNull())     {pSI->DisconnectPipeline();     };pSI     =nullptr;
	if(pUI.IsNotNull())     {pUI->DisconnectPipeline();     };pUI     =nullptr;
	if(pUC.IsNotNull())     {pUC->DisconnectPipeline();     };pUC     =nullptr;
	if(pF.IsNotNull())      {pF->DisconnectPipeline();      };pF      =nullptr;
	if(pD.IsNotNull())      {pD->DisconnectPipeline();      };pD      =nullptr;
	if(pSLL.IsNotNull())    {pSLL->DisconnectPipeline();    };pSLL    =nullptr;
	if(pULL.IsNotNull())    {pULL->DisconnectPipeline();    };pULL    =nullptr;
	if(pSS_rgb.IsNotNull()) {pSS_rgb->DisconnectPipeline(); };pSS_rgb =nullptr;
	if(pUS_rgb.IsNotNull()) {pUS_rgb->DisconnectPipeline(); };pUS_rgb =nullptr;
	if(pSI_rgb.IsNotNull()) {pSI_rgb->DisconnectPipeline(); };pSI_rgb =nullptr;
	if(pUI_rgb.IsNotNull()) {pUI_rgb->DisconnectPipeline(); };pUI_rgb =nullptr;
	if(pUC_rgb.IsNotNull()) {pUC_rgb->DisconnectPipeline(); };pUC_rgb =nullptr;
	if(pF_rgb.IsNotNull())  {pF_rgb->DisconnectPipeline();  };pF_rgb  =nullptr;
	if(pD_rgb.IsNotNull())  {pD_rgb->DisconnectPipeline();  };pD_rgb  =nullptr;
	if(pSS_rgba.IsNotNull()){pSS_rgba->DisconnectPipeline();};pSS_rgba=nullptr;
	if(pUS_rgba.IsNotNull()){pUS_rgba->DisconnectPipeline();};pUS_rgba=nullptr;
	if(pSI_rgba.IsNotNull()){pSI_rgba->DisconnectPipeline();};pSI_rgba=nullptr;
	if(pUI_rgba.IsNotNull()){pUI_rgba->DisconnectPipeline();};pUI_rgba=nullptr;
	if(pUC_rgba.IsNotNull()){pUC_rgba->DisconnectPipeline();};pUC_rgba=nullptr;
	if(pF_rgba.IsNotNull()) {pF_rgba->DisconnectPipeline(); };pF_rgba =nullptr;
	if(pD_rgba.IsNotNull()) {pD_rgba->DisconnectPipeline(); };pD_rgba =nullptr;
}

