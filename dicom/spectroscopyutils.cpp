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
#include "vectormath/scalar/vectormath.h"

//#define LOAD_SPECT_DATA

namespace
{

typedef Vectormath::Scalar::Vector3 sVector3;
typedef Vectormath::Scalar::Vector4 sVector4;
typedef Vectormath::Scalar::Matrix4 sMatrix4;

bool generate_spectorscopy_geometry(
		std::vector<SpectroscopySlice*> & spectorscopyslices,
		const std::vector<double*> & values,
		const unsigned int rows_, const unsigned int columns_,
		const double spacing_x, const double spacing_y, double * spacing_z,
		const bool ok3d, GLWidget * gl,
		bool * equi_,
		bool * one_direction_,
		double * origin_x,  double * origin_y,  double * origin_z,
		double * dircos,
		float * slices_dir_x, float * slices_dir_y, float * slices_dir_z,
		float * up_dir_x, float * up_dir_y, float * up_dir_z,
		float * center_x, float * center_y, float * center_z,
		float tolerance)
{
	const unsigned int size_ = values.size();
	if (size_ < 1) return false;
	sVector3 first = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 last  = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 v0 = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 v1 = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 up = sVector3(0.0f, 0.0f, 0.0f);
	QString tmp0;
	bool tmp1 = true, tmp2 = true;
	sVector3 tmp_p0 = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 tmp_p1 = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 tmp_p2 = sVector3(0.0f, 0.0f, 0.0f);
	sVector3 tmp_p3 = sVector3(0.0f, 0.0f, 0.0f);
	float tmp_length0 = 0.0f, tmp_length1 = 0.0f, tmp_length2 = 0.0f, tmp_length3 = 0.0f;
	double spacing_z_ = 0;
	bool invalidate_volume = false;
	for (unsigned int i = 0; i < size_; ++i)
	{
		const double * ipp_iop = values.at(i);
		const sVector4 c0 = sVector4(
					static_cast<float>(ipp_iop[3] * spacing_x),
					static_cast<float>(ipp_iop[4] * spacing_x),
					static_cast<float>(ipp_iop[5] * spacing_x),
					0.0f);
		const sVector4 c1 = sVector4(
					static_cast<float>(ipp_iop[6] * spacing_y),
					static_cast<float>(ipp_iop[7] * spacing_y),
					static_cast<float>(ipp_iop[8] * spacing_y),
					0.0f);
		const sVector4 c2 = sVector4(0.0f, 0.0f, 0.0f, 0.0f);
		const sVector4 c3 = sVector4(
					static_cast<float>(ipp_iop[0]),
					static_cast<float>(ipp_iop[1]),
					static_cast<float>(ipp_iop[2]),
					1.0f);
#if 0
		std::cout << "ipp[0]=" << ipp_iop[0] << std::endl;
		std::cout << "ipp[1]=" << ipp_iop[1] << std::endl;
		std::cout << "ipp[2]=" << ipp_iop[2] << std::endl;
#endif
		sMatrix4 m0 = sMatrix4::identity();
		m0.setCol0(c0);
		m0.setCol1(c1);
		m0.setCol2(c2);
		m0.setCol3(c3);
		const float r_ = rows_ - 1;
		const float c_ = columns_ - 1;
		const sVector4 ind0 = sVector4(0.0f ,  r_, 0.0f, 1.0f);
		const sVector4 ind1 = sVector4(0.0f, 0.0f, 0.0f, 1.0f);
		const sVector4 ind2 = sVector4(  c_,   r_, 0.0f, 1.0f);
		const sVector4 ind3 = sVector4(  c_, 0.0f, 0.0f, 1.0f);
		const sVector4 p0 = m0*ind0;
		const sVector4 p1 = m0*ind1;
		const sVector4 p2 = m0*ind2;
		const sVector4 p3 = m0*ind3;
		const float x0 = p0.getX(), y0 = p0.getY(), z0 = p0.getZ();
		const float x1 = p1.getX(), y1 = p1.getY(), z1 = p1.getZ();
		const float x2 = p2.getX(), y2 = p2.getY(), z2 = p2.getZ();
		const float x3 = p3.getX(), y3 = p3.getY(), z3 = p3.getZ();
#if 0
		std::cout << "x0=" << x0 << " y0=" << y0 << " z0=" << z0 << std::endl;
		std::cout << "x1=" << x1 << " y1=" << y1 << " z1=" << z1 << std::endl;
		std::cout << "x2=" << x2 << " y2=" << y2 << " z2=" << z2 << std::endl;
		std::cout << "x3=" << x3 << " y3=" << y3 << " z3=" << z3 << std::endl;
#endif
		const QString orientation_string = CommonUtils::get_orientation2(&ipp_iop[3]);
		if (i == 0)
		{
			first = sVector3(x0, y0, z0);
			v0 = (p0.getXYZ() + p3.getXYZ()) * 0.5f;
			sVector3 tmp_up0 = sVector3(x1, y1, z1);
			sVector3 tmp_up1 = sVector3(x0, y0, z0);
			up = normalize(tmp_up1 - tmp_up0);
			*origin_x = ipp_iop[0];
			*origin_y = ipp_iop[1];
			*origin_z = ipp_iop[2];
			for (int j = 0; j < 6; ++j) dircos[j] = ipp_iop[3 + j];
		}
		if (i == size_ - 1)
		{
			last = sVector3(x0, y0, z0);
			v1 = (p0.getXYZ()+p3.getXYZ()) * 0.5f;
		}
		CommonUtils::generate_spectroscopyslice(
			spectorscopyslices,
			orientation_string,
			ok3d, gl,
			x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3,
			columns_, rows_);
		const float length0 = Vectormath::Scalar::length(p0.getXYZ()-tmp_p0);
		const float length1 = Vectormath::Scalar::length(p1.getXYZ()-tmp_p1);
		const float length2 = Vectormath::Scalar::length(p2.getXYZ()-tmp_p2);
		const float length3 = Vectormath::Scalar::length(p3.getXYZ()-tmp_p3);
		// check orientation
		if (i != 0)
		{
			if (tmp1)
			{
				if (tmp0 != orientation_string) tmp1 = false;
				if ((length0 > (length1+tolerance))||(length0 < (length1-tolerance)))
					tmp1 = false;
				if ((length1 > (length2+tolerance))||(length1 < (length2-tolerance)))
					tmp1 = false;
				if ((length2 > (length3+tolerance))||(length2 < (length3-tolerance)))
					tmp1 = false;
				if ((length3 > (length0+tolerance))||(length3 < (length0-tolerance)))
					tmp1 = false;
			}
		}
		// check equidistance, requires 3 slices
		if (i >= 2)
		{
			if (tmp2)
			{
				if ((length0 > (tmp_length0+tolerance))||(length0 < (tmp_length0-tolerance)))
					tmp2 = false;
				if ((length1 > (tmp_length1+tolerance))||(length1 < (tmp_length1-tolerance)))
					tmp2 = false;
				if ((length2 > (tmp_length2+tolerance))||(length2 < (tmp_length2-tolerance)))
					tmp2 = false;
				if ((length3 > (tmp_length3+tolerance))||(length3 < (tmp_length3-tolerance)))
					tmp2 = false;
			}
		}
#if 0
		std::cout << "length0=" << length0 << std::endl;
		std::cout << "length1=" << length1 << std::endl;
		std::cout << "length2=" << length2 << std::endl;
		std::cout << "length3=" << length3 << std::endl;
		std::cout << "tmp1=" << tmp1 << std::endl;
		std::cout << "tmp2=" << tmp2 << std::endl;
		std::cout << "------------------------" << std::endl;
#endif
		if (i == size_ - 1)
		{
			spacing_z_ = length1;
#if 0
			std::cout << "spacing_z_=" << spacing_z_ << std::endl;
#endif
		}
		tmp0 = orientation_string;
		tmp_p0 = p0.getXYZ();
		tmp_p1 = p1.getXYZ();
		tmp_p2 = p2.getXYZ();
		tmp_p3 = p3.getXYZ();
		tmp_length0 = length0;
		tmp_length1 = length1;
		tmp_length2 = length2;
		tmp_length3 = length3;
	}
	const float row_dircos_x = static_cast<float>(dircos[0]);
	const float row_dircos_y = static_cast<float>(dircos[1]);
	const float row_dircos_z = static_cast<float>(dircos[2]);
	const float col_dircos_x = static_cast<float>(dircos[3]);
	const float col_dircos_y = static_cast<float>(dircos[4]);
	const float col_dircos_z = static_cast<float>(dircos[5]);
	const float nrm_dircos_x = row_dircos_y * col_dircos_z - row_dircos_z * col_dircos_y;
	const float nrm_dircos_y = row_dircos_z * col_dircos_x - row_dircos_x * col_dircos_z;
	const float nrm_dircos_z = row_dircos_x * col_dircos_y - row_dircos_y * col_dircos_x;
	const sVector3 direction1 = normalize(sVector3(nrm_dircos_x, nrm_dircos_y, nrm_dircos_z));
	if (size_ > 1)
	{
		const sVector3 direction0_tmp = last - first;
		if (!(
			(direction0_tmp.getX() > -0.00001f && direction0_tmp.getX() < 0.00001f) &&
			(direction0_tmp.getY() > -0.00001f && direction0_tmp.getY() < 0.00001f) &&
			(direction0_tmp.getZ() > -0.00001f && direction0_tmp.getZ() < 0.00001f)
			))
		{
			const sVector3 direction0 = normalize(direction0_tmp);
			*slices_dir_x = direction0.getX();
			*slices_dir_y = direction0.getY();
			*slices_dir_z = direction0.getZ();
			if (tmp1 &&
				((tmp2 && (size_ > 2)) || size_ == 2) &&
				!(
				(direction0.getX() < direction1.getX() + 0.001f && direction0.getX() > direction1.getX() - 0.001f) &&
				(direction0.getY() < direction1.getY() + 0.001f && direction0.getY() > direction1.getY() - 0.001f) &&
				(direction0.getZ() < direction1.getZ() + 0.001f && direction0.getZ() > direction1.getZ() - 0.001f)
				))
			{
				invalidate_volume = true;
#if 1
				std::cout
					<< "Warning:\n"
					<< "Direction cosines defined in DICOM file: "
					<< row_dircos_x << "\\" << row_dircos_y << "\\" << row_dircos_z << "\\"
					<< col_dircos_x << "\\" << col_dircos_y << "\\" << col_dircos_z << "\n"
					<< " Z direction calculated from defined cosines: "
					<< direction1.getX() << "," << direction1.getY() << "," << direction1.getZ() << "\n"
					<< " Z direction from geometry (real): "
					<< direction0.getX() << "," << direction0.getY() << "," << direction0.getZ() << "\n"
					<< " ... using image as non-uniform.\n"
					<< std::endl;
#endif
#if 0
				const QString z_inv_string =
					QString("Direction cosines defined in DICOM file:\n") +
					QVariant(static_cast<double>(row_dircos_x)).toString() + QString("\\") +
					QVariant(static_cast<double>(row_dircos_y)).toString() + QString("\\") +
					QVariant(static_cast<double>(row_dircos_z)).toString() + QString("\\") +
					QVariant(static_cast<double>(col_dircos_x)).toString() + QString("\\") +
					QVariant(static_cast<double>(col_dircos_y)).toString() + QString("\\") +
					QVariant(static_cast<double>(col_dircos_z)).toString() + QString("\n") +
					QString(" Z direction calculated from defined cosines: ") +
					QVariant(static_cast<double>(direction1.getX())).toString() + QString(",") +
					QVariant(static_cast<double>(direction1.getY())).toString() + QString(",") +
					QVariant(static_cast<double>(direction1.getZ())).toString() + QString("\n") +
					QString(" Z direction calculated from geometry (real): ") +
					QVariant(static_cast<double>(direction0.getX())).toString() + QString(",") +
					QVariant(static_cast<double>(direction0.getY())).toString() + QString(",") +
					QVariant(static_cast<double>(direction0.getZ())).toString() + QString("\n") +
					QString(" ... using image as non-uniform.\n") +
					QMessageBox mbox;
					mbox.addButton(QMessageBox::Close);
					mbox.setIcon(QMessageBox::Information);
					mbox.setText(z_inv_string);
					mbox.exec();
#endif

			}
		}
		else
		{
			*slices_dir_x = direction1.getX();
			*slices_dir_y = direction1.getY();
			*slices_dir_z = direction1.getZ();
			invalidate_volume = true;
		}
	}
	else
	{
		*slices_dir_x = direction1.getX();
		*slices_dir_y = direction1.getY();
		*slices_dir_z = direction1.getZ();
		*equi_ = true;
	}
	//
	if (invalidate_volume)
	{
		*equi_ = false;
	}
	else
	{
		if      (size_ >  2) *equi_ = (tmp1 && tmp2);
		else if (size_ == 2) *equi_ = tmp1;
	}
	//
	*up_dir_x = up.getX();
	*up_dir_y = up.getY();
	*up_dir_z = up.getZ();
	if (*equi_ == true)
	{
		const sVector3 cube_center = 0.5f * (v0 + v1);
		*center_x = cube_center.getX();
		*center_y = cube_center.getY();
		*center_z = cube_center.getZ();
		*one_direction_ = true;
	}
	else
	{
		if (tmp1 && size_ > 1) *one_direction_ = true;
		else *one_direction_ = false;
	}
	*spacing_z = (size_ > 1) ? spacing_z_ : 1.0;
	return true;
}

}

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
		s->m_DataRepresentation = std::move(DataRepresentation);
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
		s->m_SignalDomainRows = std::move(SignalDomainRows);
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
			QString orientation;
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
			const bool geom_ok = generate_spectorscopy_geometry(
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
				0.01f);
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

