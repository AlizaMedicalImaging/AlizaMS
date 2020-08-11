#ifndef MDCMAPPLYSUPPLEMENTALLUT_H
#define MDCMAPPLYSUPPLEMENTALLUT_H

#include "mdcmImageToImageFilter.h"

namespace mdcm
{

class DataElement;
/**
 * \brief ImageSupplementalLUT class
 * \details It applies SupplementalLUT to the PixelData
 * Output will be a PhotometricInterpretation=RGB image
 */
class MDCM_EXPORT ApplySupplementalLUT : public ImageToImageFilter
{
public:
  ApplySupplementalLUT() : m_RedSubscipt(0) {}
  ~ApplySupplementalLUT() {}
  bool Apply();
  int  GetRedSubscript() const { return m_RedSubscipt; };
private:
  int m_RedSubscipt;
};

} // end namespace mdcm

#endif //MDCMAPPLYSUPPLEMENTALLUT_H
