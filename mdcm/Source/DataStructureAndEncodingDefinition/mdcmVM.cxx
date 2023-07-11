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

#include "mdcmVM.h"
#include <string>

namespace mdcm
{

unsigned int VM::GetLength() const
{
  unsigned int len = 0;
  switch (VMField)
  {
    case VM::VM1:
      len = VMToLength<VM::VM1>::Length;
      break;
    case VM::VM2:
      len = VMToLength<VM::VM2>::Length;
      break;
    case VM::VM3:
      len = VMToLength<VM::VM3>::Length;
      break;
    case VM::VM4:
      len = VMToLength<VM::VM4>::Length;
      break;
    case VM::VM5:
      len = VMToLength<VM::VM5>::Length;
      break;
    case VM::VM6:
      len = VMToLength<VM::VM6>::Length;
      break;
    case VM::VM8:
      len = VMToLength<VM::VM8>::Length;
      break;
    case VM::VM9:
      len = VMToLength<VM::VM9>::Length;
      break;
    case VM::VM10:
      len = VMToLength<VM::VM10>::Length;
      break;
    case VM::VM12:
      len = VMToLength<VM::VM12>::Length;
      break;
    case VM::VM16:
      len = VMToLength<VM::VM16>::Length;
      break;
    case VM::VM18:
      len = VMToLength<VM::VM18>::Length;
      break;
    case VM::VM24:
      len = VMToLength<VM::VM24>::Length;
      break;
    case VM::VM28:
      len = VMToLength<VM::VM28>::Length;
      break;
    case VM::VM32:
      len = VMToLength<VM::VM32>::Length;
      break;
    case VM::VM35:
      len = VMToLength<VM::VM35>::Length;
      break;
    case VM::VM99:
      len = VMToLength<VM::VM99>::Length;
      break;
    case VM::VM256:
      len = VMToLength<VM::VM256>::Length;
      break;
    default:
      break;
  }
  return len;
}

// clang-format off
std::string VM::GetVMString(VMType vm)
{
  std::string r;
  switch (vm)
  {
    case VM::VM0:      r = std::string("INVALID"); break;
    case VM::VM1:      r = std::string("1");       break;
    case VM::VM2:      r = std::string("2");       break;
    case VM::VM3:      r = std::string("3");       break;
    case VM::VM4:      r = std::string("4");       break;
    case VM::VM5:      r = std::string("5");       break;
    case VM::VM6:      r = std::string("6");       break;
    case VM::VM8:      r = std::string("8");       break;
    case VM::VM9:      r = std::string("9");       break;
    case VM::VM10:     r = std::string("10");      break;
    case VM::VM12:     r = std::string("12");      break;
    case VM::VM16:     r = std::string("16");      break;
    case VM::VM18:     r = std::string("18");      break;
    case VM::VM24:     r = std::string("24");      break;
    case VM::VM28:     r = std::string("28");      break;
    case VM::VM32:     r = std::string("32");      break;
    case VM::VM35:     r = std::string("35");      break;
    case VM::VM99:     r = std::string("99");      break;
    case VM::VM256:    r = std::string("256");     break;
    case VM::VM1_n:    r = std::string("1-n");     break;
    case VM::VM1_2:    r = std::string("1-2");     break;
    case VM::VM1_3:    r = std::string("1-3");     break;
    case VM::VM1_4:    r = std::string("1-4");     break;
    case VM::VM1_5:    r = std::string("1-5");     break;
    case VM::VM1_8:    r = std::string("1-8");     break;
    case VM::VM1_32:   r = std::string("1-32");    break;
    case VM::VM1_99:   r = std::string("1-99");    break;
    case VM::VM2_4:    r = std::string("2-4");     break;
    case VM::VM3_4:    r = std::string("3-4");     break;
    case VM::VM4_5:    r = std::string("4-5");     break;
    case VM::VM2_2n:   r = std::string("2-2n");    break;
    case VM::VM2_n:    r = std::string("2-n");     break;
    case VM::VM3_3n:   r = std::string("3-3n");    break;
    case VM::VM3_n:    r = std::string("3-n");     break;
    case VM::VM4_4n:   r = std::string("4-4n");    break;
    case VM::VM6_6n:   r = std::string("6-6n");    break;
    case VM::VM6_n:    r = std::string("6-n");     break;
    case VM::VM7_7n:   r = std::string("7-7n");    break;
    case VM::VM30_30n: r = std::string("30-30n");  break;
    case VM::VM47_47n: r = std::string("47-47n");  break;
    default:           r = std::string("UNKNOWN"); break;
  }
  return r;
}
// clang-format on

// This function should be used with caution or not at all,
// this only returns a 'guess' of the VM.
VM::VMType
VM::GetVMTypeFromLength(size_t length, unsigned int size)
{
  if (length == 0 || (length % size) != 0)
    return VM::VM0;
  const unsigned int ratio = static_cast<unsigned int>(length / size);
  switch (ratio)
  {
    case 1:
      return VM::VM1;
    case 2:
      return VM::VM2;
    case 3:
      return VM::VM3;
    case 4:
      return VM::VM4;
    case 5:
      return VM::VM5;
    case 6:
      return VM::VM6;
    case 8:
      return VM::VM8;
    case 9:
      return VM::VM9;
    case 16:
      return VM::VM16;
    case 24:
      return VM::VM24;
    case 32:
      return VM::VM32;
    default:
      return VM::VM1_n;
  }
}

size_t
VM::GetNumberOfElementsFromArray(const char * array, size_t length)
{
  if (!array || length == 0)
    return 0;
  size_t       c = 0;
  const char * parray = array;
  const char * end = array + length;
  bool         valuefound = false;
  for (; parray != end; ++parray)
  {
    if (*parray == ' ')
    {
      ;
    }
    else if (*parray == '\\')
    {
      if (valuefound)
      {
        ++c;
        valuefound = false;
      }
    }
    else
    {
      valuefound = true;
    }
  }
  if (valuefound)
    ++c;
  return c;
}

} // end namespace mdcm
