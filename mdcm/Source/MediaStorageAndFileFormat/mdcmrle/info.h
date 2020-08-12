/*=========================================================================

  Program: librle, a minimal RLE library for DICOM

  Copyright (c) 2014 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#pragma once

namespace rle
{

// RLE 1 or 3 components and bpp being 8/16 or 32
class pixel_info
{
public:
  pixel_info(unsigned char nc = 1, unsigned char bpp = 8);
  explicit pixel_info(int num_segments);
  int compute_num_segments() const;
  int get_number_of_components() const;
  int get_number_of_bits_per_pixel() const;
  static bool check_num_segments(const int num_segments);

private:
  unsigned char number_components;
  unsigned char bits_per_pixel;
};

class image_info
{
public:
  image_info(int width = 0, int height = 0, pixel_info const & pi = pixel_info(),
    bool planarconfiguration = false, bool littleendian = true);
  int get_width() const { return width; }
  int get_height() const { return height; }
  pixel_info get_pixel_info() const { return pix; }
  bool get_planar_configuration() const { return planarconfiguration; }
  bool is_little_endian() const { return littleendian; }

private:
  int width;
  int height;
  pixel_info pix;
  bool planarconfiguration;
  bool littleendian;
};

} // end namespace rle
