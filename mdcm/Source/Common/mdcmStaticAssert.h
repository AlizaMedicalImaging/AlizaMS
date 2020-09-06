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

#ifndef MDCMSTATICASSERT_H
#define MDCMSTATICASSERT_H

// from BOOST static assert
namespace mdcm
{
  template <bool x>
  struct STATIC_ASSERTION_FAILURE;

  template <>
  struct STATIC_ASSERTION_FAILURE<true> { enum { value = 1 }; };

  template <int x>
  struct static_assert_test {};
}

#define MDCM_JOIN( X, Y ) MDCM_DO_JOIN( X, Y )
#define MDCM_DO_JOIN( X, Y ) MDCM_DO_JOIN2(X,Y)
#define MDCM_DO_JOIN2( X, Y ) X##Y
#define MDCM_STATIC_ASSERT( B ) \
  typedef ::mdcm::static_assert_test<\
    sizeof(::mdcm::STATIC_ASSERTION_FAILURE< (bool)( B ) >)>\
      MDCM_JOIN(mdcm_static_assert_typedef_, __LINE__)

/* Example:
 *
 * template <class T>
 * struct must_not_be_instantiated
 * {
 * // this will be triggered if this type is instantiated
 * MDCM_STATIC_ASSERT(sizeof(T) == 0);
 * };
 *
 */

#endif // MDCMSTATICASSERT_H
