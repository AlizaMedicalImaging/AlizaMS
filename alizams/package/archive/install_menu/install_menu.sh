#!/bin/bash
echo "Installing Aliza MS local user menu (s. section Graphics)"
ALIZAMS_C_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
ALIZAMS_I_DIR="${HOME}/.local/share/icons/hicolor"
if [ ! -d "${HOME}/.local/share/applications" ];
  then mkdir -p "${HOME}/.local/share/applications";
fi
if [ ! -d "${ALIZAMS_I_DIR}/16x16/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/16x16/apps";
fi
if [ ! -d "${ALIZAMS_I_DIR}/22x22/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/22x22/apps";
fi
if [ ! -d "${ALIZAMS_I_DIR}/24x24/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/24x24/apps";
fi
if [ ! -d "${ALIZAMS_I_DIR}/32x32/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/32x32/apps";
fi
if [ ! -d "${ALIZAMS_I_DIR}/36x36/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/36x36/apps";
fi
if [ ! -d "${ALIZAMS_I_DIR}/42x42/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/42x42/apps";
fi
if [ ! -d "${ALIZAMS_I_DIR}/48x48/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/48x48/apps";
fi
if [ ! -d "${ALIZAMS_I_DIR}/64x64/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/64x64/apps";
fi
if [ ! -d "${ALIZAMS_I_DIR}/72x72/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/72x72/apps";
fi
if [ ! -d "${ALIZAMS_I_DIR}/96x96/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/96x96/apps";
fi
if [ ! -d "${ALIZAMS_I_DIR}/128x128/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/128x128/apps";
fi
if [ ! -d "${ALIZAMS_I_DIR}/192x192/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/192x192/apps";
fi
if [ ! -d "${ALIZAMS_I_DIR}/256x256/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/256x256/apps";
fi
if [ ! -d "${ALIZAMS_I_DIR}/scalable/apps" ];
  then mkdir -p "${ALIZAMS_I_DIR}/scalable/apps";
fi
cp "${ALIZAMS_C_DIR}/icons/hicolor/16x16/apps/alizams.png"    "${ALIZAMS_I_DIR}/16x16/apps"
cp "${ALIZAMS_C_DIR}/icons/hicolor/22x22/apps/alizams.png"    "${ALIZAMS_I_DIR}/22x22/apps"
cp "${ALIZAMS_C_DIR}/icons/hicolor/24x24/apps/alizams.png"    "${ALIZAMS_I_DIR}/24x24/apps"
cp "${ALIZAMS_C_DIR}/icons/hicolor/32x32/apps/alizams.png"    "${ALIZAMS_I_DIR}/32x32/apps"
cp "${ALIZAMS_C_DIR}/icons/hicolor/36x36/apps/alizams.png"    "${ALIZAMS_I_DIR}/36x36/apps"
cp "${ALIZAMS_C_DIR}/icons/hicolor/42x42/apps/alizams.png"    "${ALIZAMS_I_DIR}/42x42/apps"
cp "${ALIZAMS_C_DIR}/icons/hicolor/48x48/apps/alizams.png"    "${ALIZAMS_I_DIR}/48x48/apps"
cp "${ALIZAMS_C_DIR}/icons/hicolor/64x64/apps/alizams.png"    "${ALIZAMS_I_DIR}/64x64/apps"
cp "${ALIZAMS_C_DIR}/icons/hicolor/72x72/apps/alizams.png"    "${ALIZAMS_I_DIR}/72x72/apps"
cp "${ALIZAMS_C_DIR}/icons/hicolor/96x96/apps/alizams.png"    "${ALIZAMS_I_DIR}/96x96/apps"
cp "${ALIZAMS_C_DIR}/icons/hicolor/128x128/apps/alizams.png"  "${ALIZAMS_I_DIR}/128x128/apps"
cp "${ALIZAMS_C_DIR}/icons/hicolor/192x192/apps/alizams.png"  "${ALIZAMS_I_DIR}/192x192/apps"
cp "${ALIZAMS_C_DIR}/icons/hicolor/256x256/apps/alizams.png"  "${ALIZAMS_I_DIR}/256x256/apps"
cp "${ALIZAMS_C_DIR}/icons/hicolor/scalable/apps/alizams.svg" "${ALIZAMS_I_DIR}/scalable/apps"
ALIZAMS_DS_FILE="${HOME}/.local/share/applications/alizams.desktop"
ALIZAMS_DIR=$(cd "${ALIZAMS_C_DIR}/.." && pwd -P)
ALIZAMS_EXE="${ALIZAMS_DIR}/alizams.sh"
echo "[Desktop Entry]"              > "${ALIZAMS_DS_FILE}"
echo "Type=Application"            >> "${ALIZAMS_DS_FILE}"
echo "Encoding=UTF-8"              >> "${ALIZAMS_DS_FILE}"
echo "Name=AlizaMS"                >> "${ALIZAMS_DS_FILE}"
echo "GenericName=AlizaMS"         >> "${ALIZAMS_DS_FILE}"
echo "Comment=Medical Imaging"     >> "${ALIZAMS_DS_FILE}"
echo "Exec=${ALIZAMS_EXE} %F"      >> "${ALIZAMS_DS_FILE}"
echo "Icon=alizams"                >> "${ALIZAMS_DS_FILE}"
echo "Terminal=false"              >> "${ALIZAMS_DS_FILE}"
echo "Categories=Graphics;"        >> "${ALIZAMS_DS_FILE}"
echo "StartupNotify=false"         >> "${ALIZAMS_DS_FILE}"
echo "MimeType=application/dicom;" >> "${ALIZAMS_DS_FILE}"

