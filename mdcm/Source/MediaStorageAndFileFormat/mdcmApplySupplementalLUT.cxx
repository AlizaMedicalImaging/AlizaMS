/*********************************************************
 *
 * MDCM
 *
 * github.com/issakomi
 *
 *********************************************************/

#include "mdcmApplySupplementalLUT.h"
#include <climits>
#include <limits>

namespace mdcm
{

bool
ApplySupplementalLUT::Apply()
{
  Output = Input;
  const Bitmap &      image = *Input;
  const LookupTable & lut = image.GetLUT();
  int                 bitsample = lut.GetBitSample();
  if (!bitsample)
    return false;
  const unsigned long long len = image.GetBufferLength();
  std::vector<char>        v;
  v.resize(len);
  char * p = &v[0];
  image.GetBuffer(p);
  std::stringstream is;
  if (!is.write(p, len))
  {
    mdcmErrorMacro("Could not write to stringstream");
    return false;
  }
  DataElement &     de = Output->GetDataElement();
  std::vector<char> v2;
  v2.resize(len * 3);
  const int RedSubscipt = lut.DecodeSupplemental(&v2[0], v2.size(), &v[0], v.size());
  if (RedSubscipt > INT_MIN)
    m_RedSubscipt = RedSubscipt;
  assert(v2.size() < (size_t)std::numeric_limits<uint32_t>::max());
  de.SetByteValue(&v2[0], (uint32_t)v2.size());
  Output->GetLUT().Clear();
  Output->SetPhotometricInterpretation(PhotometricInterpretation::RGB);
  if (Output->GetPixelFormat().GetPixelRepresentation() != 0)
  {
    Output->GetPixelFormat().SetPixelRepresentation(0);
  }
  Output->GetPixelFormat().SetSamplesPerPixel(3);
  Output->SetPlanarConfiguration(0);
  const TransferSyntax & ts = image.GetTransferSyntax();
  if (ts.IsLossy())
  {
    return false;
  }
  if (ts.IsExplicit())
  {
    Output->SetTransferSyntax(TransferSyntax::ExplicitVRLittleEndian);
  }
  else
  {
    assert(ts.IsImplicit());
    Output->SetTransferSyntax(TransferSyntax::ImplicitVRLittleEndian);
  }
  return true;
}

int
ApplySupplementalLUT::GetRedSubscript() const
{
  return m_RedSubscipt;
}

} // end namespace mdcm
