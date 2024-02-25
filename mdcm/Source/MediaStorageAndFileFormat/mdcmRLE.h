/*********************************************************
 *
 * MDCM
 *
 * Modifications github.com/issakomi
 *
 *********************************************************/

/*=========================================================================

  Program: librle, a minimal RLE library for DICOM

  Copyright (c) 2014 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef MDCMRLE_RLE_H
#define MDCMRLE_RLE_H

#include <stdexcept>
#include <cassert>
#include <vector>
#include <iostream>
#include "mdcmTypes.h"

namespace mdcm
{

struct RLEHeader
{
  unsigned int num_segments;
  unsigned int offset[15];
};

// 1 or 3 components and bpp being 8, 16 or 32
class RLEPixelInfo
{
public:
  RLEPixelInfo(unsigned int nc, unsigned int bpp)
    : number_components(nc), bits_per_pixel(bpp)
  {
    if (!(nc == 1 || nc == 3))
    {
      throw std::runtime_error("RLE: invalid samples per pixel");
    }
    if (!(bpp == 8 || bpp == 16 || bpp == 32))
    {
      throw std::runtime_error("RLE: invalid bits per pixel");
    }
  }

  unsigned int
  compute_num_segments() const
  {
    assert(bits_per_pixel % 8 == 0 && (number_components == 1 || number_components == 3));
    const unsigned int mult = bits_per_pixel / 8;
    const unsigned int res = number_components * mult;
    assert(res <= 12 && res > 0);
    return res;
  }

  unsigned int
  get_number_of_components() const
  {
    return number_components;
  }

  unsigned int
  get_number_of_bits_per_pixel() const
  {
    return bits_per_pixel;
  }

  static bool
  check_num_segments(const unsigned int num_segments)
  {
    bool ok{};
    switch (num_segments)
    {
      case 1:  // 1  -> Grayscale 8-bit
      case 2:  // 2  -> Grayscale 16-bit
      case 3:  // 3  -> RGB  8-bit
      case 4:  // 4  -> Grayscale 32-bit
      case 6:  // 6  -> RGB 16-bit
      case 12: // 12 -> RGB 32-bit
        ok = true;
        break;
    }
    return ok;
  }

private:
  unsigned char number_components;
  unsigned char bits_per_pixel;
};

class RLEImageInfo
{
public:
  RLEImageInfo(unsigned int         w = 0U,
               unsigned int         h = 0U,
               const RLEPixelInfo & pi = RLEPixelInfo(1U, 8U),
               bool                 pc = false,
               bool                 le = true)
    : width(w), height(h), pix(pi), planarconfiguration(pc), littleendian(le)
  {
    if (pc && pix.get_number_of_components() != 3)
    {
      throw std::runtime_error("RLE: invalid planar configuration");
    }
  }

  unsigned int
  get_width() const
  {
    return width;
  }

  unsigned int
  get_height() const
  {
    return height;
  }

  RLEPixelInfo
  get_RLEPixelInfo() const
  {
    return pix;
  }

  bool
  get_planar_configuration() const
  {
    return planarconfiguration;
  }

  bool
  is_little_endian() const
  {
    return littleendian;
  }

private:
  unsigned int width;
  unsigned int height;
  RLEPixelInfo pix;
  bool         planarconfiguration;
  bool         littleendian;
};

class RLESource
{
public:
  // This function reads in 'len' bytes so that 'out' contains N pixel
  // spread into chunks.
  // E.g. for a RGB 8-bit input, 'out' will contain RRRRR ... GGGGG .... BBBBB.
  bool
  read_into_segments(char * out, size_t len, const RLEImageInfo & ii)
  {
    RLEPixelInfo pt = ii.get_RLEPixelInfo();
    const unsigned int nc = pt.get_number_of_components();
    const unsigned int bpp = pt.get_number_of_bits_per_pixel();
    const unsigned int numsegs = pt.compute_num_segments();
    const unsigned int npadded = bpp / 8U;
    if (numsegs == 1)
    {
      const size_t nvalues = read(out, len);
      assert(nvalues == len);
      (void)nvalues;
    }
    else
    {
      assert(len % numsegs == 0);
      if (ii.get_planar_configuration() == 0)
      {
        const size_t llen = len / numsegs;
        char *       sbuf[12]; // max possible is 12
        for (size_t s = 0; s < numsegs; ++s)
        {
          sbuf[s] = out + s * llen;
        }
        char values[12];
        for (size_t l = 0; l < llen; ++l)
        {
          const size_t nvalues = read(values, numsegs);
          assert(nvalues == numsegs);
          (void)nvalues;
          for (size_t c = 0; c < nc; ++c)
          {
            for (size_t p = 0; p < npadded; ++p)
            {
              const size_t i = p + c * npadded;
              const size_t j = (npadded - 1 - p) + c * npadded; // little endian
              *sbuf[i]++ = values[j];
            }
          }
        }
      }
      else
      {
        if (numsegs == 3)
        {
          const size_t llen = len / numsegs;
          assert(ii.get_width() == llen);
          std::streamoff plane = static_cast<std::streamoff>(ii.get_width()) * ii.get_height();
          std::streampos pos = tell();
          size_t         nvalues = read(out, llen);
          assert(nvalues == llen);
          bool b = seek(pos + plane);
          assert(b);
          nvalues = read(out + llen, llen);
          assert(nvalues == llen);
          b = seek(pos + 2 * plane);
          assert(b);
          nvalues = read(out + 2 * llen, llen);
          assert(nvalues == llen);
          b = seek(pos + static_cast<std::streamoff>(llen));
          assert(b);
          (void)nvalues;
          (void)b;
        }
        else
        {
          return false;
        }
      }
    }
    return true;
  }

  virtual size_t
  read(char *, size_t) = 0;

  virtual std::streampos
  tell() = 0;

  virtual bool
  seek(std::streampos) = 0;

  virtual bool
  eof() = 0;

  virtual RLESource *
  clone() = 0;

  virtual ~RLESource() = default;
};

class RLEDestination
{
public:
  virtual size_t
  write(const char *, size_t) = 0;

  virtual bool
  seek(std::streampos) = 0;
  
  virtual ~RLEDestination() = default;
};

struct RLEEncoderInternal
{
  RLEImageInfo      img;
  RLEHeader         rh;
  RLESource *       src;
  long long         comp_pos[16];
  std::vector<char> invalues;
  std::vector<char> outvalues;
};

struct RLEDecoderInternal
{
  RLEImageInfo      img;
  RLEHeader         rh;
  RLESource **      sources;
  long long         nsources;
  std::vector<char> scanline;
  // Row crossing handling, some RLE encoder are brain dead and do cross the
  // row boundary which makes it very difficult to handle in our case since we
  // have a strict requirement of only decoding on a per-row basis.
  // Furthermore this memory storage should handle all possible segments (max 15).
  char      cross_row[16][128];
  long long nstorage[16]; // number of stored bytes from previous run
};

class RLEEncoder
{
public:
  RLEEncoder(RLESource &, const RLEImageInfo &);
  ~RLEEncoder();
  long long
  encode_row(RLEDestination &);
  // Compute and write header to dest.
  // This call is actually the main design of the encoder, since we are
  // encoding on a per line basis (well this apply for the general case where
  // bpp != 8 actually). The underlying implementation will parse once the
  // whole input to be able to compute offsets. Those offsets will then be used
  // later on to actually store the data. The RLE encoder is thus a two passes
  // process (repeats).
  bool
  write_header(RLEDestination &);
  static long long
  count_nonrepetitive_bytes(const char *, long long);

private:
  long long
  compute_compressed_length(const char *, long long);
  long long
  encode_row_internal(char *, long long, const char *, long long);
  RLEEncoderInternal * internals;
};

// This is a limited implementation this decoder is only capable of generating
// Planar Configuration = 0 output.
class RLEDecoder
{
public:
  typedef unsigned long long streamsize_t;
  // Decode all the scanlines of the image. The code simply call decode_row on
  // all scanlines.
  // Return the size of bytes written (= height * get_row_nbytes())
  streamsize_t
  decode_frame(RLEDestination &);
  // Instead of decoding an entire row, simply skip it. May return an error
  // that indicate the stream is not valid or not does not respect row
  // boundaries crossing.
  bool
  skip_row();
  // Only decompress a single row at a time. returns number of bytes written.
  // some malformed RLE stream may cross the row-boundaries in which case it
  // makes it hard to decode a single row at a time.
  // an extra memory will be used to handle those cases.
  long long
  decode_row(RLEDestination &);
  // Read the RLE header.
  // return true on success / false on error. Upon success the value nc / bpp
  // will contains the number of components and number of bits per pixel used to
  // encode the stream
  bool
  read_header(RLEPixelInfo &);
  // Compute the actual number of bytes a single row should hold.
  // Return width * sizeof(pixel)
  long long
  get_row_nbytes() const;
  RLEDecoder(RLESource &, const RLEImageInfo &);
  ~RLEDecoder();

private:
  RLEDecoderInternal * internals;
};

}

#endif
