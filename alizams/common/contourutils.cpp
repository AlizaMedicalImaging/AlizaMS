#include "contourutils.h"
#include <QMessageBox>
#include <QApplication>
#include <itkContinuousIndex.h>
#include "CG/glwidget.h"
#include "vectormath/scalar/vectormath.h"
typedef Vectormath::Scalar::Vector3 sVector3;

template <typename T> void calculate_uvt(
	const typename T::Pointer image,
	ImageVariant * ivariant)
{
	if (image.IsNull()) return;
	unsigned long count = 0;
	for (int x = 0; x < ivariant->di->rois.size(); x++)
	{
		QMap< int, Contour* >::iterator it =
			ivariant->di->rois[x].contours.begin();
		while (it != ivariant->di->rois[x].contours.end())
		{
			count += 1;
			Contour * c = it.value();
			if (c)
			{
				bool planar = true;
				int  tmp0 = -1;
				for (long k = 0; k < c->dpoints.size(); k++)
				{
					itk::ContinuousIndex<float, 3> index;
					typename T::PointType point;
					point[0] = c->dpoints.at(k).x;
					point[1] = c->dpoints.at(k).y;
					point[2] = c->dpoints.at(k).z;
					const bool ok =
						image->TransformPhysicalPointToContinuousIndex(
							point,index);
					if (ok)
					{
						c->dpoints[k].u = index[0];
						c->dpoints[k].v = index[1];
						c->dpoints[k].t = static_cast<int>(round(index[2]));
						if (
							k>0 &&
							(tmp0 != c->dpoints[k].t))
						{
							if (planar) planar = false;
						}
						tmp0 = c->dpoints[k].t;
					}
					else
					{
						if (planar) planar = false;
					}
				}
				if (!planar) c->type = 3;
			}
			if (count%5==0) QApplication::processEvents();
			++it;
		}
	}
}

float ContourUtils::distance_to_plane(
	float  x, float  y, float  z,
	float nx, float ny, float nz,
	float px, float py, float pz)
{
	const float d = nx*(x-px) + ny*(y-py) + nz*(z-pz);
	if (d < 0) return -d;
	return d;
}

long ContourUtils::get_next_contour_tmpid()
{
	static long contour_tmpid___ = 0;
	contour_tmpid___+=1;
	return contour_tmpid___;
}

void ContourUtils::calculate_rois_center(ImageVariant * iv)
{
	if (!iv) return;
	long int z = 0;
	double tmpx = 0.0, tmpy = 0.0, tmpz = 0.0;
	for (int x = 0; x < iv->di->rois.size(); x++)
	{
		QMap< int, Contour* >::const_iterator it =
			iv->di->rois.at(x).contours.constBegin();
		while (it != iv->di->rois.at(x).contours.constEnd())
		{
			const Contour * c = it.value();
			if (c)
			{
				for (int k = 0; k < c->dpoints.size(); k++)
				{
					z++;
					tmpx += (double)c->dpoints.at(k).x;
					tmpy += (double)c->dpoints.at(k).y;
					tmpz += (double)c->dpoints.at(k).z;
				}
			}
			++it;
		}
	}
	if (z>0)
	{
		iv->di->default_center_x = iv->di->center_x =
			tmpx/(double)z;
		iv->di->default_center_y = iv->di->center_y =
			tmpy/(double)z;
		iv->di->default_center_z = iv->di->center_z =
			tmpz/(double)z;
	}
}

int ContourUtils::get_new_roi_id(const ImageVariant * ivariant)
{
	if (!ivariant) return -1;
	int tmp0 = 0;
	for (int x = 0; x < ivariant->di->rois.size(); x++)
	{
		if (ivariant->di->rois.at(x).id >= tmp0)
			tmp0 = ivariant->di->rois.at(x).id;
	}
	return (tmp0 + 1);
}

void ContourUtils::generate_roi_vbos(
	GLWidget * gl,
	ROI & roi,
	bool delete_after)
{
	QMap< int, Contour* >::iterator it =
		roi.contours.begin();
	while (it != roi.contours.end())
	{
		Contour * c = it.value();
		if (c)
		{
			bool ok = true;
			if (
				GLWidget::get_max_vbos_65535() &&
				GLWidget::get_count_vbos() >= 64000)
			{
				ok = false;
			}
			unsigned long ind = 0;
			if (ok && !c->vao_initialized)
			{
				const unsigned long s = 3*c->dpoints.size();
				GLfloat * v;
				try { v = new GLfloat[s]; }
				catch(std::bad_alloc&) { return; }
				if (!v) return;
				for (long k=0; k < c->dpoints.size(); k++)
				{
					v[ind] = (GLfloat)(c->dpoints.at(k).x);
					ind+=1;
					v[ind] = (GLfloat)(c->dpoints.at(k).y);
					ind+=1;
					v[ind] = (GLfloat)(c->dpoints.at(k).z);
					ind+=1;
				}
				if (ind!=s)
				{
					std::cout
						<< "failed generating VBOs (contours)"
						<< std::endl;
					delete [] v;
					return;
				}
				if (gl)
				{
					gl->makeCurrent();
					gl->glGenVertexArrays(1, &(c->vaoid));
					gl->glBindVertexArray(c->vaoid);
					gl->glGenBuffers(1, &(c->vboid));
					GLWidget::increment_count_vbos(1);
					gl->glBindBuffer(GL_ARRAY_BUFFER, c->vboid);
					gl->glBufferData(GL_ARRAY_BUFFER, s*sizeof(GLfloat), v, GL_STATIC_DRAW);
					gl->glVertexAttribPointer(gl->frame_shader.position_handle,3, GL_FLOAT, GL_FALSE, 0, 0);
					gl->glEnableVertexAttribArray(gl->frame_shader.position_handle);
					delete [] v;
					c->vao_initialized = true;
				}
				if (delete_after) c->dpoints.clear();
			}
		}
		++it;
	}
}

void ContourUtils::copy_roi(
	ROI & dest,
	const ROI & src)
{
	const int id = dest.id;
	dest.show             = src.show;
	dest.name             = src.name;
	dest.interpreted_type = src.interpreted_type;
	dest.color.r          = src.color.r;
	dest.color.g          = src.color.g;
	dest.color.b          = src.color.b;
	dest.ref_frame_of_ref = src.ref_frame_of_ref;
	QMap< int, Contour* >::const_iterator it =
		src.contours.constBegin();
	while (it != src.contours.constEnd())
	{
		Contour * c = it.value();
		if (c)
		{
			Contour * contour = new Contour();
			contour->id = c->id;
			contour->roiid = id; 
			contour->type = c->type;
			contour->vao_initialized = false;
			for (int i = 0; i < c->dpoints.size(); i++)
				contour->dpoints.push_back(c->dpoints[i]);
			for (
				int i = 0;
				i < c->ref_sop_instance_uids.size();
				i++)
				contour->ref_sop_instance_uids
					.push_back(c->ref_sop_instance_uids.at(i));
			dest.contours[contour->id] = contour;
		}
		++it;
	}
}

void ContourUtils::calculate_uvt_nonuniform(
	ImageVariant * ivariant)
{
	if (!ivariant) return;
	for (int x = 0; x < ivariant->di->rois.size(); x++)
	{
		QMap< int, Contour* >::iterator it =
			ivariant->di->rois[x].contours.begin();
		while (it != ivariant->di->rois[x].contours.end())
		{
			Contour * c = it.value();
			if (!c) continue;
			QList<int> slices;
			for (unsigned int z = 0;
				z < (unsigned int)ivariant->di->idimz;
				z++)
			{
				bool in_slice = false;
				if (ivariant->di->image_slices.size() > z)
				{
					const float px =
						(float)ivariant->di->image_slices.at(z)->v[0];
					const float py =
						(float)ivariant->di->image_slices.at(z)->v[1];
					const float pz =
						(float)ivariant->di->image_slices.at(z)->v[2];
					const sVector3 v1 = sVector3(
						(float)(ivariant->di->image_slices.at(z)->v[3])-px,
						(float)(ivariant->di->image_slices.at(z)->v[4])-py,
						(float)(ivariant->di->image_slices.at(z)->v[5])-pz);
					const sVector3 v2 = sVector3(
						(float)(ivariant->di->image_slices.at(z)->v[6])-px,
						(float)(ivariant->di->image_slices.at(z)->v[7])-py,
						(float)(ivariant->di->image_slices.at(z)->v[8])-pz);
					const sVector3 n = Vectormath::Scalar::normalize(
						Vectormath::Scalar::cross(v1,v2));
					for (int k = 0; k < c->dpoints.size(); k++)
					{
						const float distance =
							distance_to_plane(
								c->dpoints.at(k).x,
								c->dpoints.at(k).y,
								c->dpoints.at(k).z,
								n.getX(),
								n.getY(),
								n.getZ(),
								px, py, pz);
						if (distance < 0.1)
						{
							in_slice = true;
						}
						else
						{
							in_slice = false;
							break;
						}
					}
				}
				if (in_slice) slices.push_back(z);
			}
			if (slices.size()==1)
			{
				const int idx = slices.at(0);
				for (long k = 0; k < c->dpoints.size(); k++)
				{
					ImageTypeUC::Pointer image = ImageTypeUC::New();
					const bool image_ok =
						phys_space_from_slice(
							ivariant,idx,image);
					if (image_ok)
					{
						itk::ContinuousIndex<float, 3> index;
						ImageTypeUC::PointType point;
						point[0] = c->dpoints.at(k).x;
						point[1] = c->dpoints.at(k).y;
						point[2] = c->dpoints.at(k).z;
						const bool ok = image->
							TransformPhysicalPointToContinuousIndex(
								point,index);
						if (ok)
						{
							c->dpoints[k].u = index[0];
							c->dpoints[k].v = index[1];
							c->dpoints[k].t = idx;
						}
					}
				}
			}
			QApplication::processEvents();
			++it;
		}
	}
}

void ContourUtils::calculate_contours_uv(
	ImageVariant * ivariant)
{
	if (!ivariant) return;
	if (ivariant->di->idimz == 0) return;
	if (ivariant->equi)
	{
		switch(ivariant->image_type)
		{
		case 0:
			calculate_uvt<ImageTypeSS>(ivariant->pSS, ivariant);
			break;
		case 1:
			calculate_uvt<ImageTypeUS>(ivariant->pUS, ivariant);
			break;
		case 2:
			calculate_uvt<ImageTypeSI>(ivariant->pSI, ivariant);
			break;
		case 3:
			calculate_uvt<ImageTypeUI>(ivariant->pUI, ivariant);
			break;
		case 4:
			calculate_uvt<ImageTypeUC>(ivariant->pUC, ivariant);
			break;
		case 5:
			calculate_uvt<ImageTypeF>(ivariant->pF, ivariant);
			break;
		case 6:
			calculate_uvt<ImageTypeD>(ivariant->pD, ivariant);
			break;
		case 7:
			calculate_uvt<ImageTypeSLL>(ivariant->pSLL, ivariant);
			break;
		case 8:
			calculate_uvt<ImageTypeULL>(ivariant->pULL, ivariant);
			break;
		case 10:
			calculate_uvt<RGBImageTypeSS>(ivariant->pSS_rgb, ivariant);
			break;
		case 11:
			calculate_uvt<RGBImageTypeUS>(ivariant->pUS_rgb, ivariant);
			break;
		case 12:
			calculate_uvt<RGBImageTypeSI>(ivariant->pSI_rgb, ivariant);
			break;
		case 13:
			calculate_uvt<RGBImageTypeUI>(ivariant->pUI_rgb, ivariant);
			break;
		case 14:
			calculate_uvt<RGBImageTypeUC>(ivariant->pUC_rgb, ivariant);
			break;
		case 15:
			calculate_uvt<RGBImageTypeF>(ivariant->pF_rgb, ivariant);
			break;
		case 16:
			calculate_uvt<RGBImageTypeD>(ivariant->pD_rgb, ivariant);
			break;
		default : break;
		}
	}
	else
	{
		calculate_uvt_nonuniform(ivariant);
	}
}

void ContourUtils::map_contours_uniform(
	ImageVariant * ivariant,
	int roi_id)
{
	if (!ivariant) return;
	if (ivariant->di->idimz == 0) return;
	if (ivariant->di->idimz !=
		(int)ivariant->di->image_slices.size())
	{
		std::cout
			<< "ContourUtils::map_contours: dimz != slices size"
			<< std::endl;
		return;
	}
	const float tolerance = (float)ivariant->di->iz_spacing*0.5f;
	for (int x = 0; x < ivariant->di->rois.size(); x++)
	{
		if (ivariant->di->rois.at(x).id == roi_id)
		{
			ivariant->di->rois[x].map.clear();
			QMap< int, Contour* >::const_iterator it =
				ivariant->di->rois.at(x).contours.constBegin();
			while (it != ivariant->di->rois.at(x).contours.constEnd())
			{
				const Contour * c = it.value();
				if (!c) continue;
				for (int z = 0; z < ivariant->di->idimz; z++)
				{
					const float px =
						(float)ivariant->di->image_slices.at(z)->v[0];
					const float py =
						(float)ivariant->di->image_slices.at(z)->v[1];
					const float pz =
						(float)ivariant->di->image_slices.at(z)->v[2];
					const sVector3 v1 = sVector3(
						(float)(ivariant->di->image_slices.at(z)->v[3]) - px,
						(float)(ivariant->di->image_slices.at(z)->v[4]) - py,
						(float)(ivariant->di->image_slices.at(z)->v[5]) - pz);
					const sVector3 v2 = sVector3(
						(float)(ivariant->di->image_slices.at(z)->v[6]) - px,
						(float)(ivariant->di->image_slices.at(z)->v[7]) - py,
						(float)(ivariant->di->image_slices.at(z)->v[8]) - pz);
					const sVector3 n = Vectormath::Scalar::normalize(
						Vectormath::Scalar::cross(v1,v2));
					for (int k = 0; k < c->dpoints.size(); k++)
					{
						const float distance = distance_to_plane(
							c->dpoints.at(k).x,
							c->dpoints.at(k).y,
							c->dpoints.at(k).z,
							n.getX(),
							n.getY(),
							n.getZ(),
							px,
							py,
							pz);
						if (distance < tolerance)
						{
							ivariant->di->rois[x].map.insert(z, c->id);
							break;
						}
					}
				}
				QApplication::processEvents();
				++it;
			}
			break;
		}
	}
}

void ContourUtils::map_contours_nonuniform(
	ImageVariant * ivariant,
	int roi_id)
{
	if (!ivariant) return;
	if (ivariant->di->idimz == 0) return;
	if (ivariant->di->idimz !=
		(int)ivariant->di->image_slices.size())
	{
		std::cout
		<< "ContourUtils::map_contours: dimz != slices size"
		<< std::endl;
		return;
	}
	const float tolerance = 0.1f;
	for (int x = 0; x < ivariant->di->rois.size(); x++)
	{
		if (ivariant->di->rois.at(x).id == roi_id)
		{
			ivariant->di->rois[x].map.clear();
			QMap< int, Contour* >::const_iterator it =
				ivariant->di->rois.at(x).contours.constBegin();
			while (it != ivariant->di->rois.at(x).contours.constEnd())
			{
				const Contour * c = it.value();
				if (!c) continue;
				bool in_slice = false;
				QList<int> slices;
				for (int z = 0; z < ivariant->di->idimz; z++)
				{
					const float px =
						(float)ivariant->di->image_slices.at(z)->v[0];
					const float py =
						(float)ivariant->di->image_slices.at(z)->v[1];
					const float pz =
						(float)ivariant->di->image_slices.at(z)->v[2];
					const sVector3 v1 = sVector3(
						(float)(ivariant->di->image_slices.at(z)->v[3])
							- px,
						(float)(ivariant->di->image_slices.at(z)->v[4])
							- py,
						(float)(ivariant->di->image_slices.at(z)->v[5])
							- pz);
					const sVector3 v2 = sVector3(
						(float)(ivariant->di->image_slices.at(z)->v[6])
							- px,
						(float)(ivariant->di->image_slices.at(z)->v[7])
							- py,
						(float)(ivariant->di->image_slices.at(z)->v[8])
							- pz);
					const sVector3 n = Vectormath::Scalar::normalize(
						Vectormath::Scalar::cross(v1,v2));
					for (int k = 0; k < c->dpoints.size(); k++)
					{
						const float distance = distance_to_plane(
							c->dpoints.at(k).x,
							c->dpoints.at(k).y,
							c->dpoints.at(k).z,
							n.getX(),
							n.getY(),
							n.getZ(),
							px,
							py,
							pz);
						if (distance < tolerance)
						{
							in_slice = true;
						}
						else
						{
							in_slice = false;
							break;
						}
					}
					if (in_slice) slices.push_back(z);
				}
				if (slices.size()==1)
				{
					const int idx = slices.at(0);
					ivariant->di->rois[x].map.insert(idx, c->id);
				}
				else
				{
#if 0
					std::cout
						<< "error mapping non-uniform contours: "
						<< slices.size()
						<< " slices detected("
						<< ivariant->di->rois.at(x).name.toStdString()
						<< ")" << std::endl;
#endif
				}
				QApplication::processEvents();
				++it;
			}
			break;
		}
	}
}

void ContourUtils::map_contours(
	ImageVariant * ivariant,
	int roi_id)
{
	if (!ivariant) return;
	if (ivariant->equi)
	{
		map_contours_uniform(ivariant, roi_id);
	}
	else
	{
		map_contours_nonuniform(ivariant, roi_id);
	}
}

void ContourUtils::map_contours_all(
	ImageVariant * ivariant)
{
	if (!ivariant) return;
	for (int x = 0; x < ivariant->di->rois.size(); x++)
	{
		map_contours(ivariant, ivariant->di->rois.at(x).id);
	}
}

void ContourUtils::map_contours_test_refs(
	ImageVariant * ivariant)
{
	if (!ivariant) return;
	for (int x = 0; x < ivariant->di->rois.size(); x++)
	{
		ivariant->di->rois[x].map.clear();
		for (int z = 0; z < ivariant->di->idimz; z++)
		{
			QMap< int, Contour* >::const_iterator it =
				ivariant->di->rois.at(x).contours.constBegin();
			while (it != ivariant->di->rois.at(x).contours.constEnd())
			{
				const Contour * c = it.value();
				if (c)
				{
					for (int y = 0;
						y < c->ref_sop_instance_uids.size();
						y++)
					{
						if (c->ref_sop_instance_uids.at(y) ==
							ivariant->image_instance_uids.value(z))
							ivariant->di->rois[x].map.insert(z, c->id);
					}
				}
				++it;
			}
		}
	}
}

void ContourUtils::contours_build_path(
	ImageVariant * ivariant,
	int roi_id)
{
	if (!ivariant) return;
	if (ivariant->di->idimz == 0) return;
	for (int x = 0; x < ivariant->di->rois.size(); x++)
	{
		if (ivariant->di->rois.at(x).id == roi_id)
		{
			QMap< int, Contour* >::iterator it =
				ivariant->di->rois[x].contours.begin();
			while (it != ivariant->di->rois[x].contours.end())
			{
				Contour * c = it.value();
				if (!c) continue;
				const short contour_type = c->type;
				QPainterPath path;
				// CLOSED_PLANAR, OPEN_PLANAR
				if (contour_type == 1||contour_type == 2)
				{
					for (int k = 0; k < c->dpoints.size(); k++)
					{
						if (k == 0)
						{
							const DPoint & pf_ = c->dpoints.at(k);
							path.moveTo(pf_.u, pf_.v);
						}
						else
						{
							const DPoint & pf_ = c->dpoints.at(k);
							path.lineTo(pf_.u, pf_.v);
						}
					}
					if (contour_type == 1) path.closeSubpath();
				}
				// POINT
				else if (contour_type == 4)
				{
					for (int k = 0; k < c->dpoints.size(); k++)
					{
						const DPoint & pf_ = c->dpoints.at(k);
						path.addRect(
							pf_.u-0.25f,
							pf_.v-0.25f,
							0.5,
							0.5);
					}
				}
				// NON-PLANAR, not set
				else
				{
					for (int z = 0; z < ivariant->di->idimz; z++)
					{
						for (int k = 0; k < c->dpoints.size(); k++)
						{
							const DPoint & pf_ = c->dpoints.at(k);
							if (pf_.t == z)
							{		
								path.addRect(
									pf_.u-0.25f,
									pf_.v-0.25f,
									0.5,
									0.5);
							}
						}
					}
				}
				c->path = path;
				++it;
			}
			break;
		}
	}
}

void ContourUtils::contours_build_path_all(
	ImageVariant * ivariant)
{
	if (!ivariant) return;
	if (ivariant->di->idimz == 0) return;
	for (int x = 0; x < ivariant->di->rois.size(); x++)
	{
		contours_build_path(ivariant, ivariant->di->rois.at(x).id);
		QApplication::processEvents();
	}
}

void ContourUtils::copy_imagevariant_rois_no_init(
	ImageVariant * dest,
	const ImageVariant * source)
{
	if (!dest) return;
	if (!source) return;
	int tmp0 = get_new_roi_id(dest);
	for (int x = 0; x < source->di->rois.size(); x++)
	{
		ROI roi;
		roi.id = tmp0;
		tmp0 += 1;
		copy_roi(roi, source->di->rois.at(x));
		dest->di->rois.push_back(roi);
	}
}

bool ContourUtils::phys_space_from_slice(
	const ImageVariant * ivariant,
	unsigned int x,
	ImageTypeUC::Pointer & image)
{
	if (!ivariant) return false;
	if (ivariant->di->idimz !=
			(int)ivariant->di->image_slices.size())
	{
		std::cout << "dimz != slices size" << std::endl;
		return false;
	}
	if (x >= ivariant->di->image_slices.size())
	{
		std::cout << "index out of range" << std::endl;
		return false;
	}
#if 0
	if (ivariant->di->image_slices
			.at(x)->slice_orientation_string.isEmpty())
	{
		std::cout
			<< "slice orientation string is empty"
			<< std::endl;
		return false;
	}
#endif
	const float row_dircos_x =
		(float)ivariant->di->image_slices.at(x)->ipp_iop[3];
	const float row_dircos_y =
		(float)ivariant->di->image_slices.at(x)->ipp_iop[4];
	const float row_dircos_z =
		(float)ivariant->di->image_slices.at(x)->ipp_iop[5];
	const float col_dircos_x =
		(float)ivariant->di->image_slices.at(x)->ipp_iop[6];
	const float col_dircos_y =
		(float)ivariant->di->image_slices.at(x)->ipp_iop[7];
	const float col_dircos_z =
		(float)ivariant->di->image_slices.at(x)->ipp_iop[8];
	if (	
		row_dircos_x>-0.000001f && row_dircos_x<0.000001f &&
		row_dircos_y>-0.000001f && row_dircos_y<0.000001f &&
		row_dircos_z>-0.000001f && row_dircos_z<0.000001f &&
		col_dircos_x>-0.000001f && col_dircos_x<0.000001f &&
		col_dircos_y>-0.000001f && col_dircos_y<0.000001f &&
		col_dircos_z>-0.000001f && col_dircos_z<0.000001f)
	{
		std::cout
			<< "can not process direction cosines"
			<< std::endl;
		return false;
	}
	ImageTypeUC::IndexType idx;
	idx[0] = 0;
	idx[1] = 0;
	idx[2] = 0;
	ImageTypeUC::SizeType size;
	size[0] = ivariant->di->idimx;
	size[1] = ivariant->di->idimy;
	size[2] = 1;
	ImageTypeUC::RegionType region;
	region.SetSize(size);
	region.SetIndex(idx);
	ImageTypeUC::PointType origin;
	origin[0] = ivariant->di->image_slices.at(x)->ipp_iop[0];
	origin[1] = ivariant->di->image_slices.at(x)->ipp_iop[1];
	origin[2] = ivariant->di->image_slices.at(x)->ipp_iop[2];
	ImageTypeUC::SpacingType spacing;
	spacing[0] = ivariant->di->ix_spacing;
	spacing[1] = ivariant->di->iy_spacing;
	spacing[2] = 1;
	ImageTypeUC::DirectionType direction;
	const float nrm_dircos_x =
		row_dircos_y * col_dircos_z - row_dircos_z * col_dircos_y; 
	const float nrm_dircos_y =
		row_dircos_z * col_dircos_x - row_dircos_x * col_dircos_z;
	const float nrm_dircos_z =
		row_dircos_x * col_dircos_y - row_dircos_y * col_dircos_x;
	direction[0][0] = row_dircos_x;
	direction[1][0] = row_dircos_y;
	direction[2][0] = row_dircos_z;
	direction[0][1] = col_dircos_x;
	direction[1][1] = col_dircos_y;
	direction[2][1] = col_dircos_z;
	direction[0][2] = nrm_dircos_x;
	direction[1][2] = nrm_dircos_y;
	direction[2][2] = nrm_dircos_z;
	try
	{
		image->SetRegions(region);
		image->SetOrigin(origin);
		image->SetSpacing(spacing);
		image->SetDirection(direction);
		image->Allocate();
		image->FillBuffer(
			static_cast<ImageTypeUC::PixelType>(0));
	}
	catch (itk::ExceptionObject & ex)
	{
		std::cout << ex.GetDescription() << std::endl;
		return false;
	}
	return true;
}

