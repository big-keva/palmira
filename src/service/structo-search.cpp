# include "../../service/structo-search.hpp"
# include "../toolset/zip-unzip.hpp"
# include "../reports.hpp"
# include "../toolset.hpp"
# include "quotation-tool.hpp"
# include "quotation-tool.hpp"
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

  protected:
    auto  buildRequest( const SearchArgs& ) -> mtc::api<queries::IQuery>;
    auto  getCollector( const SearchArgs& ) -> mtc::api<collect::ICollector>;
    auto  getQuoteTool( const mtc::zmap& ) const -> collect::QuotesFn;

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
        { "extra", LoadMetadata( getdoc->GetExtra() ) } } }, notify );
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
        { "extra", LoadMetadata( getdoc->GetExtra() ) } } }, notify );
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

  auto StructoSearch::buildRequest( const SearchArgs& args ) -> mtc::api<queries::IQuery>
  {
    struct DummyQuery final: queries::IQuery
    {
      auto  LastIndex() -> uint32_t {  return 0;  }
      auto  SearchDoc( uint32_t ) -> uint32_t {  return -1;  }
      auto  GetTuples( uint32_t ) -> const queries::Abstract& {  throw std::runtime_error{ "Not implemented" };  };
      auto  Duplicate( const Bounds& = {} ) -> mtc::api<IQuery> {  return this;  }

      implement_lifetime_control
    };

    if ( auto getmap = args.query.get_zmap(); getmap != nullptr )
      if ( auto getarr = getmap->get( "get" ); getarr != nullptr )
        return new DummyQuery();

    return queries::BuildRichQuery( args.query, args.terms,
      ctxIndex,
      lingProc,
      fieldMan );
  }

  auto  StructoSearch::getCollector( const SearchArgs& args ) -> mtc::api<collect::ICollector>
  {
    auto  stMode = args.order.get_charstr( "order", "score" );     // по умолчанию по релевантности
    auto  quoter = getQuoteTool( args.order.get_zmap( "quote", {} ) );

  // check 'get' queries
    if ( auto getmap = args.query.get_zmap(); getmap != nullptr )
      if ( auto getget = getmap->get( "get" ); getget != nullptr )
      {
        auto  loader = collect::LoadDumps( ctxIndex.ptr() );

        switch ( getget->get_type() )
        {
          case mtc::zval::z_zmap:
            loader.Insert(
              getget->get_zmap()->get_charstr( "id", "" ), getQuoteTool(
                getget->get_zmap()->get_zmap( "quote", { { "mode", "source" } } ) ) );
            break;
          case mtc::zval::z_array_zmap:
            for ( auto& next: *getget->get_array_zmap() )
              loader.Insert( next.get_charstr( "id", "" ), getQuoteTool(
                next.get_zmap( "quote", { { "mode", "source" } } ) ) );
            break;
          default:
            throw std::invalid_argument( "invalid request 'get' value type" );
        }

        return loader.Create();
      }

    if ( stMode == "score" )
    {
      return collect::Documents()
        .SetFirst( args.order.get_int32( "first", 1 ) )
        .SetCount( args.order.get_int32( "count", 10 ) )
        .SetAsync( asyncers )
        .SetQuote( quoter ).Create();
    }
    throw std::invalid_argument( "Unknown ordering '" + stMode + "' @" __FILE__ + ":" + LINE_STRING );
  }

  auto  StructoSearch::getQuoteTool( const mtc::zmap& quote ) const -> collect::QuotesFn
  {
    enquote::Scheme   scheme = enquote::StringToScheme( quote.get_charstr( "mode", "struct" ) );
    const mtc::zval*  pvalue;

    if ( (pvalue = quote.get( "include" )) != nullptr )
      switch ( pvalue->get_type() )
      {
        case mtc::zval::z_charstr:
          return Function( ctxIndex, fieldMan, scheme, enquote::include, { *pvalue->get_charstr() } );
        case mtc::zval::z_array_charstr:
          return Function( ctxIndex, fieldMan, scheme, enquote::include, *pvalue->get_array_charstr() );
        default:
          throw std::invalid_argument( "unexpected 'include' quotation value, has to be string or array of strings" );
      }

    if ( (pvalue = quote.get( "exclude" )) != nullptr )
      switch ( pvalue->get_type() )
      {
        case mtc::zval::z_charstr:
          return Function( ctxIndex, fieldMan, scheme, enquote::exclude, { *pvalue->get_charstr() } );
        case mtc::zval::z_array_charstr:
          return Function( ctxIndex, fieldMan, scheme, enquote::exclude, *pvalue->get_array_charstr() );
        default:
          throw std::invalid_argument( "unexpected 'exclude' quotation value, has to be string or array of strings" );
      }

    if ( (pvalue = quote.get( "fnmatch" )) != nullptr )
      switch ( pvalue->get_type() )
      {
        case mtc::zval::z_charstr:
          return Function( ctxIndex, fieldMan, scheme, enquote::fnmatch, { *pvalue->get_charstr() } );
        case mtc::zval::z_array_charstr:
          return Function( ctxIndex, fieldMan, scheme, enquote::fnmatch, *pvalue->get_array_charstr() );
        default:
          throw std::invalid_argument( "unexpected 'fnmatch' quotation value, has to be string or array of strings" );
      }

    return Function( ctxIndex, fieldMan, scheme );
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
