# include "../../service/delphi-search.hpp"
# include "../statusReport.hpp"
# include "collect.hpp"
# include "DelphiX/storage/posix-fs.hpp"
# include "DelphiX/indexer/layered-contents.hpp"
# include "DelphiX/context/processor.hpp"
# include "DelphiX/context/x-contents.hpp"
# include "DelphiX/context/pack-format.hpp"
# include "DelphiX/context/pack-images.hpp"
# include "DelphiX/queries/builder.hpp"
# include "DeliriX/DOM-load.hpp"

# include <zlib.h>
#include <DelphiX/enquote/quotations.hpp>
#include <src/object-zmap.hpp>

namespace palmira {

  class DelphiSearch final: public IService
  {
    std::atomic_long  refCount = 0;

    class Timing;

    long  Attach() override;
    long  Detach() override;

  protected:
    auto  Insert( const InsertArgs& ) -> mtc::zmap override;
    auto  Update( const UpdateArgs& ) -> mtc::zmap override;
    auto  Remove( const RemoveArgs& ) -> mtc::zmap override;
    auto  Search( const SearchArgs& ) -> mtc::zmap override;
    void  Commit() override;

    template <size_t N>
    auto  DumpMetadata( const mtc::zmap&, char (&)[N] ) const -> std::pair<std::shared_ptr<char[]>, size_t>;
    auto  LoadMetadata( const mtc::api<const mtc::IByteBuffer>& ) const -> mtc::zmap;

  public:
    DelphiSearch( mtc::api<IContentsIndex>, context::Processor&&, FnContents = context::GetMiniContents );
    DelphiSearch( mtc::api<IContentsIndex>, const context::Processor&, FnContents = context::GetMiniContents );

  private:
    auto  get_string( const mtc::zval& ) const -> mtc::charstr;

  protected:
    mtc::api<IContentsIndex>  ctxIndex;
    context::Processor        lingProc;
    context::FieldManager     fieldMan;
    FnContents                contents;
    bool                      modified = false;
  };

  class DelphiSearch::Timing
  {
    using clock_type = std::chrono::steady_clock;
    using time_point = clock_type::time_point;

  public:
    auto  elapced() const -> unsigned
    {
      return std::chrono::duration_cast<std::chrono::milliseconds>( clock_type::now() - begin ).count();
    }
    auto  operator()( const mtc::zmap& to ) const -> mtc::zmap
    {
      return mtc::zmap( to, {
        { "timer", mtc::zmap{
          { "elapsed", elapced() } } } } );
    };

  protected:
    time_point  begin = clock_type::now();
  };

  class DelphiXService::data
  {
  public:
    mtc::api<IContentsIndex>  ctxIndex;
    context::Processor        langProc;
    FnContents                contents = context::GetMiniContents;
  };

  // DelphiSearch implementation

  DelphiSearch::DelphiSearch( mtc::api<IContentsIndex> ix, context::Processor&& lp, FnContents cs ):
    ctxIndex( ix ),
    lingProc( std::move( lp ) ),
    contents( cs )
  {
  }

  DelphiSearch::DelphiSearch( mtc::api<IContentsIndex> ix, const context::Processor& lp, FnContents cs ):
    ctxIndex( ix ),
    lingProc( lp ),
    contents( cs )
  {
  }

  long  DelphiSearch::Attach()
  {
    return ++refCount;
  }

  long  DelphiSearch::Detach()
  {
    auto  rCount = --refCount;

    if ( rCount == 0 )
    {
      Commit();
      delete this;
    }
    return rCount;
  }

  auto  ZipBuf( const mtc::span<const char>& src ) -> std::vector<char>
  {
    auto  compressed_size = compressBound( src.size() );
    auto  compressed_data = std::vector<char>( compressed_size );
    auto  compress_result = compress( (Bytef*)compressed_data.data(), &compressed_size,
      (const Bytef*)src.data(), src.size() );

    if ( compress_result != Z_OK )
      throw std::range_error( "compression failed" );

    if ( compressed_size > src.size() - 100 )
      throw std::range_error( "ignore compression" );

    return compressed_data.resize( compressed_size ), compressed_data;
  }

  auto  Unpack( const mtc::span<const char>& src ) -> std::vector<char>
  {
    std::vector<char> unpack( src.size() * 2 );
    uLongf            length;
    int               nerror;

    while ( (nerror = uncompress( (Bytef*)unpack.data(), &(length = unpack.size()),
      (const Bytef*)src.data(), src.size() )) == Z_BUF_ERROR )
        unpack.resize( unpack.size() * 3 / 2 );

    if ( nerror == Z_OK ) unpack.resize( length );
      else unpack.clear();

    return unpack;
  }

  auto  DelphiSearch::Insert( const InsertArgs& insert ) -> mtc::zmap
  {
    try
    {
      char  buffer[0x400];
      auto  serial = DumpMetadata( insert.metadata, buffer );
      auto  mArena = mtc::Arena();
      auto  pwBody = mArena.Create<context::BaseImage<mtc::Arena::allocator<char>>>();
      auto  pwText = &insert.textview;
      auto  utfdoc = DeliriX::Text();
      auto  getdoc = mtc::api<const IEntity>();
      auto  enBeef = std::vector<char>();

    // check if document is utf16-encoded; recode document if not so
      if ( !IsEncoded( insert.textview, unsigned(-1) ) )
      {
        CopyUtf16( &utfdoc, insert.textview );
        pwText = &utfdoc;
      }

    // create document image
      lingProc.WordBreak( *pwBody, *pwText );
      lingProc.SetMarkup( *pwBody, *pwText );
      lingProc.Lemmatize( *pwBody );

    // create quotation image
      if ( true )
      {
        auto  quoter = mtc::zmap{
          { "ft", context::formats::Pack( pwBody->GetMarkup(), fieldMan ) } };
        auto  limage = context::imaging::Pack( pwBody->GetTokens() );

      // check if the image is big enough to compress it
        try
        {  quoter.set_array_char( "ip", std::move( ZipBuf( limage ) ) );  }
        catch ( const std::range_error& )
        {  quoter.set_array_char( "im", std::move( limage ) ); }

        enBeef.resize( quoter.GetBufLen() );
        quoter.Serialize( enBeef.data() );
      }

    // create text contents and index document
      getdoc = ctxIndex->SetEntity( insert.objectId, contents( pwBody->GetLemmas(), pwBody->GetMarkup(), fieldMan ).ptr(),
        { serial.first.get(), serial.second }, { enBeef.data(), enBeef.size() } );

      return UpdateReport{ 0, "OK", { { "metadata", LoadMetadata( getdoc->GetExtra() ) } } };
    }
    catch ( const std::bad_function_call& xp )        {  return UpdateReport{ EFAULT, xp.what() };  }
    catch ( const std::invalid_argument& xp )         {  return UpdateReport{ EINVAL, xp.what() };  }
    catch ( const DeliriX::load_as::ParseError& xp )  {  return UpdateReport{ EINVAL, xp.what() };  }
  }

  auto  DelphiSearch::Update( const UpdateArgs& update ) -> mtc::zmap
  {
    try
    {
      char  buffer[0x400];
      auto  serial = DumpMetadata( update.metadata, buffer );
      auto  getdoc = mtc::api<const IEntity>();

      if ( (getdoc = ctxIndex->SetExtras( update.objectId, { serial.first.get(), serial.second } )) == nullptr )
        return UpdateReport{ ENOENT, "document not found" };

      return UpdateReport{ 0, "OK", { { "metadata", LoadMetadata( getdoc->GetExtra() ) } } };
    }
    catch ( const std::invalid_argument& xp )         {  return UpdateReport{ EINVAL, xp.what() };  }
    catch ( const DeliriX::load_as::ParseError& xp )  {  return UpdateReport{ EINVAL, xp.what() };  }
  }

  auto  DelphiSearch::Remove( const RemoveArgs& remove ) -> mtc::zmap
  {
    try
    {
      if ( ctxIndex->DelEntity( remove.objectId ) )
        return UpdateReport( 0, "OK" );

      return UpdateReport( ENOENT, "document not found" );
    }
    catch ( const std::invalid_argument& xp )         {  return UpdateReport{ EINVAL, xp.what() };  }
    catch ( const DeliriX::load_as::ParseError& xp )  {  return UpdateReport{ EINVAL, xp.what() };  }
  }

  auto  DelphiSearch::Search( const SearchArgs& search ) -> mtc::zmap
  {
    Timing  timing;

    if ( search.query.get_type() == mtc::zval::z_zmap && search.query.get_zmap()->get( "id" ) != nullptr )
    {
      auto  ent_id = get_string( *search.query.get_zmap()->get( "id" ) );
      auto  getent = mtc::api<const IEntity>();

      if ( ent_id.empty() )
        return timing( SearchReport( EINVAL, "invalid 'id' data type, string expected" ) );

      if ( (getent = ctxIndex->GetEntity( ent_id )) != nullptr )
      {
        return timing( SearchReport( 0, "OK", {
          { "first", uint32_t(1) },
          { "found", uint32_t(1) },
          { "items", mtc::array_zmap{
            { { "id", { getent->GetId().data(), getent->GetId().size() } } } } } } ) );
      }
      return timing( SearchReport( ENOENT, "document not found" ) );
    }
      else
    {
      auto  quotate = [this]( uint32_t id, const queries::Abstract& abstr ) -> mtc::array_zval
        {
          auto  entity = ctxIndex->GetEntity( id );
          auto  bundle = entity != nullptr ? entity->GetBundle() : nullptr;
          auto  output = mtc::array_zval();

          if ( bundle != nullptr )
          {
            auto  dump = mtc::zmap( mtc::zmap::dump( bundle->GetPtr() ) );
            auto  mkup = dump.get_array_char( "ft" );
            auto  buff = std::vector<char>();
            auto  text = mtc::span<const char>();

          // if image is packed, unpack it, else use plain image
            if ( dump.get_array_char( "im" ) != nullptr ) text = *dump.get_array_char( "im" );
              else
            if ( dump.get_array_char( "ip" ) != nullptr ) text = buff = Unpack( *dump.get_array_char( "ip" ) );

            if ( !text.empty() )
              enquote::QuoteMachine( fieldMan ).Structured()( ZmapAsText( output ), text, *mkup, abstr );
          }
          return output;
        };
      auto  collect = collect::Documents()
        .SetFirst( search.order.get_word32( "first", 1 ) )
        .SetCount( search.order.get_word32( "count", 10 ) )
        .SetQuote( quotate ).Create();
      auto  request = queries::BuildRichQuery( search.query, search.terms, ctxIndex, lingProc, fieldMan );

      if ( request == nullptr )
        return timing( SearchReport( 0, "OK", { { "found", 0U } } ) );

      collect->Search( request );

      return timing( SearchReport( collect->Finish( ctxIndex ) ) );
    }
  }

  void  DelphiSearch::Commit()
  {
    if ( modified )
    {
      auto  fields = SaveFields( fieldMan );
      auto  serial = std::vector<char>( GetBufLen( fields ) );

      ::Serialize( serial.data(), fields.data(), fields.size() );

      ctxIndex->SetEntity( { "\0\0__index_mappings__\0\0", 22 }, {}, { serial.data(), serial.size() } );
    }
    ctxIndex->Commit();
  }

  template <size_t N>
  auto  DelphiSearch::DumpMetadata( const mtc::zmap& zmap, char (&buff)[N] ) const -> std::pair<std::shared_ptr<char[]>, size_t>
  {
    size_t  buflen = zmap.GetBufLen();
    auto    bufvec = std::shared_ptr<char[]>( std::shared_ptr<char[]>(), buff );

    if ( buflen > N )
      bufvec = std::shared_ptr<char[]>( new char[buflen] );

    zmap.Serialize( bufvec.get() );

    return { bufvec, buflen };
  }

  auto  DelphiSearch::LoadMetadata( const mtc::api<const mtc::IByteBuffer>& dump ) const -> mtc::zmap
  {
    auto  zmap = mtc::zmap();

    if ( dump != nullptr && dump->GetLen() != 0 )
      zmap.FetchFrom( mtc::sourcebuf( dump->GetPtr(), dump->GetLen() ).ptr() );

    return zmap;
  }

  auto  DelphiSearch::get_string( const mtc::zval& zv ) const -> mtc::charstr
  {
    return
      zv.get_type() == mtc::zval::z_charstr ? *zv.get_charstr() :
      zv.get_type() == mtc::zval::z_widestr ? codepages::widetombcs( codepages::codepage_utf8, *zv.get_widestr() ) : "";
  }

  // DelphiXService implementation

  auto  DelphiXService::Set( FnContents contents ) -> DelphiXService&
  {
    if ( init == nullptr )
      init = std::make_shared<data>();
    init->contents = contents;
      return *this;
  }

  auto  DelphiXService::Set( mtc::api<IContentsIndex> index ) -> DelphiXService&
  {
    if ( init == nullptr )
      init = std::make_shared<data>();
    init->ctxIndex = index;
      return *this;
  }

  auto  DelphiXService::Set( context::Processor&& proc ) -> DelphiXService&
  {
    if ( init == nullptr )
      init = std::make_shared<data>();
    init->langProc = std::move( proc );
      return *this;
  }

  auto  DelphiXService::Set( const context::Processor& proc ) -> DelphiXService&
  {
    if ( init == nullptr )
      init = std::make_shared<data>();
    init->langProc = proc;
      return *this;
  }

  auto  DelphiXService::Create() -> mtc::api<IService>
  {
    if ( init->contents == nullptr )
      throw std::invalid_argument( "invalid (null) contents creation callback" );
    if ( init->ctxIndex == nullptr )
      throw std::invalid_argument( "invalid (null) contents index" );
    return new DelphiSearch( init->ctxIndex, init->langProc, init->contents );
  }

}
