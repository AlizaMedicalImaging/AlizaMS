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

#define JPEGBITS_PRINT_COLORSPACES 0

#include "mdcmTrace.h"
#include "mdcmTransferSyntax.h"
#include "mdcmImageHelper.h"
#include <csetjmp>

#if 0
#ifndef _WIN32
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#endif

namespace mdcm
{

typedef struct
{
  struct jpeg_source_mgr pub;           /* public fields */
  std::istream *         infile;        /* source stream */
  JOCTET *               buffer;        /* start of buffer */
  boolean                start_of_file; /* have we gotten any data yet? */
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;

#define INPUT_BUF_SIZE 4096

// cppcheck-suppress unknownMacro
METHODDEF(void) init_source(j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr)cinfo->src;
  src->start_of_file = TRUE;
}

// cppcheck-suppress unknownMacro
METHODDEF(boolean) fill_input_buffer(j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr)cinfo->src;
  size_t     nbytes = 0;
  std::streampos pos = src->infile->tellg();
  std::streampos end = src->infile->seekg(0, std::ios::end).tellg();
  src->infile->seekg(pos, std::ios::beg);
  if (end == pos)
  {
#if 0
    std::cout << "fill_input_buffer: return FALSE" << std::endl;
#endif
    // S. comment in 'skip_input_data'
    return FALSE;
  }
  if ((end - pos) < INPUT_BUF_SIZE)
  {
    src->infile->read((char *)src->buffer, (size_t)(end - pos));
  }
  else
  {
    src->infile->read((char *)src->buffer, INPUT_BUF_SIZE);
  }
  std::streamsize gcount = src->infile->gcount();
  if (gcount <= 0)
  {
#if 0
    std::cout << "fill_input_buffer: gcount <= 0" << std::endl;
#endif
    if (src->start_of_file)
    {
      ERREXIT(cinfo, JERR_INPUT_EMPTY);
    }
    WARNMS(cinfo, JWRN_JPEG_EOF);
    src->buffer[0] = (JOCTET)0xFF;
    src->buffer[1] = (JOCTET)JPEG_EOI;
    nbytes = 2;
  }
  else
  {
    nbytes = (size_t)gcount;
  }
  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = nbytes;
  src->start_of_file = FALSE;
  return TRUE;
}

// cppcheck-suppress unknownMacro
METHODDEF(void) skip_input_data(j_decompress_ptr cinfo, IJG_LONG num_bytes)
{
  my_src_ptr src = (my_src_ptr)cinfo->src;
  if (num_bytes > 0)
  {
    while (num_bytes > (IJG_LONG)src->pub.bytes_in_buffer)
    {
      num_bytes -= (IJG_LONG)src->pub.bytes_in_buffer;
      // The comment in GDCM states that 'fill_input_buffer'
      // is supposed to "never return FALSE".
      // But it does, and everything seems to work fine.
      //
      // Example files from 'gdcmData'
      // https://sourceforge.net/p/gdcm/gdcmdata/ci/master/tree/
      //
      // gdcm-JPEG-LossLessThoravision.dcm
      // D_CLUNIE_CT1_JPLL.dcm
      // D_CLUNIE_MR1_JPLY.dcm
      // D_CLUNIE_RG1_JPLL.dcm
      // D_CLUNIE_RG2_JPLY.dcm
      // D_CLUNIE_RG3_JPLY.dcm
      // D_CLUNIE_SC1_JPLY.dcm
      // GE_RHAPSODE-16-MONO2-JPEG-Fragments.dcm
      // and probaly more.
      //
      // The files seem to have a single slice encoded in
      // multiple fragments.
      (void)fill_input_buffer(cinfo);
    }
    src->pub.next_input_byte += (size_t)num_bytes;
    src->pub.bytes_in_buffer -= (size_t)num_bytes;
  }
}

// cppcheck-suppress unknownMacro
METHODDEF(void) term_source(j_decompress_ptr) {}

// cppcheck-suppress unknownMacro
GLOBAL(void) jpeg_stdio_src(j_decompress_ptr cinfo, std::istream & infile, bool flag)
{
  my_src_ptr src;
  if (cinfo->src == nullptr)
  {
    cinfo->src =
      (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, SIZEOF(my_source_mgr));
    src = (my_src_ptr)cinfo->src;
    src->buffer =
      (JOCTET *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, INPUT_BUF_SIZE * SIZEOF(JOCTET));
  }
  src = (my_src_ptr)cinfo->src;
  src->pub.init_source = init_source;
  src->pub.fill_input_buffer = fill_input_buffer;
  src->pub.skip_input_data = skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart;
  src->pub.term_source = term_source;
  src->infile = &infile;
  if (flag)
  {
    src->pub.bytes_in_buffer = 0;
    src->pub.next_input_byte = nullptr;
  }
}

struct my_error_mgr
{
  struct jpeg_error_mgr pub;
  jmp_buf               setjmp_buffer;
};

extern "C"
{
  METHODDEF(void) my_error_exit(j_common_ptr cinfo)
  {
    my_error_mgr * myerr = (my_error_mgr *)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
  }
}

typedef struct
{
  struct jpeg_destination_mgr pub;
  std::ostream *              outfile;
  JOCTET *                    buffer;
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;

#define OUTPUT_BUF_SIZE 4096

METHODDEF(void) init_destination(j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr)cinfo->dest;
  dest->buffer =
    (JOCTET *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_IMAGE, OUTPUT_BUF_SIZE * SIZEOF(JOCTET));
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

METHODDEF(boolean) empty_output_buffer(j_compress_ptr cinfo)
{
  my_dest_ptr  dest = (my_dest_ptr)cinfo->dest;
  const size_t output_buf_size = OUTPUT_BUF_SIZE;
  if (!dest->outfile->write((char *)dest->buffer, output_buf_size))
  {
    ERREXIT(cinfo, JERR_FILE_WRITE);
  }
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
  return TRUE;
}

METHODDEF(void) term_destination(j_compress_ptr cinfo)
{
  my_dest_ptr  dest = (my_dest_ptr)cinfo->dest;
  const size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;
  if (datacount > 0)
  {
    if (!dest->outfile->write((char *)dest->buffer, datacount))
    {
      ERREXIT(cinfo, JERR_FILE_WRITE);
    }
  }
  dest->outfile->flush();
  if (dest->outfile->fail())
    ERREXIT(cinfo, JERR_FILE_WRITE);
}

GLOBAL(void) jpeg_stdio_dest(j_compress_ptr cinfo, std::ostream * outfile)
{
  my_dest_ptr dest;
  if (cinfo->dest == nullptr)
  {
    cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)(
      (j_common_ptr)cinfo, JPOOL_PERMANENT, SIZEOF(my_destination_mgr));
  }
  dest = (my_dest_ptr)cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->outfile = outfile;
}

class JPEGInternals
{
public:
  JPEGInternals()
    : cinfo()
    , cinfo_comp()
    , jerr()
    , StateSuspension(0)
    , SampBuffer(nullptr)
  {}
  jpeg_decompress_struct cinfo;
  jpeg_compress_struct   cinfo_comp;
  my_error_mgr           jerr;
  int                    StateSuspension;
  void *                 SampBuffer;
};

JPEGBITSCodec::JPEGBITSCodec()
{
  Internals = new JPEGInternals;
  BitSample = BITS_IN_JSAMPLE;
}

JPEGBITSCodec::~JPEGBITSCodec()
{
  delete Internals;
}

bool
JPEGBITSCodec::GetHeaderInfo(std::istream & is)
{
  TransferSyntax ts;
  const bool r = GetHeaderInfoAndTS(is, ts);
  (void) ts;
  return r;
}

bool
JPEGBITSCodec::GetHeaderInfoAndTS(std::istream & is, TransferSyntax & ts)
{
  jpeg_decompress_struct & cinfo = Internals->cinfo;
  my_error_mgr & jerr = Internals->jerr;
  if (Internals->StateSuspension == 0)
  {
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer))
    {
      if (jerr.pub.msg_code == JERR_BAD_PRECISION)
      {
        this->BitSample = jerr.pub.msg_parm.i[0];
        assert(this->BitSample == 1 || this->BitSample == 8 || this->BitSample == 12 || this->BitSample == 16);
        assert(this->BitSample == cinfo.data_precision);
      }
      jpeg_destroy_decompress(&cinfo);
      return false;
    }
  }
  if (Internals->StateSuspension == 0)
  {
    jpeg_create_decompress(&cinfo);
    int workaround = 0;
    if (ImageHelper::GetWorkaroundPredictorBug())
      workaround |= WORKAROUND_PREDICTOR6OVERFLOW;
    if (ImageHelper::GetWorkaroundCornellBug())
      workaround |= WORKAROUND_BUGGY_CORNELL_16BIT_JPEG_ENCODER;
    if (workaround != 0)
      cinfo.workaround_options = workaround;
    jpeg_stdio_src(&cinfo, is, true);
  }
  else
  {
    jpeg_stdio_src(&cinfo, is, false);
  }
  if (Internals->StateSuspension < 2)
  {
    if (jpeg_read_header(&cinfo, TRUE) == JPEG_SUSPENDED)
    {
      Internals->StateSuspension = 2;
    }
    if (jerr.pub.num_warnings)
    {
      if (jerr.pub.msg_code == 128)
      {
        this->BitSample = jerr.pub.msg_parm.i[0];
        jpeg_destroy_decompress(&cinfo);
        return false;
      }
      else
      {
        assert(0);
      }
    }
    this->Dimensions[1] = cinfo.image_height;
    this->Dimensions[0] = cinfo.image_width;
    const int prep = this->PF.GetPixelRepresentation();
    const int precision = cinfo.data_precision;
    if (precision == 1)
    {
      this->PF = PixelFormat(PixelFormat::SINGLEBIT);
    }
    else if (precision <= 8)
    {
      this->PF = PixelFormat(PixelFormat::UINT8);
    }
    else if (precision <= 16)
    {
      this->PF = PixelFormat(PixelFormat::UINT16);
    }
    else
    {
      mdcmAlwaysWarnMacro("Unsupported precision = " << precision);
      assert(0);
      return false;
    }
    this->PF.SetPixelRepresentation(static_cast<uint16_t>(prep));
    this->PF.SetBitsStored(static_cast<uint16_t>(precision));
    this->PlanarConfiguration = 0;
    // Setting PhotometricInterpretation here seems to be useless,
    // it will overridden later?
    if (cinfo.jpeg_color_space == JCS_UNKNOWN)
    {
      if (cinfo.num_components == 1)
      {
        PI = PhotometricInterpretation::MONOCHROME2;
        this->PF.SetSamplesPerPixel(1);
      }
      else if (cinfo.num_components == 3)
      {
        PI = PhotometricInterpretation::RGB;
        this->PF.SetSamplesPerPixel(3);
      }
      else if (cinfo.num_components == 4)
      {
        PI = PhotometricInterpretation::CMYK;
        this->PF.SetSamplesPerPixel(4);
        mdcmAlwaysWarnMacro("Not supported: JCS_UNKNOWN and cinfo.num_components = 4, trying CMYK");
      }
      else
      {
        mdcmAlwaysWarnMacro("Error: JCS_UNKNOWN and cinfo.num_components = " << cinfo.num_components);
        assert(0);
        return false;
      }
    }
    else if (cinfo.jpeg_color_space == JCS_GRAYSCALE)
    {
      assert(cinfo.num_components == 1);
      PI = PhotometricInterpretation::MONOCHROME2;
      this->PF.SetSamplesPerPixel(1);
    }
    else if (cinfo.jpeg_color_space == JCS_RGB)
    {
      assert(cinfo.num_components == 3);
      PI = PhotometricInterpretation::RGB;
      this->PF.SetSamplesPerPixel(3);
    }
    else if (cinfo.jpeg_color_space == JCS_YCbCr)
    {
      assert(cinfo.num_components == 3);
      // YBR_PARTIAL_422 is theoretically possible too,
      // but retired in 2017b and extremely rare.
      PI = PhotometricInterpretation::YBR_FULL_422;
      if (cinfo.process == JPROC_LOSSLESS)
      {
        mdcmAlwaysWarnMacro("JPROC_LOSSLESS and JCS_YCbCr");
      }
      this->PF.SetSamplesPerPixel(3);
    }
    else if (cinfo.jpeg_color_space == JCS_CMYK || cinfo.jpeg_color_space == JCS_YCCK)
    {
      assert(cinfo.num_components == 4);
      PI = PhotometricInterpretation::CMYK;
      this->PF.SetSamplesPerPixel(4);
      mdcmAlwaysWarnMacro("JCS_CMYK is not intended to be in DICOM JPEG file");
    }
    else
    {
      mdcmAlwaysWarnMacro("Error, unknown cinfo.jpeg_color_space");
      assert(0);
    }
  }
  if (cinfo.process == JPROC_LOSSLESS)
  {
    LossyFlag = false;
    const int predictor = cinfo.Ss;
    switch (predictor)
    {
      case 1:
        ts = TransferSyntax::JPEGLosslessProcess14_1;
        break;
      default:
        ts = TransferSyntax::JPEGLosslessProcess14;
        break;
    }
  }
  else if (cinfo.process == JPROC_SEQUENTIAL)
  {
    LossyFlag = true;
    if (this->BitSample == 8)
    {
      ts = TransferSyntax::JPEGBaselineProcess1;
    }
    else if (this->BitSample == 12)
    {
      ts = TransferSyntax::JPEGExtendedProcess2_4;
    }
    else
    {
      assert(0);
      return false;
    }
  }
  else if (cinfo.process == JPROC_PROGRESSIVE)
  {
    LossyFlag = true;
    if (this->BitSample == 8)
    {
      ts = TransferSyntax::JPEGFullProgressionProcess10_12;
    }
    else if (this->BitSample == 12)
    {
      ts = TransferSyntax::JPEGFullProgressionProcess10_12;
    }
    else
    {
      assert(0);
      return false;
    }
  }
  else
  {
    LossyFlag = true;
    assert(0);
    return false;
  }

  if (cinfo.density_unit != 0 || cinfo.X_density != 1 || cinfo.Y_density != 1)
  {
    mdcmWarningMacro("Pixel Density from JFIF Marker is not supported (for now)");
  }
  jpeg_destroy_decompress(&cinfo);
  Internals->StateSuspension = 0;
  return true;
}

bool
JPEGBITSCodec::DecodeByStreams(std::istream & is, std::ostream & os)
{
  jpeg_decompress_struct & cinfo = Internals->cinfo;
  my_error_mgr &      jerr = Internals->jerr;
  volatile JSAMPARRAY buffer{};
  volatile size_t     row_stride{};
  if (Internals->StateSuspension == 0)
  {
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer))
    {
      if (jerr.pub.msg_code == JERR_BAD_PRECISION)
      {
        this->BitSample = jerr.pub.msg_parm.i[0];
      }
      jpeg_destroy_decompress(&cinfo);
      return false;
    }
  }
  if (Internals->StateSuspension == 0)
  {
    jpeg_create_decompress(&cinfo);
    volatile int workaround = 0;
    if (ImageHelper::GetWorkaroundPredictorBug())
      workaround = workaround | WORKAROUND_PREDICTOR6OVERFLOW;
    if (ImageHelper::GetWorkaroundCornellBug())
      workaround = workaround | WORKAROUND_BUGGY_CORNELL_16BIT_JPEG_ENCODER;
    if (workaround != 0)
      cinfo.workaround_options = workaround;
    jpeg_stdio_src(&cinfo, is, true);
  }
  else
  {
    jpeg_stdio_src(&cinfo, is, false);
  }
  if (Internals->StateSuspension < 2)
  {
    if (jpeg_read_header(&cinfo, TRUE) == JPEG_SUSPENDED)
    {
      Internals->StateSuspension = 2;
    }
    if (jerr.pub.num_warnings > 0)
    {
      if (jerr.pub.msg_code == JWRN_MUST_DOWNSCALE)
      {
        // PHILIPS_Gyroscan-12-Jpeg_Extended_Process_2_4.dcm
        // PHILIPS_Gyroscan-12-MONO2-Jpeg_Lossless.dcm
        // MARCONI_MxTWin-12-MONO2-JpegLossless-ZeroLengthSQ.dcm
        // LJPEG_BuginMDCM12.dcm
        mdcmAlwaysWarnMacro("jerr.pub.msg_code: JWRN_MUST_DOWNSCALE\n"
                            "    cinfo.data_precision="
                            << cinfo.data_precision
                            << "\n"
                               "    this->BitSample="
                            << this->BitSample);
        this->BitSample = jerr.pub.msg_parm.i[0];
        assert(cinfo.data_precision == this->BitSample);
        jpeg_destroy_decompress(&cinfo);
        return false;
      }
      mdcmAlwaysWarnMacro("jerr.pub.num_warnings = " << jerr.pub.num_warnings);
    }
    const volatile unsigned int * dims = this->GetDimensions();
    if (cinfo.image_width != dims[0] || cinfo.image_height != dims[1])
    {
      mdcmAlwaysWarnMacro("JPEG is " << cinfo.image_width << "x" << cinfo.image_height << ", DICOM " << dims[0] << "x"
                                     << dims[1]);
    }
#if (defined JPEGBITS_PRINT_COLORSPACES && JPEGBITS_PRINT_COLORSPACES == 1)
    std::cout << "cinfo.jpeg_color_space = ";
    switch (cinfo.jpeg_color_space)
    {
      case JCS_GRAYSCALE:
        std::cout << "JCS_GRAYSCALE" << std::endl;
        break;
      case JCS_RGB:
        std::cout << "JCS_RGB" << std::endl;
        break;
      case JCS_YCbCr:
        std::cout << "JCS_YCbCr" << std::endl;
        break;
      case JCS_CMYK:
        std::cout << "JCS_CMYK" << std::endl;
        break;
      case JCS_YCCK:
        std::cout << "JCS_YCCK" << std::endl;
        break;
      case JCS_UNKNOWN:
        std::cout << "JCS_UNKNOWN" << std::endl;
        break;
      default:
        std::cout << "?" << std::endl;
        break;
    }
    std::cout << "cinfo.out_color_space  = ";
    switch (cinfo.out_color_space)
    {
      case JCS_GRAYSCALE:
        std::cout << "JCS_GRAYSCALE" << std::endl;
        break;
      case JCS_RGB:
        std::cout << "JCS_RGB" << std::endl;
        break;
      case JCS_YCbCr:
        std::cout << "JCS_YCbCr" << std::endl;
        break;
      case JCS_CMYK:
        std::cout << "JCS_CMYK" << std::endl;
        break;
      case JCS_YCCK:
        std::cout << "JCS_YCCK" << std::endl;
        break;
      case JCS_UNKNOWN:
        std::cout << "JCS_UNKNOWN" << std::endl;
        break;
      default:
        std::cout << "?" << std::endl;
        break;
    }
#endif
    switch (cinfo.jpeg_color_space)
    {
      case JCS_GRAYSCALE:
        if ((GetPhotometricInterpretation() != PhotometricInterpretation::MONOCHROME1) &&
            (GetPhotometricInterpretation() != PhotometricInterpretation::MONOCHROME2))
        {
          mdcmAlwaysWarnMacro("JPEG: PhotometricInterpretation " << GetPhotometricInterpretation()
                                                                 << ", but jpeg_color_space is "
                                                                 << (int)cinfo.jpeg_color_space);
          this->PI = PhotometricInterpretation::MONOCHROME2;
        }
        break;
      case JCS_YCbCr:
        if (ImageHelper::GetJpegPreserveYBRfull() &&
            (
             (GetPhotometricInterpretation() == PhotometricInterpretation::YBR_FULL_422) ||
             (GetPhotometricInterpretation() == PhotometricInterpretation::YBR_FULL) ||
#if 1
             (GetPhotometricInterpretation() == PhotometricInterpretation::YBR_PARTIAL_422)
#endif
            ))
        {
          cinfo.jpeg_color_space = JCS_UNKNOWN;
          cinfo.out_color_space = JCS_UNKNOWN;
        }
        // Probably should not happed, but lossy JPEG with photo-metric "RGB" and
        // cinfo.jpeg_color_space JCS_YCbCr exist. Required to open correctly.
        if (GetPhotometricInterpretation() == PhotometricInterpretation::RGB)
        {
          cinfo.jpeg_color_space = JCS_UNKNOWN;
          cinfo.out_color_space = JCS_UNKNOWN;
        }
        break;
      default:
        break;
    }
  }
  if (Internals->StateSuspension < 3)
  {
    if (jpeg_start_decompress(&cinfo) == FALSE)
    {
      Internals->StateSuspension = 3;
    }
    row_stride = cinfo.output_width * cinfo.output_components;
    row_stride = row_stride * sizeof(JSAMPLE);
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, (JDIMENSION)row_stride, 1);
    Internals->SampBuffer = buffer;
  }
  else
  {
    row_stride = cinfo.output_width * cinfo.output_components;
    row_stride = row_stride * sizeof(JSAMPLE);
    buffer = (JSAMPARRAY)Internals->SampBuffer;
  }
  while (cinfo.output_scanline < cinfo.output_height)
  {
    if (jpeg_read_scanlines(&cinfo, buffer, 1) == 0)
    {
      Internals->StateSuspension = 3;
      return true;
    }
    os.write(reinterpret_cast<char *>(buffer[0]), row_stride);
  }
  if (jpeg_finish_decompress(&cinfo) == FALSE)
  {
    Internals->StateSuspension = 4;
    return true;
  }
  if (cinfo.process == JPROC_LOSSLESS)
  {
    LossyFlag = false;
  }
  else
  {
    LossyFlag = true;
  }
  jpeg_destroy_decompress(&cinfo);
  if (jerr.pub.num_warnings > 0)
  {
    mdcmAlwaysWarnMacro("jerr.pub.num_warnings = " << jerr.pub.num_warnings);
  }
  Internals->StateSuspension = 0;
  return true;
}

bool
JPEGBITSCodec::InternalCode(const char * input, size_t len, std::ostream & os)
{
  void * vinput = const_cast<void*>(static_cast<const void*>(input));
  JSAMPLE *                     image_buffer = static_cast<JSAMPLE *>(vinput);
  const volatile unsigned int * dims = this->GetDimensions();
  const volatile int            image_height = dims[1];
  const volatile int            image_width = dims[0];
#if 1
  // https://talosintelligence.com/vulnerability_reports/TALOS-2025-2210
  const volatile size_t expected_size =
    static_cast<size_t>(image_width) * image_height * this->GetPixelFormat().GetPixelSize();
  if (len != expected_size)
  {
    mdcmErrorMacro("Frame size doesn't match");
    return false;
  }
#endif
  struct jpeg_compress_struct cinfo;
  struct my_error_mgr jerr;
  std::ostream *      outfile = &os;
  JSAMPROW            row_pointer[1]{};
  volatile size_t     row_stride{};
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  if (setjmp(jerr.setjmp_buffer))
  {
    jpeg_destroy_compress(&cinfo);
    return false;
  }
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo, outfile);
  cinfo.image_width = image_width;
  cinfo.image_height = image_height;
  switch (this->GetPhotometricInterpretation())
  {
    case PhotometricInterpretation::MONOCHROME1:
    case PhotometricInterpretation::MONOCHROME2:
    case PhotometricInterpretation::PALETTE_COLOR:
      cinfo.input_components = 1;
      cinfo.in_color_space = JCS_GRAYSCALE;
      break;
    case PhotometricInterpretation::RGB:
    case PhotometricInterpretation::YBR_RCT:
    case PhotometricInterpretation::YBR_ICT:
      cinfo.input_components = 3;
      cinfo.in_color_space = JCS_RGB;
      break;
    case PhotometricInterpretation::YBR_FULL:
    case PhotometricInterpretation::YBR_FULL_422:
    case PhotometricInterpretation::YBR_PARTIAL_420: // MPEG
    case PhotometricInterpretation::YBR_PARTIAL_422: // Retired
      cinfo.input_components = 3;
      cinfo.in_color_space = JCS_YCbCr;
      break;
    case PhotometricInterpretation::HSV:
    case PhotometricInterpretation::ARGB:
    case PhotometricInterpretation::CMYK:
    case PhotometricInterpretation::UNKNOWN:
    case PhotometricInterpretation::PI_END:
      return false;
  }
  jpeg_set_defaults(&cinfo);
  if (!LossyFlag)
  {
    jpeg_simple_lossless(&cinfo, 1, 0);
  }
  if (!LossyFlag)
  {
    assert(Quality == 100);
  }
  jpeg_set_quality(&cinfo, Quality, TRUE);
  cinfo.write_JFIF_header = 0;
  jpeg_start_compress(&cinfo, TRUE);
  row_stride = image_width * cinfo.input_components;
  if (this->GetPlanarConfiguration() == 0)
  {
    while (cinfo.next_scanline < cinfo.image_height)
    {
      row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride];
      (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
  }
  else
  {
    JSAMPLE * tempbuffer =
      (JSAMPLE *)(*cinfo.mem->alloc_small)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride * sizeof(JSAMPLE));
    row_pointer[0] = tempbuffer;
    volatile int offset = image_height * image_width;
    while (cinfo.next_scanline < cinfo.image_height)
    {
      assert(row_stride % 3 == 0);
      JSAMPLE * ptempbuffer = tempbuffer;
      JSAMPLE * red = image_buffer + cinfo.next_scanline * row_stride / 3;
      JSAMPLE * green = image_buffer + cinfo.next_scanline * row_stride / 3 + offset;
      JSAMPLE * blue = image_buffer + cinfo.next_scanline * row_stride / 3 + offset * 2;
      for (size_t i = 0; i < row_stride / 3; ++i)
      {
        *ptempbuffer++ = *red++;
        *ptempbuffer++ = *green++;
        *ptempbuffer++ = *blue++;
      }
      (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
  }
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  return true;
}

bool
JPEGBITSCodec::EncodeBuffer(std::ostream & os, const char * data, size_t datalen)
{
  (void)datalen;
  void * vdata = const_cast<void*>(static_cast<const void*>(data));
  JSAMPLE *                     image_buffer = static_cast<JSAMPLE *>(vdata);
  const volatile unsigned int * dims = this->GetDimensions();
  const volatile int            image_height = dims[1];
  const volatile int            image_width = dims[0];
  jpeg_compress_struct & cinfo = Internals->cinfo_comp;
  my_error_mgr &  jerr = Internals->jerr;
  std::ostream *  outfile = &os;
  JSAMPROW        row_pointer[1];
  volatile size_t row_stride;
  if (Internals->StateSuspension == 0)
  {
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer))
    {
      jpeg_destroy_compress(&cinfo);
      return false;
    }
    jpeg_create_compress(&cinfo);
  }
  if (Internals->StateSuspension == 0)
  {
    jpeg_stdio_dest(&cinfo, outfile);
  }
  if (Internals->StateSuspension == 0)
  {
    cinfo.image_width = image_width;
    cinfo.image_height = image_height;
  }
  if (Internals->StateSuspension == 0)
  {
    switch (this->GetPhotometricInterpretation())
    {
      case PhotometricInterpretation::MONOCHROME1:
      case PhotometricInterpretation::MONOCHROME2:
      case PhotometricInterpretation::PALETTE_COLOR:
        cinfo.input_components = 1;
        cinfo.in_color_space = JCS_GRAYSCALE;
        break;
      case PhotometricInterpretation::RGB:
      case PhotometricInterpretation::YBR_RCT:
      case PhotometricInterpretation::YBR_ICT:
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
        break;
      case PhotometricInterpretation::YBR_FULL:
      case PhotometricInterpretation::YBR_FULL_422:
      case PhotometricInterpretation::YBR_PARTIAL_420:
      case PhotometricInterpretation::YBR_PARTIAL_422:
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_YCbCr;
        break;
      case PhotometricInterpretation::HSV:
      case PhotometricInterpretation::ARGB:
      case PhotometricInterpretation::CMYK:
      case PhotometricInterpretation::UNKNOWN:
      case PhotometricInterpretation::PI_END:
        return false;
    }
  }
  if (Internals->StateSuspension == 0)
  {
    jpeg_set_defaults(&cinfo);
  }
  if (Internals->StateSuspension == 0)
  {
    if (!LossyFlag)
    {
      jpeg_simple_lossless(&cinfo, 1, 0);
    }
  }
  if (!LossyFlag)
  {
    assert(Quality == 100);
  }
  if (Internals->StateSuspension == 0)
  {
    jpeg_set_quality(&cinfo, Quality, TRUE);
  }
  if (Internals->StateSuspension == 0)
  {
    cinfo.write_JFIF_header = 0;
  }
  if (Internals->StateSuspension == 0)
  {
    jpeg_start_compress(&cinfo, TRUE);
    Internals->StateSuspension = 1;
  }
  row_stride = image_width * cinfo.input_components;
  if (Internals->StateSuspension == 1)
  {
    assert(this->GetPlanarConfiguration() == 0);
    assert(row_stride * sizeof(JSAMPLE) == datalen);
    {
      row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride * 0];
#ifdef NDEBUG
      (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
#else
      const JDIMENSION nscanline = jpeg_write_scanlines(&cinfo, row_pointer, 1);
      assert(nscanline == 1);
      assert(cinfo.next_scanline <= cinfo.image_height);
#endif
    }
    if (cinfo.next_scanline == cinfo.image_height)
    {
      Internals->StateSuspension = 2;
    }
  }
  if (Internals->StateSuspension == 2)
  {
    jpeg_finish_compress(&cinfo);
  }
  if (Internals->StateSuspension == 2)
  {
    jpeg_destroy_compress(&cinfo);
    Internals->StateSuspension = 0;
  }
  return true;
}

bool
JPEGBITSCodec::IsStateSuspension() const
{
  return Internals->StateSuspension != 0;
}

} // end namespace mdcm

#if 0
#ifndef _WIN32
#  pragma GCC diagnostic pop
#endif
#endif

#ifdef JPEGBITS_PRINT_COLORSPACES
#undef JPEGBITS_PRINT_COLORSPACES
#endif
