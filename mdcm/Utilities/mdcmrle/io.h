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

class image_info;

class source
{
public:
  typedef unsigned int streampos_t; // 32bits unsigned
  int read_into_segments( char * out, int len, image_info const & ii);
  virtual int read( char * out, int len ) = 0;
  virtual streampos_t tell() = 0;
  virtual bool seek(streampos_t pos) = 0;
  virtual bool eof() = 0;
  virtual source * clone() = 0;
  virtual ~source() {}
};

class dest
{
public:
  typedef unsigned int streampos_t; // 32bits unsigned

  virtual int write( const char * in, int len ) = 0;
  virtual bool seek( streampos_t abs_pos ) = 0; // seek to absolute position
  virtual ~dest() {}
};

} // end namespace rle
