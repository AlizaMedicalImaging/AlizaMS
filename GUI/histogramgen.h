#ifndef A_HISTOGRAMGEN_H
#define A_HISTOGRAMGEN_H
#include <QThread>

class ImageVariant;

class HistogramGen : public QThread
{
public:
	HistogramGen(ImageVariant * i) : ivariant(i) {}
	~HistogramGen() = default;
	void run() override;
	QString get_error() const;
	void gen_pixmap();
private:
	QString gen_pixmap_scalar();
	QString gen_pixmap_rgb();
	ImageVariant * ivariant;
	std::vector<int> bins;
	std::vector<int> bins0;
	std::vector<int> bins1;
	std::vector<int> bins2;
	unsigned int bins_size{};
	double factor{0.30102};
	QString error;
};

#endif
