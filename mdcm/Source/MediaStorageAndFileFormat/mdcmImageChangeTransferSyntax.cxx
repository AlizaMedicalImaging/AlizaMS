/*********************************************************
 *
 * MDCM
 *
 * Modifications github.com/issakomi
 *
 *********************************************************/

/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "mdcmImageChangeTransferSyntax.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmSequenceOfItems.h"
#include "mdcmFragment.h"
#include "mdcmPixmap.h"
#include "mdcmBitmap.h"
#include "mdcmRAWCodec.h"
#include "mdcmJPEGCodec.h"
#include "mdcmJPEGLSCodec.h"
#include "mdcmJPEG2000Codec.h"
#include "mdcmRLECodec.h"

namespace mdcm
{

void
ImageChangeTransferSyntax::SetTransferSyntax(const TransferSyntax & ts)
{
  TS = ts;
}

const TransferSyntax &
ImageChangeTransferSyntax::GetTransferSyntax() const
{
  return TS;
}

void
ImageChangeTransferSyntax::SetCompressIconImage(bool b)
{
  CompressIconImage = b;
}

// When target Transfer Syntax is identical to input target syntax, no
// operation is actually done.
// This is an issue when someone wants to re-compress using MDCM internal
// implementation a JPEG (for example) image
void
ImageChangeTransferSyntax::SetForce(bool f)
{
  Force = f;
}

// Allow user to specify exactly which codec to use. this is needed to
// specify special qualities or compression option.
// warning: if the codec 'ic' is not compatible with the TransferSyntax
// requested, it will not be used. It is the user responsibility to check
// that UserCodec->CanCode( TransferSyntax )
void
ImageChangeTransferSyntax::SetUserCodec(ImageCodec * ic)
{
  UserCodec = ic;
}

void
ImageChangeTransferSyntax::SetForceYBRFull(bool t)
{
  ForceYBRFull = t;
}

bool
ImageChangeTransferSyntax::Change()
{
  if (TS == TransferSyntax::TS_END)
  {
    if (!Force)
      return false;
    if (Input->GetTransferSyntax().IsEncapsulated() && (Input->GetTransferSyntax() != TransferSyntax::RLELossless))
    {
      Output = Input;
      return true;
    }
    return false;
  }
  if ((Input->GetPhotometricInterpretation() == PhotometricInterpretation::PALETTE_COLOR) && TS.IsLossy())
  {
    mdcmErrorMacro("PALETTE_COLOR and Lossy compression are impossible. Convert to RGB first.");
    return false;
  }
  Output = Input;
  if (Input->GetTransferSyntax() == TS && !Force)
    return true;
  if ((Input->GetTransferSyntax() != TransferSyntax::ImplicitVRLittleEndian &&
       Input->GetTransferSyntax() != TransferSyntax::ExplicitVRLittleEndian &&
       Input->GetTransferSyntax() != TransferSyntax::ExplicitVRBigEndian) ||
      ((Input->GetTransferSyntax() == TransferSyntax::ImplicitVRLittleEndian ||
        Input->GetTransferSyntax() == TransferSyntax::ExplicitVRLittleEndian ||
        Input->GetTransferSyntax() == TransferSyntax::ExplicitVRBigEndian) &&
       Input->GetPhotometricInterpretation() == PhotometricInterpretation::YBR_FULL_422) ||
      Force)
  {
    DataElement    pixeldata(Tag(0x7fe0, 0x0010));
    ByteValue *    bv0 = new ByteValue();
    const uint32_t len0 = (uint32_t)Input->GetBufferLength();
    bv0->SetLength(len0);
    const bool b = Input->GetBuffer((char *)bv0->GetPointer());
    if (!b)
    {
      mdcmErrorMacro("Error in getting buffer from input image.");
      return false;
    }
    pixeldata.SetValue(*bv0);
    bool success = false;
    if (!success)
      success = TryRAWCodec(pixeldata, *Input, *Output);
    if (!success)
      success = TryJPEGCodec(pixeldata, *Input, *Output);
    if (!success)
      success = TryJPEGLSCodec(pixeldata, *Input, *Output);
    if (!success)
      success = TryJPEG2000Codec(pixeldata, *Input, *Output);
    if (!success)
      success = TryRLECodec(pixeldata, *Input, *Output);
    Output->SetTransferSyntax(TS);
    if (!success)
    {
      return false;
    }
    DataElement iconpixeldata(Tag(0x7fe0, 0x0010));
    Bitmap &    bitmap = *Input;
    if (Pixmap * pixmap = dynamic_cast<Pixmap *>(&bitmap))
    {
      Bitmap & outbitmap = *Output;
      Pixmap * outpixmap = dynamic_cast<Pixmap *>(&outbitmap);
      if (!outpixmap)
        return false;
      if (!pixmap->GetIconImage().IsEmpty())
      {
        ByteValue *    bv = new ByteValue();
        const uint32_t len = (uint32_t)pixmap->GetIconImage().GetBufferLength();
        bv->SetLength(len);
        const bool bb = pixmap->GetIconImage().GetBuffer((char *)bv->GetPointer());
        if (!bb)
        {
          return false;
        }
        iconpixeldata.SetValue(*bv);
        success = false;
        if (!success)
          success = TryRAWCodec(iconpixeldata, pixmap->GetIconImage(), outpixmap->GetIconImage());
        if (!success)
          success = TryJPEGCodec(iconpixeldata, pixmap->GetIconImage(), outpixmap->GetIconImage());
        if (!success)
          success = TryJPEGLSCodec(iconpixeldata, pixmap->GetIconImage(), outpixmap->GetIconImage());
        if (!success)
          success = TryJPEG2000Codec(iconpixeldata, pixmap->GetIconImage(), outpixmap->GetIconImage());
        if (!success)
          success = TryRLECodec(iconpixeldata, pixmap->GetIconImage(), outpixmap->GetIconImage());
        outpixmap->GetIconImage().SetTransferSyntax(TS);
        if (!success)
        {
          return false;
        }
        assert(outpixmap->GetIconImage().GetTransferSyntax() == TS);
      }
    }
    assert(Output->GetTransferSyntax() == TS);
    return success;
  }
  bool success = false;
  if (!success)
    success = TryRAWCodec(Input->GetDataElement(), *Input, *Output);
  if (!success)
    success = TryJPEGCodec(Input->GetDataElement(), *Input, *Output);
  if (!success)
    success = TryJPEG2000Codec(Input->GetDataElement(), *Input, *Output);
  if (!success)
    success = TryJPEGLSCodec(Input->GetDataElement(), *Input, *Output);
  if (!success)
    success = TryRLECodec(Input->GetDataElement(), *Input, *Output);
  Output->SetTransferSyntax(TS);
  if (!success)
  {
    return false;
  }
  Bitmap & bitmap = *Input;
  if (Pixmap * pixmap = dynamic_cast<Pixmap *>(&bitmap))
  {
    if (!pixmap->GetIconImage().IsEmpty() && CompressIconImage)
    {
      Bitmap & outbitmap = *Output;
      Pixmap * outpixmap = dynamic_cast<Pixmap *>(&outbitmap);
      if (!outpixmap)
        return false;
      success = false;
      if (!success)
        success =
          TryRAWCodec(pixmap->GetIconImage().GetDataElement(), pixmap->GetIconImage(), outpixmap->GetIconImage());
      if (!success)
        success =
          TryJPEGCodec(pixmap->GetIconImage().GetDataElement(), pixmap->GetIconImage(), outpixmap->GetIconImage());
      if (!success)
        success =
          TryJPEGLSCodec(pixmap->GetIconImage().GetDataElement(), pixmap->GetIconImage(), outpixmap->GetIconImage());
      if (!success)
        success =
          TryJPEG2000Codec(pixmap->GetIconImage().GetDataElement(), pixmap->GetIconImage(), outpixmap->GetIconImage());
      if (!success)
        success =
          TryRLECodec(pixmap->GetIconImage().GetDataElement(), pixmap->GetIconImage(), outpixmap->GetIconImage());
      outpixmap->GetIconImage().SetTransferSyntax(TS);
      if (!success)
        return false;
      assert(outpixmap->GetIconImage().GetTransferSyntax() == TS);
    }
  }
  assert(Output->GetTransferSyntax() == TS);
  return success;
}

bool
ImageChangeTransferSyntax::TryRAWCodec(const DataElement & pixelde, Bitmap const & input, Bitmap & output)
{
  const unsigned long long len = input.GetBufferLength();
  (void)len;
  const TransferSyntax & ts = GetTransferSyntax();
  RAWCodec               codec;
  if (codec.CanCode(ts))
  {
    codec.SetDimensions(input.GetDimensions());
    codec.SetPlanarConfiguration(input.GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(input.GetPhotometricInterpretation());
    codec.SetPixelFormat(input.GetPixelFormat());
    codec.SetNeedOverlayCleanup(input.AreOverlaysInPixelData() || input.UnusedBitsPresentInPixelData());
    DataElement out;
    if (!codec.Code(pixelde, out))
      return false;
    DataElement & de = output.GetDataElement();
    de.SetValue(out.GetValue());
    return true;
  }
  return false;
}

bool
ImageChangeTransferSyntax::TryRLECodec(const DataElement & pixelde, Bitmap const & input, Bitmap & output)
{
  const unsigned long long len = input.GetBufferLength();
  (void)len;
  const TransferSyntax & ts = GetTransferSyntax();
  RLECodec               codec;
  if (codec.CanCode(ts))
  {
    codec.SetDimensions(input.GetDimensions());
    codec.SetPlanarConfiguration(input.GetPlanarConfiguration());
    codec.SetPhotometricInterpretation(input.GetPhotometricInterpretation());
    codec.SetPixelFormat(input.GetPixelFormat());
    codec.SetNeedOverlayCleanup(input.AreOverlaysInPixelData() || input.UnusedBitsPresentInPixelData());
    DataElement out;
    if (!codec.Code(pixelde, out))
      return false;
    DataElement & de = output.GetDataElement();
    de.SetValue(out.GetValue());
    if (input.GetPixelFormat().GetSamplesPerPixel() == 3)
    {
      if (ForceYBRFull || (input.GetPhotometricInterpretation() == PhotometricInterpretation::YBR_FULL))
      {
        output.SetPhotometricInterpretation(PhotometricInterpretation::YBR_FULL);
      }
    }
    return true;
  }
  return false;
}

bool
ImageChangeTransferSyntax::TryJPEGCodec(const DataElement & pixelde, Bitmap const & input, Bitmap & output)
{
  const unsigned long long len = input.GetBufferLength();
  (void)len;
  const TransferSyntax & ts = GetTransferSyntax();
  JPEGCodec              jpgcodec;
  if (ts.IsLossy())
  {
    jpgcodec.SetLossless(false);
  }
  ImageCodec * codec = &jpgcodec;
  JPEGCodec *  usercodec = dynamic_cast<JPEGCodec *>(UserCodec);
  if (usercodec && usercodec->CanCode(ts))
  {
    codec = usercodec;
  }
  if (codec->CanCode(ts))
  {
    codec->SetDimensions(input.GetDimensions());
    codec->SetPlanarConfiguration(input.GetPlanarConfiguration());
    codec->SetPhotometricInterpretation(input.GetPhotometricInterpretation());
    codec->SetPixelFormat(input.GetPixelFormat());
    codec->SetNeedOverlayCleanup(input.AreOverlaysInPixelData() || input.UnusedBitsPresentInPixelData());
    if (!input.GetPixelFormat().IsCompatible(ts))
    {
      mdcmAlwaysWarnMacro("Pixel Format incompatible with TS");
      return false;
    }
    DataElement out;
    if (!codec->Code(pixelde, out))
    {
      mdcmAlwaysWarnMacro("!codec->Code(pixelde, out)");
      return false;
    }
    output.SetPlanarConfiguration(0);
    DataElement & de = output.GetDataElement();
    de.SetValue(out.GetValue());
    if (input.GetPixelFormat().GetSamplesPerPixel() == 3)
    {
      if (ts.IsLossy())
      {
        output.SetPhotometricInterpretation(PhotometricInterpretation::YBR_FULL_422);
      }
      else if (ForceYBRFull)
      {
        output.SetPhotometricInterpretation(PhotometricInterpretation::YBR_FULL);
      }
      else
      {
        output.SetPhotometricInterpretation(PhotometricInterpretation::RGB);
      }
    }
    return true;
  }
  return false;
}

bool
ImageChangeTransferSyntax::TryJPEGLSCodec(const DataElement & pixelde, Bitmap const & input, Bitmap & output)
{
  const unsigned long long len = input.GetBufferLength();
  (void)len;
  const TransferSyntax & ts = GetTransferSyntax();
  JPEGLSCodec            jlscodec;
  ImageCodec *           codec = &jlscodec;
  JPEGLSCodec *          usercodec = dynamic_cast<JPEGLSCodec *>(UserCodec);
  if (usercodec && usercodec->CanCode(ts))
  {
    codec = usercodec;
  }
  if (codec->CanCode(ts))
  {
    codec->SetDimensions(input.GetDimensions());
    codec->SetPixelFormat(input.GetPixelFormat());
    codec->SetPlanarConfiguration(input.GetPlanarConfiguration());
    codec->SetPhotometricInterpretation(input.GetPhotometricInterpretation());
    codec->SetNeedOverlayCleanup(input.AreOverlaysInPixelData() || input.UnusedBitsPresentInPixelData());
    DataElement out;
    bool        r;
    if (input.AreOverlaysInPixelData() || input.UnusedBitsPresentInPixelData())
    {
      const ByteValue * bv = pixelde.GetByteValue();
      assert(bv);
      mdcm::DataElement tmp;
      tmp.SetByteValue(bv->GetPointer(), bv->GetLength());
      bv = tmp.GetByteValue();
      r = codec->CleanupUnusedBits((char *)bv->GetPointer(), bv->GetLength());
      if (!r)
        return false;
      r = codec->Code(tmp, out);
    }
    else
    {
      r = codec->Code(pixelde, out);
    }
    if (!r)
      return false;
    output.SetPlanarConfiguration(0);
    DataElement & de = output.GetDataElement();
    de.SetValue(out.GetValue());
    if (ForceYBRFull || (input.GetPhotometricInterpretation() == PhotometricInterpretation::YBR_FULL))
    {
      output.SetPhotometricInterpretation(PhotometricInterpretation::YBR_FULL);
    }
    return r;
  }
  return false;
}

bool
ImageChangeTransferSyntax::TryJPEG2000Codec(const DataElement & pixelde, Bitmap const & input, Bitmap & output)
{
  const unsigned long long len = input.GetBufferLength();
  (void)len;
  const TransferSyntax & ts = GetTransferSyntax();
  JPEG2000Codec          j2kcodec;
  ImageCodec *           codec = &j2kcodec;
  JPEG2000Codec *        usercodec = dynamic_cast<JPEG2000Codec *>(UserCodec);
  if (usercodec && usercodec->CanCode(ts))
  {
    codec = usercodec;
  }
  if (codec->CanCode(ts))
  {
    codec->SetDimensions(input.GetDimensions());
    codec->SetPixelFormat(input.GetPixelFormat());
    codec->SetNumberOfDimensions(input.GetNumberOfDimensions());
    codec->SetPlanarConfiguration(input.GetPlanarConfiguration());
    codec->SetPhotometricInterpretation(input.GetPhotometricInterpretation());
    codec->SetNeedOverlayCleanup(input.AreOverlaysInPixelData() || input.UnusedBitsPresentInPixelData());
    DataElement out;
    const bool  r = codec->Code(pixelde, out);
    output.SetPlanarConfiguration(0);
    if (input.GetPixelFormat().GetSamplesPerPixel() == 3)
    {
      if (ForceYBRFull || input.GetPhotometricInterpretation() == PhotometricInterpretation::YBR_FULL)
      {
        output.SetPhotometricInterpretation(PhotometricInterpretation::YBR_FULL); // TODO
      }
      else if (input.GetPhotometricInterpretation() == PhotometricInterpretation::RGB ||
               input.GetPhotometricInterpretation() == PhotometricInterpretation::YBR_ICT ||
               input.GetPhotometricInterpretation() == PhotometricInterpretation::YBR_RCT)
      {
        if (ts == TransferSyntax::JPEG2000Lossless)
        {
          // output.SetPhotometricInterpretation(PhotometricInterpretation::YBR_RCT); // TODO
          output.SetPhotometricInterpretation(PhotometricInterpretation::RGB);
        }
        else
        {
          assert(ts == TransferSyntax::JPEG2000);
          // output.SetPhotometricInterpretation(PhotometricInterpretation::YBR_ICT); // TODO
          output.SetPhotometricInterpretation(PhotometricInterpretation::RGB);
        }
      }
      // invalid
      else if (input.GetPhotometricInterpretation() == PhotometricInterpretation::YBR_FULL_422 ||
               input.GetPhotometricInterpretation() == PhotometricInterpretation::YBR_PARTIAL_422 || // retired
               input.GetPhotometricInterpretation() == PhotometricInterpretation::YBR_PARTIAL_420)   // not supported
      {
        output.SetPhotometricInterpretation(PhotometricInterpretation::YBR_FULL);
      }
      else
      {
        output.SetPhotometricInterpretation(PhotometricInterpretation::RGB);
      }
    }
    else
    {
      assert(input.GetPixelFormat().GetSamplesPerPixel() == 1);
    }
    if (!r)
      return false;
    DataElement & de = output.GetDataElement();
    de.SetValue(out.GetValue());
    return r;
  }
  return false;
}

} // end namespace mdcm
