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
#include "mdcmTrace.h"
#include "mdcmTransferSyntax.h"
#include "mdcmImageHelper.h"
/*
 * jdatasrc.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains decompression data source routines for the case of
 * reading JPEG data from a file (or any stdio stream).  While these routines
 * are sufficient for most applications, some will want to use a different
 * source manager.
 * IMPORTANT: we assume that fread() will correctly transcribe an array of
 * JOCTETs from 8-bit-wide elements on external storage.  If char is wider
 * than 8 bits on your machine, you may need to do some tweaking.
 */

/* this is not a core library module, so it doesn't define JPEG_INTERNALS */

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

#define INPUT_BUF_SIZE 4096 /* choose an efficiently fread'able size */

/*
 * Initialize source - called by jpeg_read_header
 * before any data is actually read.
 */

// cppcheck-suppress unknownMacro
METHODDEF(void) init_source(j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr)cinfo->src;
  /* We reset the empty-input-file flag for each image,
   * but we don't clear the input buffer.
   * This is correct behavior for reading a series of images from one source.
   */
  src->start_of_file = TRUE;
}

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 *
 * In typical applications, this should read fresh data into the buffer
 * (ignoring the current state of next_input_byte & bytes_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been reloaded.  It is not necessary to
 * fill the buffer entirely, only to obtain at least one more byte.
 *
 * There is no such thing as an EOF return.  If the end of the file has been
 * reached, the routine has a choice of ERREXIT() or inserting fake data into
 * the buffer.  In most cases, generating a warning message and inserting a
 * fake EOI marker is the best course of action --- this will allow the
 * decompressor to output however much of the image is there.  However,
 * the resulting error message is misleading if the real problem is an empty
 * input file, so we handle that case specially.
 *
 * In applications that need to be able to suspend compression due to input
 * not being available yet, a FALSE return indicates that no more data can be
 * obtained right now, but more may be forthcoming later.  In this situation,
 * the decompressor will return to its caller (with an indication of the
 * number of scanlines it has read, if any).  The application should resume
 * decompression after it has loaded more data into the input buffer.  Note
 * that there are substantial restrictions on the use of suspension --- see
 * the documentation.
 *
 * When suspending, the decompressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_input_byte & bytes_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point must be rescanned after resumption, so move it to
 * the front of the buffer rather than discarding it.
 */

// cppcheck-suppress unknownMacro
METHODDEF(boolean) fill_input_buffer(j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr)cinfo->src;
  size_t     nbytes = 0;
  std::streampos pos = src->infile->tellg();
  std::streampos end = src->infile->seekg(0, std::ios::end).tellg();
  src->infile->seekg(pos, std::ios::beg);
  //
  if (end == pos)
  {
    /* Start the I/O suspension simply by returning false here */
    // FIXME return value seems to be not used below
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
    if (src->start_of_file) /* Treat empty input file as fatal error */
    {
      ERREXIT(cinfo, JERR_INPUT_EMPTY);
    }
    WARNMS(cinfo, JWRN_JPEG_EOF);
    /* Insert a fake EOI marker */
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

/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip_input_data
 * is not granted the right to give a suspension return.  If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill_input_buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */

// cppcheck-suppress unknownMacro
METHODDEF(void) skip_input_data(j_decompress_ptr cinfo, IJG_LONG num_bytes)
{
  my_src_ptr src = (my_src_ptr)cinfo->src;
  /* Just a dumb implementation for now.  Could use fseek() except
   * it doesn't work on pipes.  Not clear that being smart is worth
   * any trouble anyway --- large skips are infrequent.
   */
  if (num_bytes > 0)
  {
    while (num_bytes > (IJG_LONG)src->pub.bytes_in_buffer)
    {
      num_bytes -= (IJG_LONG)src->pub.bytes_in_buffer;
      (void)fill_input_buffer(cinfo);
      /* assumed that fill_input_buffer will never return FALSE,
       * so suspension need not be handled.
       */
    }
    src->pub.next_input_byte += (size_t)num_bytes;
    src->pub.bytes_in_buffer -= (size_t)num_bytes;
  }
}

/*
 * An additional method that can be provided by data source modules is the
 * resync_to_restart method for error recovery in the presence of RST markers.
 * For the moment, this source module just uses the default resync method
 * provided by the JPEG library.  That method assumes that no backtracking
 * is possible.
 */

/*
 * Terminate source - called by jpeg_finish_decompress
 * after all data has been read.  Often a no-op.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

// cppcheck-suppress unknownMacro
METHODDEF(void) term_source(j_decompress_ptr) {}

/*
 * Prepare for input from a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing decompression.
 */

// cppcheck-suppress unknownMacro
GLOBAL(void) jpeg_stdio_src(j_decompress_ptr cinfo, std::istream & infile, bool flag)
{
  my_src_ptr src;
  /* The source object and input buffer are made permanent so that a series
   * of JPEG images can be read from the same file by calling jpeg_stdio_src
   * only before the first one.  (If we discarded the buffer at the end of
   * one image, we'd likely lose the start of the next one.)
   * This makes it unsafe to use this manager and a different source
   * manager serially with the same JPEG object.  Caveat programmer.
   */
  if (cinfo->src == NULL)
  { /* first time for this JPEG object? */
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
  src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  src->pub.term_source = term_source;
  src->infile = &infile;
  if (flag)
  {
    src->pub.bytes_in_buffer = 0;    /* forces fill_input_buffer on first read */
    src->pub.next_input_byte = NULL; /* until buffer loaded */
  }
}

struct my_error_mgr
{
  struct jpeg_error_mgr pub;           /* "public" fields */
  jmp_buf               setjmp_buffer; /* for return to caller */
};

class JPEGInternals
{
public:
  JPEGInternals()
    : cinfo()
    , cinfo_comp()
    , jerr()
    , StateSuspension(0)
    , SampBuffer(NULL)
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
  if (Internals)
  {
    delete Internals;
    Internals = NULL; // not required
  }
}

/*
 * Here's the routine that will replace the standard error_exit method:
 */
extern "C"
{
  METHODDEF(void) my_error_exit(j_common_ptr cinfo)
  {
    /* cinfo->err really points to a my_error_mgr struct */
    my_error_mgr * myerr = (my_error_mgr *)cinfo->err;
    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message)(cinfo);
    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
  }
}

bool
JPEGBITSCodec::GetHeaderInfo(std::istream & is, TransferSyntax & ts)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  jpeg_decompress_struct & cinfo = Internals->cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  my_error_mgr & jerr = Internals->jerr;
  /* More stuff */
  if (Internals->StateSuspension == 0)
  {
    // Step 1: allocate and initialize JPEG decompression object
    //
    // We set up the normal JPEG error routines, then override error_exit.
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    // Establish the setjmp return context for my_error_exit to use.
    if (setjmp(jerr.setjmp_buffer))
    {
      // If we are here, the JPEG code has signaled an error.
      // We need to clean up the JPEG object, close the input file, and return.
      // But first handle the case IJG does not like:
      if (jerr.pub.msg_code == JERR_BAD_PRECISION)
      {
        this->BitSample = jerr.pub.msg_parm.i[0];
        assert(this->BitSample == 1 || this->BitSample == 8 || this->BitSample == 12 || this->BitSample == 16);
        assert(this->BitSample == cinfo.data_precision);
      }
      jpeg_destroy_decompress(&cinfo);
      // TODO: www.dcm4che.org/jira/secure/attachment/10185/ct-implicit-little.dcm weird Icon Image from GE
      return false;
    }
  }
  if (Internals->StateSuspension == 0)
  {
    // Initialize the JPEG decompression object.
    jpeg_create_decompress(&cinfo);
    int workaround = 0;
    if (ImageHelper::GetWorkaroundPredictorBug())
      workaround |= WORKAROUND_PREDICTOR6OVERFLOW;
    if (ImageHelper::GetWorkaroundCornellBug())
      workaround |= WORKAROUND_BUGGY_CORNELL_16BIT_JPEG_ENCODER;
    if (workaround != 0)
      cinfo.workaround_options = workaround;
    // Step 2: specify data source (e.g., a file)
    jpeg_stdio_src(&cinfo, is, true);
  }
  else
  {
    jpeg_stdio_src(&cinfo, is, false);
  }
  /* Step 3: read file parameters with jpeg_read_header() */
  if (Internals->StateSuspension < 2)
  {
    if (jpeg_read_header(&cinfo, TRUE) == JPEG_SUSPENDED)
    {
      Internals->StateSuspension = 2;
    }
    // First of all, are we using the proper JPEG decoder (correct bit sample)?
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
    this->Dimensions[1] = cinfo.image_height; // Number of rows in image
    this->Dimensions[0] = cinfo.image_width;  // Number of columns in image
    const int prep = this->PF.GetPixelRepresentation();
    const int precision = cinfo.data_precision;
    // If lossy it should only be 8 or 12
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
    this->PF.SetPixelRepresentation((uint16_t)prep);
    this->PF.SetBitsStored((uint16_t)precision);
    this->PlanarConfiguration = 0;
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
      this->PlanarConfiguration = 1;
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

  // Pixel density stuff
  /*
  UINT8 density_unit
  UINT16 X_density
  UINT16 Y_density
    The resolution information to be written into the JFIF marker;
    not used otherwise,  density_unit may be 0 for unknown,
    1 for dots/inch, or 2 for dots/cm.  The default values are 0,1,1
    indicating square pixels of unknown size.
  */

  if (cinfo.density_unit != 0 || cinfo.X_density != 1 || cinfo.Y_density != 1)
  {
    mdcmWarningMacro("Pixel Density from JFIF Marker is not supported (for now)");
  }
  /* Step 8: Release JPEG decompression object */
  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);
  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   *
   * At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   *
   * In any case make sure the we reset the internal state suspension
   */
  Internals->StateSuspension = 0;
  return true;
}

/*
 * Note: see dcmdjpeg +cn option to avoid the YBR => RGB loss
 */

bool
JPEGBITSCodec::DecodeByStreams(std::istream & is, std::ostream & os)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  jpeg_decompress_struct & cinfo = Internals->cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  my_error_mgr & jerr = Internals->jerr;
  JSAMPARRAY     buffer;     /* Output row buffer */
  size_t         row_stride; /* physical row width in output buffer */
  if (Internals->StateSuspension == 0)
  {
    // Step 1: allocate and initialize JPEG decompression object
    //
    // We set up the normal JPEG error routines, then override error_exit.
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    // Establish the setjmp return context for my_error_exit to use.
    if (setjmp(jerr.setjmp_buffer))
    {
      // If we get here, the JPEG code has signaled an error.
      // We need to clean up the JPEG object, close the input file, and return.
      // But first handle the case IJG does not like.
      if (jerr.pub.msg_code == JERR_BAD_PRECISION)
      {
        this->BitSample = jerr.pub.msg_parm.i[0];
      }
      jpeg_destroy_decompress(&cinfo);
      // www.dcm4che.org/jira/secure/attachment/10185/ct-implicit-little.dcm
      // weird Icon Image from GE
      mdcmAlwaysWarnMacro("JPEG error (1)");
      return false;
    }
  }
  if (Internals->StateSuspension == 0)
  {
    // Initialize the JPEG decompression object.
    jpeg_create_decompress(&cinfo);
    int workaround = 0;
    if (ImageHelper::GetWorkaroundPredictorBug())
      workaround |= WORKAROUND_PREDICTOR6OVERFLOW;
    if (ImageHelper::GetWorkaroundCornellBug())
      workaround |= WORKAROUND_BUGGY_CORNELL_16BIT_JPEG_ENCODER;
    if (workaround != 0)
      cinfo.workaround_options = workaround;
    // Step 2: specify data source (e.g. a file)
    jpeg_stdio_src(&cinfo, is, true);
  }
  else
  {
    jpeg_stdio_src(&cinfo, is, false);
  }
  /* Step 3: read file parameters with jpeg_read_header() */
  // Use proper JPEG decoder (correct bit sample)
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
    const unsigned int * dims = this->GetDimensions();
    if (cinfo.image_width != dims[0] || cinfo.image_height != dims[1])
    {
      mdcmAlwaysWarnMacro("JPEG is " << cinfo.image_width << "x" << cinfo.image_height << ", DICOM " << dims[0] << "x"
                                     << dims[1]);
    }
#if 1
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
    // clang-format off
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
        if(
          // Preserve Y'CbCr color-space
             (GetPhotometricInterpretation() == PhotometricInterpretation::YBR_FULL)
          || (GetPhotometricInterpretation() == PhotometricInterpretation::YBR_FULL_422)
          //
          // Currently don't preserve color-space for YBR_PARTIAL_422,
          // YBR to RGB formula doesn't work well.
#if 0
          || (GetPhotometricInterpretation() == PhotometricInterpretation::YBR_PARTIAL_422)
#endif
          //
          // In fact, lossy JPEG with photo-metric "RGB" and cinfo.jpeg_color_space JCS_YCbCr exist
          // and are highly likely valid. Required to open correctly.
#if 1
          || (GetPhotometricInterpretation() == PhotometricInterpretation::RGB)
# endif
        )
        {
          //
          //
          //
          cinfo.jpeg_color_space = JCS_UNKNOWN;
          cinfo.out_color_space = JCS_UNKNOWN;
          //
          //
          //
        }
        break;
      default:
        break;
    }
    // clang-format on
    /* Step 4: set parameters for decompression */
    /* Not set */
  }
  /* Step 5: Start decompressor */
  if (Internals->StateSuspension < 3)
  {
    if (jpeg_start_decompress(&cinfo) == FALSE)
    {
      /* Suspension: jpeg_start_decompress */
      Internals->StateSuspension = 3;
    }
    /* We may need to do some setup of our own at this point before reading
     * the data.  After jpeg_start_decompress() we have the correct scaled
     * output image dimensions available, as well as the output colormap
     * if we asked for color quantization.
     * In this example, we need to make an output work buffer of the right size.
     */
    /* JSAMPLEs per row in output buffer */
    row_stride = cinfo.output_width * cinfo.output_components;
    row_stride *= sizeof(JSAMPLE);
    /* Make a one-row-high sample array that will go away when done with image */
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, (JDIMENSION)row_stride, 1);
    /* Save the buffer in case of suspension to be able to reuse it later: */
    Internals->SampBuffer = buffer;
  }
  else
  {
    /* JSAMPLEs per row in output buffer */
    row_stride = cinfo.output_width * cinfo.output_components;
    row_stride *= sizeof(JSAMPLE);
    /* Suspension: re-use the buffer: */
    buffer = (JSAMPARRAY)Internals->SampBuffer;
  }
  /* Step 6: while (scan lines remain to be read)
   * Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height)
  {
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
    if (jpeg_read_scanlines(&cinfo, buffer, 1) == 0)
    {
      /* Suspension in jpeg_read_scanlines */
      Internals->StateSuspension = 3;
      return true;
    }
    os.write((char *)buffer[0], row_stride);
  }
  /* Step 7: Finish decompression */
  if (jpeg_finish_decompress(&cinfo) == FALSE)
  {
    /* Suspension: jpeg_finish_decompress */
    Internals->StateSuspension = 4;
    return true;
  }
  /* we are done decompressing the file, now is a good time to store the type
     of compression used: lossless or not */
  if (cinfo.process == JPROC_LOSSLESS)
  {
    LossyFlag = false;
  }
  else
  {
    LossyFlag = true;
  }
  /* Step 8: Release JPEG decompression object */
  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);
  if (jerr.pub.num_warnings > 0)
  {
    mdcmAlwaysWarnMacro("jerr.pub.num_warnings = " << jerr.pub.num_warnings);
  }
  /* In any case make sure the we reset the internal state suspension */
  Internals->StateSuspension = 0;
  return true;
}

/*
 * jdatadst.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains compression data destination routines for the case of
 * emitting JPEG data to a file (or any stdio stream).  While these routines
 * are sufficient for most applications, some will want to use a different
 * destination manager.
 * IMPORTANT: we assume that fwrite() will correctly transcribe an array of
 * JOCTETs into 8-bit-wide elements on external storage.  If char is wider
 * than 8 bits on your machine, you may need to do some tweaking.
 */

/**
 * Very low level C 'structure', used to decode jpeg file
 */
typedef struct
{
  struct jpeg_destination_mgr pub;     /* public fields */
  std::ostream *              outfile; /* target stream */
  JOCTET *                    buffer;  /* start of buffer */
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;

#define OUTPUT_BUF_SIZE 4096 /* choose an efficiently fwrite'able size */

/*
 * Initialize destination - called by jpeg_start_compress
 * before any data is actually written.
 */
METHODDEF(void) init_destination(j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr)cinfo->dest;
  /* Allocate the output buffer - it will be released when done with image */
  dest->buffer =
    (JOCTET *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_IMAGE, OUTPUT_BUF_SIZE * SIZEOF(JOCTET));
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

/*
 * Empty the output buffer - called whenever buffer fills up.
 *
 * In typical applications, this should write the entire output buffer
 * (ignoring the current state of next_output_byte & free_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been dumped.
 *
 * In applications that need to be able to suspend compression due to output
 * overrun, a FALSE return indicates that the buffer cannot be emptied now.
 * In this situation, the compressor will return to its caller (possibly with
 * an indication that it has not accepted all the supplied scanlines).  The
 * application should resume compression after it has made more room in the
 * output buffer.  Note that there are substantial restrictions on the use of
 * suspension --- see the documentation.
 *
 * When suspending, the compressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_output_byte & free_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point will be regenerated after resumption, so do not
 * write it out when emptying the buffer externally.
 */
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

/*
 * Terminate destination, called by jpeg_finish_compress
 * after all data has been written.  Usually needs to flush buffer.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

METHODDEF(void) term_destination(j_compress_ptr cinfo)
{
  my_dest_ptr  dest = (my_dest_ptr)cinfo->dest;
  const size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;
  /* Write any data remaining in the buffer */
  if (datacount > 0)
  {
    if (!dest->outfile->write((char *)dest->buffer, datacount))
    {
      ERREXIT(cinfo, JERR_FILE_WRITE);
    }
  }
  dest->outfile->flush();
  /* Make sure we wrote the output file OK */
  if (dest->outfile->fail())
    ERREXIT(cinfo, JERR_FILE_WRITE);
}

/*
 * Prepare for output to a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing compression.
 */
GLOBAL(void) jpeg_stdio_dest(j_compress_ptr cinfo, std::ostream * outfile)
{
  my_dest_ptr dest;
  /* The destination object is made permanent so that multiple JPEG images
   * can be written to the same file without re-executing jpeg_stdio_dest.
   * This makes it dangerous to use this manager and a different destination
   * manager serially with the same JPEG object, because their private object
   * sizes may be different.  Caveat programmer.
   */
  if (cinfo->dest == NULL)
  {
    /* first time for this JPEG object? */
    cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)(
      (j_common_ptr)cinfo, JPOOL_PERMANENT, SIZEOF(my_destination_mgr));
  }
  dest = (my_dest_ptr)cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->outfile = outfile;
}

/*
 * Sample routine for JPEG compression.  We assume that the target file name
 * and a compression quality factor are passed in.
 */
bool
JPEGBITSCodec::InternalCode(const char * input, size_t len, std::ostream & os)
{
  int quality = 100;
  (void)len;
  (void)quality;
  JSAMPLE *            image_buffer = (JSAMPLE *)input; /* Points to large array of R,G,B-order data */
  const unsigned int * dims = this->GetDimensions();
  const int            image_height = dims[1]; /* Number of rows in image */
  const int            image_width = dims[0];  /* Number of columns in image */
  /* This struct contains the JPEG compression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   * It is possible to have several such structures, representing multiple
   * compression/decompression processes, in existence at once.  We refer
   * to any one struct (and its associated working data) as a "JPEG object".
   */
  struct jpeg_compress_struct cinfo;
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct my_error_mgr jerr;
  std::ostream *      outfile = &os;
  JSAMPROW            row_pointer[1]; /* pointer to JSAMPLE row[s] */
  size_t              row_stride;     /* physical row width in image buffer */
  /* Step 1: allocate and initialize JPEG compression object */
  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  // Establish the setjmp return context for my_error_exit to use.
  if (setjmp(jerr.setjmp_buffer))
  {
    jpeg_destroy_compress(&cinfo);
    return false;
  }
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(&cinfo);
  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  /* Here we use the library-supplied code to send compressed data to a
   * stdio stream.  You can also write your own code to do something else.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to write binary files.
   */
  jpeg_stdio_dest(&cinfo, outfile);
  /* Step 3: set parameters for compression */
  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  cinfo.image_width = image_width; /* image width and height, in pixels */
  cinfo.image_height = image_height;
  switch (this->GetPhotometricInterpretation())
  {
    case PhotometricInterpretation::MONOCHROME1:
    case PhotometricInterpretation::MONOCHROME2:
    case PhotometricInterpretation::PALETTE_COLOR:
      cinfo.input_components = 1;           /* # of color components per pixel */
      cinfo.in_color_space = JCS_GRAYSCALE; /* colorspace of input image */
      break;
    case PhotometricInterpretation::RGB:
    case PhotometricInterpretation::YBR_RCT:
    case PhotometricInterpretation::YBR_ICT:
      cinfo.input_components = 3;     /* # of color components per pixel */
      cinfo.in_color_space = JCS_RGB; /* colorspace of input image */
      break;
    case PhotometricInterpretation::YBR_FULL:
    case PhotometricInterpretation::YBR_FULL_422:
    case PhotometricInterpretation::YBR_PARTIAL_420: // MPEG
    case PhotometricInterpretation::YBR_PARTIAL_422: // Retired
      cinfo.input_components = 3;       /* # of color components per pixel */
      cinfo.in_color_space = JCS_YCbCr; /* colorspace of input image */
      break;
    case PhotometricInterpretation::HSV:
    case PhotometricInterpretation::ARGB:
    case PhotometricInterpretation::CMYK:
    case PhotometricInterpretation::UNKNOWN:
    case PhotometricInterpretation::PI_END:
      return false;
  }
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults(&cinfo);
  /*
   * predictor = 1
   * point_transform = 0
   * => lossless transformation.
   * Basicaly you need to have point_transform = 0, but you can pick predictor [1...7]
   * TODO: is there a way to pick the right predictor (best compression/fastest?)
   */
  if (!LossyFlag)
  {
    jpeg_simple_lossless(&cinfo, 1, 0);
  }
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  if (!LossyFlag)
  {
    assert(Quality == 100);
  }
  jpeg_set_quality(&cinfo, Quality, TRUE /* limit to baseline-JPEG values */);
  /*
   * See write_file_header
   */
  cinfo.write_JFIF_header = 0;
  // cinfo.density_unit = 2;
  // cinfo.X_density = 2;
  // cinfo.Y_density = 5;
  /* Step 4: Start compressor */
  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress(&cinfo, TRUE);
  /* Step 5: while (scan lines remain to be written) jpeg_write_scanlines(...); */
  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  row_stride = image_width * cinfo.input_components; /* JSAMPLEs per row in image_buffer */
  if (this->GetPlanarConfiguration() == 0)
  {
    while (cinfo.next_scanline < cinfo.image_height)
    {
      /* jpeg_write_scanlines expects an array of pointers to scanlines.
       * Here the array is only one element long, but you could pass
       * more than one scanline at a time if that's more convenient.
       */
      row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride];
      (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
  }
  else
  {
    /*
     * warning: Need to read C.7.6.3.1.3 Planar Configuration (see note about Planar Configuration dummy value)
     */
    JSAMPLE * tempbuffer = (JSAMPLE *)malloc(row_stride * sizeof(JSAMPLE));
    row_pointer[0] = tempbuffer;
    int offset = image_height * image_width;
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
    free(tempbuffer);
  }
  /* Step 6: Finish compression */
  jpeg_finish_compress(&cinfo);
  /* Step 7: release JPEG compression object */
  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&cinfo);
  return true;
}

bool
JPEGBITSCodec::EncodeBuffer(std::ostream & os, const char * data, size_t datalen)
{
  (void)datalen;
  JSAMPLE *            image_buffer = (JSAMPLE *)data; /* Points to large array of R,G,B-order data */
  const unsigned int * dims = this->GetDimensions();
  const int            image_height = dims[1]; /* Number of rows in image */
  const int            image_width = dims[0];  /* Number of columns in image */
  /* This struct contains the JPEG compression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   * It is possible to have several such structures, representing multiple
   * compression/decompression processes, in existence at once.  We refer
   * to any one struct (and its associated working data) as a "JPEG object".
   */
  jpeg_compress_struct & cinfo = Internals->cinfo_comp;
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  my_error_mgr & jerr = Internals->jerr;
  std::ostream * outfile = &os;
  JSAMPROW       row_pointer[1]; /* pointer to JSAMPLE row[s] */
  size_t         row_stride;     /* physical row width in image buffer */
  if (Internals->StateSuspension == 0)
  {
    /* Step 1: allocate and initialize JPEG compression object */
    /* We have to set up the error handler first, in case the initialization
     * step fails.  (Unlikely, but it could happen if you are out of memory.)
     * This routine fills in the contents of struct jerr, and returns jerr's
     * address which we place into the link field in cinfo.
     */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    // Establish the setjmp return context for my_error_exit to use.
    if (setjmp(jerr.setjmp_buffer))
    {
      jpeg_destroy_compress(&cinfo);
      return false;
    }
    /* Now we can initialize the JPEG compression object. */
    jpeg_create_compress(&cinfo);
    /* Step 2: specify data destination (eg, a file) */
    /* Note: steps 2 and 3 can be done in either order. */
    /* Here we use the library-supplied code to send compressed data to a
     * stdio stream.  You can also write your own code to do something else.
     * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
     * requires it in order to write binary files.
     */
  }
  if (Internals->StateSuspension == 0)
  {
    jpeg_stdio_dest(&cinfo, outfile);
  }
  /* Step 3: set parameters for compression */
  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  if (Internals->StateSuspension == 0)
  {
    cinfo.image_width = image_width; /* image width and height, in pixels */
    cinfo.image_height = image_height;
  }
  if (Internals->StateSuspension == 0)
  {
    switch (this->GetPhotometricInterpretation())
    {
      case PhotometricInterpretation::MONOCHROME1:
      case PhotometricInterpretation::MONOCHROME2:
      case PhotometricInterpretation::PALETTE_COLOR:
        cinfo.input_components = 1;           /* # of color components per pixel */
        cinfo.in_color_space = JCS_GRAYSCALE; /* colorspace of input image */
        break;
      case PhotometricInterpretation::RGB:
      case PhotometricInterpretation::YBR_RCT:
      case PhotometricInterpretation::YBR_ICT:
        cinfo.input_components = 3;     /* # of color components per pixel */
        cinfo.in_color_space = JCS_RGB; /* colorspace of input image */
        break;
      case PhotometricInterpretation::YBR_FULL:
      case PhotometricInterpretation::YBR_FULL_422:
      case PhotometricInterpretation::YBR_PARTIAL_420:
      case PhotometricInterpretation::YBR_PARTIAL_422:
        cinfo.input_components = 3;       /* # of color components per pixel */
        cinfo.in_color_space = JCS_YCbCr; /* colorspace of input image */
        break;
      case PhotometricInterpretation::HSV:
      case PhotometricInterpretation::ARGB:
      case PhotometricInterpretation::CMYK:
      case PhotometricInterpretation::UNKNOWN:
      case PhotometricInterpretation::PI_END:
        return false;
    }
  }
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  if (Internals->StateSuspension == 0)
  {
    jpeg_set_defaults(&cinfo);
  }
  /*
   * predictor = 1
   * point_transform = 0
   * => lossless transformation.
   * Basicaly you need to have point_transform = 0, but you can pick predictor [1...7]
   * TODO: is there a way to pick the right predictor (best compression/fastest?)
   */
  if (Internals->StateSuspension == 0)
  {
    if (!LossyFlag)
    {
      jpeg_simple_lossless(&cinfo, 1, 0);
    }
  }
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  if (!LossyFlag)
  {
    assert(Quality == 100);
  }
  if (Internals->StateSuspension == 0)
  {
    jpeg_set_quality(&cinfo, Quality, TRUE /* limit to baseline-JPEG values */);
  }
  if (Internals->StateSuspension == 0)
  {
    /*
     * See write_file_header
     */
    cinfo.write_JFIF_header = 0;
  }
  // cinfo.density_unit = 2;
  // cinfo.X_density = 2;
  // cinfo.Y_density = 5;
  /* Step 4: Start compressor */
  if (Internals->StateSuspension == 0)
  {
    /* TRUE ensures that we will write a complete interchange-JPEG file.
     * Pass TRUE unless you are very sure of what you're doing.
     */
    jpeg_start_compress(&cinfo, TRUE);
    Internals->StateSuspension = 1;
  }
  /* Step 5: while (scan lines remain to be written) jpeg_write_scanlines(...); */
  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  row_stride = image_width * cinfo.input_components; /* JSAMPLEs per row in image_buffer */
  if (Internals->StateSuspension == 1)
  {
    assert(this->GetPlanarConfiguration() == 0);
    assert(row_stride * sizeof(JSAMPLE) == datalen);
    {
      row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride * 0];
      const JDIMENSION nscanline = jpeg_write_scanlines(&cinfo, row_pointer, 1);
      assert(nscanline == 1);
      (void)nscanline;
      assert(cinfo.next_scanline <= cinfo.image_height);
    }
    if (cinfo.next_scanline == cinfo.image_height)
    {
      Internals->StateSuspension = 2;
    }
  }
  /* Step 6: Finish compression */
  if (Internals->StateSuspension == 2)
  {
    jpeg_finish_compress(&cinfo);
  }
  /* Step 7: release JPEG compression object */
  if (Internals->StateSuspension == 2)
  {
    /* This is an important step since it will release a good deal of memory. */
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
