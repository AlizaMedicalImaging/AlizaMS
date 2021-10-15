%global debug_package %{nil}

Name:    alizams
Version: 1.7.1
Release: 1%{?dist}
Summary: Aliza MS DICOM Viewer
License: GPLv3
BuildRequires:  cmake
BuildRequires:  g++
BuildRequires:  libuuid-devel
BuildRequires:  zlib-devel
BuildRequires:  qt5-qtsvg-devel
BuildRequires:  openjpeg2-devel >= 2.0
BuildRequires:  CharLS-devel
BuildRequires:  vxl-devel
BuildRequires:  cmake(LIBMINC)
BuildRequires:  cmake(ITK)
BuildRequires:  cmake(gdcm)
BuildRequires:  desktop-file-utils
BuildRequires:  git
Requires:       hicolor-icon-theme
Requires:       qt5-qtsvg


%description
A DICOM viewer. Very fast directory scanner, DICOMDIR. 2D and 3D views with
many tools. View uniform and non-uniform series in physical space.
Consistently de-identify DICOM. View DICOM metadata. Ultrasound with proper
measurement in regions, cine. Scout (localizer) lines. Grayscale softcopy
presentation. Structured report. Compressed images. RTSTRUCT contours.
Siemens mosaic format. United Imaging Healthcare (UIH) Grid / VFrame format.
Elscint ELSCINT1 PMSCT_RLE1 and PMSCT_RGB1

%prep
git clone --recurse-submodules https://github.com/AlizaMedicalImaging/AlizaMS.git

%build
cd AlizaMS

%cmake -DCMAKE_BUILD_TYPE:STRING=Release \
	-DALIZA_QT_VERSION:STRING=5 \
	-DMDCM_USE_SYSTEM_ZLIB:BOOL=ON \
	-DMDCM_USE_SYSTEM_OPENJPEG:BOOL=ON \
  	-DMDCM_USE_SYSTEM_CHARLS:BOOL=ON \
  	-DMDCM_USE_SYSTEM_UUID:BOOL=ON \
        -DITK_DIR=%{_libdir}/cmake/InsightToolkit

%cmake_build

%install
cd AlizaMS
%cmake_install

%files

%{_bindir}/%{name}
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/icons/hicolor/*/apps/%{name}.svg
%{_datadir}/applications/%{name}.desktop
%{_mandir}/man1/%{name}.1*

%changelog

