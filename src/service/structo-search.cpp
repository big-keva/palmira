# include "../../service/structo-search.hpp"
# include "../toolset/object-zmap.hpp"
# include "../reports.hpp"
# include "../toolset.hpp"
# include "collect.hpp"
# include "structo/storage/posix-fs.hpp"
# include "structo/indexer/layered-contents.hpp"
# include "structo/enquote/quotations.hpp"
# include "structo/context/processor.hpp"
# include "structo/context/x-contents.hpp"
# include "structo/context/pack-format.hpp"
# include "structo/context/pack-images.hpp"
# include "structo/queries/builder.hpp"
# include "DeliriX/DOM-load.hpp"
# include <zlib.h>

namespace palmira {

  class StructoSearch final: public IService
  {
    std::atomic_long  refCount = 0;

    class Timing;

    long  Attach() override;
    long  Detach() override;

  protected:
    auto  Insert( const InsertArgs&, NotifyFn ) -> mtc::api<IPending> override;
    auto  Update( const UpdateArgs&, NotifyFn ) -> mtc::api<IPending> override;
    auto  Remove( const RemoveArgs&, NotifyFn ) -> mtc::api<IPending> override;
    auto  Search( const SearchArgs&, NotifyFn ) -> mtc::api<IPending> override;
    void  Commit() override;

    template <size_t N>
    auto  DumpMetadata( const mtc::zmap&, char (&)[N] ) const -> std::pair<std::shared_ptr<char[]>, size_t>;
    auto  LoadMetadata( const mtc::api<const mtc::IByteBuffer>& ) const -> mtc::zmap;

  public:
    StructoSearch( mtc::api<IContentsIndex>, const context::Processor&,
      const context::FieldManager&, FnContents = context::GetMiniContents );

  private:
    auto  get_string( const mtc::zval& ) const -> mtc::charstr;

  protected:
    mtc::api<IContentsIndex>  ctxIndex;
    context::Processor        lingProc;
    context::FieldManager     fieldMan;
    FnContents                contents;
    bool                      modified = false;
  };

  class StructoSearch::Timing
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

  class StructoService::data
  {
  public:
    mtc::api<IContentsIndex>  ctxIndex;
    context::Processor        langProc;
    FnContents                contents = context::GetMiniContents;
    context::FieldManager     fieldMan;
  };

  // StructoSearch implementation

  StructoSearch::StructoSearch(
    mtc::api<IContentsIndex>      ix,
    const context::Processor&     lp,
    const context::FieldManager&  fm,
    FnContents                    cs ): ctxIndex( ix ), lingProc( lp ), contents( cs )
  {
    auto  fdsEnt = ctxIndex->GetEntity( { "##__index_mappings__##", 22 } );
    auto  extras = mtc::api<const mtc::IByteBuffer>();

    if ( fdsEnt != nullptr && (extras = fdsEnt->GetExtra()) != nullptr )
    {
      auto  indata = mtc::array_zmap();

      if ( ::FetchFrom( mtc::sourcebuf( extras->GetPtr(), extras->GetLen() ).ptr(), indata ) == nullptr )
        throw std::invalid_argument( "failed to deserialize fields configuration @" __FILE__ ":" LINE_STRING );

      fieldMan = JoinFields( context::LoadFields( indata ), fm );
    }
      else
    fieldMan = fm;
  }

  long  StructoSearch::Attach()
  {
    return ++refCount;
  }

  long  StructoSearch::Detach()
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

  auto  StructoSearch::Insert( const InsertArgs& insert, NotifyFn notify ) -> mtc::api<IPending>
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
      getdoc = ctxIndex->SetEntity( insert.objectId,
        contents( pwBody->GetLemmas(), pwBody->GetMarkup(), fieldMan ),
        { serial.first.get(), serial.second },
        { enBeef.data(), enBeef.size() } );

      return modified = true, Immediate( UpdateReport{ 0, "OK", {
        { "metadata", LoadMetadata( getdoc->GetExtra() ) } } }, notify );
    }
    catch ( const std::bad_function_call& xp )        {  return Immediate( UpdateReport{ EFAULT, xp.what() }, notify );  }
    catch ( const std::invalid_argument& xp )         {  return Immediate( UpdateReport{ EINVAL, xp.what() }, notify );  }
    catch ( const DeliriX::load_as::ParseError& xp )  {  return Immediate( UpdateReport{ EINVAL, xp.what() }, notify );  }
  }

  auto  StructoSearch::Update( const UpdateArgs& update, NotifyFn notify ) -> mtc::api<IPending>
  {
    try
    {
      char  buffer[0x400];
      auto  serial = DumpMetadata( update.metadata, buffer );
      auto  getdoc = mtc::api<const IEntity>();

      if ( (getdoc = ctxIndex->SetExtras( update.objectId, { serial.first.get(), serial.second } )) == nullptr )
        return Immediate( UpdateReport{ ENOENT, "document not found" }, notify );

      return modified = true, Immediate( UpdateReport{ 0, "OK", {
        { "metadata", LoadMetadata( getdoc->GetExtra() ) } } }, notify );
    }
    catch ( const std::invalid_argument& xp )         {  return Immediate( UpdateReport{ EINVAL, xp.what() }, notify );  }
    catch ( const DeliriX::load_as::ParseError& xp )  {  return Immediate( UpdateReport{ EINVAL, xp.what() }, notify );  }
  }

  auto  StructoSearch::Remove( const RemoveArgs& remove, NotifyFn notify ) -> mtc::api<IPending>
  {
    try
    {
      if ( ctxIndex->DelEntity( remove.objectId ) )
        return modified = true, Immediate( UpdateReport( 0, "OK" ), notify );
      return Immediate( UpdateReport( ENOENT, "document not found" ), notify );
    }
    catch ( const std::invalid_argument& xp )         {  return Immediate( UpdateReport{ EINVAL, xp.what() }, notify );  }
    catch ( const DeliriX::load_as::ParseError& xp )  {  return Immediate( UpdateReport{ EINVAL, xp.what() }, notify );  }
  }

  auto  StructoSearch::Search( const SearchArgs& search, NotifyFn notify ) -> mtc::api<IPending>
  {
    Timing  timing;

    if ( search.query.get_type() == mtc::zval::z_zmap && search.query.get_zmap()->get( "id" ) != nullptr )
    {
      auto  ent_id = get_string( *search.query.get_zmap()->get( "id" ) );
      auto  getent = mtc::api<const IEntity>();

      if ( ent_id.empty() )
        return Immediate( timing( SearchReport( EINVAL, "invalid 'id' data type, string expected" ) ), notify );

      if ( (getent = ctxIndex->GetEntity( ent_id )) != nullptr )
      {
        return Immediate( timing( SearchReport( 0, "OK", {
          { "first", uint32_t(1) },
          { "found", uint32_t(1) },
          { "items", mtc::array_zmap{
            { { "id", { getent->GetId().data(), getent->GetId().size() } } } } } } ) ), notify );
      }
      return Immediate( timing( SearchReport( ENOENT, "document not found" ) ), notify );
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
            const char* data;
            auto        mkup = mtc::span<const char>();
            auto        buff = std::vector<char>();
            auto        text = mtc::span<const char>();
            uint32_t    size;

          // check for formats
            if ( (data = mtc::zmap::serial::find( bundle->GetPtr(), "ft" )) != nullptr )
            {
              if ( *data++ != mtc::zval::z_array_char )
                throw std::runtime_error( "invalid object package format" );
              data = ::FetchFrom( data, size );
                mkup = { data, size };
            }
          // check compressed image
            if ( (data = mtc::zmap::serial::find( bundle->GetPtr(), "ip" )) != nullptr )
            {
              if ( *data++ != mtc::zval::z_array_char )
                throw std::runtime_error( "invalid object package format" );
              data = ::FetchFrom( data, size );
                text = (buff = Unpack( { data, size } ));
            }
              else
          // check uncompressed image
            if ( (data = mtc::zmap::serial::find( bundle->GetPtr(), "im" )) != nullptr )
            {
              if ( *data++ != mtc::zval::z_array_char )
                throw std::runtime_error( "invalid object package format" );
              data = ::FetchFrom( data, size );
                text = { data, size };
            }

            if ( !text.empty() )
              enquote::QuoteMachine( fieldMan ).Structured()( ZmapAsText( output ), text, mkup, abstr );
          }
          return output;
        };

        mtc::ThreadPool actors;

      auto  collect = collect::Documents()
        .SetFirst( search.order.get_int32( "first", 1 ) )
        .SetCount( search.order.get_int32( "count", 10 ) )
        .SetAsync( &actors )
        .SetQuote( quotate ).Create();
      auto  request = queries::BuildRichQuery( search.query, search.terms, ctxIndex, lingProc, fieldMan );

      if ( request == nullptr )
        return Immediate( timing( SearchReport( 0, "OK", { { "found", 0U } } ) ), notify );

      collect->Search( request );

      return Immediate( timing( SearchReport( collect->Finish( ctxIndex ) ) ), notify );
    }
  }

  void  StructoSearch::Commit()
  {
    if ( modified )
    {
      auto  fields = SaveFields( fieldMan );
      auto  serial = std::vector<char>( GetBufLen( fields ) );

      ::Serialize( serial.data(), fields );

      ctxIndex->SetEntity( { "##__index_mappings__##", 22 }, {}, { serial.data(), serial.size() } );

      modified = false;
    }
    ctxIndex->Commit();
  }

  template <size_t N>
  auto  StructoSearch::DumpMetadata( const mtc::zmap& zmap, char (&buff)[N] ) const -> std::pair<std::shared_ptr<char[]>, size_t>
  {
    size_t  buflen = zmap.GetBufLen();
    auto    bufvec = std::shared_ptr<char[]>( std::shared_ptr<char[]>(), buff );

    if ( buflen > N )
      bufvec = std::shared_ptr<char[]>( new char[buflen] );

    zmap.Serialize( bufvec.get() );

    return { bufvec, buflen };
  }

  auto  StructoSearch::LoadMetadata( const mtc::api<const mtc::IByteBuffer>& dump ) const -> mtc::zmap
  {
    auto  zmap = mtc::zmap();

    if ( dump != nullptr && dump->GetLen() != 0 )
      zmap.FetchFrom( mtc::sourcebuf( dump->GetPtr(), dump->GetLen() ).ptr() );

    return zmap;
  }

  auto  StructoSearch::get_string( const mtc::zval& zv ) const -> mtc::charstr
  {
    return
      zv.get_type() == mtc::zval::z_charstr ? *zv.get_charstr() :
      zv.get_type() == mtc::zval::z_widestr ? codepages::widetombcs( codepages::codepage_utf8, *zv.get_widestr() ) : "";
  }

  // StructoService implementation

  auto  StructoService::Set( FnContents contents ) -> StructoService&
  {
    if ( init == nullptr )
      init = std::make_shared<data>();
    init->contents = contents;
      return *this;
  }

  auto  StructoService::Set( mtc::api<IContentsIndex> index ) -> StructoService&
  {
    if ( init == nullptr )
      init = std::make_shared<data>();
    init->ctxIndex = index;
      return *this;
  }

  auto  StructoService::Set( context::Processor&& proc ) -> StructoService&
  {
    if ( init == nullptr )
      init = std::make_shared<data>();
    init->langProc = std::move( proc );
      return *this;
  }

  auto  StructoService::Set( const context::Processor& proc ) -> StructoService&
  {
    if ( init == nullptr )
      init = std::make_shared<data>();
    init->langProc = proc;
      return *this;
  }

  auto  StructoService::Set( const context::FieldManager& fds ) -> StructoService&
  {
    if ( init == nullptr )
      init = std::make_shared<data>();
    init->fieldMan = fds;
      return *this;
  }

  auto  StructoService::Create() -> mtc::api<IService>
  {
    if ( init->contents == nullptr )
      throw std::invalid_argument( "invalid (null) contents creation callback" );
    if ( init->ctxIndex == nullptr )
      throw std::invalid_argument( "invalid (null) contents index" );
    return new StructoSearch(
      init->ctxIndex,
      init->langProc,
      init->fieldMan,
      init->contents );
  }

}
