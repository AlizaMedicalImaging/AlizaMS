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

namespace mdcm_ns
{
  class StreamImageReader;
/**
 * \brief Reader ala DOM (Document Object Model)
 *
 * \details This class is a non-validating reader, it will only performs well-
 * formedness check only, and to some extent catch known error (non
 * well-formed document).
 *
 * Detailled description here
 *
 * A DataSet DOES NOT contains group 0x0002 (see FileMetaInformation)
 *
 * This is really a DataSet reader. This will not make sure the dataset conform
 * to any IOD at all. This is a completely different step. The reasoning was
 * that user could control the IOD there lib would handle and thus we would not
 * be able to read a DataSet if the IOD was not found Instead we separate the
 * reading from the validation.
 *
 * \note
 * From MDCM1.x. Users will realize that one feature is missing
 * from this DOM implementation. In MDCM 1.x user used to be able to
 * control the size of the Value to be read. By default it was 0xfff.
 * The main author of MDCM2 thought this was too dangerous and harmful and
 * therefore this feature did not make it into MDCM2
 *
 * \warning
 * MDCM will not produce warning for unorder (non-alphabetical order).
 *
 * \see Writer FileMetaInformation DataSet File
 */
class MDCM_EXPORT Reader
{
public:
  Reader();
  virtual ~Reader();

  /// Main function to read a file
  virtual bool Read(); // Execute()

  /// Set the filename to open. This will create a std::ifstream internally
  /// See SetStream if you are dealing with different std::istream object
  void SetFileName(const char*);
  void SetFileNameUTF8(const char*);

  /// Set the open-ed stream directly
  void SetStream(std::istream & input_stream)
  {
    Stream = &input_stream;
  }

  /// Set/Get File
  const File & GetFile() const { return *F; }

  /// Set/Get File
  File & GetFile() { return *F; }

  /// Set/Get File
  void SetFile(File & file) { F = &file; }

  /// Will read only up to Tag \param tag and skipping any tag specified in
  /// \param skiptags
  bool ReadUpToTag(const Tag & tag, std::set<Tag> const & skiptags = std::set<Tag>() );

  /// Will only read the specified selected tags.
  bool ReadSelectedTags(std::set<Tag> const & tags, bool readvalues = true);

  /// Will only read the specified selected private tags.
  bool ReadSelectedPrivateTags(std::set<PrivateTag> const & ptags, bool readvalues = true);

  /// Test whether this is a DICOM file
  /// \warning need to call either SetFileName or SetStream first
  bool CanRead() const;

  /// For wrapped language. return type is compatible with System::FileSize return type
  /// Use native std::streampos / std::streamoff directly from the stream from C++
  size_t GetStreamCurrentPosition() const;

protected:
  bool ReadPreamble();
  bool ReadMetaInformation();
  bool ReadDataSet();
  SmartPointer<File> F;
  //need to be friended to be able to grab the GetStreamPtr
  friend class StreamImageReader;
  //this function is added for the StreamImageReader, which needs to read
  //up to the pixel data and then stops right before reading the pixel data.
  //it's used to get that position, so that reading can continue
  //apace once the read function is called.
  //so, this function gets the stream directly, and then allows for position information
  //from the tellg function, and allows for stream/pointer manip in order
  //to read the pixel data.  Note, of course, that reading pixel elements
  //will still have to be subject to endianness swaps, if necessary.
  std::istream * GetStreamPtr() const { return Stream; }

private:
  template <typename T_Caller>
  bool InternalReadCommon(const T_Caller &);
  TransferSyntax GuessTransferSyntax();
  std::istream  * Stream;
  std::ifstream * Ifstream;
};

} // end namespace mdcm_ns


#endif //MDCMREADER_H
