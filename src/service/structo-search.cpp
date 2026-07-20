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
# include <mtc/utf.hpp>
# include <fnmatch.h>
# include <zlib.h>
#include <structo/rankers.hpp>

namespace palmira {

  auto  ZipBuf( const mtc::span<const char>& src ) -> std::vector<char>;
  auto  Unpack( const mtc::span<const char>& src ) -> std::vector<char>;

  class QuoteOpts
  {
    class FieldAccessor;
    class IncludeFields;
    class ExcludeFields;
    class FnMatchFields;

    struct Bundle: protected std::vector<char>, protected mtc::api<const mtc::IByteBuffer>
    {
      mtc::span<const char> mkup;
      mtc::span<const char> body;

      Bundle( const api& src ): api( src )  {}
      Bundle() = default;

      auto  packed() -> std::vector<char>&  {  return *this;  }
      bool  empty() const {  return body.empty();  }
    };

  public:
    enum class Type: unsigned
    {
      Absent  = 0,
      Scheme  = 1,
      Struct  = 3,
      Source  = 4
    };

    struct include_t {};
    struct exclude_t {};
    struct fnmatch_t {};

    static constexpr include_t include = {};
    static constexpr exclude_t exclude = {};
    static constexpr fnmatch_t fnmatch = {};

    QuoteOpts( const FieldHandler&, Type );
    QuoteOpts( const FieldHandler&, Type, const include_t&, const mtc::zval& );
    QuoteOpts( const FieldHandler&, Type, const exclude_t&, const mtc::zval& );
    QuoteOpts( const FieldHandler&, Type, const fnmatch_t&, const mtc::zval& );

    auto  GetQuoteFn( mtc::api<IContentsIndex> ) const -> collect::QuotesFn;

    static  auto  LoadBundle( const mtc::api<const mtc::IByteBuffer>& ) -> Bundle;
    static  auto  Str2Scheme( std::string_view ) -> Type;
    static  auto  Scheme2Str( Type ) -> const char*;

  protected:
    Type                            q_mode;
    std::shared_ptr<FieldAccessor>  filter;
    const FieldHandler*             fields;

  };

  class QuoteOpts::FieldAccessor: public FieldHandler
  {
  public:
    using FieldHandler::Get;

    FieldAccessor( const FieldHandler* pfd ): fields( pfd ) {}

    auto  Add( const std::string_view& ) -> FieldOptions* override              {  throw std::logic_error( "not implemented" );  }
    auto  Get( const std::string_view& ) const -> const FieldOptions* override  {  throw std::logic_error( "not implemented" );  }

  protected:
    const FieldHandler*     fields;
    std::vector<uint64_t>   filter;

  };

  class QuoteOpts::IncludeFields: public FieldAccessor
  {
    using FieldAccessor::Get;

  public:
    IncludeFields( const FieldHandler* fds, const mtc::array_charstr& inc ): FieldAccessor( fds )
      {
        for ( auto& next: inc )
          if ( auto pf = fds->Get( next ); pf != nullptr )
            mtc::bitset_set( filter, pf->id );
      }
    auto  Get( uint32_t id ) const -> const FieldOptions* override
      {
        return mtc::bitset_get( filter, id ) ? fields->Get( id ) : nullptr;
      }
  };

  class QuoteOpts::ExcludeFields: public FieldAccessor
  {
    using FieldAccessor::Get;

  public:
    ExcludeFields( const FieldHandler* fds, const mtc::array_charstr& exc ): FieldAccessor( fds )
      {
        for ( auto& next: exc )
          if ( auto pf = fds->Get( next ); pf != nullptr )
            mtc::bitset_set( filter, pf->id );
      }
    auto  Get( uint32_t id ) const -> const FieldOptions* override
      {
        return !mtc::bitset_get( filter, id ) ? fields->Get( id ) : nullptr;
      }
  };

  class QuoteOpts::FnMatchFields: public FieldAccessor
  {
    using FieldAccessor::Get;

  public:
    FnMatchFields( const FieldHandler* fds, const mtc::array_charstr& matches ): FieldAccessor( fds )
      {
        const FieldOptions* pf;

        for ( uint32_t id = 0; id == 0 || (pf = fields->Get( id )) != nullptr; ++id )
          if ( pf != nullptr )
            for ( auto& match: matches )
              if ( ::fnmatch( match.c_str(), pf->name.data(), 0 ) == 0 )
              {
                mtc::bitset_set( filter, pf->id );
                break;
              }
      }
    auto  Get( uint32_t id ) const -> const FieldOptions* override
      {
        return mtc::bitset_get( filter, id ) ? fields->Get( id ) : nullptr;
      }
  };

  QuoteOpts::QuoteOpts( const FieldHandler& fds, Type mode ): q_mode( mode ),
    fields( mode != Type::Absent ? &fds : nullptr )
  {
  }

  QuoteOpts::QuoteOpts( const FieldHandler& fds, Type mode, const include_t&, const mtc::zval& inc ): q_mode( mode )
  {
    switch ( inc.get_type() )
    {
      case mtc::zval::z_charstr:
        fields = (filter = std::shared_ptr<FieldAccessor>( new IncludeFields( &fds, { *inc.get_charstr() } ) )).get();
        break;
      case mtc::zval::z_array_charstr:
        fields = (filter = std::shared_ptr<FieldAccessor>( new IncludeFields( &fds, *inc.get_array_charstr() ))).get();
        break;
      default:
        throw std::invalid_argument( "'include' quotation has to contain charstr or array_charstr @" __FILE__ ":" LINE_STRING );
    }
  }

  QuoteOpts::QuoteOpts( const FieldHandler& fds, Type mode, const exclude_t&, const mtc::zval& exc ): q_mode( mode )
  {
    switch ( exc.get_type() )
    {
      case mtc::zval::z_charstr:
        fields = (filter = std::shared_ptr<FieldAccessor>( new ExcludeFields( &fds, { *exc.get_charstr() } ) )).get();
        break;
      case mtc::zval::z_array_charstr:
        fields = (filter = std::shared_ptr<FieldAccessor>( new ExcludeFields( &fds, *exc.get_array_charstr() ))).get();
        break;
      default:
        throw std::invalid_argument( "'exclude' quotation has to contain charstr or array_charstr @" __FILE__ ":" LINE_STRING );
    }
  }

  QuoteOpts::QuoteOpts( const FieldHandler& fds, Type mode, const fnmatch_t&, const mtc::zval& fnm ): q_mode( mode )
  {
    switch ( fnm.get_type() )
    {
      case mtc::zval::z_charstr:
        fields = (filter = std::shared_ptr<FieldAccessor>( new FnMatchFields( &fds, { *fnm.get_charstr() } ) )).get();
        break;
      case mtc::zval::z_array_charstr:
        fields = (filter = std::shared_ptr<FieldAccessor>( new FnMatchFields( &fds, *fnm.get_array_charstr() ))).get();
        break;
      default:
        throw std::invalid_argument( "'fnmatch' quotation has to contain charstr or array_charstr @" __FILE__ ":" LINE_STRING );
    }
  }

  auto  QuoteOpts::GetQuoteFn( mtc::api<IContentsIndex> ctxIndex ) const -> collect::QuotesFn
  {
    switch ( q_mode )
    {
      case Type::Absent:
        return []( uint32_t, const queries::Abstract& ) {  return mtc::array_zval();  };
      case Type::Scheme:
        return []( uint32_t, const queries::Abstract& /*abstr*/ ) -> mtc::array_zval
        {
          return {};
        };
      case Type::Source:
        return [ctxIndex, fieldMan = filter, fieldPtr = fields]( uint32_t id, const queries::Abstract& abstr )
        {
          auto  entity = ctxIndex->GetEntity( id );
          auto  bundle = entity != nullptr ? LoadBundle( entity->GetBundle() ) : Bundle();
          auto  output = mtc::array_zval();

          if ( !bundle.empty() )
            enquote::QuoteMachine( *fieldPtr ).TextSource()( ZmapAsText( output ), bundle.body, bundle.mkup, abstr );

          return output;
        };
      case Type::Struct: default:
        return [ctxIndex, fieldMan = filter, fieldPtr = fields]( uint32_t id, const queries::Abstract& abstr )
        {
          auto  entity = ctxIndex->GetEntity( id );
          auto  bundle = entity != nullptr ? LoadBundle( entity->GetBundle() ) : Bundle();
          auto  output = mtc::array_zval();

          if ( !bundle.empty() )
            enquote::QuoteMachine( *fieldPtr ).Structured()( ZmapAsText( output ), bundle.body, bundle.mkup, abstr );

          return output;
        };
    }
  }

  auto  QuoteOpts::LoadBundle( const mtc::api<const mtc::IByteBuffer>& src ) -> Bundle
  {
    Bundle      bundle( src );
    const char* source;
    size_t      srclen;

    if ( src == nullptr || src->GetPtr() == nullptr )
      return bundle;

    // check for formats
    if ( (source = mtc::zmap::serial::find( src->GetPtr(), "ft" )) != nullptr )
    {
      if ( *source++ != mtc::zval::z_array_char )
        throw std::runtime_error( "invalid object package format" );
      source = ::FetchFrom( source, srclen );
      bundle.mkup = { source, srclen };
    }
    // check compressed image
    if ( (source = mtc::zmap::serial::find( src->GetPtr(), "ip" )) != nullptr )
    {
      if ( *source++ != mtc::zval::z_array_char )
        throw std::runtime_error( "invalid object package format" );
      source = ::FetchFrom( source, srclen );
      bundle.body = (bundle.packed() = Unpack( { source, srclen } ));
    }
      else
  // check uncompressed image
    if ( (source = mtc::zmap::serial::find( src->GetPtr(), "im" )) != nullptr )
    {
      if ( *source++ != mtc::zval::z_array_char )
        throw std::runtime_error( "invalid object package format" );
      source = ::FetchFrom( source, srclen );
      bundle.body = { source, srclen };
    }

    return bundle;
  }

  auto  QuoteOpts::Scheme2Str( Type type ) -> const char*
  {
    switch ( type )
    {
      case Type::Absent:  return "absent";
      case Type::Scheme:  return "scheme";
      case Type::Source:  return "source";
      default:            return "struct";
    }
  }

  auto  QuoteOpts::Str2Scheme( std::string_view type ) -> Type
  {
    return
      type == "absent" ? Type::Absent :
      type == "scheme" ? Type::Scheme :
      type == "source" ? Type::Source : Type::Struct;
  }

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

  protected:
    auto  buildRequest( const SearchArgs& ) -> mtc::api<queries::IQuery>;
    auto  getCollector( const SearchArgs& ) -> mtc::api<collect::ICollector>;
    auto  getQuoteOpts( const mtc::zval* ) const -> QuoteOpts;

  protected:
    mtc::api<IContentsIndex>  ctxIndex;
    context::Processor        lingProc;
    context::FieldManager     fieldMan;
    FnContents                contents;
    mtc::ThreadPool           asyncers;
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
        { "timing", mtc::zmap{
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
    Timing  timeout;

    try
      {
        if ( ctxIndex->DelEntity( remove.objectId ) )
          return modified = true, Immediate( timeout( UpdateReport{ 0, "OK" } ), notify );
        return Immediate( timeout( UpdateReport{ ENOENT, "document not found" } ), notify );
      }
    catch ( const std::invalid_argument& xp )
      {  return Immediate( timeout( UpdateReport{ EINVAL, xp.what() } ), notify );  }
    catch ( const DeliriX::load_as::ParseError& xp )
      {  return Immediate( timeout( UpdateReport{ EINVAL, xp.what() } ), notify );  }
  }

  auto  StructoSearch::Search( const SearchArgs& search, NotifyFn notify ) -> mtc::api<IPending>
  {
    Timing  timeout;
    auto    results = SearchReport();
    auto    request = buildRequest( search );
    auto    collect = getCollector( search );

    if ( request == nullptr )
      return Immediate( timeout( SearchReport( 0, "OK", { { "found", 0U } } ) ), notify );

    collect->Search( request );

    return Immediate( timeout( SearchReport( collect->Finish( ctxIndex ) ) ), notify );
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

  class LoaderQuery final: public collect::IQuery
  {
    std::vector<uint32_t>                 docs;
    std::vector<uint32_t>::const_iterator next;
    queries::Abstract                     stub;

  public:
    auto  LastIndex() -> uint32_t override;
    auto  SearchDoc( uint32_t ) -> uint32_t override;
    auto  GetTuples( uint32_t ) -> const queries::Abstract& override;
    auto  Duplicate( const Bounds& = {} ) -> mtc::api<IQuery> override;

    LoaderQuery( mtc::api<IContentsIndex>, const mtc::array_charstr& );

    implement_lifetime_control
  };

  auto  LoaderQuery::LastIndex() -> uint32_t
  {
    return !docs.empty() ? docs.back() : 0;
  }

  auto  LoaderQuery::SearchDoc( uint32_t id ) -> uint32_t
  {
    while ( next != docs.end() && *next < id )
      ++next;
    return next != docs.end() ? *next : uint32_t(-1);
  }

  auto  LoaderQuery::GetTuples( uint32_t ) -> const queries::Abstract&
  {
    return stub;
  }

  auto  LoaderQuery::Duplicate( const Bounds& ) -> mtc::api<IQuery>
  {
    return this;
  }

  LoaderQuery::LoaderQuery( mtc::api<IContentsIndex> idx, const mtc::array_charstr& ids ):
    stub{ queries::Abstract::BM25, 1, {} }
  {
    for ( auto id: ids )
      if ( auto doc = idx->GetEntity( id ); doc != nullptr )
        docs.push_back( doc->GetIndex() );

    std::sort( docs.begin(), docs.end() );
      docs.resize( std::unique( docs.begin(), docs.end() ) - docs.begin() );

    next = docs.begin();
  }

  auto StructoSearch::buildRequest( const SearchArgs& args ) -> mtc::api<queries::IQuery>
  {
  // check for 'get'
    if ( auto zmap = args.query.get_zmap(); zmap != nullptr )
      if ( auto zset = zmap->get_array_charstr( "get" ); zset != nullptr )
        return zset->empty() ? nullptr : new LoaderQuery( ctxIndex, *zset );

    return queries::BuildRichQuery( args.query, args.terms,
      ctxIndex,
      lingProc,
      fieldMan );
  }

  auto  StructoSearch::getCollector( const SearchArgs& args ) -> mtc::api<collect::ICollector>
  {
    auto  stMode = args.order.get_charstr( "order", "score" );     // по умолчанию по релевантности
    auto  quoter = getQuoteOpts( args.order.get( "quote" ) );

    if ( stMode == "score" )
    {
      return collect::Documents()
        .SetFirst( args.order.get_int32( "first", 1 ) )
        .SetCount( args.order.get_int32( "count", 10 ) )
        .SetAsync( asyncers )
        .SetQuote( quoter.GetQuoteFn( ctxIndex ) ).Create();
    }
    throw std::invalid_argument( "Unknown ordering '" + stMode + "' @" __FILE__ + ":" + LINE_STRING );
  }

  auto  StructoSearch::getQuoteOpts( const mtc::zval* quote ) const -> QuoteOpts
  {
    if ( quote == nullptr )
      return QuoteOpts( fieldMan, QuoteOpts::Type::Struct );

    if ( auto as_map = quote->get_zmap(); as_map != nullptr )
    {
      auto  quoType = QuoteOpts::Str2Scheme( as_map->get_charstr( "mode", "struct" ) );

      if ( auto include = as_map->get( "include" ); include != nullptr )
        return QuoteOpts( fieldMan, quoType, QuoteOpts::include, *include );

      if ( auto exclude = as_map->get( "exclude" ); exclude != nullptr )
        return QuoteOpts( fieldMan, quoType, QuoteOpts::exclude, *exclude );

      if ( auto fnmatch = as_map->get( "fnmatch" ); fnmatch != nullptr )
        return QuoteOpts( fieldMan, quoType, QuoteOpts::fnmatch, *fnmatch );

      return QuoteOpts( fieldMan, quoType );
    }

    throw std::invalid_argument( "invalid quote options @" __FILE__ ":" LINE_STRING );
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
