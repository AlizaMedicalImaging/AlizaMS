#include "structures.h"
#include "loaddicom.h"
#include "dicomutils.h"
#include <iostream>

void LoadDicom::run()
{
	try
	{
		message = DicomUtils::read_dicom(
			ivariants,
			pdf_files,
			stl_files,
			video_files,
			spectroscopy_files,
			sr_images,
			root,
			filenames,
			ok3d,
			settingswidget,
			load_type,
			enh_type);
	}
	catch (const mdcm::ParseException & pe)
	{
		std::cout << "mdcm::ParseException in Aliza::load_dicom_series:\n"
			<< pe.GetLastElement().GetTag() << std::endl;
	}
	catch (const std::exception & ex)
	{
		std::cout << "Exception in Aliza::load_dicom_series\n"
			<< ex.what() << std::endl;
	}
}

