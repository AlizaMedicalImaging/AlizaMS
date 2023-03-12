#include <QtGlobal>
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
#include <mdcmTag.h>
#include <mdcmItem.h>
#include <mdcmElement.h>
#include <mdcmDataElement.h>
#include <mdcmVR.h>
#include <mdcmVM.h>
#include <iostream>
#include <cmath>
#include "spectroscopyutils.h"
#include "spectroscopydata.h"
#include "commonutils.h"
#include "dicomutils.h"
#include <QProgressDialog>

//#define LOAD_SPECT_DATA

bool SpectroscopyUtils::Read(const mdcm::DataSet & ds, SpectroscopyData * s)
{
	if (!s) return false;
	if (ds.IsEmpty()) return false;
	const mdcm::Tag tNumberOfFrames(0x0028,0x0008);
	const mdcm::Tag tRows(0x0028,0x0010);
	const mdcm::Tag tColumns(0x0028,0x0011);
	const mdcm::Tag tDataPointRows(0x0028,0x9001);
	const mdcm::Tag tDataPointColumns(0x0028,0x9002);
	const mdcm::Tag tDataRepresentation(0x0028,0x9108);
	const mdcm::Tag tSignalDomainColumns(0x0028,0x9003);
	const mdcm::Tag tSignalDomainRows(0x0028,0x9235);
	const mdcm::Tag tFirstOrderPhaseCorrectionAngle(0x5600,0x0010);
	const mdcm::Tag tSpectroscopyData(0x5600,0x0020);

	unsigned short Rows, Columns;
	unsigned int   DataPointRows, DataPointColumns;
	if (DicomUtils::get_us_value(ds, tRows, &Rows) &&
		DicomUtils::get_us_value(ds, tColumns, &Columns) &&
		DicomUtils::get_ul_value(ds, tDataPointRows, &DataPointRows) &&
		DicomUtils::get_ul_value(ds, tDataPointColumns, &DataPointColumns))
	{
		s->m_Rows = Rows;
		s->m_Columns = Columns;
		s->m_DataPointRows = DataPointRows;
		s->m_DataPointColumns = DataPointColumns;
	}
	else
	{
		return false;
	}

	QString DataRepresentation;
	if (DicomUtils::get_string_value(ds, tDataRepresentation, DataRepresentation))
	{
		s->m_DataRepresentation = DataRepresentation;
	}
	else
	{
		return false;
	}

	QString SignalDomainColumns;
	if (DicomUtils::get_string_value(ds, tSignalDomainColumns, SignalDomainColumns))
	{
		s->m_SignalDomainColumns = SignalDomainColumns;
	}
	else
	{
		return false;
	}

#if LOAD_SPECT_DATA
	if (!DicomUtils::get_fl_values(ds, tSpectroscopyData, s->m_SpectroscopyData))
		return false;
#endif

	int NumberOfFrames;
	if (DicomUtils::get_is_value(ds, tNumberOfFrames, &NumberOfFrames))
	{
		s->m_NumberOfFrames = NumberOfFrames;
	}

	QString SignalDomainRows;
	if (DicomUtils::get_string_value(ds, tSignalDomainRows, SignalDomainRows))
	{
		s->m_SignalDomainRows = SignalDomainRows;
	}

#if LOAD_SPECT_DATA
	DicomUtils::get_fl_values(ds, tFirstOrderPhaseCorrectionAngle, s->m_FirstOrderPhaseCorrectionAngle);
#endif

	return true;
}

QString SpectroscopyUtils::ProcessData(
	const mdcm::DataSet & ds,
	std::vector<ImageVariant*> & ivariants,
	int max_3d_tex_size, GLWidget * gl, bool ok3d,
	QProgressDialog * pb,
	float tolerance)
{
	SpectroscopyData s;
	if (!Read(ds, &s)) return QString("!(Read(ds, &s)");
	const mdcm::Tag tPerFrameFunctionalGroupsSequence(0x5200,0x9230);
	const mdcm::Tag tSharedFunctionalGroupsSequence(0x5200,0x9229);
	DimIndexSq sq;
	DimIndexValues idx_values;
	FrameGroupValues values;
	FrameGroupValues shared_values;
#if LOAD_SPECT_DATA
	std::vector<float*> data;
#endif
	DicomUtils::read_dimension_index_sq(ds, sq);
	const bool ok_f = DicomUtils::read_group_sq(
		ds,
		tPerFrameFunctionalGroupsSequence,
		sq, idx_values, values);
	const bool ok_g = DicomUtils::read_group_sq(
		ds,
		tSharedFunctionalGroupsSequence,
		sq, idx_values, shared_values);
	if (!ok_f && !ok_g) return QString("!ok_f && !ok_g");

	DicomUtils::enhanced_process_values(values, shared_values);

	// stack id/position number without dimension organisation?
	bool idx_values_rebuild = false;
	if (idx_values.empty() && !values.empty())
	{
		bool tmp12 = false;
		DimIndexValues idx_values_tmp;
		for (unsigned int x = 0; x < values.size(); ++x)
		{
			if (!(
				values.at(x).stack_id_ok &&
				values.at(x).in_stack_pos_num_ok))
			{
				tmp12 = true;
				break;
			}
		}
		if (!tmp12)
		{
			for (unsigned int x = 0; x < values.size(); ++x)
			{
				DimIndexValue tmp13;
				tmp13.id = values.at(x).id;
				tmp13.idx.push_back(values.at(x).stack_id);
				tmp13.idx.push_back(values.at(x).in_stack_pos_num);
				idx_values_tmp.push_back(tmp13);
			}
			for (unsigned int x = 0; x < idx_values_tmp.size(); ++x)
			{
				idx_values.push_back(idx_values_tmp[x]);
			}
			idx_values_tmp.clear();
			idx_values_rebuild = true;
#if 0
			std::cout << "stack id and position without dim. org."
				<< std::endl;
#endif
		}
	}

	//DicomUtils::print_sq(sq);
	//DicomUtils::print_func_group(values);

	if (values.size() != static_cast<unsigned int>(s.m_NumberOfFrames))
	{
		return QString("values.size() != m_NumberOfFrames");
	}

	int dim8th = -1;
	int dim7th = -1;
	int dim6th = -1;
	int dim5th = -1;
	int dim4th = -1;
	int dim3rd = -1;
	int enh_id = -1;
	if (idx_values_rebuild)
	{
		enh_id = 0;
		dim4th = 0;
		dim3rd = 1;
	}
	else
	{
		DicomUtils::enhanced_get_indices(
			sq,
			&dim8th, &dim7th, &dim6th, &dim5th, &dim4th, &dim3rd,
			&enh_id, 1);
	}
#if 0
	std::cout << "N" << enh_id;
	std::cout
		<< " dim8th=" << dim8th
		<< " dim7th=" << dim7th
		<< " dim6th=" << dim6th
		<< " dim5th=" << dim5th
		<< " dim4th=" << dim4th
		<< " dim3rd=" << dim3rd
		<< std::endl;
#endif

	std::vector <
		std::map<
			unsigned int,
			unsigned int,
			std::less<unsigned int> > > tmp0;
	bool ok__ = DicomUtils::enhanced_process_indices(
		tmp0, idx_values, values,
		dim8th, dim7th, dim6th, dim5th, dim4th, dim3rd, false);
#if 0
	std::cout << "enhanced_process_indices = "
		<< ok__ << std::endl;
#endif
	if (!ok__)
	{
		tmp0.clear();
		ok__ = DicomUtils::enhanced_process_indices(
			tmp0, idx_values, values,
			-1, -1, -1, -1, -1, -1, false);
	}
	if (!ok__) tmp0.clear();
	if (tmp0.empty())
	{
		return QString("tmp0.size() < 1");
	}
	for (unsigned int x = 0; x < tmp0.size(); ++x)
	{
		if (tmp0.at(x).size() < 1)
		{
			return
				QString("tmp0.at(") +
				QVariant(static_cast<int>(x)).toString() +
				QString(").size() < 1");
		}
	}

#ifdef LOAD_SPECT_DATA
	const unsigned long xy =
		s.m_SpectroscopyData.size()/values.size();
	for (unsigned int j = 0; j < values.size(); ++j)
	{
		float * p__ = new float[xy];
		memcpy(p__, &(s.m_SpectroscopyData[j * xy]), xy);
		data.push_back(p__);
	}
#endif

	for (unsigned int x = 0; x < tmp0.size(); ++x)
	{
		bool error = false;
		std::vector<float*>  tmp3;
		std::vector<double*> tmp4;
		QStringList tmp5;
		unsigned int j = 0;
		std::map<
			unsigned int,
			unsigned int,
			std::less<unsigned int> > tmp1;
		for (std::map<
				unsigned int,
				unsigned int,
				std::less<unsigned int> >::const_iterator it =
				tmp0.at(x).cbegin();
			it != tmp0.at(x).cend();
			++it)
		{
			tmp1[it->second] = it->first;
		}
		if (tmp0.at(x).size() != tmp1.size())
		{
#if 1
			std::cout << "tmp0.at(x).size() != tmp1.size()"
				<< std::endl;
#endif
			continue;
		}
		for (std::map<
				unsigned int,
				unsigned int,
				std::less<unsigned int> >::const_iterator it =
				tmp1.cbegin();
			it != tmp1.cend();
			++it)
		{
			const unsigned int idx__ = it->second;
#ifdef LOAD_SPECT_DATA
			if (idx__<data.size() &&
				data.at(idx__) &&
				idx__<values.size())
#else
			if (idx__<values.size())
#endif
			{
				if (!values.at(idx__).pat_pos.isEmpty() &&
					!values.at(idx__).pat_orient.isEmpty())
				{
					double pat_pos[3];
					double pat_orient[6];
					const bool ok_p =
						DicomUtils::get_patient_position(
							values.at(idx__).pat_pos,
							pat_pos);
					const bool ok_o =
						DicomUtils::get_patient_orientation(
							values.at(idx__).pat_orient,
							pat_orient);
					if (ok_o && ok_p)
					{
						double * ss = new double[9];
						ss[0] = pat_pos[0];
						ss[1] = pat_pos[1];
						ss[2] = pat_pos[2];
						ss[3] = pat_orient[0];
						ss[4] = pat_orient[1];
						ss[5] = pat_orient[2];
						ss[6] = pat_orient[3];
						ss[7] = pat_orient[4];
						ss[8] = pat_orient[5];
						tmp4.push_back(ss);
#if LOAD_SPECT_DATA
						tmp3.push_back(data.at(idx__));
#endif
					}
					else
					{
#if 1
						std::cout << "!(ok_o && ok_p)"
							<< std::endl;
#endif
						error = true;
						break;
					}
				}
				else
				{
#if 1
					std::cout << "pat_pos / pat_orient empty"
						<< std::endl;
#endif
					error = true;
					break;
				}
				if (!values.at(idx__).pix_spacing.isEmpty())
				{
					tmp5.push_back(values.at(idx__).pix_spacing);
				}
				else
				{
					tmp5.push_back(QString(""));
				}
				//
				++j;
			}
			else
			{
				error = true;
#if 1
				std::cout << "!(idx__<data.size() && data.at(idx__)"
					" && idx__<values.size())" << std::endl;
#endif
				break;
			}
		}
		//
		if (!error)
		{
			const unsigned int rows_ = s.m_Rows;
			const unsigned int columns_ = s.m_Columns;
			bool   equi_ = false;
			bool   one_direction_ = false;
			double origin_x_gen, origin_y_gen, origin_z_gen;
			double spacing_z;
			double dircos_gen[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
			float  slices_dir_x, slices_dir_y, slices_dir_z;
			float  up_dir_x, up_dir_y, up_dir_z;
			float  center_x, center_y, center_z;
			std::vector<SpectroscopySlice*> slices;
			std::vector<ImageSlice*> empty__;
			QString orientation("");
			double spacing_x, spacing_y;
			double spacing_tmp0[2] = {0.0, 0.0};
			double spacing_tmp1[2] = {0.0, 0.0};
			bool spacing_ok = false;
			for (int i = 0; i < tmp5.size(); ++i)
			{
				spacing_ok = DicomUtils::get_pixel_spacing(
					tmp5.at(i), spacing_tmp0);
				if (!spacing_ok) break;
				if (i > 0)
				{
					if (!(
						(spacing_tmp0[0] + 0.0001 > spacing_tmp1[0]) &&
						(spacing_tmp0[0] - 0.0001 < spacing_tmp1[0]) &&
						(spacing_tmp0[1] + 0.0001 > spacing_tmp1[1]) &&
						(spacing_tmp0[1] - 0.0001 < spacing_tmp1[1])))
					{
						spacing_ok = false;
						break;
					}
				}
				spacing_tmp1[0] = spacing_tmp0[0];
				spacing_tmp1[1] = spacing_tmp0[1];
			}
			if (spacing_ok)
			{
				spacing_x = spacing_tmp0[0];
				spacing_y = spacing_tmp0[1];
			}
			else
			{
#if 1
				std::cout << "!spacing_ok" << std::endl;
#endif
				continue;
			}
			const bool geom_ok = DicomUtils::generate_geometry(
						empty__,
						slices,
						tmp4,
						rows_, columns_,
						spacing_x, spacing_y, &spacing_z,
						ok3d, gl,
						&equi_, &one_direction_,
						&origin_x_gen, &origin_y_gen, &origin_z_gen,
						dircos_gen,
						&slices_dir_x, &slices_dir_y, &slices_dir_z,
						&up_dir_x, &up_dir_y, &up_dir_z,
						&center_x, &center_y, &center_z,
						tolerance,
						true);
			if (!geom_ok)
			{
#if 1
				std::cout << "!geom_ok" << std::endl;
#endif
				continue;
			}
			if (slices.empty())
			{
#if 1
				std::cout << "slices.size()<1" << std::endl;
#endif
				continue;
			}


			// TODO process data for preview

			//
			{
				ImageVariant * ivariant = new ImageVariant(
					CommonUtils::get_next_id(), ok3d, true, gl, 0);
				ivariant->image_type = 300;
				ivariant->equi = false;
				if (equi_ && rows_ > 1 && columns_ > 1)
				{
					bool no_orientation = false;
					for (unsigned long k = 0; k < slices.size() - 1; ++k)
					{
						if (slices.at(k)->slice_orientation_string !=
							slices.at(k + 1)->slice_orientation_string)
						{
							no_orientation = true;
							break;
						}
					}
					if (!no_orientation)
					{
						ivariant->spect_orientation_string =
							slices.at(0)->slice_orientation_string;
					}
				}
				for (unsigned int k = 0; k < slices.size(); ++k)
				{
					ivariant->di->spectroscopy_slices.push_back(slices[k]);
				}
				ivariant->di->spectroscopy_generated = true;
				DicomUtils::read_ivariant_info_tags(ds, ivariant);
				if (equi_)
				{
					ivariant->di->default_center_x = ivariant->di->center_x = center_x;
					ivariant->di->default_center_y = ivariant->di->center_y = center_y;
					ivariant->di->default_center_z = ivariant->di->center_z = center_z;
				}
				else
				{
					float cx = 0.0f, cy = 0.0f, cz = 0.0f;
					CommonUtils::calculate_center_notuniform(
						ivariant->di->spectroscopy_slices, &cx, &cy, &cz);
					ivariant->di->default_center_x = ivariant->di->center_x = cx;
					ivariant->di->default_center_y = ivariant->di->center_y = cy;
					ivariant->di->default_center_z = ivariant->di->center_z = cz;
				}
				ivariant->spectroscopy_info =
					QString("<span class='y6'>") +
					QVariant(static_cast<int>(s.m_Columns)).toString() +
					QString("x") +
					QVariant(static_cast<int>(s.m_Rows)).toString() +
					QString("x") +
					QVariant(static_cast<int>(tmp4.size())).toString() +
					QString("</span><br/><span class='y4'>") +
					QVariant(static_cast<int>(s.m_DataPointColumns)).toString() +
					QString("x") +
					QVariant(static_cast<int>(s.m_DataPointRows)).toString() +
					QString("</span><span class='y6'>&#160;") +
					s.m_DataRepresentation.toLower() +
					QString("<br/>") +
					s.m_SignalDomainColumns +
					QString("&#160;") +
					s.m_SignalDomainRows +
					QString("</span>");
				ivariant->iinfo = DicomUtils::read_enhmr_spectro_info(ds, true);
				ivariants.push_back(ivariant);
			}
		}
		//
		for (unsigned int k = 0; k < tmp4.size(); ++k)
		{
			delete [] tmp4[k];
		}
		tmp4.clear();
	}

#ifdef LOAD_SPECT_DATA
	for (unsigned int x = 0; x < data.size(); ++x)
	{
		delete [] data[x];
	}
	data.clear();
#endif
	return QString("");
}

#ifdef LOAD_SPECT_DATA
#undef LOAD_SPECT_DATA
#endif

