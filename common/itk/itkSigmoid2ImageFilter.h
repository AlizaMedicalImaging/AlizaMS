#ifndef itkSigmoid2ImageFilter_h
#define itkSigmoid2ImageFilter_h

#include <itkUnaryFunctorImageFilter.h>
#include <itkMath.h>

namespace itk
{
namespace Functor
{
template< typename TInput, typename TOutput >
class Sigmoid2
{
public:
  Sigmoid2()
  {
    m_Alpha         = 1.0;
    m_Beta          = 0.0;
    m_OutputMinimum = NumericTraits< TOutput >::min();
    m_OutputMaximum = NumericTraits< TOutput >::max();
  }

  ~Sigmoid2() {}
  bool operator!=(const Sigmoid2 & other) const
  {
    if (Math::NotExactlyEquals(m_Alpha, other.m_Alpha)
         || Math::NotExactlyEquals(m_Beta, other.m_Beta)
         || Math::NotExactlyEquals(m_OutputMaximum, other.m_OutputMaximum)
         || Math::NotExactlyEquals(m_OutputMinimum, other.m_OutputMinimum))
    {
      return true;
    }
    return false;
  }

  bool operator==(const Sigmoid2 & other) const
  {
    return !(*this != other);
  }

  inline TOutput operator()(const TInput & A) const
  {
    const double x = (static_cast<double>(A) - m_Beta) / m_Alpha;
    const double e = 1.0 / (1.0 + exp(-4.0 * x));
    const double v = (m_OutputMaximum - m_OutputMinimum) * e + m_OutputMinimum;
    return static_cast< TOutput >(v);
  }

  void SetAlpha(double alpha)
  {
    m_Alpha = alpha;
  }

  void SetBeta(double beta)
  {
    m_Beta = beta;
  }

  double GetAlpha() const
  {
    return m_Alpha;
  }

  double GetBeta() const
  {
    return m_Beta;
  }

  void SetOutputMinimum(TOutput min)
  {
    m_OutputMinimum = min;
  }

  void SetOutputMaximum(TOutput max)
  {
    m_OutputMaximum = max;
  }

  TOutput GetOutputMinimum() const
  {
    return m_OutputMinimum;
  }

  TOutput GetOutputMaximum() const
  {
    return m_OutputMaximum;
  }

private:
  double  m_Alpha;
  double  m_Beta;
  TOutput m_OutputMinimum;
  TOutput m_OutputMaximum;
};
}

template< typename TInputImage, typename TOutputImage >
class Sigmoid2ImageFilter:
  public
  UnaryFunctorImageFilter< TInputImage, TOutputImage, Functor::Sigmoid2< typename TInputImage::PixelType, typename TOutputImage::PixelType > >
{
public:
  /** Standard class typedefs. */
  typedef Sigmoid2ImageFilter Self;
  typedef UnaryFunctorImageFilter<
    TInputImage, TOutputImage,
    Functor::Sigmoid2< typename TInputImage::PixelType, typename TOutputImage::PixelType > > Superclass;

  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  typedef typename TOutputImage::PixelType OutputPixelType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Macro that provides the GetNameOfClass() method */
  itkTypeMacro(Sigmoid2ImageFilter, UnaryFunctorImageFilter);

  void SetAlpha(double alpha)
  {
    if (Math::ExactlyEquals(alpha, this->GetFunctor().GetAlpha()))
    {
      return;
    }
    this->GetFunctor().SetAlpha(alpha);
    this->Modified();
  }

  double GetAlpha() const
  {
    return this->GetFunctor().GetAlpha();
  }

  void SetBeta(double beta)
  {
    if (Math::ExactlyEquals(beta, this->GetFunctor().GetBeta()))
    {
      return;
    }
    this->GetFunctor().SetBeta(beta);
    this->Modified();
  }

  double GetBeta() const
  {
    return this->GetFunctor().GetBeta();
  }

  void SetOutputMinimum(OutputPixelType min)
  {
    if (Math::ExactlyEquals(min, this->GetFunctor().GetOutputMinimum()))
    {
      return;
    }
    this->GetFunctor().SetOutputMinimum(min);
    this->Modified();
  }

  OutputPixelType GetOutputMinimum() const
  {
    return this->GetFunctor().GetOutputMinimum();
  }

  void SetOutputMaximum(OutputPixelType max)
  {
    if (Math::ExactlyEquals(max, this->GetFunctor().GetOutputMaximum()))
    {
      return;
    }
    this->GetFunctor().SetOutputMaximum(max);
    this->Modified();
  }

  OutputPixelType GetOutputMaximum() const
  {
    return this->GetFunctor().GetOutputMaximum();
  }

#ifdef ITK_USE_CONCEPT_CHECKING
  // Begin concept checking
  itkConceptMacro(InputConvertibleToDoubleCheck, (Concept::Convertible< typename TInputImage::PixelType, double >));
  itkConceptMacro(OutputAdditiveOperatorsCheck, (Concept::AdditiveOperators< OutputPixelType >));
  itkConceptMacro(DoubleConvertibleToOutputCheck, (Concept::Convertible< double, OutputPixelType >));
  itkConceptMacro(OutputTimesDoubleCheck, (Concept::MultiplyOperator< OutputPixelType, double >));
  itkConceptMacro(OutputDoubleAdditiveOperatorsCheck, (Concept::AdditiveOperators< OutputPixelType, OutputPixelType, double >));
  // End concept checking
#endif

protected:
  Sigmoid2ImageFilter() {}
  virtual ~Sigmoid2ImageFilter() {}

private:
  ITK_DISALLOW_COPY_AND_ASSIGN(Sigmoid2ImageFilter);
};
} // end namespace itk

#endif

