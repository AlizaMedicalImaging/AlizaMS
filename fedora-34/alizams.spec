Name:    alizams
Version: 1.9.10
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
BuildRequires:  bullet-devel
BuildRequires:  lcms2-devel
BuildRequires:  desktop-file-utils
BuildRequires:  git
Requires:       hicolor-icon-theme
Requires:       qt5-qtsvg

%description
A 2D and 3D DICOM viewer with many tools and very fast directory
scanner and DICOMDIR support.
It can consistently de-identify DICOM files.

%prep
git clone https://github.com/AlizaMedicalImaging/AlizaMS.git
cd AlizaMS
rm -rf debian-10/
rm -rf debian-12-qt5/
rm -rf debian-12-qt6/
rm -rf fedora-34/
rm -rf package/apple/
rm -rf package/art/
rm -fr mdcm/Utilities/mdcmzlib/
rm -fr mdcm/Utilities/mdcmopenjpeg/
rm -fr mdcm/Utilities/mdcmcharls/
rm -fr mdcm/Utilities/mdcmuuid/
rm -fr alizalcms/
rm -fr b/
rm -fr CG/glew/

%build
cd AlizaMS
%cmake -DCMAKE_BUILD_TYPE:STRING=Release \
  -DCMAKE_SKIP_RPATH:BOOL=ON \
  -DALIZA_QT_VERSION:STRING=5 \
  -DALIZA_USE_SYSTEM_BULLET:BOOL=ON \
  -DALIZA_USE_SYSTEM_LCMS2:BOOL=ON \
  -DMDCM_USE_SYSTEM_ZLIB:BOOL=ON \
  -DMDCM_USE_SYSTEM_OPENJPEG:BOOL=ON \
  -DMDCM_USE_SYSTEM_CHARLS:BOOL=ON \
  -DMDCM_USE_SYSTEM_UUID:BOOL=ON \
  -DITK_DIR:PATH=%{_libdir}/cmake/InsightToolkit
%cmake_build

%install
cd AlizaMS
%cmake_install

%files

%{_bindir}/%{name}
%{_datadir}/icons/hicolor/*/apps/%{name}.png
%{_datadir}/icons/hicolor/*/apps/%{name}.svg
%{_datadir}/metainfo/%{name}.metainfo.xml
%{_datadir}/applications/%{name}.desktop
%{_datadir}/%{name}
%{_mandir}/man1/%{name}.1*

%changelog

