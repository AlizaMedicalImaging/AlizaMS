#!/bin/bash
lipo -create arm/AlizaMS.app/Contents/MacOS/AlizaMS x86/AlizaMS.app/Contents/MacOS/AlizaMS -output universal/AlizaMS.app/Contents/MacOS/AlizaMS

lipo -create arm/AlizaMS.app/Contents/PlugIns/platforms/libqcocoa.dylib x86/AlizaMS.app/Contents/PlugIns/platforms/libqcocoa.dylib -output universal/AlizaMS.app/Contents/PlugIns/platforms/libqcocoa.dylib

lipo -create arm/AlizaMS.app/Contents/PlugIns/styles/libqmacstyle.dylib x86/AlizaMS.app/Contents/PlugIns/styles/libqmacstyle.dylib -output universal/AlizaMS.app/Contents/PlugIns/styles/libqmacstyle.dylib

lipo -create arm/AlizaMS.app/Contents/PlugIns/sqldrivers/libqsqlite.dylib x86/AlizaMS.app/Contents/PlugIns/sqldrivers/libqsqlite.dylib -output universal/AlizaMS.app/Contents/PlugIns/sqldrivers/libqsqlite.dylib

lipo -create arm/AlizaMS.app/Contents/PlugIns/imageformats/libqjpeg.dylib x86/AlizaMS.app/Contents/PlugIns/imageformats/libqjpeg.dylib -output universal/AlizaMS.app/Contents/PlugIns/imageformats/libqjpeg.dylib

lipo -create arm/AlizaMS.app/Contents/PlugIns/imageformats/libqico.dylib x86/AlizaMS.app/Contents/PlugIns/imageformats/libqico.dylib -output universal/AlizaMS.app/Contents/PlugIns/imageformats/libqico.dylib

lipo -create arm/AlizaMS.app/Contents/PlugIns/imageformats/libqgif.dylib x86/AlizaMS.app/Contents/PlugIns/imageformats/libqgif.dylib -output universal/AlizaMS.app/Contents/PlugIns/imageformats/libqgif.dylib

lipo -create arm/AlizaMS.app/Contents/PlugIns/iconengines/libqsvgicon.dylib x86/AlizaMS.app/Contents/PlugIns/iconengines/libqsvgicon.dylib -output universal/AlizaMS.app/Contents/PlugIns/iconengines/libqsvgicon.dylib

lipo -create arm/AlizaMS.app/Contents/Frameworks/QtPrintSupport.framework/Versions/A/QtPrintSupport x86/AlizaMS.app/Contents/Frameworks/QtPrintSupport.framework/Versions/A/QtPrintSupport -output universal/AlizaMS.app/Contents/Frameworks/QtPrintSupport.framework/Versions/A/QtPrintSupport

lipo -create arm/AlizaMS.app/Contents/Frameworks/QtGui.framework/Versions/A/QtGui x86/AlizaMS.app/Contents/Frameworks/QtGui.framework/Versions/A/QtGui -output universal/AlizaMS.app/Contents/Frameworks/QtGui.framework/Versions/A/QtGui

lipo -create arm/AlizaMS.app/Contents/Frameworks/QtDBus.framework/Versions/A/QtDBus x86/AlizaMS.app/Contents/Frameworks/QtDBus.framework/Versions/A/QtDBus -output universal/AlizaMS.app/Contents/Frameworks/QtDBus.framework/Versions/A/QtDBus

lipo -create arm/AlizaMS.app/Contents/Frameworks/QtCore.framework/Versions/A/QtCore x86/AlizaMS.app/Contents/Frameworks/QtCore.framework/Versions/A/QtCore -output universal/AlizaMS.app/Contents/Frameworks/QtCore.framework/Versions/A/QtCore

lipo -create arm/AlizaMS.app/Contents/Frameworks/QtCore5Compat.framework/Versions/A/QtCore5Compat x86/AlizaMS.app/Contents/Frameworks/QtCore5Compat.framework/Versions/A/QtCore5Compat -output universal/AlizaMS.app/Contents/Frameworks/QtCore5Compat.framework/Versions/A/QtCore5Compat

lipo -create arm/AlizaMS.app/Contents/Frameworks/QtOpenGL.framework/Versions/A/QtOpenGL x86/AlizaMS.app/Contents/Frameworks/QtOpenGL.framework/Versions/A/QtOpenGL -output universal/AlizaMS.app/Contents/Frameworks/QtOpenGL.framework/Versions/A/QtOpenGL

lipo -create arm/AlizaMS.app/Contents/Frameworks/QtOpenGLWidgets.framework/Versions/A/QtOpenGLWidgets x86/AlizaMS.app/Contents/Frameworks/QtOpenGLWidgets.framework/Versions/A/QtOpenGLWidgets -output universal/AlizaMS.app/Contents/Frameworks/QtOpenGLWidgets.framework/Versions/A/QtOpenGLWidgets

lipo -create arm/AlizaMS.app/Contents/Frameworks/QtWidgets.framework/Versions/A/QtWidgets x86/AlizaMS.app/Contents/Frameworks/QtWidgets.framework/Versions/A/QtWidgets -output universal/AlizaMS.app/Contents/Frameworks/QtWidgets.framework/Versions/A/QtWidgets

lipo -create arm/AlizaMS.app/Contents/Frameworks/QtSvg.framework/Versions/A/QtSvg x86/AlizaMS.app/Contents/Frameworks/QtSvg.framework/Versions/A/QtSvg -output universal/AlizaMS.app/Contents/Frameworks/QtSvg.framework/Versions/A/QtSvg

lipo -create arm/AlizaMS.app/Contents/Frameworks/QtSql.framework/Versions/A/QtSql x86/AlizaMS.app/Contents/Frameworks/QtSql.framework/Versions/A/QtSql -output universal/AlizaMS.app/Contents/Frameworks/QtSql.framework/Versions/A/QtSql

lipo -create arm/AlizaMS.app/Contents/Frameworks/QtSql.framework/Versions/A/QtSql x86/AlizaMS.app/Contents/Frameworks/QtSql.framework/Versions/A/QtSql -output universal/AlizaMS.app/Contents/Frameworks/QtSql.framework/Versions/A/QtSql


