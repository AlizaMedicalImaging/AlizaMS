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
#ifndef MDCMDATASET_TXX
#define MDCMDATASET_TXX

#include "mdcmByteValue.h"
#include "mdcmPrivateTag.h"
#include "mdcmParseException.h"
#include <cstring>

namespace mdcm
{

template <typename TDE, typename TSwap>
std::istream &
DataSet::ReadNested(std::istream & is)
{
  DataElement de;
  const Tag   itemDelItem(0xfffe, 0xe00d);
  assert(de.GetTag() != itemDelItem);
  try
  {
    while (de.Read<TDE, TSwap>(is) && de.GetTag() != itemDelItem) // Keep that order!
    {
      InsertDataElement(de);
    }
  }
  catch (const ParseException & pe)
  {
    if (pe.GetLastElement().GetTag() == Tag(0xfffe, 0xe0dd))
    {
      // BogusItemStartItemEnd.dcm
      mdcmWarningMacro("SQ End found but no Item end found");
      de.SetTag(itemDelItem);
      is.seekg(-4, std::ios::cur);
    }
    else
    {
      // MR_Philips_Intera_PrivateSequenceExplicitVR_in_SQ_2001_e05f_item_wrong_lgt_use_NOSHADOWSEQ.dcm
      throw pe;
    }
  }
  assert(de.GetTag() == itemDelItem);
  return is;
}

template <typename TDE, typename TSwap>
std::istream &
DataSet::Read(std::istream & is)
{
  DataElement de;
  while (!is.eof() && de.Read<TDE, TSwap>(is))
  {
    InsertDataElement(de);
  }
  return is;
}

template <typename TDE, typename TSwap>
std::istream &
DataSet::ReadUpToTag(std::istream & is, const Tag & t, const std::set<Tag> & skiptags)
{
  DataElement de;
  while (!is.eof() && de.template ReadPreValue<TDE, TSwap>(is, skiptags))
  {
    // If tag read was in skiptags then we should NOT add it
    if (skiptags.count(de.GetTag()) == 0)
    {
      de.template ReadValue<TDE, TSwap>(is, skiptags);
      InsertDataElement(de);
    }
    else
    {
      assert(is.good());
      if (de.GetTag() != t)
        is.seekg(de.GetVL(), std::ios::cur);
    }
    // tag was found, we can exit the loop
    if (t <= de.GetTag())
    {
      assert(is.good());
      break;
    }
  }
  return is;
}

template <typename TDE, typename TSwap>
std::istream &
DataSet::ReadUpToTagWithLength(std::istream & is, const Tag & t, const std::set<Tag> & skiptags, VL & length)
{
  DataElement de;
  while (!is.eof() && de.template ReadPreValue<TDE, TSwap>(is, skiptags))
  {
    // If tag read was in skiptags then we should NOT add it.
    if (skiptags.count(de.GetTag()) == 0)
    {
      de.template ReadValueWithLength<TDE, TSwap>(is, length, skiptags);
      InsertDataElement(de);
    }
    else
    {
      assert(is.good());
      if (de.GetTag() != t)
        is.seekg(de.GetVL(), std::ios::cur);
    }
    // tag was found, we can exit the loop.
    if (t <= de.GetTag())
    {
      assert(is.good());
      break;
    }
  }
  return is;
}

template <typename TDE, typename TSwap>
std::istream &
DataSet::ReadSelectedTags(std::istream & inputStream, const std::set<Tag> & selectedTags, bool readvalues)
{
  if (!(selectedTags.empty() || inputStream.fail()))
  {
    const Tag     maxTag = *(selectedTags.rbegin());
    std::set<Tag> tags = selectedTags;
    DataElement   dataElem;
    while (!inputStream.eof())
    {
      static_cast<TDE &>(dataElem).template ReadPreValue<TSwap>(inputStream);
      const Tag & tag = dataElem.GetTag();
      if (inputStream.fail() || maxTag < tag)
      {
        if (inputStream.good())
        {
          const int l = dataElem.GetVR().GetLength();
          inputStream.seekg(-4 - 2 * l, std::ios::cur);
        }
        else
        {
          inputStream.clear();
          inputStream.seekg(0, std::ios::end);
        }
        // Failed to read the tag, or the read tag exceeds the maximum.
        // As we assume ascending tag ordering, we can exit the loop.
        break;
      }
      static_cast<TDE &>(dataElem).template ReadValue<TSwap>(inputStream, readvalues);
      const std::set<Tag>::iterator found = tags.find(tag);
      if (found != tags.end())
      {
        InsertDataElement(dataElem);
        tags.erase(found);

        if (tags.empty())
        {
          // All selected tags were found, we can exit the loop.
          break;
        }
      }
      if (!(tag < maxTag))
      {
        // The maximum tag was encountered, and as we assume
        // ascending tag ordering, we can exit the loop.
        break;
      }
    }
  }
  assert(inputStream.good());
  return inputStream;
}

template <typename TDE, typename TSwap>
std::istream &
DataSet::ReadSelectedPrivateTags(std::istream &               inputStream,
                                 const std::set<PrivateTag> & selectedPTags,
                                 bool                         readvalues)
{
  if (!(selectedPTags.empty() || inputStream.fail()))
  {
    assert(selectedPTags.size() == 1);
    const PrivateTag refPTag = *(selectedPTags.rbegin());
    PrivateTag       nextPTag = refPTag;
    nextPTag.SetElement(static_cast<uint16_t>(nextPTag.GetElement() + 0x1));
    assert(nextPTag.GetElement() & 0x00ff); // no wrap
    Tag maxTag;
    maxTag.SetPrivateCreator(nextPTag);
    DataElement dataElem;
    while (!inputStream.eof())
    {
      static_cast<TDE &>(dataElem).template ReadPreValue<TSwap>(inputStream);
      const Tag & tag = dataElem.GetTag();
      if (inputStream.fail() || maxTag < tag)
      {
        if (inputStream.good())
        {
          const int l = dataElem.GetVR().GetLength();
          inputStream.seekg(-4 - 2 * l, std::ios::cur);
        }
        else
        {
          inputStream.clear();
          inputStream.seekg(0, std::ios::end);
        }
        // Failed to read the tag, or the read tag exceeds the maximum.
        // As we assume ascending tag ordering, we can exit the loop.
        break;
      }
      static_cast<TDE &>(dataElem).template ReadValue<TSwap>(inputStream, readvalues);
      if (inputStream.fail())
      {
        break;
      }
      if (tag.GetPrivateCreator() == refPTag)
      {
        DES.insert(dataElem);
      }
      if (!(tag < maxTag))
      {
        // The maximum group was encountered, and as we assume
        // ascending tag ordering, we can exit the loop.
        break;
      }
    }
  }
  return inputStream;
}

template <typename TDE, typename TSwap>
std::istream &
DataSet::ReadSelectedTagsWithLength(std::istream &        inputStream,
                                    const std::set<Tag> & selectedTags,
                                    VL &                  length,
                                    bool                  readvalues)
{
  (void)length;
  if (!selectedTags.empty())
  {
    const Tag     maxTag = *(selectedTags.rbegin());
    std::set<Tag> tags = selectedTags;
    DataElement   dataElem;
    while (!inputStream.eof())
    {
      static_cast<TDE &>(dataElem).template ReadPreValue<TSwap>(inputStream);
      const Tag tag = dataElem.GetTag();
      if (inputStream.fail() || maxTag < tag)
      {
        if (inputStream.good())
        {
          const int l = dataElem.GetVR().GetLength();
          inputStream.seekg(-4 - 2 * l, std::ios::cur);
        }
        else
        {
          inputStream.clear();
          inputStream.seekg(0, std::ios::end);
        }
        // Failed to read the tag, or the read tag exceeds the maximum.
        // As we assume ascending tag ordering, we can exit the loop.
        break;
      }
      static_cast<TDE &>(dataElem).template ReadValue<TSwap>(inputStream, readvalues);
      if (inputStream.fail())
      {
        break;
      }
      const std::set<Tag>::iterator found = tags.find(tag);
      if (found != tags.end())
      {
        InsertDataElement(dataElem);
        tags.erase(found);
        if (tags.empty())
        {
          // All selected tags were found, we can exit the loop.
          break;
        }
      }
      if (!(tag < maxTag))
      {
        // The maximum tag was encountered, and as we assume
        // ascending tag ordering, we can exit the loop.
        break;
      }
    }
  }
  return inputStream;
}

template <typename TDE, typename TSwap>
std::istream &
DataSet::ReadSelectedPrivateTagsWithLength(std::istream &               inputStream,
                                           const std::set<PrivateTag> & selectedPTags,
                                           VL &                         length,
                                           bool                         readvalues)
{
  (void)length;
  if (!(selectedPTags.empty() || inputStream.fail()))
  {
    assert(selectedPTags.size() == 1);
    const PrivateTag refPTag = *(selectedPTags.rbegin());
    PrivateTag       nextPTag = refPTag;
    nextPTag.SetElement(static_cast<uint16_t>(nextPTag.GetElement() + 0x1));
    assert(nextPTag.GetElement()); // no wrap
    Tag maxTag;
    maxTag.SetPrivateCreator(nextPTag);
    DataElement dataElem;
    while (!inputStream.eof())
    {
      static_cast<TDE &>(dataElem).template ReadPreValue<TSwap>(inputStream);
      const Tag & tag = dataElem.GetTag();
      if (inputStream.fail() || maxTag < tag)
      {
        if (inputStream.good())
        {
          const int l = dataElem.GetVR().GetLength();
          inputStream.seekg(-4 - 2 * l, std::ios::cur);
        }
        else
        {
          inputStream.clear();
          inputStream.seekg(0, std::ios::end);
        }
        // Failed to read the tag, or the read tag exceeds the maximum.
        // As we assume ascending tag ordering, we can exit the loop.
        break;
      }
      static_cast<TDE &>(dataElem).template ReadValue<TSwap>(inputStream, readvalues);
      if (inputStream.fail())
      {
        break;
      }
      if (tag.GetPrivateCreator() == refPTag)
      {
        InsertDataElement(dataElem);
      }
      if (!(tag < maxTag))
      {
        // The maximum group was encountered, and as we assume
        // ascending tag ordering, we can exit the loop.
        break;
      }
    }
  }
  return inputStream;
}

template <typename TDE, typename TSwap>
std::istream &
DataSet::ReadWithLength(std::istream & is, VL & length)
{
  DataElement          de;
  VL                   l = 0;
  VL                   locallength = length;
  const std::streampos startpos = is.tellg();
  try
  {
    while (l != locallength && de.ReadWithLength<TDE, TSwap>(is, locallength))
    {
#ifndef MDCM_SUPPORT_BROKEN_IMPLEMENTATION
      assert(de.GetTag() != Tag(0xfffe, 0xe000)); // We should not read the next item
#endif
      InsertDataElement(de);
      const VL oflen = de.GetLength<TDE>();
      l += oflen;
      const std::streampos curpos = is.tellg();
      // Bug_Philips_ItemTag_3F3F (0x2005, 0x1080): for some reason computation of fails
      if (l == 70 && locallength == 63)
      {
        mdcmWarningMacro("PMS: Super bad hack. Changing length");
        length = locallength = 140;
      }
      if ((curpos - startpos) + 1 == l)
      {
        throw std::logic_error("Papyrus odd padding");
      }
      if (l > locallength)
      {
        if ((curpos - startpos) == locallength)
        {
          // this means that something went wrong somewhere, and upon recomputing the length
          // we found a discrepandy with own vendor made its layout.
          // update the length directly
          locallength = length = l;
          throw std::logic_error("Changed Length");
        }
        else
        {
          mdcmAlwaysWarnMacro("Out of range SQ detected: " << l << " while max: " << locallength);
          throw std::logic_error("Out of Range");
        }
      }
    }
  }
  catch (const ParseException & pe)
  {
    if (pe.GetLastElement().GetTag() == Tag(0xfffe, 0xe000))
    {
      // mdcm-MR-PHILIPS-16-Multi-Seq.dcm
      // Philips engineer inserted 0xfffe,0x0000 instead of an item start element
      is.seekg(-6, std::ios::cur);
      length = locallength = l;
    }
    else if (de.GetTag() == Tag(0x7fe0, 0x0010) && de.IsUndefinedLength())
    {
      // Some bozo crafted an undefined length Pixel Data, but is actually
      // defined length. Since inside SQ/Item it should be possible to
      // compute the proper length
      is.seekg(-16, std::ios::cur);
      TDE pd;
      pd.template ReadPreValue<TSwap>(is);
      if ((pd.GetTag() != Tag(0x7fe0, 0x0010)) || (pd.GetVR() != VR::OB) || !pd.IsUndefinedLength())
      {
        throw std::logic_error("Exception");
      }
      const VL pdlen = locallength - l - 12;
      pd.SetVL(pdlen);
      pd.template ReadValue<TSwap>(is, true);
      InsertDataElement(pd);
      length = locallength = l;
    }
    else
    {
      // mdcmDataExtra/mdcmBreakers/BuggedDicomWorksImage_Hopeless.dcm
      mdcmErrorMacro("Last Tag is : " << pe.GetLastElement().GetTag());
      throw std::logic_error("Unhandled");
    }
  }
  catch (const std::logic_error & pe)
  {
    if (strcmp(pe.what(), "Out of Range") == 0)
    {
      // BogugsItemAndSequenceLength.dcm
      // This is most likely the "Out of Range" one
      // Cautiously read until we find the next item starter and then stop.
      while (de.Read<TDE, TSwap>(is) && de.GetTag() != Tag(0xfffe, 0xe000) && de.GetTag().GetElement() != 0x0)
      {
        InsertDataElement(de);
        l += de.GetLength<TDE>();
      }
      // seek back since we read the next item starter
      int iteml = de.GetLength<TDE>();
      if (!de.GetTag().GetElement())
      {
        assert(iteml == 12);
        (void)iteml;
        is.seekg(-12, std::ios::cur);
      }
      else
      {
        is.seekg(-4, std::ios::cur);
      }
      // fix the length now
      length = locallength = l;
      mdcmAlwaysWarnMacro("Item length is wrong");
      throw std::logic_error("Changed Length");
    }
    else if (strcmp(pe.what(), "Papyrus odd padding") == 0)
    {
      is.get();
      throw std::logic_error("Changed Length");
    }
    else
    {
      throw pe;
    }
  }
  assert(l <= locallength);
  return is;
}

template <typename TDE, typename TSwap>
const std::ostream &
DataSet::Write(std::ostream & os) const
{
  typename DataSet::ConstIterator it = DES.begin();
  for (; it != DES.end(); ++it)
  {
    const DataElement & de = *it;
    de.Write<TDE, TSwap>(os);
  }
  return os;
}

} // end namespace mdcm

#endif // MDCMDATASET_TXX
