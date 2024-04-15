/*********************************************************
 *
 * MDCM
 *
 * Modifications github.com/issakomi
 *
 *********************************************************/

/*
zipstream Library License:
--------------------------

The zlib/libpng License Copyright (c) 2003 Jonathan de Halleux.

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use
of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim
   that you wrote the original software. If you use this software in a
   product, an acknowledgment in the product documentation would be
   appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution

Author: Jonathan de Halleux, dehalleux@pelikhan.com, 2003

Altered by: Andreas Zieringer 2003 for OpenSG project
            made it platform independent, gzip conform, fixed gzip footer

Altered by: Geoffrey Hutchison 2005 for Open Babel project
            minor namespace modifications, VC++ compatibility

Altered by: Mathieu Malaterre 2008, for GDCM project
            when reading deflate bit stream in DICOM special handling of \0 is needed
            also when writing deflate back to disk, the add_footer must be called

Altered by: Mikhail Isakov 2020, for MDCM project
*/

//*****************************************************************************
//  template class basic_zip_streambuf
//*****************************************************************************

/* Construct a zip stream
 * More info on the following parameters can be found in the zlib documentation.
 */

#ifndef _WIN32
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

template <class charT, class traits>
basic_zip_streambuf<charT, traits>::basic_zip_streambuf(ostream_reference ostream,
                                                        int               level,
                                                        EStrategy         strategy,
                                                        int               window_size,
                                                        int               memory_level,
                                                        size_t            buffer_size)
  : _ostream(ostream)
  , _output_buffer(buffer_size, 0)
  , _buffer(buffer_size, 0)
  , _crc(0)
{
  _zip_stream.zalloc = (alloc_func)NULL;
  _zip_stream.zfree = (free_func)NULL;
  _zip_stream.next_in = NULL;
  _zip_stream.avail_in = 0;
  _zip_stream.avail_out = 0;
  _zip_stream.next_out = NULL;
#if 1
  _zip_stream.total_in = 0;
  _zip_stream.total_out = 0;
  _zip_stream.msg = (char *)NULL;
  _zip_stream.opaque = (voidpf)NULL;
  _zip_stream.data_type = Z_BINARY;
  _zip_stream.adler = 0;
  _zip_stream.reserved = 0;
#endif

  if (level > 9)
    level = 9;

  if (memory_level > 9)
    memory_level = 9;

  _err = deflateInit2(&_zip_stream, level, Z_DEFLATED, window_size, memory_level, static_cast<int>(strategy));
  this->setp(&(_buffer[0]), &(_buffer[_buffer.size() - 1]));
}

/* Destructor */
template <class charT, class traits>
basic_zip_streambuf<charT, traits>::~basic_zip_streambuf(void)
{
  flush();
  // _ostream.flush(); CM already done in flush()
  _err = deflateEnd(&_zip_stream);
}

/* Do the synchronization */
template <class charT, class traits>
int
basic_zip_streambuf<charT, traits>::sync(void)
{
  if (this->pptr() && this->pptr() > this->pbase())
  {
    /*int c =*/overflow(EOF);

    // ACHTUNG wenn das drin ist hoert er nach dem ersten endl auf!
    /*
      if ( c == EOF)
      return -1;
    */
  }

  return 0;
}

template <class charT, class traits>
typename basic_zip_streambuf<charT, traits>::int_type
basic_zip_streambuf<charT, traits>::overflow(int_type c)
{
  int w = static_cast<int>(this->pptr() - this->pbase());
  if (c != EOF)
  {
    *this->pptr() = static_cast<char>(c);
    ++w;
  }
  if (zip_to_stream(this->pbase(), w))
  {
    this->setp(this->pbase(), this->epptr() - 1);
    return c;
  }
  else
  {
    return EOF;
  }
}

/* Flushes the zip buffer and output buffer.
 *
 *    This method should be called at the end of the compression. Calling flush
 *    multiple times, will lower the compression ratio.
 */
template <class charT, class traits>
std::streamsize
basic_zip_streambuf<charT, traits>::flush(void)
{
  std::streamsize total_written_byte_size = 0;

  // Updating crc
  _crc = crc32(_crc, _zip_stream.next_in, _zip_stream.avail_in);

  do
  {
    _err = deflate(&_zip_stream, Z_FINISH);
    if (_err == Z_OK || _err == Z_STREAM_END)
    {
      const std::streamsize written_byte_size = static_cast<std::streamsize>(_output_buffer.size()) - _zip_stream.avail_out;
      total_written_byte_size += written_byte_size;
      // Ouput buffer is full, dumping to ostream
      _ostream.write(reinterpret_cast<const char_type *>(_output_buffer.data()),
                     static_cast<std::streamsize>(written_byte_size / sizeof(char_type) * sizeof(char)));

      // Checking if some bytes were not written.
      const size_t remainder = written_byte_size % sizeof(char_type) != 0;
      if (remainder != 0)
      {
        // Copy to the beginning of the stream
        std::streamsize theDiff = written_byte_size - remainder;
        memcpy(_output_buffer.data(), &(_output_buffer[static_cast<unsigned int>(theDiff)]), remainder);
      }

      _zip_stream.avail_out = static_cast<uInt>(_output_buffer.size() - remainder);
      _zip_stream.next_out = &_output_buffer[remainder];
    }
  } while (_err == Z_OK);

  _ostream.flush();

  return total_written_byte_size;
}

/* Returns a reference to the output stream */
template <class charT, class traits>
inline typename basic_zip_streambuf<charT, traits>::ostream_reference
basic_zip_streambuf<charT, traits>::get_ostream(void) const
{
  return _ostream;
}

/* Returns the latest zlib error status */
template <class charT, class traits>
inline int
basic_zip_streambuf<charT, traits>::get_zerr(void) const
{
  return _err;
}

/* Returns the crc of the input data compressed so far  */
template <class charT, class traits>
inline unsigned long
basic_zip_streambuf<charT, traits>::get_crc(void) const
{
  return _crc;
}

/*  Returns the size (bytes) of the input data compressed so far */
template <class charT, class traits>
inline unsigned long
basic_zip_streambuf<charT, traits>::get_in_size(void) const
{
  return _zip_stream.total_in;
}

/*  Returns the size (bytes) of the compressed data so far */
template <class charT, class traits>
inline long
basic_zip_streambuf<charT, traits>::get_out_size(void) const
{
  return _zip_stream.total_out;
}

template <class charT, class traits>
bool
basic_zip_streambuf<charT, traits>::zip_to_stream(char_type * buffer, std::streamsize buffer_size)
{
  std::streamsize total_written_byte_size = 0;

  _zip_stream.next_in = reinterpret_cast<byte_buffer_type>(buffer);
  _zip_stream.avail_in = static_cast<uInt>(buffer_size * sizeof(char_type));
  _zip_stream.avail_out = static_cast<uInt>(_output_buffer.size());
  _zip_stream.next_out = _output_buffer.data();

  // Updating crc
  _crc = crc32(_crc, _zip_stream.next_in, _zip_stream.avail_in);

  do
  {
    _err = deflate(&_zip_stream, 0);

    if (_err == Z_OK || _err == Z_STREAM_END)
    {
      const std::streamsize written_byte_size = static_cast<std::streamsize>(_output_buffer.size()) - _zip_stream.avail_out;
      total_written_byte_size += written_byte_size;
      // Ouput buffer is full, dumping to ostream

      _ostream.write(reinterpret_cast<const char_type *>(_output_buffer.data()),
                     static_cast<std::streamsize>(written_byte_size / sizeof(char_type)));

      // Checking if some bytes were not written.
      const size_t remainder = written_byte_size % sizeof(char_type);
      if (remainder != 0)
      {
        // Copy to the beginning of the stream
        std::streamsize theDiff = written_byte_size - remainder;
        // assert(theDiff > 0 && theDiff < std::numeric_limits<unsigned int>::max());
        memcpy(_output_buffer.data(), &_output_buffer[static_cast<unsigned int>(theDiff)], remainder);
      }

      _zip_stream.avail_out = static_cast<uInt>(_output_buffer.size() - remainder);
      _zip_stream.next_out = &_output_buffer[remainder];
    }
  } while (_zip_stream.avail_in != 0 && _err == Z_OK);

  return _err == Z_OK;
}

//*****************************************************************************
//  template class basic_unzip_streambuf
//*****************************************************************************

/* Constructor */
template <class charT, class traits>
basic_unzip_streambuf<charT, traits>::basic_unzip_streambuf(istream_reference istream,
                                                            int               window_size,
                                                            size_t            read_buffer_size,
                                                            size_t            input_buffer_size)
  : _istream(istream)
  , _input_buffer(input_buffer_size)
  , _buffer(read_buffer_size)
  , _crc(0)
{
  _zip_stream.zalloc = (alloc_func)NULL;
  _zip_stream.zfree = (free_func)NULL;
  _zip_stream.next_in = NULL;
  _zip_stream.avail_in = 0;
  _zip_stream.avail_out = 0;
  _zip_stream.next_out = NULL;
#if 1
  _zip_stream.total_in = 0;
  _zip_stream.total_out = 0;
  _zip_stream.msg = (char *)NULL;
  _zip_stream.opaque = (voidpf)NULL;
  _zip_stream.data_type = Z_BINARY;
  _zip_stream.adler = 0;
  _zip_stream.reserved = 0;
#endif

  _err = inflateInit2(&_zip_stream, window_size);

  this->setg(_buffer.data() + 4,  // beginning of putback area
             _buffer.data() + 4,  // read position
             _buffer.data() + 4); // end position
}

template <class charT, class traits>
basic_unzip_streambuf<charT, traits>::~basic_unzip_streambuf(void)
{
  inflateEnd(&_zip_stream);
}

template <class charT, class traits>
typename basic_unzip_streambuf<charT, traits>::int_type
basic_unzip_streambuf<charT, traits>::underflow(void)
{
  if (this->gptr() && (this->gptr() < this->egptr()))
    return *reinterpret_cast<unsigned char *>(this->gptr());

  int n_putback = static_cast<int>(this->gptr() - this->eback());
  if (n_putback > 4)
    n_putback = 4;

  memcpy(_buffer.data() + (4 - n_putback), this->gptr() - n_putback, n_putback * sizeof(char_type));

  std::streamsize num =
    unzip_from_stream(_buffer.data() + 4, static_cast<std::streamsize>((_buffer.size() - 4) * sizeof(char_type)));

  if (num <= 0) // ERROR or EOF
    return EOF;

  // Reset buffer pointers
  this->setg(_buffer.data() + (4 - n_putback), // beginning of putback area
             _buffer.data() + 4,               // read position
             _buffer.data() + 4 + num);        // end of buffer

  // Return next character
  return *reinterpret_cast<unsigned char *>(this->gptr());
}

/* Returns the compressed input istream */
template <class charT, class traits>
inline typename basic_unzip_streambuf<charT, traits>::istream_reference
basic_unzip_streambuf<charT, traits>::get_istream(void)
{
  return _istream;
}

/* Returns the zlib stream structure */
template <class charT, class traits>
inline z_stream &
basic_unzip_streambuf<charT, traits>::get_zip_stream(void)
{
  return _zip_stream;
}

/* Returns the latest zlib error state */
template <class charT, class traits>
inline int
basic_unzip_streambuf<charT, traits>::get_zerr(void) const
{
  return _err;
}

/* Returns the crc of the uncompressed data so far */
template <class charT, class traits>
inline unsigned long
basic_unzip_streambuf<charT, traits>::get_crc(void) const
{
  return _crc;
}

/* Returns the number of uncompressed bytes */
template <class charT, class traits>
inline long
basic_unzip_streambuf<charT, traits>::get_out_size(void) const
{
  return _zip_stream.total_out;
}

/* Returns the number of read compressed bytes */
template <class charT, class traits>
inline long
basic_unzip_streambuf<charT, traits>::get_in_size(void) const
{
  return _zip_stream.total_in;
}

template <class charT, class traits>
inline void
basic_unzip_streambuf<charT, traits>::put_back_from_zip_stream(void)
{
  if (_zip_stream.avail_in == 0)
    return;

  _istream.clear(std::ios::goodbit);
  _istream.seekg(-intf(_zip_stream.avail_in), std::ios_base::cur);

  _zip_stream.avail_in = 0;
}

template <class charT, class traits>
inline std::streamsize
basic_unzip_streambuf<charT, traits>::unzip_from_stream(char_type * buffer, std::streamsize buffer_size)
{
  _zip_stream.next_out = reinterpret_cast<byte_buffer_type>(buffer);
  _zip_stream.avail_out = static_cast<uInt>(buffer_size * sizeof(char_type));
  size_t count = _zip_stream.avail_in;

  do
  {
    if (_zip_stream.avail_in == 0)
      count = fill_input_buffer();

    if (_zip_stream.avail_in)
    {
      _err = inflate(&_zip_stream, Z_SYNC_FLUSH);
    }
  } while (_err == Z_OK && _zip_stream.avail_out != 0 && count != 0);

  std::streamsize theSize = buffer_size - (static_cast<std::streamsize>(_zip_stream.avail_out)) / sizeof(char_type);
  // assert (theSize >= 0 && theSize < std::numeric_limits<uInt>::max());

  // Updating crc
  _crc = crc32(_crc, reinterpret_cast<byte_buffer_type>(buffer), static_cast<uInt>(theSize));

  std::streamsize n_read = buffer_size - _zip_stream.avail_out / sizeof(char_type);

  // Check if it is the end
  if (_err == Z_STREAM_END)
    put_back_from_zip_stream();

  return n_read;
}

template <class charT, class traits>
inline size_t
basic_unzip_streambuf<charT, traits>::fill_input_buffer(void)
{
  _zip_stream.next_in = _input_buffer.data();
  _istream.read(reinterpret_cast<char_type *>(_input_buffer.data()), static_cast<std::streamsize>(_input_buffer.size() / sizeof(char_type)));
  std::streamsize nbytesread = _istream.gcount() * sizeof(char_type);
  if (!_istream)
  {
    if (_istream.eof())
    {
      // Ok so we reached the end of file, since we did not read no header
      // we have to explicitly tell zlib the compress stream ends, therefore
      // we add an extra \0 character...it may not always be needed...
      assert(nbytesread < static_cast<std::streamsize>(_input_buffer.size() / sizeof(char_type)));
      _input_buffer[static_cast<unsigned int>(nbytesread)] = 0;
      ++nbytesread;
    }
  }

  return _zip_stream.avail_in = static_cast<uInt>(nbytesread);
}

//*****************************************************************************
//  template class basic_zip_ostream
//*****************************************************************************

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4355)
#endif

template <class charT, class traits>
inline basic_zip_ostream<charT, traits>::basic_zip_ostream(ostream_reference ostream,
                                                           bool              isgzip,
                                                           int               level,
                                                           EStrategy         strategy,
                                                           int               window_size,
                                                           int               memory_level,
                                                           size_t            buffer_size)
  : basic_zip_streambuf<charT, traits>(ostream, level, strategy, window_size, memory_level, buffer_size)
  , std::basic_ostream<charT, traits>(this)
  , _is_gzip(isgzip)
  , _added_footer(false)
{
  if (_is_gzip)
    add_header();
}

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

/* Destructor */
template <class charT, class traits>
basic_zip_ostream<charT, traits>::~basic_zip_ostream(void)
{
  // if(_is_gzip)
  add_footer();
}

/* Returns true if it is a gzip */
template <class charT, class traits>
inline bool
basic_zip_ostream<charT, traits>::is_gzip(void) const
{
  return _is_gzip;
}

/* Flush inner buffer and zipper buffer */
template <class charT, class traits>
inline basic_zip_ostream<charT, traits> &
basic_zip_ostream<charT, traits>::zflush(void)
{
  static_cast<std::basic_ostream<charT, traits> *>(this)->flush();
  static_cast<basic_zip_streambuf<charT, traits> *>(this)->flush();
  return *this;
}

template <class charT, class traits>
inline void
basic_zip_ostream<charT, traits>::finished(void)
{
  if (_is_gzip)
    add_footer();
  else
    zflush();
}

template <class charT, class traits>
basic_zip_ostream<charT, traits> &
basic_zip_ostream<charT, traits>::add_header(void)
{
  char_type zero = 0;

  this->get_ostream() << static_cast<char_type>(detail::gz_magic[0]) << static_cast<char_type>(detail::gz_magic[1])
                      << static_cast<char_type>(Z_DEFLATED) << zero // flags
                      << zero << zero << zero << zero               // time
                      << zero                                       // xflags
                      << static_cast<char_type>(OS_CODE);

  return *this;
}

template <class charT, class traits>
basic_zip_ostream<charT, traits> &
basic_zip_ostream<charT, traits>::add_footer(void)
{
  if (_added_footer)
    return *this;

  zflush();

  _added_footer = true;

  // Writes crc and length in LSB order to the stream
  unsigned long crc = this->get_crc();
  for (int n = 0; n < 4; ++n)
  {
    this->get_ostream().put(static_cast<char>(crc & 0xff));
    crc >>= 8;
  }

  unsigned long length = this->get_in_size();
  for (int m = 0; m < 4; ++m)
  {
    this->get_ostream().put(static_cast<char>(length & 0xff));
    length >>= 8;
  }

  return *this;
}

//*****************************************************************************
//  template class basic_zip_istream
//*****************************************************************************

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4355)
#endif

/* Constructor */
template <class charT, class traits>
basic_zip_istream<charT, traits>::basic_zip_istream(istream_reference istream,
                                                    int               window_size,
                                                    size_t            read_buffer_size,
                                                    size_t            input_buffer_size)
  : basic_unzip_streambuf<charT, traits>(istream, window_size, read_buffer_size, input_buffer_size)
  , std::basic_istream<charT, traits>(this)
  , _is_gzip(false)
  , _gzip_crc(0)
  , _gzip_data_size(0)
{
  if (this->get_zerr() == Z_OK)
  {
    int check = check_header();
    (void)check;
  }
}

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

/* Returns true if it is a gzip file */
template <class charT, class traits>
inline bool
basic_zip_istream<charT, traits>::is_gzip(void) const
{
  return _is_gzip;
}

/* Return crc check result
 *
 * This must be called after the reading of compressed data is finished!  This
 * method compares it to the crc of the uncompressed data.
 *
 *    return true if crc check is succesful
 */
template <class charT, class traits>
inline bool
basic_zip_istream<charT, traits>::check_crc(void)
{
  read_footer();
  return this->get_crc() == _gzip_crc;
}

/* Return data size check */
template <class charT, class traits>
inline bool
basic_zip_istream<charT, traits>::check_data_size(void) const
{
  return this->get_out_size() == _gzip_data_size;
}

/* Return the crc value in the file */
template <class charT, class traits>
inline long
basic_zip_istream<charT, traits>::get_gzip_crc(void) const
{
  return _gzip_crc;
}

/* Return the data size in the file */
template <class charT, class traits>
inline long
basic_zip_istream<charT, traits>::get_gzip_data_size(void) const
{
  return _gzip_data_size;
}

template <class charT, class traits>
int
basic_zip_istream<charT, traits>::check_header(void)
{
  int        method;    /* method byte */
  int        flagsbyte; /* flags byte */
  uInt       len;
  int        c;
  int        err = 0;
  z_stream & zip_stream = this->get_zip_stream();

  /* Check the gzip magic header */
  for (len = 0; len < 2; ++len)
  {
    c = static_cast<int>(this->get_istream().get());
    if (c != detail::gz_magic[len])
    {
      if (len != 0)
        this->get_istream().unget();
      if (c != EOF)
      {
        this->get_istream().unget();
      }

      err = zip_stream.avail_in != 0 ? Z_OK : Z_STREAM_END;
      _is_gzip = false;
      return err;
    }
  }

  _is_gzip = true;
  method = static_cast<int>(this->get_istream().get());
  flagsbyte = static_cast<int>(this->get_istream().get());
  if (method != Z_DEFLATED || (flagsbyte & detail::gz_reserved) != 0)
  {
    err = Z_DATA_ERROR;
    return err;
  }

  /* Discard time, xflags and OS code: */
  for (len = 0; len < 6; ++len)
    this->get_istream().get();

  if ((flagsbyte & detail::gz_extra_field) != 0)
  {
    /* Skip the extra field */
    len = static_cast<uInt>(this->get_istream().get());
    len += (static_cast<uInt>(this->get_istream().get())) << 8;
    /* Len is garbage if EOF but the loop below will quit anyway */
    while (len-- != 0 && this->get_istream().get() != EOF)
      ;
  }
  if ((flagsbyte & detail::gz_orig_name) != 0)
  {
    /* Skip the original file name */
    while ((c = this->get_istream().get()) != 0 && c != EOF)
      ;
  }
  if ((flagsbyte & detail::gz_comment) != 0)
  {
    /* Skip the .gz file comment */
    while ((c = this->get_istream().get()) != 0 && c != EOF)
      ;
  }
  if ((flagsbyte & detail::gz_head_crc) != 0)
  { /* Skip the header crc */
    for (len = 0; len < 2; ++len)
      this->get_istream().get();
  }
  err = this->get_istream().eof() ? Z_DATA_ERROR : Z_OK;

  return err;
}

template <class charT, class traits>
void
basic_zip_istream<charT, traits>::read_footer(void)
{
  if (_is_gzip)
  {
    _gzip_crc = 0;
    for (int n = 0; n < 4; ++n)
      _gzip_crc += (((static_cast<int>(this->get_istream().get())) & 0xff) << (8 * n));

    _gzip_data_size = 0;
    for (int n = 0; n < 4; ++n)
      _gzip_data_size += (((static_cast<int>(this->get_istream().get())) & 0xff) << (8 * n));
  }
}

#ifndef _WIN32
#  pragma GCC diagnostic pop
#endif
