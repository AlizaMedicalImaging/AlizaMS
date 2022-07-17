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

#include "mdcmWriter.h"
#include "mdcmSystem.h"
#include "mdcmFileMetaInformation.h"
#include "mdcmDataSet.h"
#include "mdcmTrace.h"
#include "mdcmSwapper.h"
#include "mdcmDataSet.h"
#include "mdcmExplicitDataElement.h"
#include "mdcmImplicitDataElement.h"
#include "mdcmValue.h"
#include "mdcmValue.h"
#include "mdcmItem.h"
#include "mdcmSequenceOfItems.h"
#include "mdcmParseException.h"
#include "mdcmDeflateStream.h"
#include <locale>

namespace mdcm
{

Writer::Writer()
  : Stream(NULL)
  , Ofstream(NULL)
  , F(new File)
  , CheckFileMetaInformation(true)
  , WriteDataSetOnly(false)
  , SkipUIDs(false)
{}

Writer::~Writer()
{
  if (Ofstream)
  {
    delete Ofstream;
    Ofstream = NULL;
    Stream = NULL;
  }
}

bool
Writer::Write()
{
  //
  std::locale current_locale = std::locale::global(std::locale::classic());
  //
  if (!Stream || !*Stream)
  {
    mdcmErrorMacro("No Filename");
    std::locale::global(current_locale);
    return false;
  }
  std::ostream &        os = *Stream;
  FileMetaInformation & Header = F->GetHeader();
  DataSet &             DS = F->GetDataSet();
  if (DS.IsEmpty())
  {
    mdcmErrorMacro("DS empty");
    std::locale::global(current_locale);
    return false;
  }
  // Check that 0002,0002 / 0008,0016 and 0002,0003 / 0008,0018 match?
  if (!WriteDataSetOnly)
  {
    if (CheckFileMetaInformation)
    {
      FileMetaInformation duplicate(Header);
      try
      {
        duplicate.FillFromDataSet(DS);
      }
      catch (const std::logic_error & ex)
      {
        mdcmAlwaysWarnMacro("Could not recreate the File Meta Header, please report:" << ex.what());
        std::locale::global(current_locale);
        return false;
      }
      duplicate.Write(os);
    }
    else
    {
      Header.Write(os);
    }
  }
  const TransferSyntax & ts = Header.GetDataSetTransferSyntax();
  if (!ts.IsValid())
  {
    mdcmErrorMacro("Invalid Transfer Syntax");
    std::locale::global(current_locale);
    return false;
  }
  if (ts == TransferSyntax::DeflatedExplicitVRLittleEndian)
  {
    try
    {
      zlib_stream::zip_ostream gzos(os);
      assert(ts.GetNegociatedType() == TransferSyntax::Explicit);
      DS.Write<ExplicitDataElement, SwapperNoOp>(gzos);
    }
    catch (...)
    {
      std::locale::global(current_locale);
      return false;
    }
    std::locale::global(current_locale);
    return true;
  }
  try
  {
    if (ts.GetSwapCode() == SwapCode::BigEndian)
    {
      // US-RGB-8-epicard.dcm is big endian
      if (ts.GetNegociatedType() == TransferSyntax::Implicit)
      {
        // There is no such thing as Implicit Big Endian... oh well
        // LIBIDO-16-ACR_NEMA-Volume.dcm
        DS.Write<ImplicitDataElement, SwapperDoOp>(os);
      }
      else
      {
        assert(ts.GetNegociatedType() == TransferSyntax::Explicit);
        DS.Write<ExplicitDataElement, SwapperDoOp>(os);
      }
    }
    else // LittleEndian
    {
      if (ts.GetNegociatedType() == TransferSyntax::Implicit)
      {
        DS.Write<ImplicitDataElement, SwapperNoOp>(os);
      }
      else
      {
        assert(ts.GetNegociatedType() == TransferSyntax::Explicit);
        DS.Write<ExplicitDataElement, SwapperNoOp>(os);
      }
    }
  }
  catch (const std::exception & ex)
  {
    mdcmAlwaysWarnMacro(ex.what());
    std::locale::global(current_locale);
    return false;
  }
  catch (...)
  {
    mdcmAlwaysWarnMacro("Unknown exception");
    std::locale::global(current_locale);
    return false;
  }
  os.flush();
  if (Ofstream)
  {
    Ofstream->close();
  }
  //
  std::locale::global(current_locale);
  //
  return true;
}

void
Writer::SetFileName(const char * p)
{
  if (Ofstream)
  {
    if (Ofstream->is_open())
    {
      Ofstream->close();
    }
    delete Ofstream;
  }
  Ofstream = new std::ofstream();
  if (p && *p)
  {
#if (defined(_MSC_VER) && defined(MDCM_WIN32_UNC))
    const std::wstring uncpath = mdcm::System::ConvertToUtf16(p);
    Ofstream->open(uncpath.c_str(), std::ios::out | std::ios::binary);
#else
    Ofstream->open(p, std::ios::out | std::ios::binary);
#endif
    assert(Ofstream->is_open());
    assert(!Ofstream->fail());
  }
  else
  {
    mdcmAlwaysWarnMacro("Reader failed (1)");
  }
  Stream = Ofstream;
}

void
Writer::SetStream(std::ostream & output_stream)
{
  Stream = &output_stream;
}

void
Writer::SetFile(const File & f)
{
  F = f;
}

File &
Writer::GetFile()
{
  return *F;
}

void
Writer::SetCheckFileMetaInformation(bool b)
{
  CheckFileMetaInformation = b;
}

void
Writer::CheckFileMetaInformationOff()
{
  CheckFileMetaInformation = false;
}

void
Writer::CheckFileMetaInformationOn()
{
  CheckFileMetaInformation = true;
}

void
Writer::SetWriteDataSetOnly(bool b)
{
  WriteDataSetOnly = b;
}

// this function is added for the StreamImageWriter, which needs to write
// up to the pixel data and then stops right before writing the pixel data.
// after that, for the raw codec at least, zeros are written for the length
// of the data
std::ostream *
Writer::GetStreamPtr() const
{
  return Stream;
}

bool
Writer::GetCheckFileMetaInformation() const
{
  return CheckFileMetaInformation;
}

void
Writer::SetSkipUIDs(bool b)
{
  SkipUIDs = b;
}

bool
Writer::GetSkipUIDs() const
{
  return SkipUIDs;
}

} // end namespace mdcm
