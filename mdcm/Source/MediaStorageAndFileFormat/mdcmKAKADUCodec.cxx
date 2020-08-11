/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "mdcmKAKADUCodec.h"
#include "mdcmTransferSyntax.h"
#include "mdcmDataElement.h"
#include "mdcmFilename.h"
#include "mdcmSystem.h"
#include "mdcmSequenceOfFragments.h"
#include "mdcmPNMCodec.h"
#include "mdcmByteSwap.txx"

namespace mdcm
{

KAKADUCodec::KAKADUCodec()
{
  //NeedByteSwap = true;
}

KAKADUCodec::~KAKADUCodec()
{
}

bool KAKADUCodec::CanDecode(TransferSyntax const &ts) const
{
#ifndef MDCM_USE_KAKADU
  (void)ts;
  return false;
#else
  return ts == TransferSyntax::JPEG2000Lossless
      || ts == TransferSyntax::JPEG2000;
#endif
}

bool KAKADUCodec::CanCode(TransferSyntax const &) const
{
  return false;
}

bool KAKADUCodec::Decode(DataElement const &in, DataElement &out)
{
#ifndef MDCM_USE_KAKADU
  (void)in;
  (void)out;
  return false;
#else
  const SequenceOfFragments *sf = in.GetSequenceOfFragments();
  if(!sf) return false;

  if(NumberOfDimensions == 2)
  {
    // FIXME
    char *tempinput  = tempnam(0, "mdcminkduexp");
    char *tempoutput = tempnam(0, "mdcmoutkduexp");
    if(!tempinput || !tempoutput)
    {
      return false;
    }
    std::string input = tempinput;
    input += ".j2k";
    std::string output = tempoutput;
    output += ".rawl";

    std::ofstream outfile(input.c_str(), std::ios::binary);
    sf->WriteBuffer(outfile);
    outfile.close();

#ifdef MDCM_USE_SYSTEM_KAKADU
    std::string kakadu_command = MDCM_KAKADU_EXPAND_EXECUTABLE;
    kakadu_command += " -quiet";
#else
#error not implemented
#endif
    // ./bin/kakadujpeg -d -s jpeg.jpg -ci 0 out.raw
    kakadu_command += " -i ";
    kakadu_command += input;
    kakadu_command += " -o ";
    kakadu_command += output;

    //std::cerr << kakadu_command << std::endl;
    mdcmDebugMacro(kakadu_command);
    int ret = system(kakadu_command.c_str());
    //std::cerr << "system: " << ret << std::endl;

    size_t len = System::FileSize(output.c_str());
    if(!len) return false;

    std::ifstream is(output.c_str(), std::ios::binary);
    char * buf = new char[len];
    is.read(buf, len);
    out.SetTag(Tag(0x7fe0,0x0010));
    out.SetByteValue(buf, len);
    delete[] buf;

    if(!System::RemoveFile(input.c_str()))
    {
      mdcmErrorMacro("Could not delete input: " << input);
    }

    if(!System::RemoveFile(output.c_str()))
    {
      mdcmErrorMacro("Could not delete output: " << output);
    }

    free(tempinput);
    free(tempoutput);
  }
  else if (NumberOfDimensions == 3)
  {
    std::stringstream os;
    if(sf->GetNumberOfFragments() != Dimensions[2])
    {
      mdcmErrorMacro("Not handled");
      return false;
    }

    for(unsigned int i = 0; i < sf->GetNumberOfFragments(); ++i)
    {
      // FIXME
      char *tempinput  = tempnam(0, "mdcminkduexp");
      char *tempoutput = tempnam(0, "mdcmoutkduexp");
      if(!tempinput || !tempoutput)
      {
        return false;
      }
      std::string input = tempinput;
      input += ".j2k";
      std::string output = tempoutput;
      output += ".rawl";

      std::ofstream outfile(input.c_str(), std::ios::binary);
      const Fragment &frag = sf->GetFragment(i);
      assert(!frag.IsEmpty());
      const ByteValue *bv = frag.GetByteValue();
      assert(bv);
      //sf->WriteBuffer(outfile);
      bv->WriteBuffer(outfile);
      outfile.close();

#ifdef MDCM_USE_SYSTEM_KAKADU
      std::string kakadu_command = MDCM_KAKADU_EXPAND_EXECUTABLE;
      kakadu_command += " -quiet";
#else
#error not implemented
#endif
      // ./bin/kakadujpeg -d -s jpeg.jpg -ci 0 out.raw
      kakadu_command += " -i ";
      kakadu_command += input;
      kakadu_command += " -o ";
      kakadu_command += output;

      //std::cerr << kakadu_command << std::endl;
      mdcmDebugMacro(kakadu_command);
      int ret = system(kakadu_command.c_str());
      //std::cerr << "system: " << ret << std::endl;

      size_t len = System::FileSize(output.c_str());
      if(!len) return false;

      std::ifstream is(output.c_str(), std::ios::binary);
      char * buf = new char[len];
      is.read(buf, len);
      os.write(buf, len);
      //out.SetByteValue(buf, len);
      delete[] buf;

      if(!System::RemoveFile(input.c_str()))
      {
        mdcmErrorMacro("Could not delete input: " << input);
      }
      if(!System::RemoveFile(output.c_str()))
      {
        mdcmErrorMacro("Could not delete output: " << output);
      }
      free(tempinput);
      free(tempoutput);
    }
    std::string str = os.str();
    assert(str.size());
    out.SetTag(Tag(0x7fe0,0x0010));
    out.SetByteValue(&str[0], str.size());
  }
  else
  {
    mdcmErrorMacro("Not handled");
    return false;
  }

  //LossyFlag = true; // FIXME

  return true;
#endif
}

bool KAKADUCodec::Code(DataElement const &in, DataElement &out)
{
#ifndef MDCM_USE_KAKADU
  (void)in;
  (void)out;
  return false;
#else
  // TODO
  return false;
#endif
}

ImageCodec * KAKADUCodec::Clone() const
{
  return NULL;
}

} // end namespace mdcm
