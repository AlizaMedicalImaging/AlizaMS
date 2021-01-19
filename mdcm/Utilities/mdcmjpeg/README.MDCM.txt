Added cmake option (s. jpegcmake.in.h) SUPPORT_JPEG_PRED6_BUG
to enable/disable 'PREDICTOR6 bug' workaround. Currently (2021)
that workaround is removed from DCMTK and GDCM (disable option
for same behaviour). Enabled by default.


Added define in jpegcmake.in.h to silence warnings for jpeg16,
s. REMOVE_OVERFLOW_WARN_JPEG16 in jpegcmake.in.h. Set to 1 to
enable, to 0 to disable the workaround.

