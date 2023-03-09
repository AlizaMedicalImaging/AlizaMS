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

#include <QtGlobal>
#include <QStringList>
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#include <QRegularExpression>
#else
#include <QRegExp>
#endif
#include <QTextCodec>
#include "codecutils.h"
#if 0
#include <iostream>
#endif

/* Qt
 *
 * Convert using Qt utilities. Code extension techniques for e.g.
 * 'ISO 2022 IR 13\ISO 2022 IR 87' or '\ISO 2022 IR 149'
 * are supported. There is an attempt to guess custom value, e.g.
 * 'WINDOWS 1251'. Use Latin1 if failed.
 */
QString CodecUtils::toUTF8(const QByteArray* ba, const char* charset, bool * ok)
{
  QString result("");
  if (!ba)
  {
    if (ok) *ok = true;
    return result;
  }
  QString cs = QString::fromLatin1(charset);
  if (cs.isEmpty())
  {
    if (ok) *ok = true;
    return QString::fromLatin1(ba->constData());
  }
  if (ok) *ok = false;
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
  const QStringList l = cs.split(QString("\\"), Qt::KeepEmptyParts);
#else
  const QStringList l = cs.split(QString("\\"), QString::KeepEmptyParts);
#endif
  bool iso2022 = false;
  for (int x = 0; x < l.size(); ++x)
  {
    const QString tmp1 = l.at(x).trimmed().toUpper();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    if (tmp1.contains(QRegularExpression(QString("ISO\\s+2022"))))
#else
    if (tmp1.contains(QRegExp(QString("ISO\\s+2022"))))
#endif
    {
      iso2022 = true;
      break;
    }
  }
  // ISO 2022
  if (iso2022)
  {
    if ((*ba).contains(0x1b)) // ESC
    {
      const QList<QByteArray> iso = ba->split(0x1b);
      // detect by ESC sequence
      for (int z = 0; z < iso.size(); ++z)
      {
        QTextCodec * codec = NULL;
        QByteArray a = iso[z];
        // ISO 2022 IR 6
        if (a.size() >= 2 && a.at(0) == 40 && a.at(1) == 66)
        {
          result += QString::fromLatin1(a.mid(2).constData());
        }
        // ISO 2022 IR 13 (ISO IR 13)
        else if (a.size() >= 2 && a.at(0) == 41 && a.at(1) == 73)
        {
          codec = QTextCodec::codecForName("ISO-2022-JP");
          if (codec)
          {
            result += codec->toUnicode(a.prepend(0x1b));
          }
        }
        // ISO 2022 IR 13 (ISO IR 14)
        else if (a.size() >= 2 && a.at(0) == 40 && a.at(1) == 74)
        {
          codec = QTextCodec::codecForName("ISO-2022-JP");
          if (codec)
          {
            result += codec->toUnicode(a.prepend(0x1b));
          }
        }
        // ISO 2022 IR 58
        else if (a.size() >= 3 && a.at(0) == 36 && a.at(1) == 41 && a.at(2) == 65)
        {
          codec = QTextCodec::codecForName("GB2312");
          if (codec)
          {
            result += codec->toUnicode(a.mid(3));
          }
        }
        // ISO 2022 IR 87
        else if (a.size() >= 2 && a.at(0) == 36 && a.at(1) == 66)
        {
          codec = QTextCodec::codecForName("ISO-2022-JP");
          if (codec)
          {
            result += codec->toUnicode(a.prepend(0x1b));
          }
        }
        // ISO 2022 IR 100
        else if (a.size() >= 2 && a.at(0) == 45 && a.at(1) == 65)
        {
          codec = QTextCodec::codecForName("ISO-8859-1");
          if (codec)
          {
            result += codec->toUnicode(a.mid(2));
          }
        }
        // ISO 2022 IR 101
        else if (a.size() >= 2 && a.at(0) == 45 && a.at(1) == 66)
        {
          codec = QTextCodec::codecForName("ISO-8859-2");
          if (codec)
          {
            result += codec->toUnicode(a.mid(2));
          }
        }
        // ISO 2022 IR 109
        else if (a.size() >= 2 && a.at(0) == 45 && a.at(1) == 67)
        {
          codec = QTextCodec::codecForName("ISO-8859-3");
          if (codec)
          {
            result += codec->toUnicode(a.mid(2));
          }
        }
        // ISO 2022 IR 110
        else if (a.size() >= 2 && a.at(0) == 45 && a.at(1) == 68)
        {
          codec = QTextCodec::codecForName("ISO-8859-4");
          if (codec)
          {
            result += codec->toUnicode(a.mid(2));
          }
        }
        // ISO 2022 IR 144
        else if (a.size() >= 2 && a.at(0) == 45 && a.at(1) == 76)
        {
          codec = QTextCodec::codecForName("ISO-8859-5");
          if (codec)
          {
            result += codec->toUnicode(a.mid(2));
          }
        }
        // ISO 2022 IR 127
        else if (a.size() >= 2 && a.at(0) == 45 && a.at(1) == 71)
        {
          codec = QTextCodec::codecForName("ISO-8859-6");
          if (codec)
          {
            result += codec->toUnicode(a.mid(2));
          }
        }
        // ISO 2022 IR 126
        else if (a.size() >= 2 && a.at(0) == 45 && a.at(1) == 70)
        {
          codec = QTextCodec::codecForName("ISO-8859-7");
          if (codec)
          {
            result += codec->toUnicode(a.mid(2));
          }
        }
        // ISO 2022 IR 138
        else if (a.size() >= 2 && a.at(0) == 45 && a.at(1) == 72)
        {
          codec = QTextCodec::codecForName("ISO-8859-8");
          if (codec)
          {
            result += codec->toUnicode(a.mid(2));
          }
        }
        // ISO 2022 IR 148
        else if (a.size() >= 2 && a.at(0) == 45 && a.at(1) == 77)
        {
          codec = QTextCodec::codecForName("ISO-8859-9");
          if (codec)
          {
            result += codec->toUnicode(a.mid(2));
          }
        }
        // ISO 2022 IR 149
        else if (a.size() >= 3 && a.at(0) == 36 && a.at(1) == 41 && a.at(2) == 67)
        {
          codec = QTextCodec::codecForName("iso-ir-149");
          if (codec)
          {
            result += codec->toUnicode(a.mid(3));
          }
          if (!codec)
          {
            codec = QTextCodec::codecForName("EUC-KR");
            if (codec)
            {
              result += codec->toUnicode(a.mid(3));
            }
          }
          if (!codec)
          {
            codec = QTextCodec::codecForName("ISO-2022-KR");
            if (codec)
            {
              result += codec->toUnicode(a.prepend(0x1b));
            }
          }
          if (!codec)
          {
            result += QString::fromLatin1(a.mid(3).constData());
          }
        }
        // ISO 2022 IR 159
        else if (a.size() >= 3 && a.at(0) == 36 && a.at(1) == 40 && a.at(2) == 68)
        {
          codec = QTextCodec::codecForName("ISO-2022-JP");
          if (codec)
          {
            result += codec->toUnicode(a.prepend(0x1b));
          }
        }
        // ISO 2022 IR 166
        else if (a.size() >= 2 && a.at(0) == 45 && a.at(1) == 85)
        {
          codec = QTextCodec::codecForName("TIS-620");
          if (codec)
          {
              result += codec->toUnicode(a.mid(2));
          }
        }
        else
        {
          // ISO IR 13
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
          const QRegularExpression re(QString("ISO\\s+2022\\s+IR\\s+13"));
#else
          const QRegExp re(QString("ISO\\s+2022\\s+IR\\s+13"));
#endif
          const int i = l.indexOf(re, Qt::CaseInsensitive);
          if (i > -1)
          {
            codec = QTextCodec::codecForName("Shift_JIS");
            if (codec)
            {
              result += codec->toUnicode(a);
            }
          }
          if (!codec)
          {
            result += QString::fromLatin1(a.constData());
          }
        }
        if (ok && codec) *ok = true;
      }
    }
    else // ISO 2022, but no ESC character
    {
      cs = cs.trimmed();
      QTextCodec * codec = NULL;
      if (cs == QString("ISO 2022 IR 149")) // data sets exist, tested
      {
        codec = QTextCodec::codecForName("iso-ir-149");
        if (!codec)
        {
          codec = QTextCodec::codecForName("EUC-KR");
        }
      }
      // below variants are not tested
      else if (cs == QString("ISO 2022 IR 58"))
      {
        codec = QTextCodec::codecForName("GB2312");
      }
      else if (cs == QString("ISO 2022 IR 13") ||
               cs == QString("ISO 2022 IR 14"))
      {
        codec = QTextCodec::codecForName("Shift_JIS");
      }
      else if (cs == QString("ISO 2022 IR 166"))
      {
        codec = QTextCodec::codecForName("TIS-620");
      }
      else if (cs == QString("ISO 2022 IR 100"))
      {
        codec = QTextCodec::codecForName("ISO-8859-1");
      }
      else if (cs == QString("ISO 2022 IR 101"))
      {
        codec = QTextCodec::codecForName("ISO-8859-2");
      }
      else if (cs == QString("ISO 2022 IR 109"))
      {
        codec = QTextCodec::codecForName("ISO-8859-3");
      }
      else if (cs == QString("ISO 2022 IR 110"))
      {
        codec = QTextCodec::codecForName("ISO-8859-4");
      }
      else if (cs == QString("ISO 2022 IR 144"))
      {
        codec = QTextCodec::codecForName("ISO-8859-5");
      }
      else if (cs == QString("ISO 2022 IR 127"))
      {
        codec = QTextCodec::codecForName("ISO-8859-6");
      }
      else if (cs == QString("ISO 2022 IR 126"))
      {
        codec = QTextCodec::codecForName("ISO-8859-7");
      }
      else if (cs == QString("ISO 2022 IR 138"))
      {
        codec = QTextCodec::codecForName("ISO-8859-8");
      }
      else if (cs == QString("ISO 2022 IR 148"))
      {
        codec = QTextCodec::codecForName("ISO-8859-9");
      }
      if (codec)
      {
        if (ok) *ok = true;
        result += codec->toUnicode(*ba);
      }
      else
      {
        if (cs == QString("ISO 2022 IR 6"))
        {
          if (ok) *ok = true;
          result += QString::fromLatin1(ba->constData());
        }
        else
        {
          result += QString::fromLatin1(ba->constData()); // error
        }
      }
    }
  }
  else if (l.size() == 1) // single value, not ISO 2022, tested
  {
    QTextCodec * codec = NULL;
    const QString s(l.at(0).trimmed().simplified().toUpper());
    // ISO IR 100
    if (s.contains(QString("IR 100")))
    {
      codec = QTextCodec::codecForName("ISO-8859-1");
    }
    // ISO IR 192
    else if (s.contains(QString("IR 192")))
    {
      codec = QTextCodec::codecForName("UTF-8");
    }
    // GB18030
    else if (s.contains(QString("GB18030")))
    {
      codec = QTextCodec::codecForName("GB18030");
    }
    // GBK
    else if (s.contains(QString("GBK")))
    {
      codec = QTextCodec::codecForName("GBK");
    }
    // ISO IR 166
    else if (s.contains(QString("IR 166")))
    {
      codec = QTextCodec::codecForName("TIS-620");
    }
    // ISO IR 148
    else if (s.contains(QString("IR 148")))
    {
      codec = QTextCodec::codecForName("ISO-8859-9");
    }
    // ISO IR 144
    else if (s.contains(QString("IR 144")))
    {
      codec = QTextCodec::codecForName("ISO-8859-5");
    }
    // ISO IR 138
    else if (s.contains(QString("IR 138")))
    {
      codec = QTextCodec::codecForName("ISO-8859-8");
    }
    // ISO IR 127
    else if (s.contains(QString("IR 127")))
    {
      codec = QTextCodec::codecForName("ISO-8859-6");
    }
    // ISO IR 126
    else if (s.contains(QString("IR 126")))
    {
      codec = QTextCodec::codecForName("ISO-8859-7");
    }
    // ISO IR 110
    else if (s.contains(QString("IR 110")))
    {
      codec = QTextCodec::codecForName("ISO-8859-4");
    }
    // ISO IR 109
    else if (s.contains(QString("IR 109")))
    {
      codec = QTextCodec::codecForName("ISO-8859-3");
    }
    // ISO IR 101
    else if (s.contains(QString("IR 101")))
    {
      codec = QTextCodec::codecForName("ISO-8859-2");
    }
    // ISO IR 13
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    else if (s.contains(QRegularExpression(QString("IR 13\\D*"))))
#else
    else if (s.contains(QRegExp(QString("IR 13\\D*"))))
#endif
    {
      codec = QTextCodec::codecForName("Shift_JIS");
    }
    // ISO IR 6
    else if (s.contains(QString("IR 6")))
    {
      if (ok) *ok = true;
	  return QString::fromLatin1(ba->constData());
    }
    else if (!s.isEmpty())
    {
      codec = QTextCodec::codecForName(s.toLatin1().constData());
    }
    if (codec)
    {
      if (ok) *ok = true;
      result = codec->toUnicode(*ba);
    }
    else // error
    {
      result = QString::fromLatin1(ba->constData());
    }
  }
  else // multiple values, but not ISO 2022, error
  {
    result = QString::fromLatin1(ba->constData());
  }
  return result;
}

// TODO
QByteArray CodecUtils::fromUTF8(const QString & i, const char * charset, bool * ok)
{
  if (i.isEmpty())
  {
    *ok = true;
    return QByteArray();
  }
  const QString cs = QString::fromLatin1(charset);
  if (cs.isEmpty())
  {
    *ok = true;
    return i.toLatin1();
  }
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
  const QStringList l = cs.split(QString("\\"), Qt::KeepEmptyParts);
#else
  const QStringList l = cs.split(QString("\\"), QString::KeepEmptyParts);
#endif
  bool iso2022 = false;
  for (int x = 0; x < l.size(); ++x)
  {
    const QString tmp1 = l.at(x).trimmed().toUpper();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    if (tmp1.contains(QRegularExpression(QString("ISO\\s+2022"))))
#else
    if (tmp1.contains(QRegExp(QString("ISO\\s+2022"))))
#endif
    {
      iso2022 = true;
      break;
    }
  }
  if (iso2022)
  {
    // TODO
#if 0
    std::cout << "Warning: ISO 2022, not supported" << std::endl;
#endif
  }
  if (l.size() == 1)
  {
    QTextCodec * codec = NULL;
    const QString s(l.at(0).trimmed().simplified().toUpper());
    // ISO IR 100
    if (s.contains(QString("IR 100")))
    {
      *ok = true;
      return i.toLatin1();
    }
    // ISO IR 192
    else if (s.contains(QString("IR 192")))
    {
      *ok = true;
      return i.toUtf8();
    }
    // GB18030
    else if (s.contains(QString("GB18030")))
    {
      codec = QTextCodec::codecForName("GB18030");
    }
    // GBK
    else if (s.contains(QString("GBK")))
    {
      codec = QTextCodec::codecForName("GBK");
    }
    // ISO IR 166
    else if (s.contains(QString("IR 166")))
    {
      codec = QTextCodec::codecForName("TIS-620");
    }
    // ISO IR 148
    else if (s.contains(QString("IR 148")))
    {
      codec = QTextCodec::codecForName("ISO-8859-9");
    }
    // ISO IR 144
    else if (s.contains(QString("IR 144")))
    {
      codec = QTextCodec::codecForName("ISO-8859-5");
    }
    // ISO IR 138
    else if (s.contains(QString("IR 138")))
    {
      codec = QTextCodec::codecForName("ISO-8859-8");
    }
    // ISO IR 127
    else if (s.contains(QString("IR 127")))
    {
      codec = QTextCodec::codecForName("ISO-8859-6");
    }
    // ISO IR 126
    else if (s.contains(QString("IR 126")))
    {
      codec = QTextCodec::codecForName("ISO-8859-7");
    }
    // ISO IR 110
    else if (s.contains(QString("IR 110")))
    {
      codec = QTextCodec::codecForName("ISO-8859-4");
    }
    // ISO IR 109
    else if (s.contains(QString("IR 109")))
    {
      codec = QTextCodec::codecForName("ISO-8859-3");
    }
    // ISO IR 101
    else if (s.contains(QString("IR 101")))
    {
      codec = QTextCodec::codecForName("ISO-8859-2");
    }
    // ISO IR 13
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    else if (s.contains(QRegularExpression(QString("IR 13\\D*"))))
#else
    else if (s.contains(QRegExp(QString("IR 13\\D*"))))
#endif
    {
      codec = QTextCodec::codecForName("Shift_JIS");
    }
    // ISO IR 6
    else if (s.contains(QString("IR 6")))
    {
      *ok = true;
      return i.toLatin1();
    }
    else if (!s.isEmpty())
    {
      codec = QTextCodec::codecForName(s.toLatin1().constData());
    }
    if (codec)
    {
      *ok = true;
      return codec->fromUnicode(i);
    }
    else
    {
      *ok = false;
#if 0
      std::cout << "fromUTF8 failed for " << charset << std::endl;
#endif
      return i.toLatin1();
    }
  }
#if 0
  std::cout << "Currently not supported" << std::endl;
#endif
  *ok = false;
  return i.toLatin1();
}

#if 0
/* DCMTK
 *
 * DCMTK's conversion dependens on underlying libraries (libiconv, ICU)
 * and actually has limitations, e.g. important 'ISO 2022 IR 159' and
 * 'ISO 2022 IR 87' are supported with libiconv, but not supported by
 * ICU 63.1.0.
 * With DCMTK is possible to convert the whole data set with convertToUTF8(),
 * but caution, it updates (0008,0005).
 * There is also a function DcmSpecificCharacterSet::isConversionAvailable()
 * to check whether the underlying character set conversion library is
 * available, it may be not available at all, if not, no conversion between
 * different character sets will be possible with DCMTK.
 *
 * The function is mostly for testing purposes.
 */
#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dcspchrs.h>
#include <dcmtk/dcmdata/dctypes.h>

QString CodecUtils::toUTF8dcmtk(const char* ba, const char* charset, bool * ok)
{
  QString res;
  OFString r;
  DcmSpecificCharacterSet converter;
  converter.selectCharacterSet(charset);
  OFCondition c = converter.convertString(ba, r); // to UTF-8
  if (c.good())
  {
    if (ok) *ok = true;
    res = QString::fromUtf8(r.data());
  }
  else
  {
    if (ok) *ok = false;
    res = QString::fromLatin1(ba);
  }
  converter.clear();
  return res;
}
#endif

