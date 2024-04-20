#include "splituihgridfilter.h"
#include <mdcmTag.h>
#include <mdcmPrivateTag.h>
#include <mdcmDataElement.h>
#include <mdcmAttribute.h>
#include <mdcmImageHelper.h>
#include <mdcmDataSet.h>
#include <mdcmVR.h>
#include <mdcmVM.h>
#include <mdcmVL.h>
#include <cmath>
#include <vector>
#include "dicomutils.h"
#ifdef ALIZA_VERBOSE
#include <iostream>
#endif

namespace
{

template<typename T> bool reorganize_uih_grid(
	const T * input,
	const unsigned int * indims,
	const unsigned int * outdims,
	T * output)
{
	const unsigned int div =
		static_cast<unsigned int>(ceil(sqrt(static_cast<double>(outdims[2]))));
	if (!(div > 0)) return false;
	const size_t insize  = indims[0] * indims[1] * indims[2];
	const size_t outsize = outdims[0] * outdims[1] * outdims[2];
	for (unsigned int x = 0; x < outdims[0]; ++x)
	{
		for (unsigned int y = 0; y < outdims[1]; ++y)
		{
			for (unsigned int z = 0; z < outdims[2]; ++z)
			{
				const size_t outidx =
					x + y * outdims[0] + z * outdims[0] * outdims[1];
				const size_t inidx =
					(x + (z % div) * outdims[0]) +
					(y + (z / div) * outdims[1]) * indims[0];
				if (!(outidx < outsize && inidx < insize))
					return false;
				output[outidx] = input[inidx];
			}
		}
	}
	return true;
}

}

namespace mdcm
{

void SplitUihGridFilter::SetImage(
	const Image & image)
{
	I = image;
}

bool SplitUihGridFilter::ComputeUihGridDimensions(
	unsigned int * dims)
{
	const PrivateTag tMRNumberOfSliceInVolume(
		0x0065, 0x50, "Image Private Header");
	int z = 0;
	const DataSet & ds = GetFile().GetDataSet();
	if (ds.FindDataElement(tMRNumberOfSliceInVolume))
	{
		std::vector<double> result;
		if (DicomUtils::priv_get_ds_values(
				ds,tMRNumberOfSliceInVolume,result))
			z = static_cast<int>(result[0]);
	}
	else if (ds.FindDataElement(Tag(0x0065,0x1050)))
	{
		std::vector<double> result;
		if (DicomUtils::get_ds_values(
				ds,Tag(0x0065,0x1050),result))
			z = static_cast<int>(result[0]);
	}
	else
	{
		return false;
	}
	if (z < 1) return false;
	std::vector<unsigned int> idims =
		ImageHelper::GetDimensionsValue(GetFile());
	const unsigned int x =
		(idims[0] >= idims[1])
		?
		idims[0] / ceil(sqrt(z))
		:
		idims[1] / ceil(sqrt(z));
	dims[0] = x;
	dims[1] = x;
	dims[2] = static_cast<unsigned int>(z);
	if (dims[0] * dims[1] * dims[2] >
			idims[0] * idims[1] * idims[2])
		return false;
	return true;
}

bool SplitUihGridFilter::ComputeUihGridSliceNormal(
	double * n)
{
	const DataSet & ds = GetFile().GetDataSet();
	const PrivateTag tSliceNormalVector(
		0x0065,0x14,"Image Private Header");
	if (ds.FindDataElement(tSliceNormalVector))
	{
		std::vector<double> result;
		if (DicomUtils::priv_get_ds_values(
			ds,tSliceNormalVector,result))
		{
			if (result.size() == 3)
			{
				n[0] = result[0];
				n[1] = result[1];
				n[2] = result[2];
				return true;
			}
		}
	}
	else if (ds.FindDataElement(Tag(0x0065,0x1014)))
	{
		std::vector<double> result;
		if (DicomUtils::get_ds_values(
				ds,Tag(0x0065,0x1014),result))
		{
			if (result.size() == 3)
			{
				n[0] = result[0];
				n[1] = result[1];
				n[2] = result[2];
				return true;
			}
		}
	}
	return false;
}

bool SplitUihGridFilter::ComputeUihGridSlicePosition(
	double * pos)
{
	DataSet & ds = GetFile().GetDataSet();
	const PrivateTag tMRVFrameSequence(
		0x0065,0x51,"Image Private Header");
	SmartPointer<SequenceOfItems> sq;
	if (ds.FindDataElement(tMRVFrameSequence))
	{
		const DataElement & e =
			ds.GetDataElement(tMRVFrameSequence);
		sq = e.GetValueAsSQ();
		if (!(sq && sq->GetNumberOfItems()>0))
			return false;
	}
	else if (ds.FindDataElement(Tag(0x0065,0x1051)))
	{
		const DataElement & e =
			ds.GetDataElement(Tag(0x0065,0x1051));
		sq = e.GetValueAsSQ();
		if (!(sq && sq->GetNumberOfItems()>0))
			return false;
	}
	else
	{
		return false;
	}
	// position of the first frame
	const Item & item = sq->GetItem(1);
	const DataSet & nds = item.GetNestedDataSet();
	const Tag tImagePositionPatient(0x0020,0x0032);
	std::vector<double> result;
	if (DicomUtils::get_ds_values(
			nds,tImagePositionPatient,result))
	{
		if (result.size() == 3)
		{
			pos[0] = result[0];
			pos[1] = result[1];
			pos[2] = result[2];
			return true;
		}
	}
	return false;
}

bool SplitUihGridFilter::Split()
{
	unsigned int dims[3]{};
	if (!ComputeUihGridDimensions(dims))
	{
#ifdef ALIZA_VERBOSE
		std::cout << "!ComputeUihGridDimensions" << std::endl;
#endif
		return false;
	}
#if 0
	// TODO check again
	double normal[3];
	if (!ComputeUihGridSliceNormal(normal))
	{
#ifdef ALIZA_VERBOSE
		std::cout << "!ComputeUihGridSliceNormal" << std::endl;
#endif
		return false;
	}
#endif
	double origin[3];
	if (!ComputeUihGridSlicePosition(origin))
	{
#ifdef ALIZA_VERBOSE
		std::cout << "!ComputeUihGridSlicePosition" << std::endl;
#endif
		return false;
	}
	const Image & inputimage = GetImage();
	unsigned long l = inputimage.GetBufferLength();
	std::vector<char> buf;
	buf.resize(l);
	inputimage.GetBuffer(buf.data());
	DataElement pixeldata(Tag(0x7fe0,0x0010));
	std::vector<char> outbuf;
	outbuf.resize(l);
	const void * vbuf = static_cast<void*>(buf.data());
	void * voutbuf = static_cast<void*>(outbuf.data());
	bool b = false;
	if (inputimage.GetPixelFormat() == PixelFormat::UINT16)
	{
		b = reorganize_uih_grid<unsigned short>(
			static_cast<const unsigned short*>(vbuf),
			inputimage.GetDimensions(),
			dims,
			static_cast<unsigned short*>(voutbuf));
	}
	else if (inputimage.GetPixelFormat() == PixelFormat::INT16)
	{
		b = reorganize_uih_grid<signed short>(
			static_cast<const signed short*>(vbuf),
			inputimage.GetDimensions(),
			dims,
			static_cast<signed short*>(voutbuf));
	}
	else if (inputimage.GetPixelFormat() == PixelFormat::UINT8)
	{
		b = reorganize_uih_grid<unsigned char>(
			static_cast<const unsigned char*>(vbuf),
			inputimage.GetDimensions(),
			dims,
			static_cast<unsigned char*>(voutbuf));
	}
	else if (inputimage.GetPixelFormat() == PixelFormat::INT8)
	{
		b = reorganize_uih_grid<signed char>(
			static_cast<const signed char*>(vbuf),
			inputimage.GetDimensions(),
			dims,
			static_cast<signed char*>(voutbuf));
	}
	if (!b)
	{
#ifdef ALIZA_VERBOSE
		std::cout << "!reorganize_uih_grid" << std::endl;
#endif
		return false;
	}
	pixeldata.SetByteValue(outbuf.data(), static_cast<VL::Type>(outbuf.size()));
	Image & image = GetImage();
	const TransferSyntax &ts = image.GetTransferSyntax();
	if (ts.IsExplicit())
	{
		image.SetTransferSyntax(
			TransferSyntax::ExplicitVRLittleEndian);
	}
	else
	{
		image.SetTransferSyntax(
			TransferSyntax::ImplicitVRLittleEndian);
	}
	image.SetNumberOfDimensions(3);
	image.SetDimension(0, dims[0]);
	image.SetDimension(1, dims[1]);
	image.SetDimension(2, dims[2]);
	image.SetOrigin(origin);
	image.SetDataElement(pixeldata);
	return true;
}

}

