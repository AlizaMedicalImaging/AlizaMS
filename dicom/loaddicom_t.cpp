#include "structures.h"
#include "loaddicom_t.h"
#include "dicomutils.h"
#include <iostream>

void LoadDicom_T::run()
{
	try
	{
		message = DicomUtils::read_dicom(
			ivariants,
			pdf_files,
			stl_files,
			video_files,
			spectroscopy_files,
			sr_files,
			root,
			filenames,
			ok3d,
			settingswidget,
			load_type,
			enh_type);
	}
	catch (const mdcm::ParseException & pe)
	{
#ifdef ALIZA_VERBOSE
		std::cout << "mdcm::ParseException in LoadDicom_T::run()\n"
			<< pe.GetLastElement().GetTag() << std::endl;
#else
		(void)pe;
#endif
	}
	catch (const std::exception & ex)
	{
#ifdef ALIZA_VERBOSE
		std::cout << "Exception in LoadDicom_T::run()\n"
			<< ex.what() << std::endl;
#else
		(void)ex;
#endif
	}
}

