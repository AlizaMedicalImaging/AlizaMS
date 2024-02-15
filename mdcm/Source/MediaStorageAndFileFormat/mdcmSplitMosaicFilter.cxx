/*********************************************************
 *
 * MDCM
 *
 * Modifications github.com/issakomi
 *
 *********************************************************/

/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2024 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "mdcmSplitMosaicFilter.h"
#include "mdcmCSAHeader.h"
#include "mdcmAttribute.h"
#include "mdcmImageHelper.h"
#include "mdcmDirectionCosines.h"
#include <cmath>

namespace
{

template<typename T> bool
reorganize_mosaic(const T *            input,
                  const unsigned int * inputdims,
                  unsigned int         square,
                  const unsigned int * outputdims,
                  T *                  output)
{
  const size_t outputdims_0 = static_cast<size_t>(outputdims[0]);
  const size_t outputdims_1 = static_cast<size_t>(outputdims[1]);
  const size_t outputdims_2 = static_cast<size_t>(outputdims[2]);
  for (size_t x = 0; x < outputdims_0; ++x)
  {
    for (size_t y = 0; y < outputdims_1; ++y)
    {
      for (size_t z = 0; z < outputdims_2; ++z)
      {
        const size_t outputidx = x + y * outputdims_0 + z * outputdims_0 * outputdims_1;
        const size_t inputidx =
          (x + (z % square) * outputdims_0) + (y + (z / square) * outputdims_1) * inputdims[0];
        output[outputidx] = input[inputidx];
      }
    }
  }
  return true;
}

template<typename T> bool
reorganize_mosaic_invert(const T *            input,
                         const unsigned int * inputdims,
                         unsigned int         square,
                         const unsigned int * outputdims,
                         T *                  output)
{
  const size_t outputdims_0 = static_cast<size_t>(outputdims[0]);
  const size_t outputdims_1 = static_cast<size_t>(outputdims[1]);
  const size_t outputdims_2 = static_cast<size_t>(outputdims[2]);
  for (size_t x = 0; x < outputdims_0; ++x)
  {
    for (size_t y = 0; y < outputdims_1; ++y)
    {
      for (size_t z = 0; z < outputdims_2; ++z)
      {
        const size_t outputidx = x + y * outputdims_0 + (outputdims_2 - 1 - z) * outputdims_0 * outputdims_1;
        const size_t inputidx =
          (x + (z % square) * outputdims_0) + (y + (z / square) * outputdims_1) * inputdims[0];
        output[outputidx] = input[inputidx];
      }
    }
  }
  return true;
}

}

namespace mdcm
{

SplitMosaicFilter::SplitMosaicFilter()
  : F(new File), I(new Image)
{}

void
SplitMosaicFilter::SetImage(const Image & image)
{
  I = image;
}

bool SplitMosaicFilter::GetAcquisitionSize(unsigned int size[2], const DataSet & ds)
{
  // Dimensions of the acquired frequency and phase data before reconstruction:
  // frequency rows \ frequency columns \ phase rows \ phase columns
  Attribute<0x0018, 0x1310, VR::US, VM::VM4> acquisitionMatrix;
  acquisitionMatrix.SetFromDataSet(ds);
  const unsigned short * pMat = acquisitionMatrix.GetValues();
  // Enumerated Values:
  //   ROW -- phase encoded in rows
  //   COL -- phase encoded in columns
  Attribute<0x0018, 0x1312> inPlanePhaseEncodingDirection;
  inPlanePhaseEncodingDirection.SetFromDataSet(ds);
  const CSComp val = inPlanePhaseEncodingDirection.GetValue();
  const std::string d = val.Trim();
  if (d == "COL")
  {
    // columns, rows
    size[0] = pMat[3];
    size[1] = pMat[0];
  }
  else if (d == "ROW")
  {
    size[0] = pMat[1];
    size[1] = pMat[2];
  }
  else
  {
    size[0] = 0;
    size[1] = 0;
  }
  return (size[0] && size[1]);
}

bool
SplitMosaicFilter::ComputeMOSAICDimensions(unsigned int dims[3])
{
  CSAHeader          csa;
  DataSet &          ds = GetFile().GetDataSet();
  const PrivateTag & t1 = csa.GetCSAImageHeaderInfoTag();
  int                numberOfImagesInMosaic{};
  if (csa.LoadFromDataElement(ds.GetDataElement(t1)))
  {
    if (csa.FindCSAElementByName("NumberOfImagesInMosaic"))
    {
      const CSAElement & csael4 = csa.GetCSAElementByName("NumberOfImagesInMosaic");
      if (!csael4.IsEmpty())
      {
        Element<VR::IS, VM::VM1> el4 = { { 0 } };
        el4.Set(csael4.GetValue());
        numberOfImagesInMosaic = el4.GetValue();
      }
    }
  }
  if (numberOfImagesInMosaic == 0)
  {
    PrivateTag t2(0x0019, 0x0a, "SIEMENS MR HEADER");
    if (ds.FindDataElement(t2))
    {
      const DataElement & de = ds.GetDataElement(t2);
      const ByteValue *   bv = de.GetByteValue();
      if (bv)
      {
        Element<VR::US, VM::VM1> el1 = { { 0 } };
        std::istringstream       is;
        is.str(std::string(bv->GetPointer(), bv->GetLength()));
        el1.Read(is);
        numberOfImagesInMosaic = el1.GetValue();
      }
    }
  }
  //
  const std::vector<unsigned int> col_row = ImageHelper::GetDimensionsValue(GetFile());
  if (numberOfImagesInMosaic == 0)
  {
    unsigned int mosaic_size[2]{};
    if (GetAcquisitionSize(mosaic_size, ds))
    {
      // May contain trailing empty slices.
      if (col_row[0] % mosaic_size[0] == 0 && col_row[1] % mosaic_size[1] == 0)
      {
        numberOfImagesInMosaic = (col_row[0] / mosaic_size[0]) * (col_row[1] / mosaic_size[1]);
      }
    }
  }
  if (numberOfImagesInMosaic == 0)
  {
    mdcmErrorMacro("Could not find NumberOfImagesInMosaic");
    return false;
  }
  dims[0] = col_row[0];
  dims[1] = col_row[1];
  const unsigned int div = static_cast<unsigned int>(ceil(sqrt(static_cast<double>(numberOfImagesInMosaic))));
  if (div > 0)
  {
    dims[0] /= div;
    dims[1] /= div;
    dims[2] = numberOfImagesInMosaic;
  }
  else
  {
    return false;
  }
  return true;
}

bool
SplitMosaicFilter::ComputeMOSAICSliceNormal(double slicenormalvector[3], bool & inverted)
{
  CSAHeader          csa;
  DataSet &          ds = GetFile().GetDataSet();
  double             normal[3];
  bool               snvfound{};
  const PrivateTag & t1 = csa.GetCSAImageHeaderInfoTag();
  static const char  snvstr[] = "SliceNormalVector";
  if (csa.LoadFromDataElement(ds.GetDataElement(t1)))
  {
    if (csa.FindCSAElementByName(snvstr))
    {
      const CSAElement & snv_csa = csa.GetCSAElementByName(snvstr);
      if (!snv_csa.IsEmpty())
      {
        const ByteValue *  bv = snv_csa.GetByteValue();
        const std::string  str(bv->GetPointer(), bv->GetLength());
        std::istringstream is;
        is.str(str);
        char sep;
        if (is >> normal[0] >> sep >> normal[1] >> sep >> normal[2])
        {
          snvfound = true;
        }
      }
    }
  }
  if (snvfound)
  {
    Attribute<0x0020, 0x0037> iop;
    iop.SetFromDataSet(ds);
    DirectionCosines dc(iop.GetValues());
    double           z[3];
    dc.Cross(z);
    const double snv_dot = dc.Dot(normal, z);
    if ((1.0 - snv_dot) < 1e-6)
    {
      inverted = false;
    }
    else if ((-1.0 - snv_dot) < 1e-6)
    {
      // TODO find files to test
#if 1
      mdcmAlwaysWarnMacro("SplitMosaicFilter: seems to be inverted");
#endif
      inverted = true;
    }
    else
    {
      mdcmAlwaysWarnMacro("SplitMosaicFilter: unexpected normal for Mosaic image");
      return false;
    }
    slicenormalvector[0] = normal[0];
    slicenormalvector[1] = normal[1];
    slicenormalvector[2] = normal[2];
  }
  return snvfound;
}

void SplitMosaicFilter::ComputeMOSAICImagePositionPatient(double ret[3], 
                                                          const double ipp[6],
                                                          const double dircos[6],
                                                          const double pixelspacing[3],
                                                          const unsigned int image_dims[3],
                                                          const unsigned int mosaic_dims[3],
                                                          bool inverted)
{
  // This is an attempt to calculate the origin, it may be wrong.
  CSAHeader        csa;
  DataSet &        ds = GetFile().GetDataSet();
  DirectionCosines dc(dircos);
  dc.Normalize();
  const double * dircos_normalized = dc;
  const double * x = dircos_normalized;
  const double * y = dircos_normalized + 3;
  double ipp_csa[3]{};
  double ipp_dcm[3]{};
  bool hasIppCsa{};
  MrProtocol mrprot;
  if (csa.GetMrProtocol(ds, mrprot)) 
  {
    // https://www.nmr.mgh.harvard.edu/~greve/dicom-unpack
    //
    // Mosaics - DICOM (20,32) is incorrect for mosaics. The value in
    // this field gives where the origin of an image the size of the
    // mosaic would have been had such an image been collected. This puts
    // the origin outside of the scanner.  However, the center of a slice
    // can be obtained from the ASCII header from lines of the form
    // "sSliceArray.asSlice[N].sPosition.dAAA", where N is the slice
    // number and AAA is Sag (x), Cor (y), and Tra (z). This may be off by
    // half a voxel. Given this information, the direction cosines, the
    // voxel size, and dimension, the origin can be computed.
    MrProtocol::SliceArray sa;
    const bool b = mrprot.GetSliceArray(sa);
    if (b)
    {
      const size_t s = sa.Slices.size();
      if (s > 0 && s == mosaic_dims[2])
      {
        const size_t                idx = inverted ? s - 1 : 0;
        const MrProtocol::Slice &   slice = sa.Slices[idx];
        const MrProtocol::Vector3 & p = slice.Position;
        const double                pos[3]{p.dSag, p.dCor, p.dTra};
        const double                tmp0 = mosaic_dims[0] / 2.0 * pixelspacing[0];
        const double                tmp1 = mosaic_dims[1] / 2.0 * pixelspacing[1];
        ipp_csa[0] = pos[0] - tmp0 * x[0] - tmp1 * y[0];
        ipp_csa[1] = pos[1] - tmp0 * x[1] - tmp1 * y[1];
        ipp_csa[2] = pos[2] - tmp0 * x[2] - tmp1 * y[2];
        hasIppCsa = true;
      }
      else if (s == 1)
      {
        // TODO find files to test
		//
        // MM: there is a single SliceArray but multiple mosaics, assume
        // this is exactly the center one
        double z[3];
        dc.Cross(z);
        DirectionCosines::Normalize(z);
        MrProtocol::Slice &   slice = sa.Slices[0];
        MrProtocol::Vector3 & p = slice.Position;
        const double          pos[3]{p.dSag, p.dCor, p.dTra};
        const double          tmp0 = mosaic_dims[0] / 2.0 * pixelspacing[0];
        const double          tmp1 = mosaic_dims[1] / 2.0 * pixelspacing[1];
        const double          tmp3 = (mosaic_dims[2] - 1) / 2.0 * pixelspacing[2];
        ipp_csa[0] = pos[0] - tmp0 * x[0] - tmp1 * y[0] - tmp3 * z[0];
        ipp_csa[1] = pos[1] - tmp0 * x[1] - tmp1 * y[1] - tmp3 * z[1];
        ipp_csa[2] = pos[2] - tmp0 * x[2] - tmp1 * y[2] - tmp3 * z[2];
        hasIppCsa = true;
      }
    }
  }
  {
    // https://nipy.org/nibabel/dicom/dicom_mosaic.html#dicom-mosaic
    const double tmp0 = (image_dims[0] - mosaic_dims[0]) / 2.0 * pixelspacing[0];
    const double tmp1 = (image_dims[1] - mosaic_dims[1]) / 2.0 * pixelspacing[1];
    ipp_dcm[0] = ipp[0] + tmp0 * x[0] + tmp1 * y[0];
    ipp_dcm[1] = ipp[1] + tmp0 * x[1] + tmp1 * y[1];
    ipp_dcm[2] = ipp[2] + tmp0 * x[2] + tmp1 * y[2];
  }
  //
  if (hasIppCsa)
  {
#if 1
    const double d[3]{ipp_dcm[0] - ipp_csa[0], ipp_dcm[1] - ipp_csa[1], ipp_dcm[2] - ipp_csa[2]};
    const double n = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    if (n > 1e-4)
    {
      mdcmAlwaysWarnMacro("SplitMosaicFilter: ImagePositionPatient\n from DCM: "
                          << ipp_dcm[0] << " " << ipp_dcm[1] << " " << ipp_dcm[2] << "\n from CSA: "
                          << ipp_csa[0] << " " << ipp_csa[1] << " " << ipp_csa[2]);
    }
#endif
    ret[0] = ipp_csa[0];
    ret[1] = ipp_csa[1];
    ret[2] = ipp_csa[2];
  }
  else
  {
    ret[0] = ipp_dcm[0];
    ret[1] = ipp_dcm[1];
    ret[2] = ipp_dcm[2];
  }
}

bool
SplitMosaicFilter::Split()
{
  const Image & inputimage = GetImage();
  unsigned long long l = inputimage.GetBufferLength();
  if (l >= 0xffffffff)
  {
    mdcmAlwaysWarnMacro("SplitMosaicFilter: buffer length = " << l);
    return false;
  }
  DataSet &    ds = GetFile().GetDataSet();
  unsigned int dims[3]{};
  if (!ComputeMOSAICDimensions(dims))
  {
    return false;
  }
  const unsigned int div = static_cast<unsigned int>(ceil(sqrt(static_cast<double>(dims[2]))));
  bool               inverted{};
  double             normal[3];
  if (!ComputeMOSAICSliceNormal(normal, inverted))
  {
    return false;
  }
  double origin[3]{};
  ComputeMOSAICImagePositionPatient(origin,
                                    inputimage.GetOrigin(),
                                    inputimage.GetDirectionCosines(),
                                    inputimage.GetSpacing(),
                                    inputimage.GetDimensions(),
                                    dims,
                                    inverted);
  std::vector<char> buf;
  buf.resize(l);
  inputimage.GetBuffer(buf.data());
  DataElement       pixeldata(Tag(0x7fe0, 0x0010));
  std::vector<char> outbuf;
  outbuf.resize(l);
  void * vbuf = static_cast<void*>(buf.data());
  void * voutbuf = static_cast<void*>(outbuf.data());
  bool b{};
  if (inverted)
  {
    if (inputimage.GetPixelFormat() == PixelFormat::UINT16)
    {
      b = reorganize_mosaic_invert<unsigned short>(static_cast<unsigned short *>(vbuf),
                                                   inputimage.GetDimensions(),
                                                   div,
                                                   dims,
                                                   static_cast<unsigned short *>(voutbuf));
    }
    else if (inputimage.GetPixelFormat() == PixelFormat::INT16)
    {
      b = reorganize_mosaic_invert<signed short>(static_cast<signed short *>(vbuf),
                                                 inputimage.GetDimensions(),
                                                 div,
                                                 dims,
                                                 static_cast<signed short *>(voutbuf));
    }
    else if (inputimage.GetPixelFormat() == PixelFormat::UINT8)
    {
      b = reorganize_mosaic_invert<unsigned char>(static_cast<unsigned char *>(vbuf),
                                                  inputimage.GetDimensions(),
                                                  div,
                                                  dims,
                                                  static_cast<unsigned char *>(voutbuf));
    }
    else if (inputimage.GetPixelFormat() == PixelFormat::INT8)
    {
      b = reorganize_mosaic_invert<signed char>(static_cast<signed char *>(vbuf),
                                                inputimage.GetDimensions(),
                                                div,
                                                dims,
                                                static_cast<signed char *>(voutbuf));
    }
  }
  else
  {
    if (inputimage.GetPixelFormat() == PixelFormat::UINT16)
    {
      b = reorganize_mosaic<unsigned short>(static_cast<unsigned short *>(vbuf),
                                            inputimage.GetDimensions(),
                                            div,
                                            dims,
                                            static_cast<unsigned short *>(voutbuf)); 
    }
    else if (inputimage.GetPixelFormat() == PixelFormat::INT16)
    {
      b = reorganize_mosaic<signed short>(static_cast<signed short *>(vbuf),
                                          inputimage.GetDimensions(),
                                          div,
                                          dims,
                                          static_cast<signed short *>(voutbuf));
    }
    else if (inputimage.GetPixelFormat() == PixelFormat::UINT8)
    {
      b = reorganize_mosaic<unsigned char>(static_cast<unsigned char *>(vbuf),
                                           inputimage.GetDimensions(),
                                           div,
                                           dims,
                                           static_cast<unsigned char *>(voutbuf));
    }
    else if (inputimage.GetPixelFormat() == PixelFormat::INT8)
    {
      b = reorganize_mosaic<signed char>(static_cast<signed char *>(vbuf),
                                         inputimage.GetDimensions(),
                                         div,
                                         dims,
                                         static_cast<signed char *>(voutbuf));
    }
  }
  if (!b)
    return false;
  const unsigned long long outbuf_size = outbuf.size();
  if (outbuf_size >= 0xffffffff)
  {
    mdcmAlwaysWarnMacro("SplitMosaicFilter: output buffer size " << outbuf_size);
    return false;
  }
  pixeldata.SetByteValue(outbuf.data(), static_cast<VL::Type>(outbuf_size));
  Image &                image = GetImage();
  const TransferSyntax & ts = image.GetTransferSyntax();
  if (ts.IsExplicit())
  {
    image.SetTransferSyntax(TransferSyntax::ExplicitVRLittleEndian);
  }
  else
  {
    image.SetTransferSyntax(TransferSyntax::ImplicitVRLittleEndian);
  }
  image.SetNumberOfDimensions(3);
  image.SetDimension(0, dims[0]);
  image.SetDimension(1, dims[1]);
  image.SetDimension(2, dims[2]);
  image.SetOrigin(origin);
  PhotometricInterpretation pi;
  pi = PhotometricInterpretation::MONOCHROME2;
  image.SetDataElement(pixeldata);
  MediaStorage ms = MediaStorage::SecondaryCaptureImageStorage;
  ms.SetFromFile(GetFile());
#if 0
  if (ms != MediaStorage::MRImageStorage)
  {
    mdcmWarningMacro("SplitMosaicFilter: expected MR Image Storage");
  }
#endif
  DataElement    de(Tag(0x0008, 0x0016));
  const char *   msstr = MediaStorage::GetMSString(ms);
  const VL::Type strlenMsstr = static_cast<VL::Type>(strlen(msstr));
  de.SetByteValue(msstr, strlenMsstr);
  de.SetVR(Attribute<0x0008, 0x0016>::GetVR());
  ds.Replace(de);
  return true;
}

}
