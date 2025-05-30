#include "structures.h"
#include "iconutils.h"
#include <itkExtractImageFilter.h>
#include <itkIntensityWindowingImageFilter.h>
#include <QPainter>
#include <QImage>
#include <QColor>
#include <QThread>
#include <vector>

class ProcessImageThread_ : public QThread
{
public:
	ProcessImageThread_(
		const Image2DTypeUC::Pointer & image_,
		unsigned char * p_,
		const int size_0_,
		const int size_1_,
		const int index_0_,
		const int index_1_,
		const unsigned int j_)
		:
		image(image_),
		p(p_),
		size_0(size_0_), size_1(size_1_),
		index_0(index_0_), index_1(index_1_),
		j(j_)
	{
	}

	~ProcessImageThread_()
	{
	}

	void run()
	{
		Image2DTypeUC::SizeType size;
		size[0] = size_0;
		size[1] = size_1;
		Image2DTypeUC::IndexType index;
		index[0] = index_0;
		index[1] = index_1;
		Image2DTypeUC::RegionType region;
		region.SetSize(size);
		region.SetIndex(index);
		itk::ImageRegionConstIterator<Image2DTypeUC> iterator(image, region);
		iterator.GoToBegin();
		unsigned int j_ = j;
		while (!iterator.IsAtEnd())
		{
			p[j_ + 2] = p[j_ + 1] = p[j_] = iterator.Get();
			j_ += 3;
			++iterator;
		}
	}

private:
	Image2DTypeUC::Pointer image;
	unsigned char * p;
	const int size_0;
	const int size_1;
	const int index_0;
	const int index_1;
	const unsigned int j;
};

namespace
{

template<typename Tin, typename Tout> void extract_icon(
	const typename Tin::Pointer & image, ImageVariant * ivariant,
	const int isize = 96)
{
	if (!ivariant) return;
	if (image.IsNull()) return;
	//
	typedef  itk::ExtractImageFilter<Tin, Tout> FilterType;
	typedef  itk::IntensityWindowingImageFilter<Tout, Image2DTypeUC>
		IntensityWindowingImageFilterType;
	typename Tout::Pointer tmp0;
	typename Image2DTypeUC::Pointer tmp1;
	unsigned int size_x, size_y;
	double spacing_x, spacing_y;
	typename FilterType::Pointer filter = FilterType::New();
	typename Tin::RegionType inRegion = image->GetLargestPossibleRegion();
	typename Tin::SizeType size = inRegion.GetSize();
	typename Tin::IndexType index = inRegion.GetIndex();
	index[2] = size[2] / 2;
	typename Tin::SizeType out_size;
	out_size[0] = size[0];
	out_size[1] = size[1];
	out_size[2] = 0;
	typename Tin::RegionType outRegion;
	outRegion.SetSize(out_size);
	outRegion.SetIndex(index);
	try
	{
		filter->SetInput(image);
		filter->SetExtractionRegion(outRegion);
		filter->SetDirectionCollapseToIdentity();
		filter->Update();
		tmp0 = filter->GetOutput();
	}
	catch (const itk::ExceptionObject &)
	{
		return;
	}
	if (tmp0.IsNull()) return;
	else tmp0->DisconnectPipeline();
	//
	typename IntensityWindowingImageFilterType::Pointer intensity_filter =
		IntensityWindowingImageFilterType::New();
	try
	{
		intensity_filter->SetInput(tmp0);
		intensity_filter->SetWindowLevel(
			ivariant->di->us_window_width,
			ivariant->di->us_window_center);
		intensity_filter->Update();
		tmp1 = intensity_filter->GetOutput();
	}
	catch (const itk::ExceptionObject &)
	{
		return;
	}
	if (tmp1.IsNull()) return;
	else tmp1->DisconnectPipeline();
	//
	const typename Image2DTypeUC::RegionType region_ = tmp1->GetLargestPossibleRegion();
	size_x = region_.GetSize()[0];
	size_y = region_.GetSize()[1];
	const typename Image2DTypeUC::SpacingType spacing_ = tmp1->GetSpacing();
	spacing_x = spacing_[0];
	spacing_y = spacing_[1];
	const unsigned int p_size = 3 * size_x * size_y;
	unsigned char * p;
	try
	{
		p = new unsigned char[p_size];
	}
	catch (const std::bad_alloc&)
	{
		return;
	}
	const int num_threads = QThread::idealThreadCount();
	const int tmp99 = size_y % num_threads;
	std::vector<ProcessImageThread_*> icon_threads;
	if (tmp99 == 0)
	{
		unsigned int j{};
		for (int i = 0; i < num_threads; ++i)
		{
			const int size_0 = size_x;
			const int size_1 = size_y / num_threads;
			const int index_0 = 0;
			const int index_1 = i * size_1;
			ProcessImageThread_ * t__ = new ProcessImageThread_(tmp1,
						p,
						size_0, size_1,
						index_0, index_1, j);
			j += 3 * size_0 * size_1;
			icon_threads.push_back(t__);
			t__->start();
		}
	}
	else
	{
		unsigned int j{};
		unsigned int block{64};
		if (static_cast<float>(size_y) / static_cast<float>(block) > 16.0f)
		{
			block = 128;
		}
		const int tmp100 = size_y % block;
		const int incr = static_cast<int>(floor(size_y / static_cast<double>(block)));
		if (size_y > block)
		{
			for (int i = 0; i < incr; ++i)
			{
				const int size_0  = size_x;
				const int index_0{};
				const int index_1 = i * block;
				ProcessImageThread_ * t__ = new ProcessImageThread_(tmp1,
							p,
							size_0, block,
							index_0, index_1, j);
				j += 3 * size_0 * block;
				icon_threads.push_back(t__);
				t__->start();
			}
			ProcessImageThread_ * lt__ = new ProcessImageThread_(tmp1,
						p,
						size_x, tmp100,
						0, incr * block, j);
			icon_threads.push_back(lt__);
			lt__->start();
		}
		else
		{
			ProcessImageThread_ * lt__ = new ProcessImageThread_(tmp1,
						p,
						size_x, size_y,
						0, 0, 0);
			icon_threads.push_back(lt__);
			lt__->start();
		}

	}
	//
	const size_t threads_size = icon_threads.size();
	for (size_t i = 0; i < threads_size; ++i)
	{
		icon_threads[i]->wait();
	}
	for (size_t i = 0; i < threads_size; ++i)
	{
		delete icon_threads[i];
		icon_threads[i] = nullptr;
	}
	icon_threads.clear();
	//
	QImage tmpi(p, size_x, size_y, 3 * size_x, QImage::Format_RGB888);
	bool flip_x{};
	bool flip_y{};
	if (!ivariant->orientation_string.isEmpty() && ivariant->orientation_string.size() >= 3)
	{
		if (ivariant->orientation_string.at(1) == QChar('I') ||
			ivariant->orientation_string.at(1) == QChar('P'))
		{
			flip_y = true;
		}
		if (ivariant->orientation_string.at(0) == QChar('L'))
		{
			flip_x = true;
		}
	}
#if QT_VERSION >= QT_VERSION_CHECK(6,9,0)
	if (flip_x && flip_y) tmpi.flip(Qt::Horizontal | Qt::Vertical);
	else if (flip_y)      tmpi.flip(Qt::Vertical);
	else if (flip_x)      tmpi.flip(Qt::Horizontal);
#elif QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	if (flip_x && flip_y) tmpi.mirror(true, true);
	else if (flip_y)      tmpi.mirror(false, true);
	else if (flip_x)      tmpi.mirror(true, false);
#else
	if (flip_x && flip_y) tmpi = tmpi.mirrored(true, true);
	else if (flip_y)      tmpi = tmpi.mirrored(false, true);
	else if (flip_x)      tmpi = tmpi.mirrored(true, false);
#endif
	//
	if (spacing_x == spacing_y)
	{
		ivariant->icon = QPixmap::fromImage(tmpi)
			.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}
	else
	{
		const unsigned int tmp_size_x =
			static_cast<unsigned int>(ceil(size_x * (spacing_x / spacing_y)));
		QPixmap tmp_pixmap = QPixmap::fromImage(tmpi)
			.scaled(tmp_size_x, size_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		ivariant->icon = tmp_pixmap
			.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}
	delete [] p;
	//
	if (!ivariant->icon.isNull() &&
		(ivariant->icon.width() < isize || ivariant->icon.height() < isize))
	{
		const float x__ = static_cast<float>(isize - ivariant->icon.width()) / 2.0f;
		const float y__ = static_cast<float>(isize - ivariant->icon.height()) / 2.0f;
		QPixmap empty_(isize, isize);
		empty_.fill(QColor(0, 0, 0));
		QPainter painter(&empty_);
		painter.drawPixmap(QPointF(x__,y__), ivariant->icon);
		painter.end();
		ivariant->icon = std::move(empty_);
	}
	//
	if (!ivariant->icon.isNull() &&
		ivariant->di->slices_generated &&
		ivariant->di->slices_from_dicom &&
		(ivariant->di->idimz == static_cast<int>(ivariant->di->image_slices.size())) &&
		(ivariant->di->idimz == ivariant->image_instance_uids.size()))
	{
		const unsigned int s__ = isize / 16;
		const float p__ = static_cast<float>(isize - s__);
		QPixmap quad_(s__, s__);
		if (ivariant->equi)
		{
			quad_.fill(QColor(10, 240, 10));
		}
		else
		{
			quad_.fill(QColor(240, 200, 10));
		}
		QPainter painter(&ivariant->icon);
		painter.drawPixmap(QPointF(p__, p__), quad_);
		painter.end();
	}
	IconUtils::update_icon(ivariant, isize);
}

template<typename Tin, typename Tout> void extract_icon_rgb(
	const typename Tin::Pointer & image, ImageVariant * ivariant,
	const int isize = 96)
{
	if (!ivariant) return;
	if (image.IsNull()) return;
	//
	typedef  itk::ExtractImageFilter<Tin, Tout> FilterType;
	typename Tout::Pointer tmp0;
	unsigned int size_x, size_y;
	double spacing_x, spacing_y;
	typename FilterType::Pointer filter = FilterType::New();
	typename Tin::RegionType inRegion = image->GetLargestPossibleRegion();
	typename Tin::SizeType size = inRegion.GetSize();
	typename Tin::IndexType index = inRegion.GetIndex();
	index[2] = size[2] / 2;
	typename Tin::SizeType out_size;
	out_size[0] = size[0];
	out_size[1] = size[1];
	out_size[2] = 0;
	typename Tin::RegionType outRegion;
	outRegion.SetSize(out_size);
	outRegion.SetIndex(index);
	try
	{
		filter->SetInput(image);
		filter->SetExtractionRegion(outRegion);
		filter->SetDirectionCollapseToIdentity();
		filter->Update();
		tmp0 = filter->GetOutput();
	}
	catch (const itk::ExceptionObject &)
	{
		return;
	}
	if (tmp0.IsNull()) return;
	else tmp0->DisconnectPipeline();
	//
	bool flip_x{};
	bool flip_y{};
	if (!ivariant->orientation_string.isEmpty() && ivariant->orientation_string.size() >= 3)
	{
		if (ivariant->orientation_string.at(1) == QChar('I') ||
			ivariant->orientation_string.at(1) == QChar('P'))
		{
			flip_y = true;
		}
		if (ivariant->orientation_string.at(0) == QChar('L'))
		{
			flip_x = true;
		}
	}
	//
	if (ivariant->image_type == 11)
	{
		const unsigned short bits_allocated = ivariant->di->bits_allocated;
		const unsigned short bits_stored    = ivariant->di->bits_stored;
		const double tmp_max =
			(bits_allocated > 0 && bits_stored > 0 && bits_stored < bits_allocated)
				? pow(2.0, static_cast<double>(bits_stored)) - 1.0
				: static_cast<double>(USHRT_MAX);
		const typename Tout::RegionType region = tmp0->GetLargestPossibleRegion();
		const typename Tout::SizeType size_ = region.GetSize();
		const typename Tout::SpacingType spacing_ = tmp0->GetSpacing();
		spacing_x = spacing_[0];
		spacing_y = spacing_[1];
		size_x = size_[0];
		size_y = size_[1];
		unsigned char * p;
		try
		{
			p = new unsigned char[size[0] * size[1] * 3];
		}
		catch (const std::bad_alloc&)
		{
			return;
		}
		try
		{
			unsigned long j_{};
			itk::ImageRegionConstIterator<Tout> iterator(tmp0, region);
			iterator.GoToBegin();
			while (!iterator.IsAtEnd())
			{
				p[j_ + 2] = static_cast<unsigned char>(
					(static_cast<double>(iterator.Get().GetBlue())  / tmp_max) * 255.0);
				p[j_ + 1] = static_cast<unsigned char>(
					(static_cast<double>(iterator.Get().GetGreen()) / tmp_max) * 255.0);
				p[j_] = static_cast<unsigned char>(
					(static_cast<double>(iterator.Get().GetRed())   / tmp_max) * 255.0);
				j_ += 3;
				++iterator;
			}
		}
		catch (const itk::ExceptionObject &)
		{
			;
		}
		QImage tmpi(p, size_[0], size_[1], 3 * size_[0], QImage::Format_RGB888);
#if QT_VERSION >= QT_VERSION_CHECK(6,9,0)
		if (flip_x && flip_y) tmpi.flip(Qt::Horizontal | Qt::Vertical);
		else if (flip_y)      tmpi.flip(Qt::Vertical);
		else if (flip_x)      tmpi.flip(Qt::Horizontal);
#elif QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		if (flip_x && flip_y) tmpi.mirror(true, true);
		else if (flip_y)      tmpi.mirror(false, true);
		else if (flip_x)      tmpi.mirror(true, false);
#else
		if (flip_x && flip_y) tmpi = tmpi.mirrored(true, true);
		else if (flip_y)      tmpi = tmpi.mirrored(false, true);
		else if (flip_x)      tmpi = tmpi.mirrored(true, false);
#endif
		if (spacing_x == spacing_y)
		{
			ivariant->icon = QPixmap::fromImage(tmpi)
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		else
		{
			const unsigned int tmp_size_x = static_cast<unsigned int>(
				ceil(size_x * (spacing_x / spacing_y)));
			QPixmap tmp_pixmap = QPixmap::fromImage(tmpi)
				.scaled(tmp_size_x, size_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			ivariant->icon = tmp_pixmap
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		delete [] p;
	}
	else if (ivariant->image_type == 14)
	{
		unsigned char * p_;
		try
		{
			p_ = reinterpret_cast<unsigned char *>(tmp0->GetBufferPointer());
		}
		catch (const itk::ExceptionObject &)
		{
			return;
		}
		if (!p_) return;
		const typename Tout::RegionType region = tmp0->GetLargestPossibleRegion();
		const typename Tout::SizeType size_ = region.GetSize();
		const typename Tout::SpacingType spacing_ = tmp0->GetSpacing();
		spacing_x = spacing_[0];
		spacing_y = spacing_[1];
		size_x = size_[0];
		size_y = size_[1];
		QImage tmpi(p_, size_[0], size_[1], 3 * size_[0], QImage::Format_RGB888);
#if QT_VERSION >= QT_VERSION_CHECK(6,9,0)
		if (flip_x && flip_y) tmpi.flip(Qt::Horizontal | Qt::Vertical);
		else if (flip_y)      tmpi.flip(Qt::Vertical);
		else if (flip_x)      tmpi.flip(Qt::Horizontal);
#elif QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		if (flip_x && flip_y) tmpi.mirror(true, true);
		else if (flip_y)      tmpi.mirror(false, true);
		else if (flip_x)      tmpi.mirror(true, false);
#else
		if (flip_x && flip_y) tmpi = tmpi.mirrored(true, true);
		else if (flip_y)      tmpi = tmpi.mirrored(false, true);
		else if (flip_x)      tmpi = tmpi.mirrored(true, false);
#endif
		if (spacing_x == spacing_y)
		{
			ivariant->icon = QPixmap::fromImage(tmpi)
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio,Qt::SmoothTransformation);
		}
		else
		{
			const unsigned int tmp_size_x = static_cast<unsigned int>(
				ceil(size_x * (spacing_x / spacing_y)));
			QPixmap tmp_pixmap = QPixmap::fromImage(tmpi)
				.scaled(tmp_size_x, size_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			ivariant->icon = tmp_pixmap
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
	}
	else
	{
		const double vmin = ivariant->di->vmin;
		const double vmax = ivariant->di->vmax;
		const double vrange = vmax - vmin;
		if (vrange <= 0.0) return;
		const typename Tout::RegionType region = tmp0->GetLargestPossibleRegion();
		const typename Tout::SizeType size_ = region.GetSize();
		const typename Tout::SpacingType spacing_ = tmp0->GetSpacing();
		spacing_x = spacing_[0];
		spacing_y = spacing_[1];
		size_x = size_[0];
		size_y = size_[1];
		unsigned char * p;
		try
		{
			p = new unsigned char[size_[0] * size_[1] * 3];
		}
		catch (const std::bad_alloc&)
		{
			return;
		}
		try
		{
			unsigned long j_{};
			itk::ImageRegionConstIterator<Tout> iterator(tmp0, region);
			iterator.GoToBegin();
			while(!iterator.IsAtEnd())
			{
				const double b = static_cast<double>(iterator.Get().GetBlue());
				const double g = static_cast<double>(iterator.Get().GetGreen());
				const double r = static_cast<double>(iterator.Get().GetRed());
				p[j_ + 2] = static_cast<unsigned char>(255.0 * ((b + (-vmin)) / vrange));
				p[j_ + 1] = static_cast<unsigned char>(255.0 * ((g + (-vmin)) / vrange));
				p[j_]     = static_cast<unsigned char>(255.0 * ((r + (-vmin)) / vrange));
				j_ += 3;
				++iterator;
			}
		}
		catch (const itk::ExceptionObject &)
		{
			;
		}
		QImage tmpi(p, size_[0], size_[1], 3 * size_[0], QImage::Format_RGB888);
#if QT_VERSION >= QT_VERSION_CHECK(6,9,0)
		if (flip_x && flip_y) tmpi.flip(Qt::Horizontal | Qt::Vertical);
		else if (flip_y)      tmpi.flip(Qt::Vertical);
		else if (flip_x)      tmpi.flip(Qt::Horizontal);
#elif QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		if (flip_x && flip_y) tmpi.mirror(true, true);
		else if (flip_y)      tmpi.mirror(false, true);
		else if (flip_x)      tmpi.mirror(true, false);
#else
		if (flip_x && flip_y) tmpi = tmpi.mirrored(true, true);
		else if (flip_y)      tmpi = tmpi.mirrored(false, true);
		else if (flip_x)      tmpi = tmpi.mirrored(true, false);
#endif
		if (spacing_x == spacing_y)
		{
			ivariant->icon = QPixmap::fromImage(tmpi)
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		else
		{
			const unsigned int tmp_size_x = static_cast<unsigned int>(
				ceil(size_x * (spacing_x / spacing_y)));
			QPixmap tmp_pixmap = QPixmap::fromImage(tmpi)
				.scaled(tmp_size_x, size_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			ivariant->icon = tmp_pixmap
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		delete [] p;
	}
	if (!ivariant->icon.isNull() &&
		(ivariant->icon.width() < isize || ivariant->icon.height() < isize))
	{
		const float x__ = static_cast<float>(isize - ivariant->icon.width() ) / 2.0f;
		const float y__ = static_cast<float>(isize - ivariant->icon.height()) / 2.0f;
		QPixmap empty_(isize, isize);
		empty_.fill(QColor(0, 0, 0));
		QPainter painter(&empty_);
		painter.drawPixmap(QPointF(x__, y__), ivariant->icon);
		painter.end();
		ivariant->icon = std::move(empty_);
	}
	if (!ivariant->icon.isNull() &&
		ivariant->equi &&
		ivariant->di->slices_generated &&
		ivariant->di->slices_from_dicom &&
		(ivariant->di->idimz == static_cast<int>(ivariant->di->image_slices.size())) &&
		(ivariant->di->idimz == ivariant->image_instance_uids.size()))
	{
		const unsigned int s__ = isize / 16;
		const float p__ = static_cast<float>(isize - s__);
		QPixmap quad_(s__, s__);
		quad_.fill(QColor(10, 240, 10));
		QPainter painter(&ivariant->icon);
		painter.drawPixmap(QPointF(p__, p__), quad_);
		painter.end();
	}
	IconUtils::update_icon(ivariant, isize);
}

template<typename Tin, typename Tout> void extract_icon_rgba(
	const typename Tin::Pointer & image, ImageVariant * ivariant,
	const int isize = 96)
{
	if (!ivariant) return;
	if (image.IsNull()) return;
	typedef  itk::ExtractImageFilter<Tin, Tout> FilterType;
	typename Tout::Pointer tmp0;
	unsigned int size_x, size_y;
	double spacing_x, spacing_y;
	typename FilterType::Pointer filter;
	typename Tin::RegionType inRegion;
	typename Tin::SizeType size;
	typename Tin::SizeType out_size;
	typename Tin::IndexType index;
	typename Tin::RegionType outRegion;
	try
	{
		filter = FilterType::New();
		inRegion = image->GetLargestPossibleRegion();
		size = inRegion.GetSize();
		index = inRegion.GetIndex();
		index[2] = size[2] / 2;
		out_size[0] = size[0];
		out_size[1] = size[1];
		out_size[2] = 0;
		outRegion.SetSize(out_size);
		outRegion.SetIndex(index);
		filter->SetInput(image);
		filter->SetExtractionRegion(outRegion);
		filter->SetDirectionCollapseToIdentity();
		filter->Update();
		tmp0 = filter->GetOutput();
		tmp0->DisconnectPipeline();
	}
	catch (const itk::ExceptionObject &)
	{
		return;
	}
	if (tmp0.IsNull()) return;
	//
	bool flip_x{};
	bool flip_y{};
	if (!ivariant->orientation_string.isEmpty() && ivariant->orientation_string.size() >= 3)
	{
		if (ivariant->orientation_string.at(1) == QChar('I') ||
			ivariant->orientation_string.at(1) == QChar('P'))
		{
			flip_y = true;
		}
		if (ivariant->orientation_string.at(0) == QChar('L'))
		{
			flip_x = true;
		}
	}
	//
	if (ivariant->image_type == 21)
	{
		const unsigned short bits_allocated = ivariant->di->bits_allocated;
		const unsigned short bits_stored    = ivariant->di->bits_stored;
		const double tmp_max =
			(bits_allocated > 0 && bits_stored > 0 && bits_stored < bits_allocated)
				? pow(2.0, static_cast<double>(bits_stored)) - 1.0
				: static_cast<double>(USHRT_MAX);
		const typename Tout::RegionType region = tmp0->GetLargestPossibleRegion();
		const typename Tout::SizeType size_ = region.GetSize();
		const typename Tout::SpacingType spacing_ = tmp0->GetSpacing();
		spacing_x = spacing_[0];
		spacing_y = spacing_[1];
		size_x = size_[0];
		size_y = size_[1];
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		unsigned char * p;
		try
		{
			p = new unsigned char[size[0] * size[1] * 4];
		}
		catch (const std::bad_alloc&)
		{
			return;
		}
		try
		{
			unsigned long j_{};
			itk::ImageRegionConstIterator<Tout> iterator(tmp0, region);
			iterator.GoToBegin();
			while (!iterator.IsAtEnd())
			{
				p[j_ + 3] = static_cast<unsigned char>(
					(static_cast<double>(iterator.Get().GetAlpha()) / tmp_max) * 255.0);
				p[j_ + 2] = static_cast<unsigned char>(
					(static_cast<double>(iterator.Get().GetBlue())  / tmp_max) * 255.0);
				p[j_ + 1] = static_cast<unsigned char>(
					(static_cast<double>(iterator.Get().GetGreen()) / tmp_max) * 255.0);
				p[j_] = static_cast<unsigned char>(
					(static_cast<double>(iterator.Get().GetRed())   / tmp_max) * 255.0);
				j_ += 4;
				++iterator;
			}
		}
		catch (const itk::ExceptionObject &)
		{
			;
		}
		QImage tmpi(p, size_[0], size_[1], 4 * size_[0], QImage::Format_RGBA8888);
#if QT_VERSION >= QT_VERSION_CHECK(6,9,0)
		if (flip_x && flip_y) tmpi.flip(Qt::Horizontal | Qt::Vertical);
		else if (flip_y)      tmpi.flip(Qt::Vertical);
		else if (flip_x)      tmpi.flip(Qt::Horizontal);
#elif QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		if (flip_x && flip_y) tmpi.mirror(true, true);
		else if (flip_y)      tmpi.mirror(false, true);
		else if (flip_x)      tmpi.mirror(true, false);
#else
		if (flip_x && flip_y) tmpi = tmpi.mirrored(true, true);
		else if (flip_y)      tmpi = tmpi.mirrored(false, true);
		else if (flip_x)      tmpi = tmpi.mirrored(true, false);
#endif
		if (spacing_x == spacing_y)
		{
			ivariant->icon = QPixmap::fromImage(tmpi)
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		else
		{
			const unsigned int tmp_size_x = static_cast<unsigned int>(
				ceil(size_x * (spacing_x / spacing_y)));
			QPixmap tmp_pixmap = QPixmap::fromImage(tmpi)
				.scaled(tmp_size_x, size_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			ivariant->icon = tmp_pixmap
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		delete [] p;
#else
		unsigned char * p;
		try
		{
			p = new unsigned char[size[0] * size[1] * 3];
		}
		catch (const std::bad_alloc&)
		{
			return;
		}
		try
		{
			unsigned long j_{};
			itk::ImageRegionConstIterator<Tout> iterator(tmp0, region);
			iterator.GoToBegin();
			while (!iterator.IsAtEnd())
			{
				if (iterator.Get().GetAlpha() > 0)
				{
					const double alpha = static_cast<double>(iterator.Get().GetAlpha()) / tmp_max;
					const double one_minus_alpha = 1.0 - alpha;
					const double tmp_whi = one_minus_alpha * USHRT_MAX;
					const double tmp_red = tmp_whi +
						alpha * static_cast<double>(iterator.Get().GetRed());
					const double tmp_gre = tmp_whi +
						alpha * static_cast<double>(iterator.Get().GetGreen());
					const double tmp_blu = tmp_whi +
						alpha * static_cast<double>(iterator.Get().GetBlue());
					p[j_ + 2] = static_cast<unsigned char>((tmp_blu / tmp_max) * 255.0);
					p[j_ + 1] = static_cast<unsigned char>((tmp_gre / tmp_max) * 255.0);
					p[j_]     = static_cast<unsigned char>((tmp_red / tmp_max) * 255.0);
				}
				else
				{
					p[j_ + 2] = 255;
					p[j_ + 1] = 255;
					p[j_]     = 255;
				}
				j_ += 3;
				++iterator;
			}
		}
		catch (const itk::ExceptionObject &)
		{
			;
		}
		QImage tmpi(p, size_[0], size_[1], 3 * size_[0], QImage::Format_RGB888);
		if (flip_x && flip_y) tmpi = tmpi.mirrored(true, true);
		else if (flip_y)      tmpi = tmpi.mirrored(false, true);
		else if (flip_x)      tmpi = tmpi.mirrored(true, false);
		if (spacing_x == spacing_y)
		{
			ivariant->icon = QPixmap::fromImage(tmpi)
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		else
		{
			const unsigned int tmp_size_x = static_cast<unsigned int>(
				ceil(size_x * (spacing_x / spacing_y)));
			QPixmap tmp_pixmap = QPixmap::fromImage(tmpi)
				.scaled(tmp_size_x, size_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			ivariant->icon = tmp_pixmap
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		delete [] p;
#endif
	}
	else if (ivariant->image_type == 24)
	{
		const typename Tout::RegionType region = tmp0->GetLargestPossibleRegion();
		const typename Tout::SizeType size_ = region.GetSize();
		const typename Tout::SpacingType spacing_ = tmp0->GetSpacing();
		spacing_x = spacing_[0];
		spacing_y = spacing_[1];
		size_x = size_[0];
		size_y = size_[1];
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		unsigned char * p_;
		try
		{
			p_ = reinterpret_cast<unsigned char *>(tmp0->GetBufferPointer());
		}
		catch (const itk::ExceptionObject &)
		{
			return;
		}
		if (!p_) return;
		QImage tmpi(p_, size_[0], size_[1], 4 * size_[0], QImage::Format_RGBA8888);
#if QT_VERSION >= QT_VERSION_CHECK(6,9,0)
		if (flip_x && flip_y) tmpi.flip(Qt::Horizontal | Qt::Vertical);
		else if (flip_y)      tmpi.flip(Qt::Vertical);
		else if (flip_x)      tmpi.flip(Qt::Horizontal);
#elif QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		if (flip_x && flip_y) tmpi.mirror(true, true);
		else if (flip_y)      tmpi.mirror(false, true);
		else if (flip_x)      tmpi.mirror(true, false);
#else
		if (flip_x && flip_y) tmpi = tmpi.mirrored(true, true);
		else if (flip_y)      tmpi = tmpi.mirrored(false, true);
		else if (flip_x)      tmpi = tmpi.mirrored(true, false);
#endif
		if (spacing_x == spacing_y)
		{
			ivariant->icon = QPixmap::fromImage(tmpi)
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		else
		{
			const unsigned int tmp_size_x = static_cast<unsigned int>(
				ceil(size_x * (spacing_x / spacing_y)));
			QPixmap tmp_pixmap = QPixmap::fromImage(tmpi)
				.scaled(tmp_size_x, size_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			ivariant->icon = tmp_pixmap
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
#else
		unsigned char * p;
		try
		{
			p = new unsigned char[size[0] * size[1] * 3];
		}
		catch (const std::bad_alloc&)
		{
			return;
		}
		try
		{
			unsigned long j_{};
			itk::ImageRegionConstIterator<Tout> iterator(tmp0, region);
			iterator.GoToBegin();
			while (!iterator.IsAtEnd())
			{
				if (iterator.Get().GetAlpha() > 0)
				{
					const double a = static_cast<double>(iterator.Get().GetAlpha()) / 255.0;
					const double one_minus_alpha = 1.0 - a;
					const double tmp_whi = one_minus_alpha * 255.0;
					const double tmp_red = tmp_whi +
						a * static_cast<double>(iterator.Get().GetRed());
					const double tmp_gre = tmp_whi +
						a * static_cast<double>(iterator.Get().GetGreen());
					const double tmp_blu = tmp_whi +
						a * static_cast<double>(iterator.Get().GetBlue());
					p[j_ + 2] = static_cast<unsigned char>(tmp_blu);
					p[j_ + 1] = static_cast<unsigned char>(tmp_gre);
					p[j_]     = static_cast<unsigned char>(tmp_red);
				}
				else
				{
					p[j_ + 2] = 255;
					p[j_ + 1] = 255;
					p[j_]     = 255;
				}
				j_ += 3;
				++iterator;
			}
		}
		catch (const itk::ExceptionObject &)
		{
			;
		}
		QImage tmpi(p, size[0], size[1], 3 * size[0], QImage::Format_RGB888);
		if (flip_x && flip_y) tmpi = tmpi.mirrored(true, true);
		else if (flip_y)      tmpi = tmpi.mirrored(false, true);
		else if (flip_x)      tmpi = tmpi.mirrored(true, false);
		if (spacing_x == spacing_y)
		{
			ivariant->icon = QPixmap::fromImage(tmpi)
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		else
		{
			const unsigned int tmp_size_x = static_cast<unsigned int>(
				ceil(size_x * (spacing_x / spacing_y)));
			QPixmap tmp_pixmap = QPixmap::fromImage(tmpi)
				.scaled(tmp_size_x, size_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			ivariant->icon = tmp_pixmap
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		delete [] p;
#endif
	}
	else
	{
		const typename Tout::RegionType region = tmp0->GetLargestPossibleRegion();
		const typename Tout::SizeType size_ = region.GetSize();
		const typename Tout::SpacingType spacing_ = tmp0->GetSpacing();
		spacing_x = spacing_[0];
		spacing_y = spacing_[1];
		size_x = size_[0];
		size_y = size_[1];
		const double vmin = ivariant->di->vmin;
		const double vmax = ivariant->di->vmax;
		const double vrange = vmax - vmin;
		if (vrange <= 0.0) return;
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
		unsigned char * p;
		try
		{
			p = new unsigned char[size[0] * size[1] * 4];
		}
		catch (const std::bad_alloc&)
		{
			return;
		}
		try
		{
			unsigned long j_{};
			itk::ImageRegionConstIterator<Tout> iterator(tmp0, region);
			iterator.GoToBegin();
			while(!iterator.IsAtEnd())
			{
				const double a = static_cast<double>(iterator.Get().GetAlpha());
				const double b = static_cast<double>(iterator.Get().GetBlue());
				const double g = static_cast<double>(iterator.Get().GetGreen());
				const double r = static_cast<double>(iterator.Get().GetRed());
				p[j_ + 3] = static_cast<unsigned char>(255.0 * ((a + (-vmin)) / vrange));
				p[j_ + 2] = static_cast<unsigned char>(255.0 * ((b + (-vmin)) / vrange));
				p[j_ + 1] = static_cast<unsigned char>(255.0 * ((g + (-vmin)) / vrange));
				p[j_]     = static_cast<unsigned char>(255.0 * ((r + (-vmin)) / vrange));
				j_ += 4;
				++iterator;
			}
		}
		catch (const itk::ExceptionObject &)
		{
			;
		}
		QImage tmpi(p, size[0], size[1], 4 * size[0], QImage::Format_RGBA8888);
#if QT_VERSION >= QT_VERSION_CHECK(6,9,0)
		if (flip_x && flip_y) tmpi.flip(Qt::Horizontal | Qt::Vertical);
		else if (flip_y)      tmpi.flip(Qt::Vertical);
		else if (flip_x)      tmpi.flip(Qt::Horizontal);
#elif QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		if (flip_x && flip_y) tmpi.mirror(true, true);
		else if (flip_y)      tmpi.mirror(false, true);
		else if (flip_x)      tmpi.mirror(true, false);
#else
		if (flip_x && flip_y) tmpi = tmpi.mirrored(true, true);
		else if (flip_y)      tmpi = tmpi.mirrored(false, true);
		else if (flip_x)      tmpi = tmpi.mirrored(true, false);
#endif
		if (spacing_x == spacing_y)
		{
			ivariant->icon = QPixmap::fromImage(tmpi)
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		else
		{
			const unsigned int tmp_size_x = static_cast<unsigned int>(
				ceil(size_x * (spacing_x / spacing_y)));
			QPixmap tmp_pixmap = QPixmap::fromImage(tmpi)
				.scaled(tmp_size_x, size_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			ivariant->icon = tmp_pixmap
				.scaled(QSize(isize, isize),Qt::KeepAspectRatio ,Qt::SmoothTransformation);
		}
		delete [] p;
#else
		unsigned char * p;
		try
		{
			p = new unsigned char[size[0] * size[1] * 3];
		}
		catch (const std::bad_alloc&)
		{
			return;
		}
		try
		{
			unsigned long j_{};
			itk::ImageRegionConstIterator<Tout> iterator(tmp0, region);
			iterator.GoToBegin();
			while(!iterator.IsAtEnd())
			{
				const double a = static_cast<double>(iterator.Get().GetAlpha());
				const double b = static_cast<double>(iterator.Get().GetBlue());
				const double g = static_cast<double>(iterator.Get().GetGreen());
				const double r = static_cast<double>(iterator.Get().GetRed());
				const double one_minus_alpha = 1.0 - ((a + (-vmin)) / vrange);
				const double tmp_whi = one_minus_alpha * 255.0;
				const double tmp_red = tmp_whi + a * (255.0 * ((r + (-vmin)) / vrange));
				const double tmp_gre = tmp_whi + a * (255.0 * ((g + (-vmin)) / vrange));
				const double tmp_blu = tmp_whi + a * (255.0 * ((b + (-vmin) )/ vrange));
				p[j_ + 2] = static_cast<unsigned char>(tmp_blu);
				p[j_ + 1] = static_cast<unsigned char>(tmp_gre);
				p[j_]     = static_cast<unsigned char>(tmp_red);
				j_ += 3;
				++iterator;
			}
		}
		catch (const itk::ExceptionObject &)
		{
			;
		}
		QImage tmpi(p, size[0], size[1], 3 * size[0], QImage::Format_RGB888);
		if (flip_x && flip_y) tmpi = tmpi.mirrored(true, true);
		else if (flip_y)      tmpi = tmpi.mirrored(false, true);
		else if (flip_x)      tmpi = tmpi.mirrored(true, false);
		if (spacing_x == spacing_y)
		{
			ivariant->icon = QPixmap::fromImage(tmpi)
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		else
		{
			const unsigned int tmp_size_x = static_cast<unsigned int>(
				ceil(size_x * (spacing_x / spacing_y)));
			QPixmap tmp_pixmap = QPixmap::fromImage(tmpi)
				.scaled(tmp_size_x, size_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			ivariant->icon = tmp_pixmap
				.scaled(QSize(isize, isize), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
		delete [] p;
#endif
	}
	if (!ivariant->icon.isNull() &&
		(ivariant->icon.width() < isize || ivariant->icon.height() < isize))
	{
		const float x__ = static_cast<float>(isize - ivariant->icon.width()) / 2.0f;
		const float y__ = static_cast<float>(isize - ivariant->icon.height()) / 2.0f;
		QPixmap empty_(isize, isize);
		empty_.fill(QColor(0, 0, 0));
		QPainter painter(&empty_);
		painter.drawPixmap(QPointF(x__, y__), ivariant->icon);
		painter.end();
		ivariant->icon = std::move(empty_);
	}
	if (!ivariant->icon.isNull() &&
		ivariant->equi &&
		ivariant->di->slices_generated &&
		ivariant->di->slices_from_dicom &&
		(ivariant->di->idimz == static_cast<int>(ivariant->di->image_slices.size())) &&
		(ivariant->di->idimz == ivariant->image_instance_uids.size()))
	{
		const unsigned int s__ = isize / 16;
		const float p__ = static_cast<float>(isize - s__);
		QPixmap quad_(s__, s__);
		quad_.fill(QColor(10, 240, 10));
		QPainter painter(&ivariant->icon);
		painter.drawPixmap(QPointF(p__, p__), quad_);
		painter.end();
	}
	IconUtils::update_icon(ivariant, isize);
}

}

void IconUtils::update_icon(ImageVariant * v, const int isize)
{
	if (v->icon.isNull()) return;
	const int R = static_cast<int>(v->di->R * 255.0f);
	const int G = static_cast<int>(v->di->G * 255.0f);
	const int B = static_cast<int>(v->di->B * 255.0f);
	const unsigned int s__ = isize / 16;
	const float p__ = static_cast<float>(isize - s__);
	QPixmap quad_(s__, s__);
	quad_.fill(QColor(R, G, B, 255));
	QPainter painter(&v->icon);
	painter.drawPixmap(QPointF(0, p__), quad_);
	painter.end();
}

void IconUtils::icon(ImageVariant * ivariant)
{
	if (!ivariant) return;
	switch(ivariant->image_type)
	{
	case 0:
		extract_icon<ImageTypeSS,Image2DTypeSS>(ivariant->pSS, ivariant);
		break;
	case 1:
		extract_icon<ImageTypeUS,Image2DTypeUS>(ivariant->pUS, ivariant);
		break;
	case 2:
		extract_icon<ImageTypeSI,Image2DTypeSI>(ivariant->pSI, ivariant);
		break;
	case 3:
		extract_icon<ImageTypeUI,Image2DTypeUI>(ivariant->pUI, ivariant);
		break;
	case 4:
		extract_icon<ImageTypeUC,Image2DTypeUC>(ivariant->pUC, ivariant);
		break;
	case 5:
		extract_icon<ImageTypeF,Image2DTypeF>(ivariant->pF, ivariant);
		break;
	case 6:
		extract_icon<ImageTypeD,Image2DTypeD>(ivariant->pD, ivariant);
		break;
	case 7:
		extract_icon<ImageTypeSLL,Image2DTypeSLL>(ivariant->pSLL, ivariant);
		break;
	case 8:
		extract_icon<ImageTypeULL,Image2DTypeULL>(ivariant->pULL, ivariant);
		break;
	case 10:
		extract_icon_rgb<RGBImageTypeSS,RGBImage2DTypeSS>(ivariant->pSS_rgb, ivariant);
		break;
	case 11:
		extract_icon_rgb<RGBImageTypeUS,RGBImage2DTypeUS>(ivariant->pUS_rgb, ivariant);
		break;
	case 12:
		extract_icon_rgb<RGBImageTypeSI,RGBImage2DTypeSI>(ivariant->pSI_rgb, ivariant);
		break;
	case 13:
		extract_icon_rgb<RGBImageTypeUI,RGBImage2DTypeUI>(ivariant->pUI_rgb, ivariant);
		break;
	case 14:
		extract_icon_rgb<RGBImageTypeUC,RGBImage2DTypeUC>(ivariant->pUC_rgb, ivariant);
		break;
	case 15:
		extract_icon_rgb<RGBImageTypeF,RGBImage2DTypeF>(ivariant->pF_rgb, ivariant);
		break;
	case 16:
		extract_icon_rgb<RGBImageTypeD,RGBImage2DTypeD>(ivariant->pD_rgb, ivariant);
		break;
	case 20:
		extract_icon_rgba<RGBAImageTypeSS,RGBAImage2DTypeSS>(ivariant->pSS_rgba, ivariant);
		break;
	case 21:
		extract_icon_rgba<RGBAImageTypeUS,RGBAImage2DTypeUS>(ivariant->pUS_rgba, ivariant);
		break;
	case 22:
		extract_icon_rgba<RGBAImageTypeSI,RGBAImage2DTypeSI>(ivariant->pSI_rgba, ivariant);
		break;
	case 23:
		extract_icon_rgba<RGBAImageTypeUI,RGBAImage2DTypeUI>(ivariant->pUI_rgba, ivariant);
		break;
	case 24:
		extract_icon_rgba<RGBAImageTypeUC,RGBAImage2DTypeUC>(ivariant->pUC_rgba, ivariant);
		break;
	case 25:
		extract_icon_rgba<RGBAImageTypeF,RGBAImage2DTypeF>(ivariant->pF_rgba, ivariant);
		break;
	case 26:
		extract_icon_rgba<RGBAImageTypeD,RGBAImage2DTypeD>(ivariant->pD_rgba, ivariant);
		break;
	default :
		break;
	}
}

