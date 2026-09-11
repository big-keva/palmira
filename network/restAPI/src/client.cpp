# include <IXWebSocket/ixwebsocket/IXHttpClient.h>
# include <service.hpp>
# include <reports.hpp>
# include <mtc/json.h>
# include <mtc/recursive_shared_mutex.hpp>
# include <mtc/threadPool.hpp>
# include <mtc/config.h>

template <>
inline  std::string*  ::Serialize( std::string* o, const void* p, size_t l )
{
  return o->append( (const char*)p, l ), o;
}

namespace restAPI
{
  using IService = palmira::IService;
  using InsertArgs = palmira::InsertArgs;
  using UpdateArgs = palmira::UpdateArgs;
  using RemoveArgs = palmira::RemoveArgs;
  using SearchArgs = palmira::SearchArgs;

  class Client final: public IService
  {
    using clock_type = std::chrono::steady_clock;
    using time_point = std::chrono::time_point<clock_type>;

    class Waiter final: public IPending
    {
      const NotifyFn          notify;

      std::mutex              mxWait;
      std::condition_variable cvWait;

      mtc::zmap               result;
      volatile bool           bReady = false;

    public:
      Waiter( NotifyFn fn ): notify( fn )  {}

      void  Done( const mtc::zmap& );
      auto  Wait( double = -1.0 ) -> mtc::zmap override;

      implement_lifetime_control
    };

    enum ContentType { json, zmap };

    std::string                       targetAddr;
    std::shared_ptr<mtc::ThreadPool>  threadPool;
    std::shared_ptr<ix::HttpClient>   httpClient;
    ContentType                       serialType = zmap;

  public:
    Client( const mtc::config& );
   ~Client() = default;

    auto  Insert( const InsertArgs&, NotifyFn = []( const mtc::zmap& ){} ) -> mtc::api<IPending> override;
    auto  Update( const UpdateArgs&, NotifyFn = []( const mtc::zmap& ){} ) -> mtc::api<IPending> override;
    auto  Remove( const RemoveArgs&, NotifyFn = []( const mtc::zmap& ){} ) -> mtc::api<IPending> override;
    auto  Search( const SearchArgs&, NotifyFn = []( const mtc::zmap& ){} ) -> mtc::api<IPending> override;
    void  Commit() override {}

  protected:
    auto  DoCall( const std::string&, const mtc::zmap&, NotifyFn = []( const mtc::zmap& ){} ) -> mtc::api<IPending>;

    implement_lifetime_control
  };

  // Client implementation

  Client::Client( const mtc::config& config ):
    httpClient( std::make_shared<ix::HttpClient>( true ) )
  {
    auto  serialMode = config.get_charstr( "mode", "json" );

    if ( (targetAddr = config.get_charstr( "addr", "?" )) == "?" )
      throw std::invalid_argument( "undefined 'addr'" );

    if ( serialMode == "json" )  serialType = json;
      else
    if ( serialMode == "zmap" )  serialType = zmap;
      else
    throw std::invalid_argument( "invalid 'mode'" );
  }

  auto  Client::Remove( const RemoveArgs& remove, NotifyFn notify ) -> mtc::api<IPending>
  {
    mtc::zmap   zmap;

    return DoCall( "/remove", remove.Serialize( zmap ), notify );
  }

  auto  Client::Update( const UpdateArgs& update, NotifyFn notify ) -> mtc::api<IPending>
  {
    mtc::zmap   zmap;

    return DoCall( "/update", update.Serialize( zmap ), notify );
  }

  auto  Client::Insert( const InsertArgs& insert, NotifyFn notify ) -> mtc::api<IPending>
  {
    mtc::zmap   zmap;

    return DoCall( "/insert", insert.Serialize( zmap, palmira::text::as::dump ), notify );
  }

  auto  Client::Search( const SearchArgs& search, NotifyFn notify ) -> mtc::api<IPending>
  {
    mtc::zmap   zmap;

    return DoCall( "/search", search.Serialize( zmap ), notify );
  }

 /*
  * Отправка zmap-запроса:
  *   - json
  *   - zmap
  * Создание future, обработчик либо в thread pool, либо в потоке клиента - по настройкам.
  * По умолчанию работа в служебном потоке клиента.
  */
  auto  Client::DoCall( const std::string& uri, const mtc::zmap& req, NotifyFn nfn ) -> mtc::api<IPending>
  {
    auto  waiter = mtc::api( new Waiter( nfn ) );
    auto  sQuery = std::string();
    auto  thereq = httpClient->createRequest( (targetAddr + uri), "POST" );

    switch ( serialType )
    {
      case json:
        thereq->extraHeaders = {
          { "Content-Type", "application/json" },
          { "Accept", "application/json" } };
        mtc::json::Print( &thereq->body, req, mtc::json::print::decorated() );
        break;
      case zmap:  default:
        thereq->extraHeaders = {
          { "Content-Type", "application/vnd.mtc.zmap" },
          { "Cache-Control", "no-transform" },
          { "Accept", "application/vnd.mtc.zmap" },
          { "Accept", "application/json" } };
        req.Serialize( &thereq->body );
        break;
    }

    thereq->transferTimeout = 1800 /* ms */ ;

  // async call http
    httpClient->performRequest( thereq, [this, waiter]( const ix::HttpResponsePtr& report )
    {
      auto  action = [waiter, report]()
      {
        auto  result = mtc::zmap();

        if ( report->statusCode != 200 && report->statusCode != 201 )
        {
          if ( report->errorMsg == "Timeout" )
            return waiter->Done( palmira::StatusReport( ETIMEDOUT, "request timed out" ) );
          return waiter->Done( palmira::StatusReport( EFAULT, report->errorMsg ) );
        }

        for ( auto it = report->headers.find( "Accept" ); it != report->headers.end(); ++it )
          if ( it->second.find( "application/vnd.mtc.zmap" ) != it->second.npos )
          {
            if ( result.FetchFrom( mtc::sourcebuf( report->body ).ptr() ) != nullptr )
              return waiter->Done( result );
            return waiter->Done( palmira::StatusReport( EINVAL, "Invalid response format" ) );
          }

        if ( mtc::json::Parse( mtc::sourcebuf( report->body ).ptr(), result ) != nullptr )
          return waiter->Done( result );
        return waiter->Done( palmira::StatusReport( EINVAL, "Invalid response format" ) );
      };

      return threadPool != nullptr ? (void)threadPool->Insert( action ) : action();
    } );

    return waiter.ptr();
  }

  // Client::Waiter implementation

 /*
  * укладка результата, вызов коллбяки и сигнал синхронизатору
  */
  void  Client::Waiter::Done( const mtc::zmap& res )
  {
    auto  exlock = mtc::make_unique_lock( mxWait );
      result = res;

    bReady = true;

    if ( notify != nullptr )
      notify( result );

    cvWait.notify_all();
  }

 /*
  * Ожидание результата, синхронизатор
  */
  auto  Client::Waiter::Wait( double timeout ) -> mtc::zmap
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

}

extern "C"  int   CreateClient(
  palmira::IService** output,       // target IService object
  const mtc::config&  config )      // configuration path anchor
{
  if ( output == nullptr )
    return EINVAL;

  (*output = new restAPI::Client( config ))->Attach();
    return (void)config, 0;
}
