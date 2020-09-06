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
#ifndef MDCMBYTESWAP_TXX
#define MDCMBYTESWAP_TXX

#include "mdcmByteSwap.h"
#include <iostream>
#include <cstdlib>

namespace mdcm
{

// Machine definitions
#ifdef MDCM_WORDS_BIGENDIAN
template <class T>
bool ByteSwap<T>::SystemIsBigEndian() { return true; }
template <class T>
bool ByteSwap<T>::SystemIsLittleEndian() { return false; }
#else
template <class T>
bool ByteSwap<T>::SystemIsBigEndian() { return false; }
template <class T>
bool ByteSwap<T>::SystemIsLittleEndian() { return true; }
#endif

template <class T>
void ByteSwap<T>::Swap(T &p)
{
#ifdef MDCM_WORDS_BIGENDIAN
    ByteSwap<T>::SwapFromSwapCodeIntoSystem(p, SwapCode::LittleEndian);
#else
    ByteSwap<T>::SwapFromSwapCodeIntoSystem(p, SwapCode::BigEndian);
#endif
}

// Swaps the bytes so they agree with the processor order
template <class T>
void ByteSwap<T>::SwapFromSwapCodeIntoSystem(T &a, SwapCode const &swapcode)
{
  switch(sizeof(T))
  {
  case 1:
    break;
  case 2:
    Swap4(a, swapcode);
    break;
  case 4:
    Swap8(a, swapcode);
    break;
  default:
    std::cerr << "Impossible" << std::endl;
    abort();
  }
}

template <class T>
void ByteSwap<T>::SwapRange(T *p, unsigned int num)
{
  for(unsigned int i=0; i<num; i++)
  {
    ByteSwap<T>::Swap(p[i]);
  }
}

template <class T>
void ByteSwap<T>::SwapRangeFromSwapCodeIntoSystem(T *p, SwapCode const &sc,
  std::streamoff num)
{
  for(std::streamoff i=0; i<num; i++)
  {
    ByteSwap<T>::SwapFromSwapCodeIntoSystem(p[i], sc);
  }
}

template<class T>
void Swap4(T &a, SwapCode const &swapcode)
{
#ifndef MDCM_WORDS_BIGENDIAN
  if (swapcode == 4321 || swapcode == 2143)
    a = (T)((a << 8) | ( a >> 8 ));
#else
  if (swapcode == 1234 || swapcode == 3412)
    a = (a << 8) | ( a >> 8 );
  // On big endian as long as the SwapCode is Unknown let's pretend we were
  // on a LittleEndian system (might introduce overhead on those system).
  else if (swapcode == SwapCode::Unknown)
    a = (a << 8) | ( a >> 8 );
#endif
}

//note: according to http://www.parashift.com/c++-faq-lite/templates.html#faq-35.8
//the inlining of the template class means that the specialization doesn't cause linker errors
template<class T>
inline void Swap8(T &a, SwapCode const &swapcode)
{
  switch (swapcode)
  {
  case SwapCode::Unknown:
#ifdef MDCM_WORDS_BIGENDIAN
    a= ((a<<24) | ((a<<8)  & 0x00ff0000) | ((a>>8) & 0x0000ff00) | (a>>24));
#endif
    break;
  case 1234 :
#ifdef MDCM_WORDS_BIGENDIAN
    a= ((a<<24) | ((a<<8)  & 0x00ff0000) | ((a>>8) & 0x0000ff00) | (a>>24));
#endif
    break;
  case 4321 :
#ifndef MDCM_WORDS_BIGENDIAN
    a= ((a<<24) | ((a<<8)  & 0x00ff0000) | ((a>>8) & 0x0000ff00) | (a>>24));
#endif
    break;
  case 3412 :
    a= ((a<<16) | (a>>16) );
    break;
  case 2143 :
    a= (((a<< 8) & 0xff00ff00) | ((a>>8) & 0x00ff00ff));
    break;
  default :
    std::cerr << "Unexpected swap code:" << swapcode;
  }
}

template <>
inline void Swap8<uint16_t>(uint16_t &a, SwapCode const &swapcode)
{
  switch (swapcode)
  {
  case SwapCode::Unknown:
#ifdef MDCM_WORDS_BIGENDIAN
    a= ((a<<24) | ((a<<8)  & 0x00ff0000) | ((a>>8) & 0x0000ff00) | (a>>24));
#endif
    break;
  case 1234 :
#ifdef MDCM_WORDS_BIGENDIAN
    a= ((a<<24) | ((a<<8)  & 0x00ff0000) | ((a>>8) & 0x0000ff00) | (a>>24));
#endif
    break;
  case 4321 :
#ifndef MDCM_WORDS_BIGENDIAN
//    probably not really useful since the lowest 0x0000 are what's used in unsigned shorts
//    a= ((a<<24) | ((a<<8)  & 0x00ff0000) | ((a>>8) & 0x0000ff00) | (a>>24));
#endif
    break;
  case 3412 :
    //a= ((a<<16) | (a>>16) );//do nothing, a = a
    break;
  case 2143 :
    a= (uint16_t)(((a<< 8) & 0xff00) | ((a>>8) & 0x00ff));
    break;
  default :
    std::cerr << "Unexpected swap code:" << swapcode;
  }
}

} // end namespace mdcm

#endif // MDCMBYTESWAP_TXX
