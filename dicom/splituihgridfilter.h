#ifndef A_SPLITUIHGRIDFILTER_H
#define A_SPLITUIHGRIDFILTER_H

#include <mdcmFile.h>
#include <mdcmImage.h>

namespace mdcm
{
class SplitUihGridFilter
{
public:
  SplitUihGridFilter() : F(new File), I(new Image) {}
  ~SplitUihGridFilter() {}
  bool Split();
  bool ComputeUihGridDimensions(unsigned int*);
  bool ComputeUihGridSliceNormal(double*);
  bool ComputeUihGridSlicePosition(double*);
  void SetImage(const Image &);
  const Image & GetImage() const { return *I; }
  Image & GetImage() { return *I; }
  void SetFile(const File & f) { F = f; }
  File & GetFile() { return *F; }
  const File & GetFile() const { return *F; }
private:
  SmartPointer<File>  F;
  SmartPointer<Image> I;
};

} // end namespace mdcm

#endif
