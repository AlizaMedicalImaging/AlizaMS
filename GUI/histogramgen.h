#ifndef HistogramGen_H
#define HistogramGen_H

class ImageVariant;

class HistogramGen
{
public:
	HistogramGen(ImageVariant * i) : ivariant(i) {}
	~HistogramGen() {}
	void run();
	QString get_error() const;
private:
	ImageVariant * ivariant;
	QString error;
};

#endif // HistogramGen_H

