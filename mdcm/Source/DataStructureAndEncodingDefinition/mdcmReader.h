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
#ifndef MDCMREADER_H
#define MDCMREADER_H

#include "mdcmFile.h"
#include <fstream>

namespace mdcm
{
  class StreamImageReader;
/**
 * Reader
 *
 * This class is a non-validating reader, it will only performs well-
 * formedness check only, and to some extent catch known error (non
 * well-formed document).
 *
 * A DataSet DOES NOT contains group 0x0002 (see FileMetaInformation)
 *
 * This is really a DataSet reader. This will not make sure the dataset conform
 * to any IOD at all.
 * MDCM will not produce warning for non-alphabetical order.
 *
 */
class MDCM_EXPORT Reader
{
public:
  Reader();
  virtual ~Reader();
  virtual bool Read();
  void SetFileName(const char*);
  void SetStream(std::istream & input_stream) { Stream = &input_stream; }
  const File & GetFile() const { return *F; }
  File & GetFile() { return *F; }
  void SetFile(File & file) { F = &file; }
  bool ReadUpToTag(const Tag & tag, std::set<Tag> const & skiptags = std::set<Tag>());
  bool ReadSelectedTags(std::set<Tag> const & tags, bool readvalues = true);
  bool ReadSelectedPrivateTags(std::set<PrivateTag> const & ptags, bool readvalues = true);
  bool CanRead() const;
#if 0
  // For wrapped language, return type is compatible with System::FileSize return type
  // Use native std::streampos / std::streamoff directly from the stream from C++
  size_t GetStreamCurrentPosition() const;
#endif

protected:
  bool ReadPreamble();
  bool ReadMetaInformation();
  bool ReadDataSet();
  SmartPointer<File> F;
#if 0
  friend class StreamImageReader; //to grab the GetStreamPtr
  //MM: this function is added for the StreamImageReader, which needs to read
  //up to the pixel data and then stops right before reading the pixel data.
  //it's used to get that position, so that reading can continue
  //apace once the read function is called.
  //so, this function gets the stream directly, and then allows for position information
  //from the tellg function, and allows for stream/pointer manip in order
  //to read the pixel data.  Note, of course, that reading pixel elements
  //will still have to be subject to endianness swaps, if necessary.
  std::istream * GetStreamPtr() const { return Stream; }
#endif

private:
  template <typename T_Caller>
  bool InternalReadCommon(const T_Caller &);
  TransferSyntax GuessTransferSyntax();
  std::istream  * Stream;
  std::ifstream * Ifstream;
};

} // end namespace mdcm

#endif //MDCMREADER_H
