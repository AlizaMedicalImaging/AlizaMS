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
#include "mdcmPVRGCodec.h"
#include "mdcmTransferSyntax.h"
#include "mdcmDataElement.h"
#include "mdcmFilename.h"
#include "mdcmSystem.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmByteSwap.h"

#ifdef MDCM_USE_PVRG

#  include <sys/stat.h>
#  ifdef _WIN32
#    include <windows.h>
#  endif
#  if defined(_WIN32) && (defined(_MSC_VER) || defined(__WATCOMC__) || defined(__BORLANDC__) || defined(__MINGW32__))
#    include <io.h>
#    include <direct.h>
#    define _unlink unlink
#  else
#    include <dlfcn.h>
#    include <sys/types.h>
#    include <fcntl.h>
#    include <unistd.h>
#    include <strings.h>
#  endif

namespace mdcm
{

static bool
GetPermissions(const char * file, unsigned short & mode)
{
  if (!file)
    return false;
  struct stat st;
  if (stat(file, &st) < 0)
    return false;
  mode = (short)st.st_mode;
  return true;
}

static bool
SetPermissions(const char * file, unsigned short mode)
{
  if (!file)
  {
    return false;
  }
  if (!System::FileExists(file))
    return false;
  if (chmod(file, mode) < 0)
    return false;
  return true;
}

static bool
RemoveFile(const char * source)
{
#  ifdef _WIN32
  unsigned short mode;
  if (!GetPermissions(source, mode))
    return false;
  System::SetPermissions(source, S_IWRITE);
#  endif
  const bool res = unlink(source) != 0 ? false : true;
#  ifdef _WIN32
  if (!res)
    SetPermissions(source, mode);
#  endif
  return res;
}

} // namespace mdcm
#endif

namespace mdcm
{
// http://groups.google.com/group/comp.compression/browse_thread/thread/e2e20d85a436cfa5
// PHILIPS_Gyroscan-12-Jpeg_Extended_Process_2_4.dcm
PVRGCodec::PVRGCodec()
{
  NeedByteSwap = true;
}

bool
PVRGCodec::CanDecode(TransferSyntax const & ts) const
{
#ifndef MDCM_USE_PVRG
  (void)ts;
  return false;
#else
  return ts == TransferSyntax::JPEGBaselineProcess1 || ts == TransferSyntax::JPEGExtendedProcess2_4 ||
         ts == TransferSyntax::JPEGExtendedProcess3_5 || ts == TransferSyntax::JPEGSpectralSelectionProcess6_8 ||
         ts == TransferSyntax::JPEGFullProgressionProcess10_12 || ts == TransferSyntax::JPEGLosslessProcess14 ||
         ts == TransferSyntax::JPEGLosslessProcess14_1;
#endif
}

/*
 * ./bin/pvrg-jpeg -d -s jpeg.jpg -ci 0 out.raw
 *
 * means decompress input file: jpeg.jpg into out.raw
 * warning the -ci is important otherwise JFIF is assumed
 * and comp # is assumed to be 1, -u reduce verbosity
 *
 */
bool
PVRGCodec::Decode(DataElement const & in, DataElement & out)
{
#ifndef MDCM_USE_PVRG
  (void)in;
  (void)out;
  return false;
#else
  const SequenceOfFragments * sf = in.GetSequenceOfFragments();
  if (!sf)
  {
    mdcmDebugMacro("Could not find SequenceOfFragments");
    return false;
  }
#  ifdef MDCM_USE_SYSTEM_PVRG
  std::string pvrg_command = MDCM_PVRG_JPEG_EXECUTABLE;
#  else
#    ifdef _WIN32
  std::string pvrg_command = "mdcmpvrg";
#    else
  Filename    fn(System::GetCurrentProcessFileName());
  std::string executable_path = fn.GetPath();
  std::string pvrg_command = executable_path + "/mdcmpvrg";
#    endif
  //
#  endif
  if (!System::FileExists(pvrg_command.c_str()))
  {
    mdcmErrorMacro("Could not find: " << pvrg_command);
    return false;
  }
  // http://msdn.microsoft.com/en-us/library/hs3e7355.aspx
  char   name2[L_tmpnam];
  char * input = std::tmpnam(name2);
  if (!input)
    return false;
  std::ofstream outfile(input, std::ios::binary);
  sf->WriteBuffer(outfile);
  outfile.close();
  // -u : set Notify to 0 (less verbose)
  pvrg_command += " -d -u ";
  pvrg_command += "-s ";
  pvrg_command += input;
  int ret = system(pvrg_command.c_str());
  if (ret != 0)
  {
    mdcmErrorMacro("PVRG error: " << ret);
    return false;
  }
  int         numoutfile = GetPixelFormat().GetSamplesPerPixel();
  std::string wholebuf;
  for (int file = 0; file < numoutfile; ++file)
  {
    std::ostringstream os;
    os << input;
    os << ".";
    os << file;
    const std::string altfile = os.str();
    const size_t      len = System::FileSize(altfile.c_str());
    if (!len)
    {
      mdcmDebugMacro("Output file is empty: " << altfile);
      return false;
    }
    const char * rawfile = altfile.c_str();
    mdcmDebugMacro("Processing: " << rawfile);
    std::ifstream is(rawfile, std::ios::binary);
    std::string   buf;
    buf.resize(len);
    is.read(buf.data(), len);
    out.SetTag(Tag(0x7fe0, 0x0010));
    if (PF.GetBitsAllocated() == 16)
    {
      ByteSwap<uint16_t>::SwapRangeFromSwapCodeIntoSystem(reinterpret_cast<uint16_t *>(buf.data()),
#  ifdef MDCM_WORDS_BIGENDIAN
                                                          SwapCode::LittleEndian,
#  else
                                                          SwapCode::BigEndian,
#  endif
                                                          len / 2);
    }
    wholebuf.insert(wholebuf.end(), buf.begin(), buf.end());
    if (!RemoveFile(rawfile))
    {
      mdcmErrorMacro("Could not delete output: " << rawfile);
    }
  }
  out.SetByteValue(wholebuf.data(), static_cast<uint32_t>(wholebuf.size()));
  if (numoutfile == 3)
  {
    this->PlanarConfiguration = 1;
  }
  if (!RemoveFile(input))
  {
    mdcmErrorMacro("Could not delete input: " << input);
  }
  LossyFlag = true; // FIXME
  return true;
#endif
}

bool
PVRGCodec::Code(DataElement const &, DataElement &)
{
  return false;
}

} // end namespace mdcm
