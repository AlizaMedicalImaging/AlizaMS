/*********************************************************
 *
 * MDCM
 *
 * github.com/issakomi
 *
 *********************************************************/

#ifndef MDCMAPPLYSUPPLEMENTALLUT_H
#define MDCMAPPLYSUPPLEMENTALLUT_H

#include "mdcmImageToImageFilter.h"

namespace mdcm
{

class DataElement;

class MDCM_EXPORT ApplySupplementalLUT : public ImageToImageFilter
{
public:
  ApplySupplementalLUT() = default;
  ~ApplySupplementalLUT() = default;
  bool
  Apply();
  int
  GetRedSubscript() const;

private:
  int m_RedSubscipt{};
};

} // end namespace mdcm

#endif // MDCMAPPLYSUPPLEMENTALLUT_H
