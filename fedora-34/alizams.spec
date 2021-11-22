%global debug_package %{nil}

Name:    alizams
Version: 1.7.2
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
BuildRequires:  bullet-devel >= 2.97
BuildRequires:  desktop-file-utils
BuildRequires:  git
Requires:       hicolor-icon-theme
Requires:       qt5-qtsvg


%description
DICOM viewer

%prep
git clone https://github.com/AlizaMedicalImaging/AlizaMS.git

%build
cd AlizaMS
%cmake -DCMAKE_BUILD_TYPE:STRING=Release \
  -DCMAKE_SKIP_RPATH:BOOL=ON \
  -DALIZA_QT_VERSION:STRING=5 \
  -DALIZA_USE_SYSTEM_BULLET:BOOL=ON \
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
%{_datadir}/applications/%{name}.desktop
%{_datadir}/%{name}
%{_mandir}/man1/%{name}.1*

%changelog

