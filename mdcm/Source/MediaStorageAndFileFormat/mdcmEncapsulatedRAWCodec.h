/*********************************************************
 *
 * MDCM
 *
 * github.com/issakomi
 *
 *********************************************************/

#ifndef MDCMENCAPSULATEDRAWCODEC_H
#define MDCMENCAPSULATEDRAWCODEC_H

#include "mdcmImageCodec.h"

namespace mdcm
{

class RAWInternals;

class MDCM_EXPORT EncapsulatedRAWCodec : public ImageCodec
{
public:
  EncapsulatedRAWCodec();
  ~EncapsulatedRAWCodec() override;
  bool
  CanCode(TransferSyntax const &) const override;
  bool
  CanDecode(TransferSyntax const &) const override;
  bool
  Code(const char *, unsigned long long, DataElement &);
  bool
  Decode2(DataElement const &, char *, unsigned long long);
  bool
  GetHeaderInfo(std::istream &, TransferSyntax &) override;
  bool
  DecodeBytes(const char *, size_t, char *, size_t);

protected:
  bool
  DecodeByStreams(std::istream &, std::ostream &) override;
};

} // end namespace mdcm

#endif // MDCMENCAPSULATEDRAWCODEC_H
