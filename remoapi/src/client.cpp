# include "../client.hpp"
# include "../reports.hpp"
# include "../toolset.hpp"
# include "unpack.hpp"
# include <DeliriX/DOM-dump.hpp>
# include <remottp/src/server/rest.hpp>
# include <remottp/http-client.hpp>
# include <remottp/src/events.hpp>
# include <remottp/message.hpp>
# include <mtc/recursive_shared_mutex.hpp>
# include <condition_variable>

template <>
inline  std::vector<char>* Serialize( std::vector<char>* o, const void* p, size_t l )
  {  return o->insert( o->end(), (const char*)p, l + (const char*)p ), o;  }

namespace remoapi
{

  class Client::impl final: public palmira::IService
  {
    friend class Client;

    struct Waiter;
    struct Action;

    http::Channel     channel;
    CompressionMethod zipMeth = CompressionMethod::None;
    double            timeout = -1.0;    // default timeout for all connections

  public:
    auto  Insert( const palmira::InsertArgs&, NotifyFn ) -> mtc::api<IPending> override;
    auto  Update( const palmira::UpdateArgs&, NotifyFn ) -> mtc::api<IPending> override;
    auto  Remove( const palmira::RemoveArgs&, NotifyFn ) -> mtc::api<IPending> override;
    auto  Search( const palmira::SearchArgs&, NotifyFn ) -> mtc::api<IPending> override;
    void  Commit() override {}

    void  SetChannel( const http::Channel& newChannel ) {  channel = newChannel;  }
    auto  GetChannel() const -> const http::Channel&    {  return channel;  }

    impl() {}

  protected:
    template <class Request>
    auto  Modify( const Request&, const std::string& uristr, NotifyFn ) -> mtc::api<IPending>;

    implement_lifetime_control

  };

  struct Client::impl::Waiter final: IPending
  {
    auto  Wait( double = -1.0 ) -> mtc::zmap override;
    void  Done( const mtc::zmap& );

    Waiter( NotifyFn fn ): notify( fn ) {}

  protected:
    const NotifyFn          notify;

    std::mutex              mxWait;
    std::condition_variable cvWait;

    mtc::zmap               result;
    volatile bool           bReady = false;

    implement_lifetime_control
  };

  struct Client::impl::Action
  {
    mtc::api<Waiter> waiter;

    void  operator()( http::ExecStatus status, const http::Respond& res, mtc::api<mtc::IByteStream> stm )
    {
      switch ( status )
      {
        case http::ExecStatus::Ok:
          waiter->Done( LoadReport( res, stm ) );
          break;
        case http::ExecStatus::Failed:
        case http::ExecStatus::Invalid:
          waiter->Done( palmira::StatusReport( EFAULT, "network exchange error" ) );
          break;
        case http::ExecStatus::TimedOut:  default:
          waiter->Done( palmira::StatusReport( ETIMEDOUT, "request timed out" ) );
          break;
      }
      WashStream( stm );
    }
  protected:
    auto  LoadReport( const http::Respond&, mtc::IByteStream* ) -> palmira::StatusReport;
    void  WashStream( mtc::IByteStream* );
  };

  std::atomic<http::Client*>  globalClient = nullptr;

  // Client::impl::Action implementation

  auto  Client::impl::Action::LoadReport( const http::Respond& res, mtc::IByteStream* stm ) -> palmira::StatusReport
  {
    mtc::zmap out;
    auto      src = mtc::api( stm );

    if ( stm != nullptr ) src = Inflate( res, stm );
      else return out;

    if ( res.GetHeaders().get( "Content-Type" ).substr( 0, 16 ) == "application/json" )
    {
      try
      {  mtc::json::Parse( src.ptr(), out );  }
      catch ( const mtc::json::parse::error& xp )
      {  out = palmira::StatusReport( EFAULT, "Could not parse the report" );  }
    }
      else
    if ( res.GetHeaders().get( "Content-Type" ).substr( 0, 24 ) == "application/octet-stream")
    {
      out.FetchFrom( stm );
    }
      else
    throw std::invalid_argument( "Unexpected content-type" );

    return out;
  }

  void  Client::impl::Action::WashStream( mtc::IByteStream* stm )
  {
    char  buffer[1024];

    while ( stm != nullptr && stm->Get( buffer, sizeof(buffer) ) > 0 )
      (void)NULL;
  }

  // Client::impl::Waiter implementation

  void  Client::impl::Waiter::Done( const mtc::zmap& res )
  {
    auto  exlock = mtc::make_unique_lock( mxWait );
      result = res;

    bReady = true;

    if ( notify != nullptr )
      notify( result );

    cvWait.notify_all();
  }

  auto  Client::impl::Waiter::Wait( double timeout ) -> mtc::zmap
  {
    auto  exlock = mtc::make_unique_lock( mxWait );

    if ( timeout > 0.0 )
    {
      auto  tlimit = std::chrono::steady_clock::now() + std::chrono::milliseconds( unsigned(timeout * 1000) );

      if ( cvWait.wait_until( exlock, tlimit, [this](){  return bReady;  } ) )
        return result.copy();
    }
      else
    {
      cvWait.wait( exlock, [this](){  return bReady;  } );
      return result.copy();
    }
    return {};
  }

  auto  MakeContents( mtc::zmap& zmap, const palmira::TimingArgs& args ) -> mtc::zmap&
  {
    if ( args.fTimeout >= 0 )
      zmap["timeout"] = args.fTimeout;
    return zmap;
  }

  auto  MakeContents( mtc::zmap& zmap, const palmira::AccessArgs& args ) -> mtc::zmap&
  {
    zmap["id"] = args.objectId;
    return MakeContents( zmap, (const palmira::TimingArgs&)args );
  }

  auto  MakeContents( mtc::zmap& zmap, const palmira::RemoveArgs& args ) -> mtc::zmap&
  {
    if ( !args.ifClause.empty() )
      zmap["if_clause"] = args.ifClause;
    return MakeContents( zmap, (const palmira::AccessArgs&)args );
  }

  auto  MakeContents( mtc::zmap& zmap, const palmira::UpdateArgs& args ) -> mtc::zmap&
  {
    if ( !args.metadata.empty() )
      zmap["metadata"] = args.ifClause;
    return MakeContents( zmap, (const palmira::RemoveArgs&)args );
  }

  auto  MakeContents( mtc::zmap& zmap, const palmira::InsertArgs& args ) -> mtc::zmap&
  {
    if ( args.textview.GetLength() != 0 )
      zmap["document"] = "after:dump";
    return MakeContents( zmap, (const palmira::UpdateArgs&)args );
  }

  template <class Request>
  auto  MakeContents( Request& req ) -> std::vector<char>
  {
    mtc::zmap         header;
    std::vector<char> serial;

    return std::move(
      *MakeContents( header, req ).Serialize( &serial ) );
  }

  auto  MakeContents( const palmira::InsertArgs& req ) -> std::vector<char>
  {
    mtc::zmap         header;
    std::vector<char> serial;

    return std::move( *req.textview.Serialize(
      MakeContents( header, req ).Serialize( &serial ) ) );
  }

  auto  MakeContents( const palmira::SearchArgs& req ) -> std::vector<char>
  {
    mtc::zmap         header;
    std::vector<char> serial;

    return std::move( *MakeContents( header, req ).Serialize( &serial ) );
  }

  // ClientAPI implementation

  template <class Request>
  auto  Client::impl::Modify( const Request& args, const std::string& uristr, NotifyFn nfn ) -> mtc::api<IPending>
  {
    auto  waiter = mtc::api( new Waiter( nfn ) );
    auto  thereq = mtc::ptr::clean( globalClient.load() )->NewRequest( http::Method::POST, uristr )
      .SetCallback( Action{ waiter } );
    auto  serial = MakeContents( args );    // create serialized data
    auto  timing = args.fTimeout > 0.0 ? args.fTimeout : timeout;

    // try compress it
    if ( serial.size() > 0x200 )
    {
      if ( zipMeth == CompressionMethod::Deflate )
      {
        try
        {
          serial = std::move( Deflate( serial ) );
          thereq.GetHeaders()["Content-Encoding"] = "deflate";
        }
        catch ( ... ) {}
      }
    }

    thereq
      .SetHeaders( { { "Content-Type", "application/octet-stream" } } )
      .SetBody( std::move( serial ) );

    if ( timing > 0.0 )
      thereq.SetTimeout( timing );

    return thereq.Start( channel ), waiter.ptr();
  }

  auto  Client::impl::Insert( const palmira::InsertArgs& args, NotifyFn notf ) -> mtc::api<IPending>
  {
    return Modify( args, mtc::strprintf( "/insert?id=%s", http::UriEncode( args.objectId ).c_str() ), notf );
  }

  auto  Client::impl::Update( const palmira::UpdateArgs& args, NotifyFn notf ) -> mtc::api<IPending>
  {
    return Modify( args, mtc::strprintf( "/update?id=%s", http::UriEncode( args.objectId ).c_str() ), notf );
  }

  auto  Client::impl::Remove( const palmira::RemoveArgs& args, NotifyFn notf ) -> mtc::api<IPending>
  {
    return Modify( args, mtc::strprintf( "/remove?id=%s", http::UriEncode( args.objectId ).c_str() ), notf );
  }

  auto  Client::impl::Search( const palmira::SearchArgs& args, NotifyFn notf ) -> mtc::api<IPending>
  {
    return Modify( args, "/search", notf );
  }

  // Client implementation

  Client::Client()
    {  }

  Client::Client( const char* address )
    {  SetAddress( address );  }

  Client::Client( const char* host, uint16_t port )
    {  SetAddress( host, port );  }

  Client::~Client()
    {  }

  auto  Client::SetCompressionMethod( CompressionMethod method ) -> Client&
  {
    if ( internal == nullptr )
      internal = mtc::api<impl>( new Client::impl() );
    return internal->zipMeth = method, *this;
  }

  auto  Client::SetDefaultTimeout( double timeout ) -> Client&
  {
    if ( internal == nullptr )
      internal = mtc::api<impl>( new Client::impl() );
    return internal->timeout = timeout, *this;
  }

  auto  Client::SetAddress( const char* address ) -> Client&
  {
    char      szhost[0x100];
    char*     outstr = szhost;
    uint32_t  dwport;

  // parse host::port
    while ( outstr != std::end(szhost) && *address != '\0' && *address != ':' )
      *outstr++ = *address++;

    if ( outstr == std::end(szhost) )
      throw std::invalid_argument( "host address is too long @" __FILE__ ":" LINE_STRING );

    if ( *address++ != ':' )
      throw std::invalid_argument( "host:port expected @" __FILE__ ":" LINE_STRING );

    *outstr = '\0';

    if ( (dwport = strtoul( address, &outstr, 10 )) == 0 || uint16_t(dwport) != dwport || *outstr != '\0' )
      throw std::invalid_argument( "port as uint16 expected @" __FILE__ ":" LINE_STRING );

    return SetAddress( szhost, dwport );
  }

  auto  Client::SetAddress( const char* host, uint16_t port ) -> Client&
  {
    auto  global = mtc::ptr::clean( globalClient.load() );

  // check global http client to be created
    while ( global == nullptr && !globalClient.compare_exchange_strong( global, mtc::ptr::dirty( global ) ) )
      global = mtc::ptr::clean( global );

    if ( global == nullptr )
      globalClient.store( global = new http::Client( 4 /* !!! */) );

  // check if client is initialized
    if ( internal == nullptr )
      internal = mtc::api<impl>( new Client::impl() );

  // initialize the channel
    internal->SetChannel( global->GetChannel( host, port ) );

    return *this;
  }

  auto  Client::Create( const char* host, uint16_t port ) -> mtc::api<palmira::IService>
  {
    return SetAddress( host, port ).Create();
  }

  auto  Client::Create( const char* address ) -> mtc::api<palmira::IService>
  {
    return SetAddress( address ).Create();
  }

  auto  Client::Create() -> mtc::api<palmira::IService>
  {
    if ( internal == nullptr || !internal->GetChannel().IsInitialized() )
      throw std::logic_error( "client channel address is not initialized" );
    return internal.release();
  }

 /*
  * client creation api
  */
  auto  CreateClient( const char* addr ) -> mtc::api<palmira::IService>
  {
    return Client().Create( addr );
  }

}
