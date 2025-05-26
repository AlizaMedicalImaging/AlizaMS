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

#include "mdcmScanner.h"
#include "mdcmReader.h"
#include "mdcmDictEntry.h"
#include "mdcmElement.h"
#include "mdcmByteValue.h"
#include "mdcmAttribute.h"
#include "mdcmDataSetHelper.h"
#include "mdcmProgressEvent.h"
#include "mdcmFileNameEvent.h"
#include <algorithm>
#include <utility>

#define stringBinaryVR(type)                                \
  case VR::type:                                            \
  {                                                         \
    Element<VR::type, VM::VM1_n> el;                        \
    if (!de.IsEmpty())                                      \
    {                                                       \
      el.Set(de.GetValue());                                \
      if (el.GetLength())                                   \
      {                                                     \
        os << el.GetValue();                                \
        for (unsigned int i = 1; i < el.GetLength(); ++i)   \
        {                                                   \
          os << '\\' << el.GetValue(i);                     \
        }                                                   \
        retvalue = os.str();                                \
      }                                                     \
    }                                                       \
  }                                                         \
  break


namespace mdcm
{

std::string
Scanner::GetString(const DataElement & de, const DataSet & ds, const bool implicit, const Dict & dict) const
{
  std::string r("");
  if (ds.IsEmpty())
    return r;
  const Tag t = de.GetTag();
  if (t.IsIllegal() || t.IsPrivate() || t.IsPrivateCreator())
    return r;
  const DataElement & de2 = ds.GetDataElement(t);
  if (de2.IsEmpty())
    return r;
  VR                vr;
  const DictEntry & e = dict.GetDictEntry(t);
  if (!implicit)
  {
    vr = de2.GetVR();
  }
  else
  {
    vr = e.GetVR();
  }
  if (vr == VR::UN)
  {
    vr = e.GetVR();
  }
  if (vr == VR::INVALID || vr == VR::UN)
    return r;
  if (vr == VR::US_SS || vr == VR::US_SS_OW)
  {
    vr = VR::SS; // FIXME
  }
  const ByteValue * bv = de.GetByteValue();
  if (!bv)
    return r;
  if (VR::IsASCII2(vr))
  {
    if (de.GetVL() > 0)
    {
      std::string s = std::string(bv->GetPointer(), bv->GetLength());
      // Remove trailing '\0', strlen is lower or equal to size()
      s.resize(std::min(s.size(), strlen(s.c_str())));
      r = std::move(s);
    }
  }
  else
  {
    std::ostringstream os;
    std::string        retvalue("");
    switch (vr)
    {
      stringBinaryVR(AT);
      stringBinaryVR(FL);
      stringBinaryVR(FD);
      stringBinaryVR(SS);
      stringBinaryVR(SL);
      stringBinaryVR(SV);
      stringBinaryVR(US);
      stringBinaryVR(UL);
      stringBinaryVR(UV);
      default:
        break;
    }
    r = std::move(retvalue);
  }
  return r;
}

void
Scanner::AddTag(const Tag & t)
{
  Tags.insert(t);
}

void
Scanner::ClearTags()
{
  Tags.clear();
}

bool
Scanner::Scan(const std::vector<std::string> & filenames, const Dict & dict)
{
  this->InvokeEvent(StartEvent());
  if (!Tags.empty())
  {
    Mappings.clear();
    TagToValue d0;
    Mappings[""] = std::move(d0);
    const size_t filenames_size = filenames.size();
    Filenames.clear();
    for (size_t x = 0; x < filenames_size; ++x)
    {
      Filenames.push_back(filenames.at(x));
    }
    // Find the tag with the highest value (get the one from the end of the std::set)
    Tag last;
    if (!Tags.empty())
    {
      TagsType::const_reverse_iterator it1 = Tags.rbegin();
      const Tag &                      publiclast = *it1;
      last = publiclast;
    }
    Progress = 0.0;
    const double progresstick =
      (filenames_size > 0) ? 1.0 / static_cast<double>(filenames_size) : 0.0;
    for (std::vector<std::string>::const_iterator it = Filenames.cbegin(); it != Filenames.cend(); ++it)
    {
      Reader       reader;
      const char * filename = it->c_str();
      assert(filename);
      reader.SetFileName(filename);
      bool read{};
      try
      {
        read = reader.ReadUpToTag(last);
      }
      catch (...)
      {
        mdcmAlwaysWarnMacro("Failed to read:" << filename);
      }
      if (read)
      {
        const File & file = reader.GetFile();
        ProcessPublicTag(filename, file, dict);
      }
      Progress += progresstick;
      ProgressEvent pe;
      pe.SetProgress(Progress);
      this->InvokeEvent(pe);
      FileNameEvent fe(filename);
      this->InvokeEvent(fe);
    }
  }
  this->InvokeEvent(EndEvent());
  return true;
}

const std::vector<std::string> &
Scanner::GetFilenames() const
{
  return Filenames;
}

bool
Scanner::IsKey(const char * filename) const
{
  if (filename && *filename)
  {
    MappingType::const_iterator it2 = Mappings.find(filename);
    return (it2 != Mappings.cend());
  }
  return false;
}

std::vector<std::string>
Scanner::GetKeys() const
{
  std::vector<std::string> keys;
  for (std::vector<std::string>::const_iterator file = Filenames.cbegin(); file != Filenames.cend(); ++file)
  {
    const char * filename = file->c_str();
    if (IsKey(filename))
    {
      keys.push_back(filename);
    }
  }
  assert(keys.size() <= Filenames.size());
  return keys;
}

// Tag 't' should have been added via AddTag() prior to the Scan() call
const char *
Scanner::GetValue(const char * filename, const Tag & t) const
{
  assert(Tags.find(t) != Tags.end());
  const TagToValue & ftv = GetMapping(filename);
  if (ftv.find(t) != ftv.end())
  {
    return ftv.find(t)->second;
  }
  return nullptr;
}

const Scanner::ValuesType &
Scanner::GetValues() const
{
  return Values;
}

Scanner::ValuesType
Scanner::GetValues(const Tag & t) const
{
  ValuesType vt;
  for (std::vector<std::string>::const_iterator file = Filenames.cbegin(); file != Filenames.cend(); ++file)
  {
    const char *       filename = file->c_str();
    const TagToValue & ttv = GetMapping(filename);
    if (ttv.find(t) != ttv.end())
    {
      vt.insert(ttv.find(t)->second);
    }
  }
  return vt;
}

std::vector<std::string>
Scanner::GetOrderedValues(const Tag & t) const
{
  std::vector<std::string> theReturn;
  for (std::vector<std::string>::const_iterator file = Filenames.cbegin(); file != Filenames.cend(); ++file)
  {
    const char *       filename = file->c_str();
    const TagToValue & ttv = GetMapping(filename);
    if (ttv.find(t) != ttv.end())
    {
      std::string theVal = std::string(ttv.find(t)->second);
      if (std::find(theReturn.begin(), theReturn.end(), theVal) == theReturn.end())
      {
        theReturn.push_back(std::move(theVal)); // only add new tags to the list
      }
    }
  }
  return theReturn;
}

const Scanner::TagToValue &
Scanner::GetMapping(const char * filename) const
{
  if (filename && *filename)
  {
    if (Mappings.find(filename) != Mappings.end())
    {
      return Mappings.find(filename)->second;
    }
  }
  if (Mappings.find("") != Mappings.end()) // to silence Synopsys Coverity detection
  {
    return Mappings.find("")->second;
  }
  return NoOpTagToValue;
}

const char *
Scanner::GetFilenameFromTagToValue(const Tag & t, const char * valueref) const
{
  const char * filenameref{};
  if (valueref)
  {
    size_t len = strlen(valueref);
    if (len && valueref[len - 1] == ' ')
    {
      --len;
    }
    for (std::vector<std::string>::const_iterator file = Filenames.cbegin(); file != Filenames.cend() && !filenameref; ++file)
    {
      const char * filename = file->c_str();
      const char * value = GetValue(filename, t);
      if (value && strncmp(value, valueref, len) == 0)
      {
        filenameref = filename;
      }
    }
  }
  return filenameref;
}

std::vector<std::string>
Scanner::GetAllFilenamesFromTagToValue(const Tag & t, const char * valueref) const
{
  std::vector<std::string> theReturn;
  if (valueref)
  {
    const std::string valueref_str = String<>::Trim(valueref);
    for (std::vector<std::string>::const_iterator file = Filenames.cbegin(); file != Filenames.cend(); ++file)
    {
      const char *      filename = file->c_str();
      const char *      value = GetValue(filename, t);
      const std::string value_str = String<>::Trim(value);
      if (value_str == valueref_str)
      {
        theReturn.push_back(filename);
      }
    }
  }
  return theReturn;
}

const Scanner::TagToValue &
Scanner::GetMappingFromTagToValue(const Tag & t, const char * valueref) const
{
  return GetMapping(GetFilenameFromTagToValue(t, valueref));
}

void
Scanner::ProcessPublicTag(const char * filename, const File & file, const Dict & dict)
{
  if (!(filename && *filename))
    return;
  TagToValue &                 mapping = Mappings[filename];
  const DataSet &              ds = file.GetDataSet();
  const FileMetaInformation &  header = file.GetHeader();
  const mdcm::TransferSyntax & ts = header.GetDataSetTransferSyntax();
  const bool                   implicit = ts.IsImplicit();
  for (TagsType::const_iterator tag = Tags.cbegin(); tag != Tags.cend(); ++tag)
  {
    if (tag->GetGroup() == 0x2)
    {
      const DataElement & de = header.GetDataElement(*tag);
      if (de != DataElement(Tag(0xffff, 0xffff)))
      {
        std::string s(GetString(de, header, implicit, dict));
        Values.insert(s);
        if (Values.find(s) != Values.end())
        {
          mapping.insert(TagToValue::value_type(*tag, Values.find(s)->c_str()));
        }
      }
    }
    else
    {
      const DataElement & de = ds.GetDataElement(*tag);
      if (de != DataElement(Tag(0xffff, 0xffff)))
      {
        std::string s(GetString(de, ds, implicit, dict));
        Values.insert(s);
        if (Values.find(s) != Values.end())
        {
          mapping.insert(TagToValue::value_type(*tag, Values.find(s)->c_str()));
        }
      }
    }
  }
}

#if 0
void Scanner::Print(std::ostream & os) const
{
  os << "Values:\n";
  for(ValuesType::const_iterator it = Values.cbegin(); it != Values.cend();
    ++it)
  {
    os << *it << '\n';
  }
  os << "Mapping:\n";
  for(std::vector<std::string>::const_iterator file = Filenames.cbegin(); file != Filenames.cend(); ++file)
  {
    const char * filename = file->c_str();
    assert(filename && *filename);
    bool b = IsKey(filename);
    const char * comment = (!b) ? "could not read" : "could read";
    os << "Filename: " << filename << " (" << comment << ")\n";
    if(Mappings.find(filename) != Mappings.end())
    {
      const TagToValue & mapping = GetMapping(filename);
      TagToValue::const_iterator it = mapping.cbegin();
      for(; it != mapping.cend(); ++it)
      {
        const Tag & tag = it->first;
        const char * value = it->second;
        os << tag << " -> [" << value << "]\n";
      }
    }
  }
}
#else
void
Scanner::Print(std::ostream &) const
{}
#endif

} // end namespace mdcm
