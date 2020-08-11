/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "mdcmPGXCodec.h"
#include "mdcmTransferSyntax.h"
#include "mdcmSystem.h"
#include "mdcmDataElement.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmSwapper.h"
#include "mdcmFilenameGenerator.h"

namespace mdcm
{

PGXCodec::PGXCodec()
{
}

PGXCodec::~PGXCodec()
{
}

bool PGXCodec::CanDecode(TransferSyntax const &) const
{
  return false;
}

bool PGXCodec::CanCode(TransferSyntax const &) const
{
  return false;
}

bool PGXCodec::Write(const char * filename, const DataElement &out) const
{
  if(!filename) return false;
  std::vector<std::string> filenames;
  const PixelFormat& pf = GetPixelFormat();
  unsigned short nsamples = pf.GetSamplesPerPixel();
  FilenameGenerator fg;
  std::string base = filename;
  std::string::size_type dot_pos = base.size() - 4;
  std::string prefix = base.substr(0, dot_pos );
  fg.SetPrefix(prefix.c_str());
  fg.SetPattern("_%d.pgx");
  size_t zdim = Dimensions[2];
  size_t num_images = zdim * nsamples;
  fg.SetNumberOfFilenames(num_images);
  if(!fg.Generate()) return false;
  const ByteValue *bv = out.GetByteValue();
  if(!bv)
  {
    mdcmErrorMacro( "PGX Codec does not handle compress syntax."
      "You need to decompress first." );
    return false;
  }
  const unsigned int * dims = this->GetDimensions();
  size_t image_size = dims[0] * dims[1];
  const char * img_buffer = bv->GetPointer();
  for(unsigned int i = 0; i < num_images; ++i, img_buffer += image_size)
  {
    const char * targetname = fg.GetFilename(i);
    std::ofstream os(targetname, std::ios::binary);
    os << "PG ML ";
    os << (pf.GetPixelRepresentation() ? "-" : "+");
    os << " ";
    os << pf.GetBitsStored();
    os << " ";
    os << dims[0] << " " << dims[1] << "\n";
    os.write(img_buffer, image_size);
    os.close();
  }
  return true;
}

bool PGXCodec::Read(const char *, DataElement &) const
{
  return false;
}

bool PGXCodec::GetHeaderInfo(std::istream &, TransferSyntax &)
{
  return false;
}

ImageCodec * PGXCodec::Clone() const
{
  return NULL;
}

} // end namespace mdcm
