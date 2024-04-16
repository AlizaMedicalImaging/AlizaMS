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
  EncapsulatedRAWCodec() = default;
  ~EncapsulatedRAWCodec() = default;
  bool
  CanCode(const TransferSyntax &) const override;
  bool
  CanDecode(const TransferSyntax &) const override;
  bool
  Code(const char *, unsigned long long, DataElement &);
  bool
  Decode2(const DataElement &, char *, unsigned long long);
  bool
  GetHeaderInfo(std::istream &) override;
  bool
  DecodeBytes(const char *, size_t, char *, size_t);

protected:
  bool
  DecodeByStreams(std::istream &, std::ostream &) override;
};

} // end namespace mdcm

#endif // MDCMENCAPSULATEDRAWCODEC_H
