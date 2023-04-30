/*=========================================================================

  Copyright (c) Mihail Isakov

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

#ifndef A_CODECUTILS_H
#define A_CODECUTILS_H

#include <QByteArray>
#include <QString>

class CodecUtils
{
public:
  /* Converting different DICOM character sets to UTF-8.
   *   - ba : byte array to encode
   *   - charset : Specific Character Set 0008|0005 byte value
   *   - ok : optional success flag
   *
   * Applicable to value representations LO, LT, PN, SH, ST, UC and UT.
   */
  static QString toUTF8(const QByteArray*, const char*, bool(*) = nullptr);
  /* Convert from Utf8
   */
  static QByteArray fromUTF8(const QString&, const char*, bool*);
#if 0
  static QString toUTF8dcmtk(const char* ba, const char* charset, bool* ok = nullptr);
#endif
  static void set_force_cp1251(bool);
  static bool get_force_cp1251();

private:
  static bool force_cp1251;
};

#endif
