# include "../../readers/read-zip.hpp"
# include <minizip/unzip.h>
# include <stdexcept>
# include <cstring>
#include <dirent.h>

namespace palmira {
namespace minizip {

  class ByteView final: public mtc::IByteBuffer
  {
    implement_lifetime_stub

  public:
    ByteView( const char* p, size_t l ):
      data( p ),
      size( l ) {}

    const char* GetPtr() const override {  return data;  }
    size_t      GetLen() const override {  return size;  }
    int         SetBuf( const void*, size_t ) override {  return EFAULT;  }
    int         SetLen( size_t              ) override {  return EFAULT;  }

    operator const IByteBuffer* () const {  return this;  }

  protected:
    const char* data;
    size_t      size;

  };

  class ByteBuff final: public std::vector<char>, public mtc::IByteBuffer
  {
    implement_lifetime_control

  public:
    const char* GetPtr() const override {  return data();  }
    size_t      GetLen() const override {  return size();  }
    int         SetBuf( const void*, size_t ) override  {  return -1;  }
    int         SetLen( size_t ) override {  return -1;  }
  };

  struct STM
  {
    mtc::api<const mtc::IByteBuffer>  data;
    int64_t                           cpos = 0;
    int                               nerr = 0;
  };

  static  void* zip_open_file ( void*, const char*, int );
  static  uLong zip_read_file ( void*, void*, void*, uLong );
  static  uLong zip_write_file( void*, void*, const void*, uLong );
  static  long  zip_tell_file ( void*, void* );
  static  long  zip_seek_file ( void*, void*, uLong, int );
  static  int   zip_close_file( void*, void* );
  static  int   zip_error_file( void*, void* );
  static  auto  zip_file_funcs() -> zlib_filefunc_def_s;

  void  Read( ReadTextFn  fnTxt, const DelphiX::Slice<const char>& input, const std::vector<std::string>& names )
  {
    auto  fns = zip_file_funcs();
    auto  buf = ByteView( input.data(), input.size() );
    auto  unz = unzOpen2( (const char*)(const mtc::IByteBuffer*)buf, &fns );

    if ( fnTxt == nullptr )
      throw std::invalid_argument( "undefined minizip::Read( fRead... ) parameter" );

    if ( input.empty() )
      throw std::invalid_argument( "empty minizip::Read( ...input ... ) parameter" );

    if ( unz == nullptr )
      return fnTxt( input, names );

    if ( unzGoToFirstFile( unz ) == UNZ_OK ) do
    {
      unz_file_info fiinfo;
      char          szname[0x400];

      if ( unzGetCurrentFileInfo( unz, &fiinfo, szname, sizeof(szname), nullptr, 0, nullptr, 0 ) == UNZ_OK )
      {
        if ( unzOpenCurrentFile( unz ) == UNZ_OK )
        {
          std::vector<char> zipbuf;
          char              buffer[0x400];
          long              cbread;

          while ( (cbread = unzReadCurrentFile( unz, buffer, sizeof(buffer) )) > 0 )
            zipbuf.insert( zipbuf.end(), buffer, buffer + cbread );

          unzCloseCurrentFile( unz );

          if ( !zipbuf.empty() )
          {
            auto  anames = names;
              anames.push_back( szname );
            Read( fnTxt, zipbuf, anames );
          }
        }
      }
    } while ( unzGoToNextFile( unz ) == UNZ_OK );
    unzClose( unz );
  }

  // zip interface functions

  static  void* zip_open_file( void*, const char* name, int )
  {
    return new STM{ mtc::api<const mtc::IByteBuffer>( (mtc::IByteBuffer*)name ) };
  }

  static  uLong zip_read_file( void*, void* stm, void* ptr, uLong len )
  {
    auto  mem = static_cast<STM*>( stm );
    auto  cbr = std::min( mem->data->GetLen() - mem->cpos, len );

    if ( cbr > 0 )
      memcpy( ptr, mem->data->GetPtr() + mem->cpos, cbr );
    return mem->nerr = 0, mem->cpos += cbr, cbr;
  }

  static  uLong zip_write_file( void*, void* stm, const void*, uLong )
  {
    auto  mem = static_cast<STM*>( stm );

    return mem->nerr = EINVAL, -1;
  }

  static  long  zip_tell_file( void*, void* stm )
  {
    auto  mem = static_cast<STM*>( stm );

    return mem->nerr = 0, mem->cpos;
  }

  static  long  zip_seek_file( void*, void* stm, uLong pos32, int origin )
  {
    auto  mem = static_cast<STM*>( stm );

    switch ( origin )
    {
      case SEEK_SET:
        if ( pos32 > mem->data->GetLen() )
          return mem->nerr = ERANGE, -1;
        return mem->cpos = pos32, 0;
      case SEEK_CUR:
        return 0;
      case SEEK_END:
        return mem->cpos = mem->data->GetLen(), 0;
      default:
        return mem->nerr = EINVAL, -1;
    }
  }

  static  int   zip_close_file( void*, void* stm )
  {
    return delete (STM*)stm, 0;
  }

  static  int   zip_error_file( void*, void* stm )
  {
    auto  mem = static_cast<STM*>( stm );
    auto  err = mem->nerr;
    return mem->nerr = 0, err;
  }

  static  auto  zip_file_funcs() -> zlib_filefunc_def_s
  {
    zlib_filefunc_def_s   fn;

    fn.zopen_file = zip_open_file;
    fn.zread_file = zip_read_file;
    fn.zwrite_file = zip_write_file;
    fn.ztell_file = zip_tell_file;
    fn.zseek_file = zip_seek_file;
    fn.zclose_file = zip_close_file;
    fn.zerror_file = zip_error_file;

    fn.opaque = nullptr;

    return fn;
  }

}}
