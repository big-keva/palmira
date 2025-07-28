# include "delphi-search.hpp"
# include "../statusReport.hpp"
# include "z-arguments.hpp"
# include "DelphiX/storage/posix-fs.hpp"
# include "DelphiX/indexer/layered-contents.hpp"
//# include <DelphiX/context/decomposer.hpp>
//# include "DelphiX/context/make-image.hpp"
//# include "DelphiX/textAPI/word-break.hpp"
# include "DelphiX/textAPI/DOM-text.hpp"
# include "DelphiX/textAPI/DOM-load.hpp"
# include "DelphiX/lang-api.hpp"
# include <map>

using namespace DelphiX;

namespace palmira {

  class DelphiSearch
  {
    using CallMethod = UpdateReport (DelphiSearch::*)( const mtc::zmap& );
    using MethodsMap = std::map<std::string, CallMethod>;

    MethodsMap  methodsMap = {
      { "Insert", &DelphiSearch::Insert },
      { "Update", &DelphiSearch::Update },
      { "Remove", &DelphiSearch::Remove }
    };

  public:
    auto  operator()( const mtc::zmap& ) -> mtc::zmap;    // root callback

  protected:
    auto  Insert( const mtc::zmap& ) -> UpdateReport;
    auto  Update( const mtc::zmap& ) -> UpdateReport;
    auto  Remove( const mtc::zmap& ) -> UpdateReport;
    
    template <size_t N>
    auto  DumpMetadata( const mtc::zmap&, char (&)[N] ) const -> std::pair<std::shared_ptr<char[]>, size_t>;
    auto  LoadMetadata( const mtc::api<const mtc::IByteBuffer>& ) const -> mtc::zmap;

  public:
//    DelphiSearch( mtc::api<IContentsIndex>, std::function<mtc::api<IImage>()> );

  protected:
    /*
    mtc::api<IContentsIndex>          contextIndex;
    std::function<mtc::api<IImage>()> makeContents;
    context::Decomposer               decomposerFn = context::DefaultDecomposer();
    */
  };

// DelphiSearch implementation
/*
  DelphiSearch::DelphiSearch( mtc::api<IContentsIndex> ix, std::function<mtc::api<IImage>()> mf ):
    contextIndex( ix ),
    makeContents( mf )
  {
  }

  auto  DelphiSearch::operator()( const mtc::zmap& request ) -> mtc::zmap
  {
    auto  cmdstr = request.get_charstr( "cmd", "" );
    auto  method = methodsMap.find( cmdstr );

    if ( method != methodsMap.end() )
      return (this->*method->second)( request.get_zmap( "arg", {} ) );

    return UpdateReport( EINVAL, mtc::strprintf( "unknown function '%s'", cmdstr.c_str() ) );
  }

  auto  DelphiSearch::Insert( const mtc::zmap& args ) -> UpdateReport
  {
    try
    {
      char  buffer[0x400];
      auto  insert = InsertArgs( args );
      auto  serial = DumpMetadata( insert.metadata, buffer );
      auto  mArena = mtc::Arena();
      auto  pwBody = mArena.Create<textAPI::BaseBody<mtc::Arena::allocator<char>>>();
      auto  pwText = &insert.document;
      auto  pimage = makeContents();
      auto  getdoc = mtc::api<const IEntity>();

    // check if document is utf16-encoded; recode document if not so
      if ( !insert.document.IsEncoded( unsigned(-1) ) )
        insert.document.CopyUtf16( pwText = mArena.Create<textAPI::BaseDocument<mtc::Arena::allocator<char>>>() );

    // create document representation in words
      CopyMarkup(
      BreakWords( *pwBody,
        pwText->GetBlocks() ),
        pwText->GetMarkup() );

    // create document image
      decomposerFn( pimage,
        pwBody->GetTokens(),
        pwBody->GetMarkup() );

      getdoc = contextIndex->SetEntity( insert.objectId, pimage.ptr(),
        { serial.first.get(), serial.second } );

      return { 0, "OK", { { "metadata", LoadMetadata( getdoc->GetExtra() ) } } };
    }
    catch ( const std::bad_function_call& xp )        {  return { EFAULT, xp.what() };  }
    catch ( const std::invalid_argument& xp )         {  return { EINVAL, xp.what() };  }
    catch ( const textAPI::load_as::ParseError& xp )  {  return { EINVAL, xp.what() };  }
  }

  auto  DelphiSearch::Update( const mtc::zmap& args ) -> UpdateReport
  {
    try
    {
      char  buffer[0x400];
      auto  update = UpdateArgs( args );
      auto  serial = DumpMetadata( update.metadata, buffer );
      auto  getdoc = mtc::api<const IEntity>();

      if ( (getdoc = contextIndex->SetExtras( update.objectId, { serial.first.get(), serial.second } )) == nullptr )
        return { ENOENT, "document not found" };

      return { 0, "OK", { { "metadata", LoadMetadata( getdoc->GetExtra() ) } } };
    }
    catch ( const std::invalid_argument& xp )         {  return { EINVAL, xp.what() };  }
    catch ( const textAPI::load_as::ParseError& xp )  {  return { EINVAL, xp.what() };  }
  }

  auto  DelphiSearch::Remove( const mtc::zmap& args ) -> UpdateReport
  {
    try
    {
      RemoveArgs  remove( args );

      if ( contextIndex->DelEntity( remove.objectId ) )
        return UpdateReport( 0, "OK" );

      return UpdateReport( ENOENT, "document not found" );
    }
    catch ( const std::invalid_argument& xp )         {  return { EINVAL, xp.what() };  }
    catch ( const textAPI::load_as::ParseError& xp )  {  return { EINVAL, xp.what() };  }
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

  auto  CreateIndexService( const mtc::zmap& config ) -> SearchService
  {
    auto  spolicy = storage::posixFS::StoragePolicies::Open( "/home/keva/dev/palmira/!test" );
    auto  storage = storage::posixFS::Open( spolicy );
    auto  layered = indexer::layered::Index().Set( storage ).Create();

    return DelphiSearch( layered, [](){  return context::Rich::Create();  } );
  }
*/
}
/*
# include <mtc/json.h>

int   main()
{
  auto  config = mtc::zmap();
  auto  fnFind = palmira::CreateIndexService( config );

  mtc::json::Print( stdout, fnFind( {
    { "cmd", "Insert" },
    { "arg", mtc::zmap{
      { "id", "document-1" },
      { "metadata", mtc::zmap{
        { "arg", 1 } } },
      { "json",
        "["
        "  { \"title\": \"document title\" },"
        "  { \"body\": [\n"
        "      \"first body string\","
        "      \"second body string\""
        "  ] }"
        "]" } } } } ), mtc::json::print::decorated() );

  return 0;
}
  */