/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "mdcmPVRGCodec.h"
#include "mdcmTransferSyntax.h"
#include "mdcmDataElement.h"
#include "mdcmFilename.h"
#include "mdcmSystem.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmPNMCodec.h"
#include "mdcmByteSwap.txx"

namespace mdcm
{
/*
http://groups.google.com/group/comp.compression/browse_thread/thread/e2e20d85a436cfa5
*/

PVRGCodec::PVRGCodec()
{
  NeedByteSwap = true;
}

PVRGCodec::~PVRGCodec()
{
}

bool PVRGCodec::CanDecode(TransferSyntax const &ts) const
{
#ifndef MDCM_USE_PVRG
  (void)ts;
  return false;
#else
  return ts == TransferSyntax::JPEGBaselineProcess1
      || ts == TransferSyntax::JPEGExtendedProcess2_4
      || ts == TransferSyntax::JPEGExtendedProcess3_5
      || ts == TransferSyntax::JPEGSpectralSelectionProcess6_8
      || ts == TransferSyntax::JPEGFullProgressionProcess10_12
      || ts == TransferSyntax::JPEGLosslessProcess14
      || ts == TransferSyntax::JPEGLosslessProcess14_1;
#endif
}

bool PVRGCodec::CanCode(TransferSyntax const &) const
{
  return false;
}

/*
 * ./bin/pvrg-jpeg -d -s jpeg.jpg -ci 0 out.raw
 *
 * means decompress input file: jpeg.jpg into out.raw
 * warning the -ci is important otherwise JFIF is assumed
 * and comp # is assumed to be 1, -u reduce verbosity
 *
 */
bool PVRGCodec::Decode(DataElement const &in, DataElement &out)
{
#ifndef MDCM_USE_PVRG
  (void)in;
  (void)out;
  return false;
#else
  const SequenceOfFragments *sf = in.GetSequenceOfFragments();
  if(!sf)
  {
    mdcmDebugMacro("Could not find SequenceOfFragments");
    return false;
  }
#ifdef MDCM_USE_SYSTEM_PVRG
  std::string pvrg_command = MDCM_PVRG_JPEG_EXECUTABLE;
#else
  Filename fn(System::GetCurrentProcessFileName());
  std::string executable_path = fn.GetPath();
  std::string pvrg_command = executable_path + "/mdcmpvrg";
#endif
  if(!System::FileExists(pvrg_command.c_str()))
  {
    mdcmErrorMacro("Could not find: " << pvrg_command);
    return false;
  }

  // http://msdn.microsoft.com/en-us/library/hs3e7355.aspx
  char name2[L_tmpnam];
  char * input = std::tmpnam(name2);
  if(!input) return false;

  std::ofstream outfile(input, std::ios::binary);
  sf->WriteBuffer(outfile);
  outfile.close();

  // -u : set Notify to 0 (less verbose)
  pvrg_command += " -d -u ";
  pvrg_command += "-s ";
  pvrg_command += input;

  mdcmDebugMacro(pvrg_command);
  int ret = system(pvrg_command.c_str());
  if(ret != 0)
  {
    mdcmErrorMacro("PVRG error: " << ret);
    return false;
  }

  int numoutfile = GetPixelFormat().GetSamplesPerPixel();
  std::string wholebuf;
  for(int file = 0; file < numoutfile; ++file)
  {
    std::ostringstream os;
    os << input;
    os << ".";
    os << file;
    const std::string altfile = os.str();
    const size_t len = System::FileSize(altfile.c_str());
    if(!len)
    {
      mdcmDebugMacro("Output file is empty: " << altfile);
      return false;
    }
    const char * rawfile = altfile.c_str();

    mdcmDebugMacro("Processing: " << rawfile);
    std::ifstream is(rawfile, std::ios::binary);
    std::string buf;
    buf.resize(len);
    is.read(&buf[0], len);
    out.SetTag(Tag(0x7fe0,0x0010));

    if(PF.GetBitsAllocated() == 16)
    {
      ByteSwap<uint16_t>::SwapRangeFromSwapCodeIntoSystem((uint16_t*)
        &buf[0],
#ifdef MDCM_WORDS_BIGENDIAN
        SwapCode::LittleEndian,
#else
        SwapCode::BigEndian,
#endif
        len/2);
    }
    wholebuf.insert(wholebuf.end(), buf.begin(), buf.end());
    if(!System::RemoveFile(rawfile))
    {
      mdcmErrorMacro("Could not delete output: " << rawfile);
    }
  }
  out.SetByteValue(&wholebuf[0], (uint32_t)wholebuf.size());
  if(numoutfile == 3)
  {
    this->PlanarConfiguration = 1;
  }

  if(!System::RemoveFile(input))
  {
    mdcmErrorMacro("Could not delete input: " << input);
  }

  LossyFlag = true; // FIXME
  return true;
#endif
}

void PVRGCodec::SetLossyFlag(bool l)
{
  LossyFlag = l;
}

bool PVRGCodec::Code(DataElement const &, DataElement &)
{
  return false;
}

ImageCodec * PVRGCodec::Clone() const
{
  return NULL;
}

} // end namespace mdcm
