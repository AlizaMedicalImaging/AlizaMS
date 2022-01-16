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
#ifndef MDCMSWAPPER_H
#define MDCMSWAPPER_H

#include "mdcmSwapCode.h"

namespace mdcm
{

#ifdef MDCM_WORDS_BIGENDIAN

/*
In the default case of Little Endian encoding, Big Endian Machines
interpreting Data Sets shall do 'byte swapping' before interpreting
or operating on certain Data Elements. The Data Elements affected
are all those having VRs that are multiple byte Values and that are
not a character string of 8-bit single byte codes. VRs constructed
of a string of characters of 8-bit single byte codes are really
constructed of a string of individual bytes, and are therefore not
affected by byte ordering. The VRs that are not a string of
characters and consist of multiple bytes are:

    2-byte US, SS, OW and each component of AT

    4-byte OF, UL, SL, and FL

    8 byte OD, FD

Note

For the above VRs, the multiple bytes are presented in increasing
order of significance when in Little Endian format. For example,
an 8-byte Data Element with VR of FD, might be written in
hexadecimal as 68AF4B2CH, but encoded in Little Endian would
be 2C4BAF68H.
*/

class SwapperDoOp
{
public:
  template <typename T>
  static T
  Swap(T val)
  {
    return val;
  }
  template <typename T>
  static void
  SwapArray(T *, size_t)
  {}
};

class SwapperNoOp
{
public:
  template <typename T>
  static T
  Swap(T val);
  template <typename T>
  static void
  SwapArray(T * array, size_t n)
  {
    for (size_t i = 0; i < n; ++i)
    {
      array[i] = Swap<T>(array[i]);
    }
  }
};

#else

class SwapperNoOp
{
public:
  template <typename T>
  static T
  Swap(T val)
  {
    return val;
  }
  template <typename T>
  static void
  SwapArray(T *, size_t)
  {}
};

class SwapperDoOp
{
public:
  template <typename T>
  static T
  Swap(T val);
  template <typename T>
  static void
  SwapArray(T * array, size_t n)
  {
    for (size_t i = 0; i < n; ++i)
    {
      array[i] = Swap<T>(array[i]);
    }
  }
};

#endif

} // end namespace mdcm

#include "mdcmSwapper.hxx"

#endif // MDCMSWAPPER_H
