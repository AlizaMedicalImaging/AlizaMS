![Aliza MS](package/archive/usr/share/icons/hicolor/128x128/apps/alizams.png)

Aliza MS - DICOM Viewer
=======================

[Releases](https://github.com/AlizaMedicalImaging/AlizaMS/releases)

View short introductory videos on [Youtube Channel](https://www.youtube.com/channel/UCPGvoSYX7PC5XCp-81Q4MAg) _(new)_

Quick start
-----------

Linux
-----

Packages:

E.g. __Fedora 34__ and higher: install with _sudo dnf install alizams_

Open from Menu (Graphics section) or run in terminal (type _alizams_)

Archive distribution:

Extract archive and run _alizams.sh_

```
cd alizams-1.8.2_linux
./alizams.sh
```

Optionally install local desktop menu entry

```
cd alizams-1.8.2_linux/install_menu
./install_menu.sh
```

To remove local menu entry

```
cd alizams-1.8.2_linux/install_menu
./uninstall_menu.sh
```

macOS
-----

Open DMG file (signed and notarized), in the window move icon onto Applications, eject,

s. _Launchpad_ or use from terminal

```
/Applications/AlizaMS.app/Contents/MacOS/AlizaMS
```

Windows
-------

Extract archive, click or run from terminal _alizams.exe_.

View
----

Select _DICOM scanner_ tab, open directory with DICOM files or DICOMDIR file (or drag-and-drop).

Select one or more series and click _arrow_ action (or double-click selected row) to load.


![Open](package/art/start0.png)


Highlights
----------

 * Very fast directory scanner, DICOMDIR
 * 2D and 3D views with many tools
 * View uniform and non-uniform series in physical space
 * DICOM Study multi-view with intersection lines
 * Structured report
 * RTSTRUCT contours
 * 2D+t and 3D+t animations
 * Consistently de-identify DICOM
 * View DICOM metadata
 * Ultrasound incl. proper measurement in regions, cine
 * Grayscale softcopy presentation
 * Siemens mosaic format
 * United Imaging Healthcare (UIH) Grid / VFrame format
 * etc.


Build
-----

 * S. ![wiki](https://github.com/AlizaMedicalImaging/AlizaMS/wiki) for example how to build on Linux.

