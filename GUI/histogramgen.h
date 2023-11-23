#ifndef A_HISTOGRAMGEN_H
#define A_HISTOGRAMGEN_H

class ImageVariant;

class HistogramGen
{
public:
	HistogramGen(ImageVariant * i) : ivariant(i) {}
	~HistogramGen() = default;
	void run();
	QString get_error() const;

private:
	ImageVariant * ivariant;
	QString error;
};

#endif

