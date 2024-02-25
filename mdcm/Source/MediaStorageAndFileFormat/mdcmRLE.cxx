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

#include "mdcmRLE.h"
#include <vector>
#include <algorithm>
#include <cstring>
#include "mdcmTrace.h"

namespace
{

constexpr unsigned int max_number_offset = 15U;

// check_header behavior is twofold:
//  - on first pass it will detect if user input was found to be invalid and
// update RLEPixelInfo accordingly.
// - on the second pass when the user update with the proper RLEPixelInfo, then
// the code will fails if the remaining of the header was found to be invalid
// as per DICOM spec.
bool
check_header(const mdcm::RLEHeader & rh, mdcm::RLEPixelInfo & pt)
{
  // first operation is to update RLEPixelInfo from the header
  const unsigned int ns = pt.compute_num_segments();
  const bool         ok = ns == rh.num_segments;
  if (!ok)
  {
    // in case num segments is valid, update pt from the derived info
    if (mdcm::RLEPixelInfo::check_num_segments(rh.num_segments))
    {
      unsigned int nc, bpp;
      if (ns % 3 == 0)
      {
        nc = 3U;
        bpp = (ns / 3) * 8;
      }
      else
      {
        nc = 1U;
        bpp = ns;
      }
      pt = mdcm::RLEPixelInfo(nc, bpp);
    }
    return false;
  }
  // at least one offset is required. By convention in DICOM, it should not be
  // padded (no extra blank space), thus value is offset 64
  if (rh.offset[0] != 64)
    return false;
  for (unsigned int i = 1; i < rh.num_segments; ++i)
  {
    // basic error checking
    if (rh.offset[i - 1] >= rh.offset[i])
      return false;
  }
  // DICOM mandates all unused segments to have their offsets be 0
  for (unsigned int i = rh.num_segments; i < max_number_offset; ++i)
  {
    if (rh.offset[i] != 0)
      return false;
  }
  return true;
}

/*
G.3 THE RLE ALGORITHM
The RLE algorithm described in this section is used to compress Byte Segments into RLE Segments.
There is a one-to-one correspondence between Byte Segments and RLE Segments. Each RLE segment
must be an even number of bytes or padded at its end with zero to make it even.

G.3.1 The RLE encoder
A sequence of identical bytes (Replicate Run) is encoded as a two-byte code:

  < -count + 1 > <byte value>, where

count = the number of bytes in the run, and 2 <= count <= 128

and a non-repetitive sequence of bytes (Literal Run) is encoded as:

  < count - 1 > <Literal sequence of bytes>, where

count = number of bytes in the sequence, and 1 <= count <= 128.

The value of -128 may not be used to prefix a byte value.
Note: It is common to encode a 2-byte repeat run as a Replicate Run except when preceded and followed by
a Literal Run, in which case it's best to merge the three runs into a Literal Run.
 - Three-byte repeats shall be encoded as Replicate Runs.
 - Each row of the image shall be encoded separately and not cross a row boundary.
*/
long long
count_identical_bytes(const char * start, long long len)
{
  assert(len > 0);
  const char      ref = start[0];
  long long       count{1LL};
  const long long cmin = std::min(128LL, len);
  while (count < cmin && start[count] == ref)
  {
    ++count;
  }
  assert(1 <= count && count <= 128);
  return count;
}



bool
skip_row_internal(mdcm::RLESource & s, const long long width)
{
  long long numOutBytes{};
  char      b;
  bool      re{}; // read error
  while (numOutBytes < width && !re && !s.eof())
  {
    const long long check = s.read(&b, 1);
    if (check != 1)
      re = true;
    if (b >= 0 /*&& b <= 127*/) /* 2nd is always true */
    {
      char            buffer[128];
      const long long nbytes = s.read(buffer, b + 1);
      if (nbytes != b + 1)
        re = true;
      numOutBytes += nbytes;
    }
    else if (b <= -1 && b >= -127)
    {
      char            nextByte;
      const long long nbytes = s.read(&nextByte, 1);
      if (nbytes != 1)
        re = true;
      numOutBytes += -b + 1;
    }
    /* else b == -128 */
  }
  return numOutBytes == width && !re && !s.eof() ? true : false;
}

void
memcpy_withstride(char * output, const char * input, size_t len, long long stride_idx, long long nstride)
{
  assert(nstride >= 0);
  if (nstride == 0)
  {
    assert(stride_idx == 0);
    memcpy(output, input, len);
  }
  else
  {
    for (size_t i = 0; i < len; ++i)
    {
      output[nstride * i + stride_idx] = input[i];
    }
  }
}

long long
decode_internal(char *            output,
                mdcm::RLESource & s,
                const long long   maxlen,
                const long long   stride_idx,
                const long long   nstride,
                char *            cross_row,
                long long &       nstorage)
{
  assert(output && cross_row && maxlen > 0 && nstorage >= 0);
  long long numOutBytes{};
  char *    cur = output;
  // Initialize from previous RLE run
  if (nstorage)
  {
    memcpy_withstride(cur, cross_row, nstorage, stride_idx, nstride);
    cur += nstride * nstorage;
    numOutBytes += nstorage;
  }
  // Real RLE
  while (numOutBytes < maxlen && !s.eof())
  {
    char         buffer[128];
    char         b;
    const size_t check = s.read(&b, 1);
    assert(check == 1);
    (void)check;
    if (b >= 0 /*&& b <= 127*/) // 2nd is always true
    {
      long long nbytes = s.read(buffer, b + 1);
      if (nbytes != b + 1)
      {
        assert(s.eof());
        break;
      }
      assert((cur - output) % nstride == 0);
      const long long diff = (cur - output) / nstride + nbytes - maxlen;
      if (diff > 0) // handle row crossing artefacts
      {
        nbytes -= diff;
        memcpy(cross_row, buffer + nbytes, diff); // actual memcpy
        nstorage = diff;
        assert(numOutBytes + nbytes == maxlen);
      }
      memcpy_withstride(cur, buffer, nbytes, stride_idx, nstride);
      cur += nbytes * nstride;
      numOutBytes += nbytes;
    }
    else if (b <= -1 && b >= -127)
    {
      char         nextByte;
      const size_t nbytes = s.read(&nextByte, 1);
      assert(nbytes == 1);
      (void)nbytes;
      long long nrep = -b + 1; // number of repetitions
      memset(buffer, nextByte, nrep);
      assert((cur - output) % nstride == 0);
      const long long diff = (cur - output) / nstride + nrep - maxlen;
      if (diff > 0)
      {
        nrep -= diff;
        memcpy(cross_row, buffer + nrep, diff);
        nstorage = diff;
        assert(numOutBytes + nrep == maxlen);
      }
      memcpy_withstride(cur, buffer, nrep, stride_idx, nstride);
      cur += nrep * nstride;
      numOutBytes += nrep;
    }
    /* else b == -128 */
  }
  assert(cur - output == nstride * numOutBytes);
  assert(numOutBytes <= maxlen); // ALOKA_SSD-8-MONO2-RLE-SQ.rle
  return numOutBytes;
}

}

namespace mdcm
{

RLEEncoder::RLEEncoder(RLESource & s, const RLEImageInfo & ii) : internals(nullptr)
{
  if (!ii.is_little_endian())
  {
    throw std::runtime_error("RLE: big endian is not supported");
  }
  internals = new RLEEncoderInternal;
  internals->img = ii;
  internals->src = s.clone();
  memset(reinterpret_cast<char *>(&internals->rh), 0, sizeof(RLEHeader));
}

RLEEncoder::~RLEEncoder()
{
  delete internals->src;
  delete internals;
}

bool
RLEEncoder::write_header(RLEDestination & d)
{
  RLESource *        src = internals->src;
  const unsigned int w = internals->img.get_width();
  const unsigned int h = internals->img.get_height();
  RLEPixelInfo       pt = internals->img.get_RLEPixelInfo();
  const unsigned int nsegs = pt.compute_num_segments();
  internals->invalues.resize(w * nsegs);
  char *      buffer = &internals->invalues[0];
  size_t      buflen = internals->invalues.size();
  RLEHeader & rh = internals->rh;
  rh.num_segments = nsegs;
  std::streampos start = src->tell();  // remember start position
  long long      comp_len[16]{}; // 15 is the max
  for (long long y = 0; y < h; ++y)
  {
    src->read_into_segments(buffer, buflen, internals->img);
    for (unsigned int s = 0; s < nsegs; ++s)
    {
      const long long ret = compute_compressed_length(buffer + s * w, w);
      assert(ret > 0);
      comp_len[s] += ret;
    }
  }
  rh.offset[0] = 64; // required
  for (unsigned int s = 1; s < nsegs; ++s)
  {
    const long long tmp0 = comp_len[s - 1] + rh.offset[s - 1];
    if (tmp0 < 0 || tmp0 > 0xffffffff)
    {
      mdcmAlwaysWarnMacro("Overflow in RLEEncoder::write_header, " << tmp0);
      return false;
    }
    rh.offset[s] += static_cast<unsigned int>(tmp0);
  }
  assert(check_header(rh, pt));
  d.write(reinterpret_cast<char *>(&rh), sizeof(rh));
  long long            comp_pos[16]{};
  const unsigned int * offsets = internals->rh.offset;
  for (unsigned int s = 0; s < nsegs; ++s)
  {
    comp_pos[s] = offsets[s];
  }
  memcpy(internals->comp_pos, comp_pos, sizeof(comp_pos));
  src->seek(start); // go back to start position
  return true;
}

// After a long debate
// There is no single possible solution. I need to compress in two passes. One
// will computes the offset the second one will do the writing. Since the
// offset are precomputed this should limit the writing of data back-n-forth
long long
RLEEncoder::compute_compressed_length(const char * source, long long sourcelen)
{
  long long    pout{};
  const char * pin = source;
  long long    length = sourcelen;
  while (pin != source + sourcelen)
  {
    assert(length <= sourcelen);
    assert(pin <= source + sourcelen);
    long long count = count_identical_bytes(pin, length);
    if (count > 1)
    {
      // Repeat case
      ++pout;
      ++pout;
    }
    else
    {
      // Non-repeat case
      // OK need to compute non-repeat
      count = count_nonrepetitive_bytes(pin, length);
      ++pout;
      pout += count;
    }
    // Count byte where read, move pin to new position
    pin += count;
    // Compute remaining length
    assert(count <= length);
    length -= count;
  }
  return pout;
}

long long
RLEEncoder::count_nonrepetitive_bytes(const char * start, long long len)
{
  // This version properly encode: 0 1 1 0 as: 3 0 1 1 0
  assert(start && len > 0);
  const long long cmin = std::min(128LL, len);
  long long       count{1LL};
  for (; count < cmin; ++count)
  {
    if (start[count] == start[count - 1])
    {
      if (count + 1 < cmin && start[count] != start[count + 1])
      {
        continue;
      }
      // 'count' can go negative
      --count;
      break;
    }
  }
  assert(1LL <= count && count <= 128LL);
  return count;
}

long long
RLEEncoder::encode_row(RLEDestination & d)
{
  RLESource *          src = internals->src;
  const long long      w = internals->img.get_width();
  const RLEPixelInfo & pt = internals->img.get_RLEPixelInfo();
  const long long      nc = pt.get_number_of_components();
  const long long      bpp = pt.get_number_of_bits_per_pixel();
  const long long      numsegs = internals->rh.num_segments;
  assert(numsegs == (bpp / 8) * nc);
  (void)bpp;
  (void)nc;
  internals->invalues.resize(w * numsegs);
  internals->outvalues.resize(w * 2); // Worst possible case?
  src->read_into_segments(&internals->invalues[0], internals->invalues.size(), internals->img);
  long long * comp_pos = internals->comp_pos;
  long long   n{};
  for (long long s = 0; s < numsegs; ++s)
  {
    const long long ret =
      encode_row_internal(&internals->outvalues[0], internals->outvalues.size(), &internals->invalues[0] + s * w, w);
    if (ret < 0)
      return -1;
    n += ret;
    d.seek(comp_pos[s]);
    d.write(&internals->outvalues[0], ret);
    comp_pos[s] += ret;
  }
  return n;
}

long long
RLEEncoder::encode_row_internal(char * dest, long long destlen, const char * source, long long sourcelen)
{
  char *       pout = dest;
  const char * pin = source;
  long long    length = sourcelen;
  while (pin != source + sourcelen)
  {
    assert(length <= sourcelen);
    assert(pin <= source + sourcelen);
    long long count = count_identical_bytes(pin, length);
    if (count > 1)
    {
      // Test first we are allowed to write two bytes
      if (pout + 1 + 1 > dest + destlen)
        return -1;
      *pout = static_cast<char>(-count + 1);
      assert(*pout <= -1 && *pout >= -127);
      ++pout;
      *pout = *pin;
      ++pout;
    }
    else
    {
      // Non-repeat case
      count = count_nonrepetitive_bytes(pin, length);
      // First test: is it allowed to write 1 + count bytes in the output buffer
      if (pout + count + 1 > dest + destlen)
        return -1;
      *pout = static_cast<char>(count - 1);
      assert(*pout != -128 && *pout + 1 == count);
      assert(*pout >= 0);
      ++pout;
      memcpy(pout, pin, count);
      pout += count;
    }
    pin += count;
    assert(count <= length);
    length -= count;
  }
  return pout - dest;
}

RLEDecoder::RLEDecoder(RLESource & s, const RLEImageInfo & ii)
  : internals(nullptr)
{
  internals = new RLEDecoderInternal;
  memset(reinterpret_cast<char *>(&internals->rh), 0, sizeof(RLEHeader));
  internals->img = ii;
  const unsigned int ns = ii.get_RLEPixelInfo().compute_num_segments();
  internals->sources = new RLESource *[ns];
  internals->sources[0] = s.clone(); // Only one for now (minimum for read_header)
  for (unsigned int i = 1; i < ns; ++i)
    internals->sources[i] = nullptr;
  internals->nsources = ns;
  for (unsigned int i = 0; i < 16; ++i)
    internals->nstorage[i] = 0;
}

RLEDecoder::~RLEDecoder()
{
  for (long long i = 0; i < internals->nsources; ++i)
  {
    delete internals->sources[i];
  }
  delete[] internals->sources;
  delete internals;
}

bool
RLEDecoder::skip_row()
{
  for (long long i = 0; i < internals->nsources; ++i)
  {
    RLESource * s = internals->sources[i];
    bool        b = skip_row_internal(*s, internals->img.get_width());
    if (!b)
      return false;
  }
  return true;
}

long long
RLEDecoder::decode_row(RLEDestination & d)
{
  const RLEPixelInfo & pt = internals->img.get_RLEPixelInfo();
  const long long      nc = pt.get_number_of_components();
  const long long      bpp = pt.get_number_of_bits_per_pixel();
  const long long      nsegs = pt.compute_num_segments();
  const long long      npadded = bpp / 8;
  assert(internals->nsources == nsegs);
  const size_t scanlen = internals->img.get_width() * nsegs;
  internals->scanline.resize(scanlen);
  char *    scanbuf = &internals->scanline[0];
  long long numOutBytesFull = 0;
  for (long long c = 0; c < nc; ++c)
  {
    for (long long p = 0; p < npadded; ++p)
    {
      const long long i = p + c * npadded;
      RLESource *     s = internals->sources[i];
      const long long j = (npadded - 1 - p) + c * npadded; // Little-endian
      const long long numOutBytes = decode_internal(scanbuf,
                                              *s,
                                              internals->img.get_width(),
                                              j,
                                              internals->nsources,
                                              internals->cross_row[i],
                                              internals->nstorage[i]);
      assert(numOutBytes <= internals->img.get_width());
      numOutBytesFull += numOutBytes;
    }
  }
  d.write(scanbuf, scanlen);
  return numOutBytesFull;
}

RLEDecoder::streamsize_t
RLEDecoder::decode_frame(RLEDestination & d)
{
  for (long long i = 0; i < internals->nsources; ++i)
  {
    RLESource * s = internals->sources[i];
    assert(s->tell() == internals->rh.offset[i]);
    (void)s;
  }
  long long       numOutBytesFull = 0;
  const long long mult = internals->img.get_RLEPixelInfo().compute_num_segments();
  for (long long h = 0; h < internals->img.get_height(); ++h)
  {
    const long long numOutBytes = decode_row(d);
    assert(numOutBytes <= internals->img.get_width() * mult);
    (void)mult;
    numOutBytesFull += numOutBytes;
  }
  return numOutBytesFull;
}

long long
RLEDecoder::get_row_nbytes() const
{
  const long long mult = internals->img.get_RLEPixelInfo().compute_num_segments();
  return internals->img.get_width() * mult;
}

bool
RLEDecoder::read_header(RLEPixelInfo & pi)
{
  RLEHeader &     rh = internals->rh;
  const long long size = sizeof(rh);
  RLESource *     s = internals->sources[0];
  assert(s);
  const long long nbytes = s->read(reinterpret_cast<char *>(&rh), sizeof(rh));
  // Positioned exactly at offset 64, to read the first segment
  if (nbytes != size)
    return false;
  // RLEHeader has been read, fill value from user input
  pi = internals->img.get_RLEPixelInfo();
  // Check those values against what the decoder has been fed with
  if (!check_header(rh, pi))
    return false;
  // Initialize all sources
  assert(internals->nsources == static_cast<long long>(internals->rh.num_segments));
  for (long long i = 1; i < internals->nsources; ++i)
  {
    internals->sources[i] = s->clone();
    internals->sources[i]->seek(internals->rh.offset[i]);
  }
  return true;
}

}
