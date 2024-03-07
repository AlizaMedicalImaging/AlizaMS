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
#ifndef MDCMSEQUENCEOFFRAGMENTS_H
#define MDCMSEQUENCEOFFRAGMENTS_H

#include "mdcmValue.h"
#include "mdcmVL.h"
#include "mdcmFragment.h"
#include "mdcmBasicOffsetTable.h"
#include <exception>

namespace mdcm
{

/**
 * Class to represent a Sequence Of Fragments
 * MM: I do not enforce that Sequence of Fragments ends with a SQ end del TODO
 */

class MDCM_EXPORT SequenceOfFragments : public Value
{
public:
  typedef std::vector<Fragment>          FragmentVector;
  typedef FragmentVector::size_type      SizeType;
  typedef FragmentVector::iterator       Iterator;
  typedef FragmentVector::const_iterator ConstIterator;

  SequenceOfFragments() = default;

  Iterator
  Begin()
  {
    return Fragments.begin();
  }

  Iterator
  End()
  {
    return Fragments.end();
  }

  ConstIterator
  Begin() const
  {
    return Fragments.begin();
  }

  ConstIterator
  End() const
  {
    return Fragments.end();
  }

  VL
  GetLength() const override
  {
    return SequenceLengthField;
  }

  void
  SetLength(VL length) override
  {
    SequenceLengthField = length;
  }

  void
  Clear() override;

  void
  AddFragment(Fragment const &);

  unsigned long long
  ComputeByteLength() const;

  VL
  ComputeLength() const;

  bool
  GetBuffer(char *, unsigned long long) const;

  bool
  GetFragBuffer(unsigned int, char *, unsigned long long &) const;

  SizeType
  GetNumberOfFragments() const;

  const Fragment GetFragment(SizeType) const;

  bool
  WriteBuffer(std::ostream &) const;

  const BasicOffsetTable &
  GetTable() const
  {
    return Table;
  }

  BasicOffsetTable &
  GetTable()
  {
    return Table;
  }

  template <typename TSwap>
  std::istream &
  Read(std::istream & is, bool readvalues = true)
  {
    assert(SequenceLengthField.IsUndefined());
    ReadPreValue<TSwap>(is);
    return ReadValue<TSwap>(is, readvalues);
  }

  template <typename TSwap>
  std::istream &
  ReadPreValue(std::istream & is)
  {
    // First item is the basic offset table
    try
    {
      Table.Read<TSwap>(is);
      mdcmDebugMacro("Table: " << Table);
    }
    catch (...)
    {
      // Bug_Siemens_PrivateIconNoItem.dcm
      // First thing first let's rewind
      is.seekg(-4, std::ios::cur);
      // FF D8 <=> Start of Image (SOI) marker
      // FF E0 <=> APP0 Reserved for Application Use
      if (Table.GetTag() == Tag(0xd8ff, 0xe0ff))
      {
        Table = BasicOffsetTable();
        Fragment frag;
        if (FillFragmentWithJPEG(frag, is))
        {
          Fragments.push_back(frag);
        }
        return is;
      }
      else
      {
        throw std::logic_error("Seq. of frag. : ReadPreValue exception");
      }
    }
    return is;
  }

  template <typename TSwap>
  std::istream &
  ReadValue(std::istream & is, bool /*readvalues*/)
  {
    const Tag seqDelItem(0xfffe, 0xe0dd);
    Fragment frag;
    try
    {
      while (frag.Read<TSwap>(is) && frag.GetTag() != seqDelItem)
      {
        Fragments.push_back(frag);
      }
      assert(frag.GetTag() == seqDelItem && frag.GetVL() == 0);
    }
    catch (const std::exception & ex)
    {
#ifdef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
      // MM: In all cases the whole file was read, because
      // Fragment::Read only fail on eof() reached 1.
      // SIEMENS-JPEG-CorruptFrag.dcm is more difficult to deal with, we have a
      // partial fragment, we decide to add it anyway to the stack of
      // fragments (eof was reached so we need to clear error bit).
      if (frag.GetTag() == Tag(0xfffe, 0xe000))
      {
        mdcmAlwaysWarnMacro("PixelData fragment could be corrupted, use the file at your own risk");
        Fragments.push_back(frag);
        is.clear();
      }
      // GENESIS_SIGNA-JPEG-CorruptFrag.dcm
      else if (frag.GetTag() == Tag(0xddff, 0x00e0))
      {
        assert(Fragments.size() == 1);
        const ByteValue * bv = Fragments[0].GetByteValue();
        assert(static_cast<unsigned char>(bv->GetPointer()[bv->GetLength() - 1]) == 0xfe);
        // MM: Yes this is an extra copy, this is a bug anyway.
        Fragments[0].SetByteValue(bv->GetPointer(), bv->GetLength() - 1);
        mdcmAlwaysWarnMacro("Fragment length was declared with an extra byte at the end, stripped");
        is.clear();
      }
      // LEICA/WSI
      else if ((frag.GetTag().GetGroup() == 0x00ff) && ((frag.GetTag().GetElement() & 0x00ff) == 0xe0))
      {
        // MM: Looks like there is a mess with offset and odd byte array
        // We are going first to backtrack one byte back, and then use a
        // ReadBacktrack function which in turn may backtrack up to 10 bytes
        // backward. This appears to be working on a set of DICOM/WSI files from
        // LEICA.
        mdcmAlwaysWarnMacro("Fragment: odd value length, fixed (1)");
        assert(Fragments.size());
        const size_t      lastf = Fragments.size() - 1;
        const ByteValue * bv = Fragments[lastf].GetByteValue();
        const char *      a = bv->GetPointer();
        assert(static_cast<unsigned char>(a[bv->GetLength() - 1]) == 0xfe);
        (void)a;
        Fragments[lastf].SetByteValue(bv->GetPointer(), bv->GetLength() - 1);
        is.seekg(-9, std::ios::cur);
        assert(is.good());
        while (frag.ReadBacktrack<TSwap>(is) && frag.GetTag() != seqDelItem)
        {
          mdcmDebugMacro("Fragment: " << frag);
          Fragments.push_back(frag);
        }
        assert(frag.GetTag() == seqDelItem && frag.GetVL() == 0);
      }
      // LEICA/WSI (bis)
      else if (frag.GetTag().GetGroup() == 0xe000)
      {
        // MM: Looks like there is a mess with offset and odd byte array
        // We are going first to backtrack one byte back, and then use a
        // ReadBacktrack function which in turn may backtrack up to 10 bytes
        // backward. This appears to be working on a set of DICOM/WSI files from
        // LEICA.
        mdcmAlwaysWarnMacro("Fragment: odd value length, fixed (2)");
        assert(Fragments.size());
        const size_t      lastf = Fragments.size() - 1;
        const ByteValue * bv = Fragments[lastf].GetByteValue();
        const char *      a = bv->GetPointer();
        assert(static_cast<unsigned char>(a[bv->GetLength() - 2]) == 0xfe);
        (void)a;
        Fragments[lastf].SetByteValue(bv->GetPointer(), bv->GetLength() - 2);
        is.seekg(-10, std::ios::cur);
        assert(is.good());
        while (frag.ReadBacktrack<TSwap>(is) && frag.GetTag() != seqDelItem)
        {
          mdcmDebugMacro("Fragment: " << frag);
          Fragments.push_back(frag);
        }
        assert(frag.GetTag() == seqDelItem && frag.GetVL() == 0);
      }
      // LEICA/WSI (ter)
      else if ((frag.GetTag().GetGroup() & 0x00ff) == 0x00e0 &&
               (frag.GetTag().GetElement() & 0xff00) == 0x0000)
      {
        // MM: Looks like there is a mess with offset and odd byte array
        // We are going first to backtrack one byte back, and then use a
        // ReadBacktrack function which in turn may backtrack up to 10 bytes
        // backward. This appears to be working on a set of DICOM/WSI files from
        // LEICA.
        mdcmAlwaysWarnMacro("Fragment: odd value length, fixed (3)");
        assert(Fragments.size());
        const size_t      lastf = Fragments.size() - 1;
        const ByteValue * bv = Fragments[lastf].GetByteValue();
        const char *      a = bv->GetPointer();
        assert(static_cast<unsigned char>(a[bv->GetLength() - 3]) == 0xfe);
        (void)a;
        Fragments[lastf].SetByteValue(bv->GetPointer(), bv->GetLength() - 3);
        is.seekg(-11, std::ios::cur);
        assert(is.good());
        while (frag.ReadBacktrack<TSwap>(is) && frag.GetTag() != seqDelItem)
        {
          mdcmDebugMacro("Fragment: " << frag);
          Fragments.push_back(frag);
        }
        assert(frag.GetTag() == seqDelItem && frag.GetVL() == 0);
      }
      else
      {
        // MM: mdcm-JPEG-LossLess3a.dcm: easy case, an extra tag was found
        // instead of terminator (eof is the next char)
        mdcmAlwaysWarnMacro("Failed at " << frag.GetTag() << " Index #" << Fragments.size()
                            << " Offset " << is.tellg() << '\n' << ex.what());
      }
#endif /* MDCM_SUPPORT_BROKEN_IMPLEMENTATION */
    }
    return is;
  }

  template <typename TSwap>
  std::ostream const &
  Write(std::ostream & os) const
  {
    if (!Table.Write<TSwap>(os))
    {
      assert(0 && "Should not happen");
      return os;
    }
    for (ConstIterator it = Begin(); it != End(); ++it)
    {
      it->Write<TSwap>(os);
    }
    // seq del item is not stored, write it
    const Tag seqDelItem(0xfffe, 0xe0dd);
    seqDelItem.Write<TSwap>(os);
    VL zero = 0;
    zero.Write<TSwap>(os);
    return os;
  }

  static SmartPointer<SequenceOfFragments>
  New()
  {
    return new SequenceOfFragments();
  }

  void
  Print(std::ostream & os) const override
  {
    os << "SQ L= " << SequenceLengthField << "\nTable:\n" << Table << '\n';
    for (ConstIterator it = Begin(); it != End(); ++it)
    {
      os << ' ' << *it << '\n';
    }
    assert(SequenceLengthField.IsUndefined());
    {
      const Tag seqDelItem(0xfffe, 0xe0dd);
      VL        zero = 0;
      os << seqDelItem << '\t' << zero;
    }
  }

  bool
  operator==(const Value & val) const override
  {
    const SequenceOfFragments & sqf = dynamic_cast<const SequenceOfFragments &>(val);
    return (Table == sqf.Table && SequenceLengthField == sqf.SequenceLengthField && Fragments == sqf.Fragments);
  }

private:
  bool
  FillFragmentWithJPEG(Fragment &, std::istream &);

  BasicOffsetTable Table{};
  VL               SequenceLengthField{0xffffffff};
  FragmentVector   Fragments{};
};

} // end namespace mdcm

#endif // MDCMSEQUENCEOFFRAGMENTS_H
