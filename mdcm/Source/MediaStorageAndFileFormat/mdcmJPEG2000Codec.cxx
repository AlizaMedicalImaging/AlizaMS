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

#include "mdcmJPEG2000Codec.h"
#include "mdcmTransferSyntax.h"
#include "mdcmTrace.h"
#include "mdcmDataElement.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmSwapper.h"
#include <cstring>
#include <cstdio>
#include <numeric>
#ifdef _WIN32
#  define snprintf _snprintf
#endif
#include "mdcm_openjpeg.h"

namespace mdcm
{

/* Part 1  Table A.2 List of markers and marker segments */
typedef enum
{
  FF30 = 0xFF30,
  FF31 = 0xFF31,
  FF32 = 0xFF32,
  FF33 = 0xFF33,
  FF34 = 0xFF34,
  FF35 = 0xFF35,
  FF36 = 0xFF36,
  FF37 = 0xFF37,
  FF38 = 0xFF38,
  FF39 = 0xFF39,
  FF3A = 0xFF3A,
  FF3B = 0xFF3B,
  FF3C = 0xFF3C,
  FF3D = 0xFF3D,
  FF3E = 0xFF3E,
  FF3F = 0xFF3F,
  SOC = 0xFF4F,
  CAP = 0xFF50,
  SIZ = 0xFF51,
  COD = 0xFF52,
  COC = 0xFF53,
  TLM = 0xFF55,
  PLM = 0XFF57,
  PLT = 0XFF58,
  QCD = 0xFF5C,
  QCC = 0xFF5D,
  RGN = 0xFF5E,
  POC = 0xFF5F,
  PPM = 0XFF60,
  PPT = 0XFF61,
  CRG = 0xFF63,
  COM = 0xFF64,
  SOT = 0xFF90,
  SOP = 0xFF91,
  EPH = 0XFF92,
  SOD = 0xFF93,
  EOC = 0XFFD9 /* EOI in old jpeg */
} MarkerType;

typedef enum
{
  JP = 0x6a502020,
  FTYP = 0x66747970,
  JP2H = 0x6a703268,
  JP2C = 0x6a703263,
  JP2 = 0x6a703220,
  IHDR = 0x69686472,
  COLR = 0x636f6c72,
  XML = 0x786d6c20,
  CDEF = 0x63646566,
  CMAP = 0x636D6170,
  PCLR = 0x70636c72,
  RES = 0x72657320
} OtherType;

static inline bool
hasnolength(uint_fast16_t marker)
{
  switch (marker)
  {
    case FF30:
    case FF31:
    case FF32:
    case FF33:
    case FF34:
    case FF35:
    case FF36:
    case FF37:
    case FF38:
    case FF39:
    case FF3A:
    case FF3B:
    case FF3C:
    case FF3D:
    case FF3E:
    case FF3F:
    case SOC:
    case SOD:
    case EOC:
    case EPH:
      return true;
  }
  return false;
}

static inline bool
read16(const char ** input, size_t * len, uint16_t * ret)
{
  if (*len >= 2)
  {
    union
    {
      uint16_t v;
      char     bytes[2];
    } u;
    memcpy(u.bytes, *input, 2);
    *ret = SwapperDoOp::Swap(u.v);
    *input += 2;
    *len -= 2;
    return true;
  }
  return false;
}

static inline bool
read32(const char ** input, size_t * len, uint32_t * ret)
{
  if (*len >= 4)
  {
    union
    {
      uint32_t v;
      char     bytes[4];
    } u;
    memcpy(u.bytes, *input, 4);
    *ret = SwapperDoOp::Swap(u.v);
    *input += 4;
    *len -= 4;
    return true;
  }
  return false;
}

static inline bool
read64(const char ** input, size_t * len, uint64_t * ret)
{
  if (*len >= 8)
  {
    union
    {
      uint64_t v;
      char     bytes[8];
    } u;
    memcpy(u.bytes, *input, 8);
    *ret = SwapperDoOp::Swap(u.v);
    *input += 8;
    *len -= 8;
    return true;
  }
  return false;
}

static bool
parsej2k_imp(const char * const stream, const size_t file_size, bool * lossless, bool * mct)
{
  uint16_t     marker;
  size_t       lenmarker;
  const char * cur = stream;
  size_t       cur_size = file_size;
  *lossless = false;
  while (read16(&cur, &cur_size, &marker))
  {
    if (!hasnolength(marker))
    {
      uint16_t   l;
      const bool r = read16(&cur, &cur_size, &l);
      if (!r || l < 2)
        break;
      lenmarker = (size_t)l - 2;
      if (marker == COD)
      {
        const uint8_t MCTransformation = *(cur + 4);
        if (MCTransformation == 0x0)
        {
          *mct = false;
        }
        else if (MCTransformation == 0x1)
        {
          *mct = true;
        }
        else
        {
          return false;
        }
        const uint8_t Transformation = *(cur + 9);
        if (Transformation == 0x0)
        {
          *lossless = false;
          return true;
        }
        else if (Transformation == 0x1)
        {
          *lossless = true;
        }
        else
        {
          return false;
        }
      }
      cur += lenmarker;
      cur_size -= lenmarker;
    }
    else if (marker == SOD)
    {
      return true;
    }
  }
  return false;
}

static bool
parsejp2_imp(const char * const stream, const size_t file_size, bool * lossless, bool * mct)
{
  uint32_t     marker;
  uint64_t     len64; /* ref */
  uint32_t     len32; /* local 32bits op */
  const char * cur = stream;
  size_t       cur_size = file_size;
  while (read32(&cur, &cur_size, &len32))
  {
    const bool b0 = read32(&cur, &cur_size, &marker);
    if (!b0)
      break;
    len64 = len32;
    if (len32 == 1) /* 64bits ? */
    {
      const bool b = read64(&cur, &cur_size, &len64);
      assert(b);
      (void)b;
      len64 -= 8;
    }
    if (marker == JP2C)
    {
      const size_t start = cur - stream;
      if (!len64)
      {
        len64 = (size_t)(file_size - start + 8);
      }
      assert(len64 >= 8);
      return parsej2k_imp(cur, (size_t)(len64 - 8), lossless, mct);
    }
    const size_t lenmarker = (size_t)(len64 - 8);
    cur += lenmarker;
  }
  return false;
}

#define J2K_CFMT 0
#define JP2_CFMT 1
#define JPT_CFMT 2
#define PXM_DFMT 10
#define PGX_DFMT 11
#define BMP_DFMT 12
#define YUV_DFMT 13
#define TIF_DFMT 14
#define RAW_DFMT 15
#define TGA_DFMT 16
#define PNG_DFMT 17
#define CODEC_JP2 OPJ_CODEC_JP2
#define CODEC_J2K OPJ_CODEC_J2K
#define CLRSPC_GRAY OPJ_CLRSPC_GRAY
#define CLRSPC_SRGB OPJ_CLRSPC_SRGB
#define CLRSPC_CMYK OPJ_CLRSPC_CMYK
#define CLRSPC_EYCC OPJ_CLRSPC_EYCC
#define CLRSPC_SYCC OPJ_CLRSPC_SYCC
#define CLRSPC_UNKNOWN OPJ_CLRSPC_UNKNOWN
#define CLRSPC_UNSPECIFIED OPJ_CLRSPC_UNSPECIFIED

struct myfile
{
  char * mem;
  char * cur;
  size_t len;
};

void
mdcm_error_callback(const char * msg, void *)
{
  fprintf(stderr, "%s", msg);
}

OPJ_SIZE_T
opj_read_from_memory(void * p_buffer, OPJ_SIZE_T p_nb_bytes, myfile * p_file)
{
  OPJ_SIZE_T l_nb_read;
  if (p_file->cur + p_nb_bytes <= p_file->mem + p_file->len)
  {
    l_nb_read = 1 * p_nb_bytes;
  }
  else
  {
    l_nb_read = (OPJ_SIZE_T)(p_file->mem + p_file->len - p_file->cur);
    assert(l_nb_read < p_nb_bytes);
  }
  memcpy(p_buffer, p_file->cur, l_nb_read);
  p_file->cur += l_nb_read;
  assert(p_file->cur <= p_file->mem + p_file->len);
  return ((l_nb_read) ? l_nb_read : ((OPJ_SIZE_T)-1));
}

OPJ_SIZE_T
opj_write_from_memory(void * p_buffer, OPJ_SIZE_T p_nb_bytes, myfile * p_file)
{
  OPJ_SIZE_T l_nb_write;
  l_nb_write = 1 * p_nb_bytes;
  memcpy(p_file->cur, p_buffer, l_nb_write);
  p_file->cur += l_nb_write;
  p_file->len += l_nb_write;
  return l_nb_write;
}

OPJ_OFF_T
opj_skip_from_memory(OPJ_OFF_T p_nb_bytes, myfile * p_file)
{
  if (p_file->cur + p_nb_bytes <= p_file->mem + p_file->len)
  {
    p_file->cur += p_nb_bytes;
    return p_nb_bytes;
  }
  p_file->cur = p_file->mem + p_file->len;
  return -1;
}

OPJ_BOOL
opj_seek_from_memory(OPJ_OFF_T p_nb_bytes, myfile * p_file)
{
  assert(p_nb_bytes >= 0);
  if ((size_t)p_nb_bytes <= p_file->len)
  {
    p_file->cur = p_file->mem + p_nb_bytes;
    return OPJ_TRUE;
  }
  p_file->cur = p_file->mem + p_file->len;
  return OPJ_FALSE;
}

opj_stream_t * OPJ_CALLCONV
               opj_stream_create_memory_stream(myfile * p_mem, OPJ_SIZE_T p_size, bool p_is_read_stream)
{
  opj_stream_t * l_stream = NULL;
  if (!p_mem)
    return NULL;
  l_stream = opj_stream_create(p_size, p_is_read_stream);
  if (!l_stream)
    return NULL;
  opj_stream_set_user_data(l_stream, p_mem, NULL);
  opj_stream_set_read_function(l_stream, (opj_stream_read_fn)opj_read_from_memory);
  opj_stream_set_write_function(l_stream, (opj_stream_write_fn)opj_write_from_memory);
  opj_stream_set_skip_function(l_stream, (opj_stream_skip_fn)opj_skip_from_memory);
  opj_stream_set_seek_function(l_stream, (opj_stream_seek_fn)opj_seek_from_memory);
  opj_stream_set_user_data_length(l_stream, p_mem->len);
  return l_stream;
}

/*
 * Divide an integer by a power of 2 and round upwards.
 *
 * a divided by 2^b
 */
inline int
int_ceildivpow2(int a, int b)
{
  return ((a + (1 << b) - 1) >> b);
}

class JPEG2000Internals
{
public:
  JPEG2000Internals()
    : nNumberOfThreadsForDecompression(0)
  {
    memset(&coder_param, 0, sizeof(coder_param));
    opj_set_default_encoder_parameters(&coder_param);
  }
  opj_cparameters coder_param;
  int             nNumberOfThreadsForDecompression;
};

template <typename T>
void
rawtoimage_fill2(const T *     inputbuffer,
                 int           w,
                 int           h,
                 int           numcomps,
                 opj_image_t * image,
                 int           pc,
                 int           bitsallocated,
                 int           bitsstored,
                 int           highbit,
                 int           sign)
{
  uint16_t pmask = 0xffff;
  pmask = (uint16_t)(pmask >> (bitsallocated - bitsstored));
  const T * p = inputbuffer;
  if (sign)
  {
    // smask : to check the 'sign' when BitsStored != BitsAllocated
    uint16_t smask = 0x0001;
    smask = (uint16_t)(smask << (16 - (bitsallocated - bitsstored + 1)));
    // nmask : to propagate sign bit on negative values
    int16_t nmask = (int16_t)0x8000;
    nmask = (int16_t)(nmask >> (bitsallocated - bitsstored - 1));
    if (pc)
    {
      for (int compno = 0; compno < numcomps; ++compno)
      {
        for (int i = 0; i < w * h; ++i)
        {
          /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
          uint16_t c = *p;
          c = (uint16_t)(c >> (bitsstored - highbit - 1));
          if (c & smask)
          {
            c = (uint16_t)(c | nmask);
          }
          else
          {
            c = c & pmask;
          }
          int16_t fix;
          memcpy(&fix, &c, sizeof fix);
          image->comps[compno].data[i] = fix;
          ++p;
        }
      }
    }
    else
    {
      for (int i = 0; i < w * h; ++i)
      {
        for (int compno = 0; compno < numcomps; ++compno)
        {
          /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
          uint16_t c = *p;
          c = (uint16_t)(c >> (bitsstored - highbit - 1));
          if (c & smask)
          {
            c = (uint16_t)(c | nmask);
          }
          else
          {
            c = c & pmask;
          }
          int16_t fix;
          memcpy(&fix, &c, sizeof fix);
          image->comps[compno].data[i] = fix;
          ++p;
        }
      }
    }
  }
  else
  {
    if (pc)
    {
      for (int compno = 0; compno < numcomps; ++compno)
      {
        for (int i = 0; i < w * h; ++i)
        {
          /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
          uint16_t c = *p;
          c = (uint16_t)((c >> (bitsstored - highbit - 1)) & pmask);
          image->comps[compno].data[i] = c;
          ++p;
        }
      }
    }
    else
    {
      for (int i = 0; i < w * h; ++i)
      {
        for (int compno = 0; compno < numcomps; ++compno)
        {
          /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
          uint16_t c = *p;
          c = (uint16_t)((c >> (bitsstored - highbit - 1)) & pmask);
          image->comps[compno].data[i] = c;
          ++p;
        }
      }
    }
  }
}

template <typename T>
void
rawtoimage_fill(const T * inputbuffer, int w, int h, int numcomps, opj_image_t * image, int pc)
{
  const T * p = inputbuffer;
  if (pc)
  {
    for (int compno = 0; compno < numcomps; ++compno)
    {
      for (int i = 0; i < w * h; ++i)
      {
        /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
        image->comps[compno].data[i] = *p;
        ++p;
      }
    }
  }
  else
  {
    for (int i = 0; i < w * h; ++i)
    {
      for (int compno = 0; compno < numcomps; ++compno)
      {
        /* compno : 0 = GREY, (0, 1, 2) = (R, G, B) */
        image->comps[compno].data[i] = *p;
        ++p;
      }
    }
  }
}

opj_image_t *
rawtoimage(const char *        inputbuffer,
           opj_cparameters_t * parameters,
           size_t              fragment_size,
           int                 image_width,
           int                 image_height,
           int                 sample_pixel,
           int                 bitsallocated,
           int                 bitsstored,
           int                 highbit,
           int                 sign,
           int                 quality,
           int                 pc)
{
  (void)quality;
  (void)fragment_size;
  int                  w, h;
  int                  numcomps;
  OPJ_COLOR_SPACE      color_space;
  opj_image_cmptparm_t cmptparm[3]; /* maximum of 3 components */
  opj_image_t *        image = NULL;
  if (sample_pixel == 1)
  {
    numcomps = 1;
    color_space = CLRSPC_GRAY;
  }
  else if (sample_pixel == 3)
  {
    numcomps = 3;
    color_space = CLRSPC_SRGB;
    // TODO Does OpenJPEG support CLRSPC_SYCC?
  }
  else
  {
    mdcmAlwaysWarnMacro("Error: sample_pixel == 3 || sample_pixel == 1");
    return NULL;
  }
  if (bitsallocated % 8 != 0)
  {
    mdcmAlwaysWarnMacro("Error: bitsallocated % 8 != 0");
    return NULL;
  }
  // eg. fragment_size == 63532 and 181 * 117 * 3 * 8 == 63531
  assert(((fragment_size + 1) / 2) * 2 ==
         ((size_t)(image_height * image_width * numcomps * (bitsallocated / 8) + 1) / 2) * 2);
  int subsampling_dx = parameters->subsampling_dx;
  int subsampling_dy = parameters->subsampling_dy;
  // FIXME
  w = image_width;
  h = image_height;
  /* initialize image components */
  memset(&cmptparm[0], 0, 3 * sizeof(opj_image_cmptparm_t));
  for (int i = 0; i < numcomps; ++i)
  {
    // cmptparm[i].prec = bitsstored;
    cmptparm[i].prec = bitsallocated; // FIXME
    cmptparm[i].bpp = bitsallocated;
    cmptparm[i].sgnd = sign;
    cmptparm[i].dx = subsampling_dx;
    cmptparm[i].dy = subsampling_dy;
    cmptparm[i].w = w;
    cmptparm[i].h = h;
  }
  /* create the image */
  image = opj_image_create(numcomps, &cmptparm[0], color_space);
  if (!image)
    return NULL;
  /* set image offset and reference grid */
  image->x0 = parameters->image_offset_x0;
  image->y0 = parameters->image_offset_y0;
  image->x1 = parameters->image_offset_x0 + (w - 1) * subsampling_dx + 1;
  image->y1 = parameters->image_offset_y0 + (h - 1) * subsampling_dy + 1;
  /* set image data */
  if (bitsallocated <= 8)
  {
    if (sign)
    {
      rawtoimage_fill<int8_t>((const int8_t *)inputbuffer, w, h, numcomps, image, pc);
    }
    else
    {
      rawtoimage_fill<uint8_t>((const uint8_t *)inputbuffer, w, h, numcomps, image, pc);
    }
  }
  else if (bitsallocated <= 16)
  {
    if (bitsallocated != bitsstored)
    {
      if (sign)
      {
        rawtoimage_fill2<int16_t>(
          (const int16_t *)inputbuffer, w, h, numcomps, image, pc, bitsallocated, bitsstored, highbit, sign);
      }
      else
      {
        rawtoimage_fill2<uint16_t>(
          (const uint16_t *)inputbuffer, w, h, numcomps, image, pc, bitsallocated, bitsstored, highbit, sign);
      }
    }
    else
    {
      if (sign)
      {
        rawtoimage_fill<int16_t>((const int16_t *)inputbuffer, w, h, numcomps, image, pc);
      }
      else
      {
        rawtoimage_fill<uint16_t>((const uint16_t *)inputbuffer, w, h, numcomps, image, pc);
      }
    }
  }
  else if (bitsallocated <= 32)
  {
    if (sign)
    {
      rawtoimage_fill<int32_t>((const int32_t *)inputbuffer, w, h, numcomps, image, pc);
    }
    else
    {
      rawtoimage_fill<uint32_t>((const uint32_t *)inputbuffer, w, h, numcomps, image, pc);
    }
  }
  else
  {
    mdcmAlwaysWarnMacro("Error: bitsallocated > 32");
    opj_image_destroy(image);
    return NULL;
  }
  return image;
}

static inline bool
check_comp_valid(opj_image_t * image)
{
  opj_image_comp_t * comp = &image->comps[0];
  if (comp->prec > 32)
  {
    mdcmAlwaysWarnMacro("Can not handle precision: " << comp->prec);
    return false;
  }
  bool invalid = false;
  if (image->numcomps == 3)
  {
    opj_image_comp_t * comp1 = &image->comps[1];
    opj_image_comp_t * comp2 = &image->comps[2];
    if (comp->prec != comp1->prec)
      invalid = true;
    if (comp->prec != comp2->prec)
      invalid = true;
    if (comp->sgnd != comp1->sgnd)
      invalid = true;
    if (comp->sgnd != comp2->sgnd)
      invalid = true;
    if (comp->h != comp1->h)
      invalid = true;
    if (comp->h != comp2->h)
      invalid = true;
    if (comp->w != comp1->w)
      invalid = true;
    if (comp->w != comp2->w)
      invalid = true;
  }
  return !invalid;
}

JPEG2000Codec::JPEG2000Codec()
{
  Internals = new JPEG2000Internals;
#if (OPJ_VERSION_MAJOR == 2 && OPJ_VERSION_MINOR >= 3)
  if (opj_has_thread_support())
  {
    const int x = opj_get_num_cpus();
    Internals->nNumberOfThreadsForDecompression = (x == 1) ? 0 : x;
  }
#endif
}

JPEG2000Codec::~JPEG2000Codec()
{
  if (Internals)
  {
    delete Internals;
    Internals = NULL;
  }
}

bool
JPEG2000Codec::CanDecode(TransferSyntax const & ts) const
{
  return (ts == TransferSyntax::JPEG2000Lossless || ts == TransferSyntax::JPEG2000 ||
          ts == TransferSyntax::JPEG2000Part2Lossless || ts == TransferSyntax::JPEG2000Part2);
  // Part 2 is not tested, TODO
}

bool
JPEG2000Codec::CanCode(TransferSyntax const & ts) const
{
  return (ts == TransferSyntax::JPEG2000Lossless || ts == TransferSyntax::JPEG2000);
}

/*
A.4.4 JPEG 2000 image compression

  If the object allows multi-frame images in the pixel data field, then for these JPEG 2000 Part 1 Transfer
  Syntaxes, each frame shall be encoded separately. Each fragment shall contain encoded data from a
  single frame.
  Note: That is, the processes defined in ISO/IEC 15444-1 shall be applied on a per-frame basis. The proposal
  for encapsulation of multiple frames in a non-DICOM manner in so-called 'Motion-JPEG' or 'M-JPEG'
  defined in 15444-3 is not used.
*/
bool
JPEG2000Codec::Decode(DataElement const & in, DataElement & out)
{
  if (NumberOfDimensions == 2)
  {
    const SequenceOfFragments * sf = in.GetSequenceOfFragments();
    const ByteValue *           j2kbv = in.GetByteValue();
    if (!sf && !j2kbv)
      return false;
    SmartPointer<SequenceOfFragments> sf_bug = new SequenceOfFragments;
    if (j2kbv)
    {
      mdcmWarningMacro("Pixel Data is not encapsulated correctly. Continuing anyway");
      assert(!sf);
      std::stringstream is;
      const size_t      j2kbv_len = j2kbv->GetLength();
      char *            mybuffer;
      try
      {
        mybuffer = new char[j2kbv_len];
      }
      catch (const std::bad_alloc &)
      {
        return false;
      }
      const bool b = j2kbv->GetBuffer(mybuffer, (unsigned long long)j2kbv_len);
      if (b)
        is.write(mybuffer, j2kbv_len);
      delete[] mybuffer;
      if (!b)
        return false;
      try
      {
        sf_bug->Read<SwapperNoOp>(is, true);
      }
      catch (...)
      {
        return false;
      }
      sf = &*sf_bug;
    }
    if (!sf)
      return false;
    std::stringstream        is;
    const unsigned long long totalLen = sf->ComputeByteLength();
    char *                   buffer;
    try
    {
      buffer = new char[totalLen];
    }
    catch (const std::bad_alloc &)
    {
      return false;
    }
    sf->GetBuffer(buffer, totalLen);
    is.write(buffer, totalLen);
    delete[] buffer;
    std::stringstream os;
    const bool        r = DecodeByStreams(is, os);
    if (!r)
      return false;
    out = in;
    std::string str = os.str();
    out.SetByteValue(&str[0], (uint32_t)str.size());
    return r;
  }
  else if (NumberOfDimensions == 3)
  {
    /* I cannot figure out how to use openjpeg to support multiframes
     * as encoded in DICOM
     * MM: Hack. If we are lucky enough the number of encapsulated fragments actually match
     * the number of Z frames.
     * MM: hopefully this is the standard so people are following it
     */
    const SequenceOfFragments * sf = in.GetSequenceOfFragments();
    if (!sf)
      return false;
    std::stringstream os;
    if (sf->GetNumberOfFragments() != Dimensions[2])
    {
      mdcmErrorMacro("Not handled");
      return false;
    }
    for (unsigned int i = 0; i < sf->GetNumberOfFragments(); ++i)
    {
      std::stringstream is;
      const Fragment &  frag = sf->GetFragment(i);
      if (frag.IsEmpty())
        return false;
      const ByteValue * bv = frag.GetByteValue();
      if (!bv)
        return false;
      const size_t bv_len = bv->GetLength();
      char *       mybuffer;
      try
      {
        mybuffer = new char[bv_len];
      }
      catch (const std::bad_alloc &)
      {
        return false;
      }
      bv->GetBuffer(mybuffer, bv->GetLength());
      is.write(mybuffer, bv->GetLength());
      delete[] mybuffer;
      const bool r = DecodeByStreams(is, os);
      if (!r)
        return false;
    }
    std::string str = os.str();
    assert(str.size());
    const unsigned long long str_size = str.size();
    if (str_size >= 0xffffffff)
    {
      mdcmAlwaysWarnMacro("JPEGLSCodec: value too big for ByteValue");
      return false;
    }
    out.SetByteValue(&str[0], (uint32_t)str_size);
    return true;
  }
  return false;
}

bool
JPEG2000Codec::Decode2(DataElement const & in, char * out_buffer, size_t len)
{
  if (NumberOfDimensions == 2)
  {
    const SequenceOfFragments * sf = in.GetSequenceOfFragments();
    const ByteValue *           j2kbv = in.GetByteValue();
    if (!sf && !j2kbv)
      return false;
    SmartPointer<SequenceOfFragments> sf_bug = new SequenceOfFragments;
    if (j2kbv)
    {
      mdcmWarningMacro("Pixel Data is not encapsulated correctly. Continuing anyway");
      assert(!sf);
      std::stringstream is;
      const size_t      j2kbv_len = j2kbv->GetLength();
      char *            mybuffer;
      try
      {
        mybuffer = new char[j2kbv_len];
      }
      catch (const std::bad_alloc &)
      {
        return false;
      }
      const bool b = j2kbv->GetBuffer(mybuffer, (unsigned long long)j2kbv_len);
      if (b)
        is.write(mybuffer, j2kbv_len);
      delete[] mybuffer;
      if (!b)
        return false;
      try
      {
        sf_bug->Read<SwapperNoOp>(is, true);
      }
      catch (...)
      {
        return false;
      }
      sf = &*sf_bug;
    }
    if (!sf)
      return false;
    std::stringstream        is;
    const unsigned long long totalLen = sf->ComputeByteLength();
    char *                   buffer;
    try
    {
      buffer = new char[totalLen];
    }
    catch (const std::bad_alloc &)
    {
      return false;
    }
    sf->GetBuffer(buffer, totalLen);
    is.write(buffer, totalLen);
    delete[] buffer;
    std::stringstream os;
    const bool        r = DecodeByStreams(is, os);
    if (!r)
      return false;
    os.seekg(0, std::ios::end);
    const size_t len2 = os.tellg();
    if (len != len2)
    {
      mdcmAlwaysWarnMacro("JPEG2000Codec::Decode2 len=" << len << " len2=" << len2);
      if (len > len2)
      {
        memset(out_buffer, 0, len);
      }
    }
    os.seekp(0, std::ios::beg);
#if 1
    std::stringbuf * pbuf = os.rdbuf();
    pbuf->sgetn(out_buffer, ((len < len2) ? len : len2));
#else
    const std::string & tmp0 = os.str();
    const char * tmp1 = tmp0.data();
    memcpy(
      out_buffer,
      tmp1,
      ((len < len2) ? len : len2));
#endif
    return r;
  }
  else if (NumberOfDimensions == 3)
  {
    /* I cannot figure out how to use openjpeg to support multiframes
     * as encoded in DICOM
     * MM: Hack. If we are lucky enough the number of encapsulated fragments actually match
     * the number of Z frames.
     * MM: hopefully this is the standard so people are following it
     */
    const SequenceOfFragments * sf = in.GetSequenceOfFragments();
    if (!sf)
      return false;
    std::stringstream os;
    if (sf->GetNumberOfFragments() != Dimensions[2])
    {
      mdcmErrorMacro("Not handled");
      return false;
    }
    for (unsigned int i = 0; i < sf->GetNumberOfFragments(); ++i)
    {
      std::stringstream is;
      const Fragment &  frag = sf->GetFragment(i);
      if (frag.IsEmpty())
        return false;
      const ByteValue * bv = frag.GetByteValue();
      if (!bv)
        return false;
      const size_t bv_len = bv->GetLength();
      char *       mybuffer;
      try
      {
        mybuffer = new char[bv_len];
      }
      catch (const std::bad_alloc &)
      {
        return false;
      }
      bv->GetBuffer(mybuffer, bv->GetLength());
      is.write(mybuffer, bv->GetLength());
      delete[] mybuffer;
      const bool r = DecodeByStreams(is, os);
      if (!r)
        return false;
    }
    os.seekg(0, std::ios::end);
    const size_t len2 = os.tellg();
    if (len != len2)
    {
      mdcmAlwaysWarnMacro("JPEG2000Codec::Decode2 len=" << len << " len2=" << len2);
      if (len > len2)
      {
        memset(out_buffer, 0, len);
      }
    }
    os.seekp(0, std::ios::beg);
#if 1
    std::stringbuf * pbuf = os.rdbuf();
    pbuf->sgetn(out_buffer, ((len < len2) ? len : len2));
#else
    const std::string & tmp0 = os.str();
    const char * tmp1 = tmp0.data();
    memcpy(
      out_buffer,
      tmp1,
      ((len < len2) ? len : len2));
#endif
    return true;
  }
  return false;
}

// Compress into JPEG
bool
JPEG2000Codec::Code(DataElement const & in, DataElement & out)
{
  out = in;
  // Create a Sequence Of Fragments
  SmartPointer<SequenceOfFragments> sq = new SequenceOfFragments;
  const unsigned int *              dims = this->GetDimensions();
  const int                         image_width = dims[0];
  const int                         image_height = dims[1];
  const ByteValue *                 bv = in.GetByteValue();
  if (!bv)
    return false;
  const char * input = bv->GetPointer();
  const size_t len = bv->GetLength();
  const size_t image_len = len / dims[2];
  const size_t inputlength = image_len;
  for (unsigned int dim = 0; dim < dims[2]; ++dim)
  {
    const char *      inputdata = input + dim * image_len;
    std::vector<char> rgbyteCompressed;
    rgbyteCompressed.resize(image_width * image_height * 4);
    size_t     cbyteCompressed;
    const bool b = this->CodeFrameIntoBuffer(
      (char *)&rgbyteCompressed[0], rgbyteCompressed.size(), cbyteCompressed, inputdata, inputlength);
    if (!b)
      return false;
    Fragment frag;
    assert(cbyteCompressed <= rgbyteCompressed.size()); // default alloc would be bogus
    frag.SetByteValue(&rgbyteCompressed[0], (uint32_t)cbyteCompressed);
    sq->AddFragment(frag);
  }
  assert(sq->GetNumberOfFragments() == dims[2]);
  out.SetValue(*sq);
  return true;
}

bool
JPEG2000Codec::GetHeaderInfo(std::istream & is, TransferSyntax & ts)
{
  is.seekg(0, std::ios::end);
  const size_t buf_size = is.tellg();
  char *       dummy_buffer;
  try
  {
    dummy_buffer = new char[buf_size];
  }
  catch (const std::bad_alloc &)
  {
    return false;
  }
  is.seekg(0, std::ios::beg);
  is.read(dummy_buffer, buf_size);
  const bool b = GetHeaderInfo(dummy_buffer, buf_size, ts);
  delete[] dummy_buffer;
  return b;
}

void
JPEG2000Codec::SetRate(unsigned int idx, double rate)
{
  Internals->coder_param.tcp_rates[idx] = (float)rate;
  if (Internals->coder_param.tcp_numlayers <= (int)idx)
  {
    Internals->coder_param.tcp_numlayers = idx + 1;
  }
  Internals->coder_param.cp_disto_alloc = 1;
}

double
JPEG2000Codec::GetRate(unsigned int idx) const
{
  return (double)Internals->coder_param.tcp_rates[idx];
}

void
JPEG2000Codec::SetQuality(unsigned int idx, double q)
{
  Internals->coder_param.tcp_distoratio[idx] = (float)q;
  if (Internals->coder_param.tcp_numlayers <= (int)idx)
  {
    Internals->coder_param.tcp_numlayers = idx + 1;
  }
  Internals->coder_param.cp_fixed_quality = 1;
}

double
JPEG2000Codec::GetQuality(unsigned int idx) const
{
  return (double)Internals->coder_param.tcp_distoratio[idx];
}

void
JPEG2000Codec::SetTileSize(unsigned int tx, unsigned int ty)
{
  Internals->coder_param.cp_tdx = tx;
  Internals->coder_param.cp_tdy = ty;
  Internals->coder_param.tile_size_on = true;
}

void
JPEG2000Codec::SetNumberOfResolutions(unsigned int nres)
{
  Internals->coder_param.numresolution = nres;
}

void
JPEG2000Codec::SetReversible(bool res)
{
  LossyFlag = !res;
  Internals->coder_param.irreversible = !res;
}

/*
"The JPEG 2000 bit stream specifies whether or not a reversible or
irreversible multi-component (color) transformation [ISO 15444-1 Annex G],
if any, has been applied. If no multi-component transformation has been
applied, then the components shall correspond to those specified by the
DICOM Attribute Photometric Interpretation (0028,0004). If the JPEG 2000
Part 1 reversible multi-component transformation has been applied then
the DICOM Attribute Photometric Interpretation (0028,0004) shall be YBR_RCT.
If the JPEG 2000 Part 1 irreversible multi-component transformation has been
applied then the DICOM Attribute Photometric Interpretation (0028,0004)
shall be YBR_ICT."

"... a Photometric Interpretation of RGB could be specified as long as no
multi-component transformation [ISO 15444-1 Annex G] was specified by the
JPEG 2000 bit stream"

Looks like -
for 3 samples/px:
Input RGB      + MCT 0 -> Photometric Interpretation RGB
Input YBR_FULL + MCT 0 -> Photometric Interpretation YBR_FULL
Input RGB      + MCT 1 -> Photometric Interpretation YBR_RCT or YBR_ICT
Input YBR_FULL + MCT 1 -> Not allowed

for 1 sample/px:
Input PI + MCT 0 -> PI
Input PI + MCT 1 -> Not allowed
*/
void
JPEG2000Codec::SetMCT(bool mct)
{
  // Set the Multiple Component Transformation value (COD -> SGcod)
  // 0 for none, 1 to apply to components 0, 1, 2
  if (mct) Internals->coder_param.tcp_mct = 1;
  else     Internals->coder_param.tcp_mct = 0;
}

bool
JPEG2000Codec::DecodeExtent(char *         buffer,
                            unsigned int   xmin,
                            unsigned int   xmax,
                            unsigned int   ymin,
                            unsigned int   ymax,
                            unsigned int   zmin,
                            unsigned int   zmax,
                            std::istream & is)
{
  BasicOffsetTable bot;
  bot.Read<SwapperNoOp>(is);
  const unsigned int * dimensions = this->GetDimensions();
  // retrieve pixel format after DecodeByStreamsCommon
  const PixelFormat pf = this->GetPixelFormat();
  assert(pf.GetBitsAllocated() % 8 == 0);
  assert(pf != PixelFormat::SINGLEBIT);
  assert(pf != PixelFormat::UINT12 && pf != PixelFormat::INT12);
  if (NumberOfDimensions == 2)
  {
    char *            dummy_buffer = NULL;
    std::vector<char> vdummybuffer;
    size_t            buf_size = 0;
    const Tag         seqDelItem(0xfffe, 0xe0dd);
    Fragment          frag;
    while (frag.ReadPreValue<SwapperNoOp>(is) && frag.GetTag() != seqDelItem)
    {
      const size_t fraglen = frag.GetVL();
      const size_t oldlen = vdummybuffer.size();
      if (fraglen == 0 && oldlen == 0)
        break;
      buf_size = fraglen + oldlen;
      vdummybuffer.resize(buf_size);
      dummy_buffer = &vdummybuffer[0];
      is.read(&vdummybuffer[oldlen], fraglen);
    }
    assert(frag.GetTag() == seqDelItem && frag.GetVL() == 0);
    assert(zmin == zmax);
    assert(zmin == 0);
    std::pair<char *, size_t> raw_len = this->DecodeByStreamsCommon(dummy_buffer, buf_size);
    if (!raw_len.first || !raw_len.second)
      return false;
    // check pixel format after DecodeByStreamsCommon
    const PixelFormat & pf2 = this->GetPixelFormat();
    // SC16BitsAllocated_8BitsStoredJ2K.dcm
    if (pf.GetSamplesPerPixel() != pf2.GetSamplesPerPixel() || pf.GetBitsAllocated() != pf2.GetBitsAllocated())
    {
      mdcmErrorMacro("Invalid PixelFormat found (mismatch DICOM vs J2K)");
      return false;
    }
    char *       raw = raw_len.first;
    const size_t rowsize = xmax - xmin + 1;
    const size_t colsize = ymax - ymin + 1;
    const size_t bytesPerPixel = pf.GetPixelSize();
    const char * tmpBuffer1 = raw;
    size_t       z = 0;
    for (size_t y = ymin; y <= ymax; ++y)
    {
      const size_t theOffset =
        (z * (size_t)dimensions[1] * (size_t)dimensions[0] + y * (size_t)dimensions[0] + xmin) * (size_t)bytesPerPixel;
      tmpBuffer1 = raw + theOffset;
      memcpy(&(buffer[((z - zmin) * rowsize * colsize + (y - ymin) * rowsize) * bytesPerPixel]),
             tmpBuffer1,
             rowsize * bytesPerPixel);
    }
    delete[] raw_len.first;
  }
  else if (NumberOfDimensions == 3)
  {
    const Tag           seqDelItem(0xfffe, 0xe0dd);
    Fragment            frag;
    std::streamoff      thestart = is.tellg();
    unsigned int        numfrags = 0;
    std::vector<size_t> offsets;
    while (frag.ReadPreValue<SwapperNoOp>(is) && frag.GetTag() != seqDelItem)
    {
      std::streamoff off = frag.GetVL();
      offsets.push_back((size_t)off);
      is.seekg(off, std::ios::cur);
      ++numfrags;
    }
    assert(frag.GetTag() == seqDelItem && frag.GetVL() == 0);
    assert(numfrags == offsets.size());
    if (numfrags != Dimensions[2])
    {
      mdcmErrorMacro("Not handled");
      return false;
    }
    for (unsigned int z = zmin; z <= zmax; ++z)
    {
      size_t curoffset = std::accumulate(offsets.begin(), offsets.begin() + z, size_t(0));
      is.seekg(thestart + curoffset + 8 * z, std::ios::beg);
      is.seekg(8, std::ios::cur);
      const size_t buf_size = offsets[z];
      char *       dummy_buffer;
      try
      {
        dummy_buffer = new char[buf_size];
      }
      catch (const std::bad_alloc &)
      {
        return false;
      }
      is.read(dummy_buffer, buf_size);
      std::pair<char *, size_t> raw_len = this->DecodeByStreamsCommon(dummy_buffer, buf_size);
      delete[] dummy_buffer;
      if (!raw_len.first || !raw_len.second)
        return false;
      const PixelFormat & pf2 = this->GetPixelFormat();
      if ((pf.GetSamplesPerPixel() != pf2.GetSamplesPerPixel()) || (pf.GetBitsAllocated() != pf2.GetBitsAllocated()))
      {
        return false;
      }
      char *       raw = raw_len.first;
      const size_t rowsize = xmax - xmin + 1;
      const size_t colsize = ymax - ymin + 1;
      const size_t bytesPerPixel = pf.GetPixelSize();
      const char * tmpBuffer1 = raw;
      for (size_t y = ymin; y <= ymax; ++y)
      {
        const size_t theOffset = 0 + (0 * dimensions[1] * dimensions[0] + y * dimensions[0] + xmin) * bytesPerPixel;
        tmpBuffer1 = raw + theOffset;
        memcpy(&(buffer[((z - zmin) * rowsize * colsize + (y - ymin) * rowsize) * bytesPerPixel]),
               tmpBuffer1,
               rowsize * bytesPerPixel);
      }
      delete[] raw_len.first;
    }
  }
  return true;
}

bool
JPEG2000Codec::DecodeByStreams(std::istream & is, std::ostream & os)
{
  // TODO may be could be done better?
  is.seekg(0, std::ios::end);
  const size_t buf_size = is.tellg();
  char *       dummy_buffer;
  try
  {
    dummy_buffer = new char[buf_size];
  }
  catch (const std::bad_alloc &)
  {
    return false;
  }
  is.seekg(0, std::ios::beg);
  is.read(dummy_buffer, buf_size);
  std::pair<char *, size_t> raw_len = this->DecodeByStreamsCommon(dummy_buffer, buf_size);
  /* free the memory containing the code-stream */
  delete[] dummy_buffer;
  if (!raw_len.first || !raw_len.second)
    return false;
  os.write(raw_len.first, raw_len.second);
  delete[] raw_len.first;
  return true;
}

bool
JPEG2000Codec::StartEncode(std::ostream &)
{
  return true;
}

bool
JPEG2000Codec::IsRowEncoder()
{
  return false;
}

bool
JPEG2000Codec::IsFrameEncoder()
{
  return true;
}

bool
JPEG2000Codec::AppendRowEncode(std::ostream &, const char *, size_t)
{
  return false;
}

bool
JPEG2000Codec::AppendFrameEncode(std::ostream & out, const char * data, size_t datalen)
{
  const unsigned int * dimensions = this->GetDimensions();
  const PixelFormat &  pf = this->GetPixelFormat();
  assert(datalen == dimensions[0] * dimensions[1] * pf.GetPixelSize());
  (void)pf;
  std::vector<char> rgbyteCompressed;
  rgbyteCompressed.resize(dimensions[0] * dimensions[1] * 4);
  size_t     cbyteCompressed;
  const bool b =
    this->CodeFrameIntoBuffer((char *)&rgbyteCompressed[0], rgbyteCompressed.size(), cbyteCompressed, data, datalen);
  if (!b)
    return false;
  out.write((char *)&rgbyteCompressed[0], cbyteCompressed);
  return true;
}

bool
JPEG2000Codec::StopEncode(std::ostream &)
{
  return true;
}

std::pair<char *, size_t>
JPEG2000Codec::DecodeByStreamsCommon(char * dummy_buffer, size_t buf_size)
{
  if (!dummy_buffer)
    return std::make_pair((char *)NULL, 0);
  opj_dparameters_t parameters;   // decompression parameters
  opj_codec_t *     dinfo = NULL; // handle to a decompressor
  opj_stream_t *    cio = NULL;
  opj_image_t *     image = NULL;
  unsigned char *   src = (unsigned char *)dummy_buffer;
  size_t file_length = buf_size;
  // WARNING: OpenJPEG is very picky when there is a trailing 00 at the end of the JPC
  // so we need to make sure to remove it:
  // See for example: DX_J2K_0Padding.dcm
  //             and D_CLUNIE_CT1_J2KR.dcm
  //  Marker 0xffd9 EOI End of Image (JPEG 2000 EOC End of codestream)
  // mdcmData/D_CLUNIE_CT1_J2KR.dcm contains a trailing 0xFF which apparently is ok
  while (file_length > 0 && src[file_length - 1] != 0xd9)
  {
    file_length--;
  }
  if (file_length < 1)
  {
    return std::make_pair((char *)NULL, 0);
  }
  if (src[file_length - 1] != 0xd9)
  {
    return std::make_pair((char *)NULL, 0);
  }
  /* set decoding parameters to default values */
  opj_set_default_decoder_parameters(&parameters);
  const char jp2magic[] = "\x00\x00\x00\x0C\x6A\x50\x20\x20\x0D\x0A\x87\x0A";
  if (memcmp(src, jp2magic, sizeof(jp2magic)) == 0)
  {
    /* JPEG-2000 compressed image data */
    // mdcmData/ELSCINT1_JP2vsJ2K.dcm
    // mdcmData/MAROTECH_CT_JP2Lossy.dcm
    mdcmWarningMacro("J2K start like JPEG-2000 compressed image data instead of codestream");
    parameters.decod_format = JP2_CFMT;
    assert(parameters.decod_format == JP2_CFMT);
  }
  else
  {
    /* JPEG-2000 codestream */
    parameters.decod_format = J2K_CFMT;
    assert(parameters.decod_format == J2K_CFMT);
  }
  parameters.cod_format = PGX_DFMT;
  assert(parameters.cod_format == PGX_DFMT);
  /* get a decoder handle */
  switch (parameters.decod_format)
  {
    case J2K_CFMT:
      dinfo = opj_create_decompress(CODEC_J2K);
      break;
    case JP2_CFMT:
      dinfo = opj_create_decompress(CODEC_JP2);
      break;
    default:
      mdcmErrorMacro("Error: parameters.decod_format");
      return std::make_pair<char *, size_t>((char *)NULL, 0);
  }
#if (OPJ_VERSION_MAJOR == 2 && OPJ_VERSION_MINOR >= 3)
  if (opj_has_thread_support())
  {
    opj_codec_set_threads(dinfo, Internals->nNumberOfThreadsForDecompression);
  }
#endif
  myfile   mysrc;
  myfile * fsrc = &mysrc;
  fsrc->mem = fsrc->cur = (char *)src;
  fsrc->len = file_length;
  OPJ_UINT32 * s[2];
  // the following hack is used for the file: DX_J2K_0Padding.dcm
  // see the function j2k_read_sot in openjpeg (line: 5946)
  // to deal with zero length Psot
  OPJ_UINT32 fl = file_length - 100;
  s[0] = &fl;
  s[1] = NULL;
  opj_set_error_handler(dinfo, mdcm_error_callback, s);
  cio = opj_stream_create_memory_stream(fsrc, OPJ_J2K_STREAM_CHUNK_SIZE, true);
  /* setup the decoder decoding parameters using user parameters */
  OPJ_BOOL bResult;
  bResult = opj_setup_decoder(dinfo, &parameters);
  if (!bResult)
  {
    opj_destroy_codec(dinfo);
    opj_stream_destroy(cio);
    mdcmErrorMacro("opj_setup_decoder failure");
    return std::make_pair<char *, size_t>(0, 0);
  }
  bResult = opj_read_header(cio, dinfo, &image);
  if (!bResult)
  {
    opj_destroy_codec(dinfo);
    opj_stream_destroy(cio);
    mdcmErrorMacro("opj_setup_decoder failure");
    return std::make_pair<char *, size_t>(0, 0);
  }
#if 0
  /* Optional if you want decode the entire image */
  opj_set_decode_area(dinfo, image, (OPJ_INT32)parameters.DA_x0,
     (OPJ_INT32)parameters.DA_y0, (OPJ_INT32)parameters.DA_x1, (OPJ_INT32)parameters.DA_y1);
#endif
  bResult = opj_decode(dinfo, cio, image);
  if (!bResult)
  {
    opj_destroy_codec(dinfo);
    opj_stream_destroy(cio);
    mdcmErrorMacro("opj_decode failed");
    return std::make_pair<char *, size_t>(0, 0);
  }
  bResult = bResult && (image != NULL);
  bResult = bResult && opj_end_decompress(dinfo, cio);
  if (!image || !check_comp_valid(image))
  {
    opj_destroy_codec(dinfo);
    opj_stream_destroy(cio);
    mdcmErrorMacro("opj_decode failed");
    return std::make_pair<char *, size_t>(0, 0);
  }
  bool reversible = false;
  bool lossless = false;
  bool mct = false;
  bool b = false;
  if (parameters.decod_format == JP2_CFMT)
  {
    b = parsejp2_imp(dummy_buffer, buf_size, &lossless, &mct);
  }
  else if (parameters.decod_format == J2K_CFMT)
  {
    b = parsej2k_imp(dummy_buffer, buf_size, &lossless, &mct);
  }
  else
  {
    mdcmAlwaysWarnMacro("Unhandled parameters.decod_format");
  }
  if (b)
    reversible = lossless;
  LossyFlag = !reversible;
  assert(image->numcomps == this->GetPixelFormat().GetSamplesPerPixel());
  assert(image->numcomps == this->GetPhotometricInterpretation().GetSamplesPerPixel());
#if 0
  std::cout << "GetPhotometricInterpretation() = "
    << this->GetPhotometricInterpretation()
    << ", mct = " << (int)mct << std::endl;
#endif
#if 0
  if(this->GetPhotometricInterpretation() == PhotometricInterpretation::RGB
#  if 1
     || this->GetPhotometricInterpretation() == PhotometricInterpretation::YBR_FULL
#  endif
    )
  {
    if(mct) mdcmAlwaysWarnMacro("JPEG2000 warning: (RGB || YBR_FULL), mct = " << (int)mct);
  }
  else if(this->GetPhotometricInterpretation() == PhotometricInterpretation::YBR_RCT ||
          this->GetPhotometricInterpretation() == PhotometricInterpretation::YBR_ICT)
  {
    if(!mct) mdcmAlwaysWarnMacro("JPEG2000 warning: (YBR_ICT || YBR_RCT), mct = " << (int)mct);
  }
#endif
  /* close the byte stream */
  opj_stream_destroy(cio);
  const size_t len = (size_t)Dimensions[0] * (size_t)Dimensions[1] * (PF.GetBitsAllocated() / 8) * image->numcomps;
  char *                   raw;
  try
  {
    raw = new char[len];
  }
  catch (const std::bad_alloc &)
  {
    return std::make_pair((char *)NULL, 0);
  }
  for (unsigned int compno = 0; compno < (unsigned int)image->numcomps; ++compno)
  {
    opj_image_comp_t * comp = &image->comps[compno];
    int                w = image->comps[compno].w;
    int                wr = int_ceildivpow2(image->comps[compno].w, image->comps[compno].factor);
    int                hr = int_ceildivpow2(image->comps[compno].h, image->comps[compno].factor);
    // ELSCINT1_JP2vsJ2K.dcm
    // -> prec = 12, bpp = 0, sgnd = 0
    if (comp->sgnd != PF.GetPixelRepresentation())
    {
      PF.SetPixelRepresentation((uint16_t)comp->sgnd);
    }
#ifndef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
    assert(comp->prec == PF.GetBitsStored()); // D_CLUNIE_RG3_JPLY.dcm
    assert(comp->prec - 1 == PF.GetHighBit());
#endif
    if (comp->prec != PF.GetBitsStored())
    {
      if (comp->prec <= 8)
        PF.SetBitsAllocated(8);
      else if (comp->prec <= 16)
        PF.SetBitsAllocated(16);
      else if (comp->prec <= 32)
        PF.SetBitsAllocated(32);
      PF.SetBitsStored((unsigned short)comp->prec);
      PF.SetHighBit((unsigned short)(comp->prec - 1));
    }
    assert(PF.IsValid());
    assert(comp->prec <= 32);
    if (comp->prec <= 8)
    {
      uint8_t * data8 = (uint8_t *)raw + compno;
      for (int i = 0; i < wr * hr; ++i)
      {
        int v = image->comps[compno].data[i / wr * w + i % wr];
        *data8 = (uint8_t)v;
        data8 += image->numcomps;
      }
    }
    else if (comp->prec <= 16)
    {
      // ELSCINT1_JP2vsJ2K.dcm is a 12bits image
      uint16_t * data16 = (uint16_t *)raw + compno;
      for (int i = 0; i < wr * hr; ++i)
      {
        int v = image->comps[compno].data[i / wr * w + i % wr];
        *data16 = (uint16_t)v;
        data16 += image->numcomps;
      }
    }
    else
    {
      uint32_t * data32 = (uint32_t *)raw + compno;
      for (int i = 0; i < wr * hr; ++i)
      {
        int v = image->comps[compno].data[i / wr * w + i % wr];
        *data32 = (uint32_t)v;
        data32 += image->numcomps;
      }
    }
  }
  /* free remaining structures */
  if (dinfo)
  {
    opj_destroy_codec(dinfo);
  }
  /* free image data structure */
  opj_image_destroy(image);
  return std::make_pair(raw, len);
}

bool
JPEG2000Codec::CodeFrameIntoBuffer(char *       outdata,
                                   size_t       outlen,
                                   size_t &     complen,
                                   const char * inputdata,
                                   size_t       inputlength)
{
  complen = 0;
#if 0
  if(NeedOverlayCleanup)
  {
    mdcmErrorMacro("TODO");
    return false;
  }
#endif
  const unsigned int * dims = this->GetDimensions();
  const int            image_width = dims[0];
  const int            image_height = dims[1];
  const PixelFormat &  pf = this->GetPixelFormat();
  const int            sample_pixel = pf.GetSamplesPerPixel();
  const int            bitsallocated = pf.GetBitsAllocated();
  const int            bitsstored = pf.GetBitsStored();
  const int            highbit = pf.GetHighBit();
  const int            sign = pf.GetPixelRepresentation();
  const int            quality = 100;
  // input_buffer is ONE image
  // fragment_size is the size of this image (fragment)
  int numZ = 0;
  (void)numZ;
  opj_cparameters_t parameters; /* compression parameters */
  opj_image_t *     image = NULL;
  memcpy(&parameters, &(Internals->coder_param), sizeof(parameters));
  if ((parameters.cp_disto_alloc || parameters.cp_fixed_alloc || parameters.cp_fixed_quality) &&
      (!(parameters.cp_disto_alloc ^ parameters.cp_fixed_alloc ^ parameters.cp_fixed_quality)))
  {
    mdcmErrorMacro("Error: options -r -q and -f cannot be used together.");
    return false;
  } /* mod fixed_quality */
  /* if no rate entered, lossless by default */
  if (parameters.tcp_numlayers == 0)
  {
    parameters.tcp_rates[0] = 0;
    parameters.tcp_numlayers = 1;
    parameters.cp_disto_alloc = 1;
  }
  if (parameters.cp_comment == NULL)
  {
    const char   comment[] = "Created by MDCM/OpenJPEG version %s";
    const char * vers = opj_version();
    parameters.cp_comment = (char *)malloc(strlen(comment) + 10);
    snprintf(parameters.cp_comment, strlen(comment) + 10, comment, vers);
    /* no need to delete parameters.cp_comment on exit */
  }
  // Compute the proper number of resolutions to use.
  // This is mostly done for images smaller than 64 pixels
  // along any dimension.
  unsigned int numberOfResolutions = 0;
  unsigned int tw = image_width >> 1;
  unsigned int th = image_height >> 1;
  while (tw && th)
  {
    numberOfResolutions++;
    tw >>= 1;
    th >>= 1;
  }
  // Clamp the number of resolutions to 6.
  if (numberOfResolutions > 6)
  {
    numberOfResolutions = 6;
  }
  parameters.numresolution = numberOfResolutions;
  /* decode the source image */
  image = rawtoimage((const char *)inputdata,
                     &parameters,
                     inputlength,
                     image_width,
                     image_height,
                     sample_pixel,
                     bitsallocated,
                     bitsstored,
                     highbit,
                     sign,
                     quality,
                     this->GetPlanarConfiguration());
  if (!image)
    return false;
  /* encode the destination image */
  parameters.cod_format = J2K_CFMT; /* J2K format output */
  size_t         codestream_length;
  opj_codec_t *  cinfo = NULL;
  opj_stream_t * cio = NULL;
  /* get a J2K compressor handle */
  cinfo = opj_create_compress(CODEC_J2K);
  /* setup the encoder parameters using the current image and using user parameters */
  opj_setup_encoder(cinfo, &parameters, image);
  myfile   mysrc;
  myfile * fsrc = &mysrc;
  char *   buffer_j2k; // overallocated
  try
  {
    buffer_j2k = new char[inputlength * 2];
  }
  catch (const std::bad_alloc &)
  {
    return false;
  }
  fsrc->mem = fsrc->cur = buffer_j2k;
  fsrc->len = 0; // inputlength
  /* open a byte stream for writing */
  /* allocate memory for all tiles */
  cio = opj_stream_create_memory_stream(fsrc, OPJ_J2K_STREAM_CHUNK_SIZE, false);
  if (!cio)
  {
    return false;
  }
  /* encode the image */
  bool ok = (opj_start_compress(cinfo, image, cio) == OPJ_TRUE) ? true : false;
  ok = ok && opj_encode(cinfo, cio);
  ok = ok && opj_end_compress(cinfo, cio);
  if (!ok)
  {
    opj_stream_destroy(cio);
    return false;
  }
  codestream_length = mysrc.len;
#if 0
  static int c = 0;
  std::ostringstream os;
  os << "/tmp/debug";
  os << c;
  c++;
  os << ".j2k";
  std::ofstream debug(os.str().c_str(), std::ios::binary);
  debug.write((char*)(cio->buffer), codestream_length);
  debug.close();
#endif
  ok = false;
  if (codestream_length <= outlen)
  {
    ok = true;
    memcpy(outdata, (char *)(mysrc.mem), codestream_length);
  }
  delete[] buffer_j2k;
  /* close and free the byte stream */
  opj_stream_destroy(cio);
  /* free remaining compression structures */
  opj_destroy_codec(cinfo);
  complen = codestream_length;
  /* free user parameters structure */
  if (parameters.cp_comment)
    free(parameters.cp_comment);
  if (parameters.cp_matrice)
    free(parameters.cp_matrice);
  /* free image data */
  opj_image_destroy(image);
  return ok;
}

bool
JPEG2000Codec::GetHeaderInfo(const char * dummy_buffer, size_t buf_size, TransferSyntax & ts)
{
  if (!dummy_buffer)
    return false;
  opj_dparameters_t     parameters;   // decompression parameters
  opj_codec_t *         dinfo = NULL; // handle to a decompressor
  opj_stream_t *        cio = NULL;
  opj_image_t *         image = NULL;
  const unsigned char * src = (const unsigned char *)dummy_buffer;
  const size_t          file_length = buf_size;
  /* set decoding parameters to default values */
  opj_set_default_decoder_parameters(&parameters);
  const char   jp2magic[] = "\x00\x00\x00\x0C\x6A\x50\x20\x20\x0D\x0A\x87\x0A";
  const size_t jp2magic_size = sizeof(jp2magic);
  if ((file_length >= jp2magic_size) && (memcmp(src, jp2magic, jp2magic_size) == 0))
  {
    // mdcmData/ELSCINT1_JP2vsJ2K.dcm
    mdcmWarningMacro("J2K starts like JPEG2000 image data instead of codestream");
    parameters.decod_format = JP2_CFMT;
  }
  else
  {
    parameters.decod_format = J2K_CFMT;
  }
  parameters.cod_format = PGX_DFMT;
  /* get a decoder handle */
  switch (parameters.decod_format)
  {
    case J2K_CFMT:
      dinfo = opj_create_decompress(CODEC_J2K);
      break;
    case JP2_CFMT:
      dinfo = opj_create_decompress(CODEC_JP2);
      break;
    default:
      return false;
  }
#if (OPJ_VERSION_MAJOR == 2 && OPJ_VERSION_MINOR >= 3)
  if (opj_has_thread_support())
  {
    opj_codec_set_threads(dinfo, Internals->nNumberOfThreadsForDecompression);
  }
#endif
  myfile   mysrc;
  myfile * fsrc = &mysrc;
  fsrc->mem = fsrc->cur = (char *)src;
  fsrc->len = file_length;
  // the hack is not used when reading meta-info of a j2k stream
  opj_set_error_handler(dinfo, mdcm_error_callback, NULL);
  cio = opj_stream_create_memory_stream(fsrc, OPJ_J2K_STREAM_CHUNK_SIZE, true);
  /* setup the decoder decoding parameters using user parameters */
  opj_setup_decoder(dinfo, &parameters);
  const bool bResult = opj_read_header(cio, dinfo, &image) ? true : false;
  if (!bResult)
  {
    opj_stream_destroy(cio);
    return false;
  }
  bool reversible = false;
  int  mct = 0;
  bool lossless = false;
  bool mctb;
  bool b = false;
  if (parameters.decod_format == JP2_CFMT)
    b = parsejp2_imp(dummy_buffer, buf_size, &lossless, &mctb);
  else if (parameters.decod_format == J2K_CFMT)
    b = parsej2k_imp(dummy_buffer, buf_size, &lossless, &mctb);
  if (b)
  {
    reversible = lossless;
    mct = mctb;
#if 0
    std::cout << "MCT: " << (int)mct << " reversible: " << (int)reversible << std::endl;
#endif
  }
#if 0
  else
  {
    std::cout << "Warning: parameters.decod_format=" << parameters.decod_format << std::endl;
  }
#endif
  LossyFlag = !reversible;
  opj_image_comp_t * comp = &image->comps[0];
  if (!check_comp_valid(image))
  {
    mdcmErrorMacro("Validation failed");
    return false;
  }
  this->Dimensions[0] = comp->w;
  this->Dimensions[1] = comp->h;
  if (comp->prec <= 8)
  {
    this->PF = PixelFormat(PixelFormat::UINT8);
  }
  else if (comp->prec <= 16)
  {
    this->PF = PixelFormat(PixelFormat::UINT16);
  }
  else if (comp->prec <= 32)
  {
    this->PF = PixelFormat(PixelFormat::UINT32);
  }
  this->PF.SetBitsStored((unsigned short)comp->prec);
  this->PF.SetHighBit((unsigned short)(comp->prec - 1));
  this->PF.SetPixelRepresentation((unsigned short)comp->sgnd);
  if (image->numcomps == 1)
  {
    // usually we have codec only, but in some case we have a JP2 with
    // color space info:
    // - mdcmData/MAROTECH_CT_JP2Lossy.dcm
    // - mdcmData/D_CLUNIE_CT1_J2KI.dcm -> color_space = 32767
    PI = PhotometricInterpretation::MONOCHROME2;
    this->PF.SetSamplesPerPixel(1);
  }
  else if (image->numcomps == 3)
  {
    /*
    8.2.4 JPEG 2000 IMAGE COMPRESSION
    The JPEG 2000 bit stream specifies whether or not a reversible or irreversible
    multi-component (color) transformation, if any, has been applied. If no
    multi-component transformation has been applied, then the components shall
    correspond to those specified by the DICOM Attribute Photometric Interpretation
    (0028,0004). If the JPEG 2000 Part 1 reversible multi-component transformation
    has been applied then the DICOM Attribute Photometric Interpretation
    (0028,0004) shall be YBR_RCT. If the JPEG 2000 Part 1 irreversible
    multi-component transformation has been applied then the DICOM Attribute
    Photometric Interpretation (0028,0004) shall be YBR_ICT.  Notes: 1. For
    example, single component may be present, and the Photometric Interpretation
    (0028,0004) may be MONOCHROME2.  2. Though it would be unusual, would not take
    advantage of correlation between the red, green and blue components, and would
    not achieve effective compression, a Photometric Interpretation of RGB could be
    specified as long as no multi-component transformation was specified by the
    JPEG 2000 bit stream.  3. Despite the application of a multi-component color
    transformation and its reflection in the Photometric Interpretation attribute,
    the color space remains undefined.  There is currently no means of conveying
    standard color spaces either by fixed values (such as sRGB) or by ICC
    profiles. Note in particular that the JP2 file header is not sent in the JPEG
    2000 bitstream that is encapsulated in DICOM.
    */
    if (mct)
      PI = PhotometricInterpretation::YBR_RCT;
    else
      PI = PhotometricInterpretation::RGB;
    this->PF.SetSamplesPerPixel(3);
  }
  else if (image->numcomps == 4)
  {
    // http://www.crc.ricoh.com/~gormish/jpeg2000conformance/
    // jpeg2000testimages/Part4TestStreams/codestreams_profile0/p0_06.j2k
    mdcmErrorMacro("Image has 4 components, it is not supported in DICOM (ARGB is retired)");
    return false;
  }
  else
  {
    // http://www.crc.ricoh.com/~gormish/jpeg2000conformance/
    // jpeg2000testimages/Part4TestStreams/codestreams_profile0/p0_13.j2k
    mdcmErrorMacro("Image has " << image->numcomps << " components, it is not supported in DICOM");
    return false;
  }
#if 0
  switch(image->color_space)
  {
  case CLRSPC_GRAY:
    std::cout << "CLRSPC_GRAY" << std::endl;
    break;
  case CLRSPC_SRGB:
    std::cout << "CLRSPC_SRGB" << std::endl;
    break;
  case CLRSPC_CMYK:
    std::cout << "CLRSPC_CMYK" << std::endl;
    break;
  case CLRSPC_EYCC:
    std::cout << "CLRSPC_EYCC" << std::endl;
    break;
  case CLRSPC_SYCC:
    std::cout << "CLRSPC_SYCC" << std::endl;
    break;
  case CLRSPC_UNKNOWN:
    std::cout << "CLRSPC_UNKNOWN" << std::endl;
    break;
  case CLRSPC_UNSPECIFIED:
    std::cout << "CLRSPC_UNSPECIFIED" << std::endl;
    break;
  default:
    std::cout << "Colospace unknown: " << (unsigned int)image->color_space << std::endl;
    break;
  }
#endif
  assert(PI != PhotometricInterpretation::UNKNOWN);
  const bool bmct = false; // FIXME
  if (bmct)
  {
    if (reversible)
    {
      ts = TransferSyntax::JPEG2000Part2Lossless;
    }
    else
    {
      ts = TransferSyntax::JPEG2000Part2;
      if (PI == PhotometricInterpretation::YBR_RCT)
      {
        // FIXME
        PI = PhotometricInterpretation::YBR_ICT;
      }
    }
  }
  else
  {
    if (reversible)
    {
      ts = TransferSyntax::JPEG2000Lossless;
    }
    else
    {
      ts = TransferSyntax::JPEG2000;
      if (PI == PhotometricInterpretation::YBR_RCT)
      {
        // FIXME
        PI = PhotometricInterpretation::YBR_ICT;
      }
    }
  }
  if (this->GetPhotometricInterpretation().IsLossy())
  {
    assert(ts.IsLossy());
  }
  if (!ts.IsLossy())
  {
    assert(this->GetPhotometricInterpretation().IsLossless());
  }
  /* close the byte stream */
  opj_stream_destroy(cio);
  /* free remaining structures */
  if (dinfo)
  {
    opj_destroy_codec(dinfo);
  }
  /* free image data structure */
  opj_image_destroy(image);
  return true;
}

} // end namespace mdcm
