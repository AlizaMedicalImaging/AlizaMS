Workaround for predictor bug, like in DCMTK, option, disabled by default


Added define in jpegcmake.in.h to silence warnings for jpeg16,
s. REMOVE_OVERFLOW_WARN_JPEG16 in jpegcmake.in.h. Set to 1 to
enable, to 0 to disable the workaround.

