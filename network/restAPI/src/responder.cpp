# include "responder.hpp"
# include <mtc/json.h>

namespace restAPI
{
  using namespace palmira;

  template <class HttpResponse>
  class ResponseImpl final: public Responder::Response
  {
    HttpResponse* answer;

  public:
    ResponseImpl( HttpResponse* rsp ): answer( rsp )  {}

    Response* WriteStatus( std::string_view status ) override
      {  return answer = answer->writeStatus( status ), this;  }
    Response* WriteBuffer( std::string_view buffer ) override
      {  return answer->write( buffer ), this;  }
    Response* WriteHeader( std::string_view key, std::string_view val ) override
      {  return answer = answer->writeHeader( key, val ), this;  }
    void      FinishWrite( std::string_view finish ) override
      {  return answer->end( finish );  }

    implement_lifetime_control
  };

  // Responder::Response implementation

  void  Responder::Response::Instant( std::string_view status, std::string_view message )
  {
      WriteStatus( status )
    ->WriteHeader( "Content-Type", "text/plain; charset=\"utf-8\"" )
    ->WriteHeader( "Connection", "keep-alive" )
    ->FinishWrite( message );
  }

  void  Responder::Response::Instant( std::string_view status, const mtc::zmap& message )
  {
    mtc::json::Print(
        WriteStatus( status )
      ->WriteHeader( "Content-Type", "application/json; charset=\"utf-8\"" )
      ->WriteHeader( "Connection", "keep-alive" ), message, mtc::json::print::decorated() )
      ->FinishWrite();
  }

  // Responder implementation

  Responder::Responder( mtc::api<IService> service, mtc::api<Response> respond ):
    search( service ),
    answer( respond ) {}

  void  Responder::abort()
  {
    *cancel = true;
  }

  void  Responder::chunk( std::string_view chunk, bool final )
  {
    try
    {
      char* bufend;

      if ( *cancel )
        return;

      if ( bufptr == nullptr )
      {
        buffer = std::make_shared<std::vector<char>>( 4 * 1024 * 1024 );
        bufptr = buffer->data();
      }

      bufend = buffer->data() + buffer->size() - 0x100;

      if ( bufptr + chunk.size() > bufend )
        return Instant( "413 Content Too Large", "Request too large to be loaded" );

      bufptr = chunk.size() + (char*)memcpy( bufptr, chunk.data(), chunk.size() );

      return final ? ready() : (void)NULL;
    }

    catch ( const std::bad_alloc& xp )
    {
      return Instant( "500 Internal Server Error", "Process could not allocate 4Mb memory" );
    }
  }

  void  Responder::Instant( std::string_view status, std::string_view message ) const
  {
    return answer->Instant( status, message ), void(*cancel = true);
  }

  void  Responder::Instant( std::string_view status, const mtc::zmap& message ) const
  {
    return answer->Instant( status, message ), void(*cancel = true);
  }

  void  Responder::Delayed( std::string_view status, std::string_view message ) const
  {
    return evLoop->defer( [
      answer = this->answer,
      cancel = this->cancel, state = std::string( status ), msg = std::string( message ) ]() mutable
    {
      answer
        ->WriteStatus( state )
        ->WriteHeader( "Content-Type", "text/plain; charset=\"utf-8\"" )
        ->FinishWrite( msg );
      *cancel = true;
    } );
  }

  void  Responder::Delayed( std::string_view status, const mtc::zmap& report ) const
  {
    return evLoop->defer( [
      answer = this->answer,
      cancel = this->cancel, state = std::string( status ), report]() mutable
    {
      mtc::json::Print( answer
        ->WriteStatus( state )
        ->WriteHeader( "Content-Type", "application/json; charset=\"utf-8\"" ), report, mtc::json::print::decorated() )
        ->FinishWrite();
      *cancel = true;
    } );
  }

  auto  Responder::SetThreads( mtc::ThreadPool& threads ) -> Responder&
  {
    return thPool = &threads, *this;
  }

  auto  Responder::SetTimeout( double timeout ) -> Responder&
  {
    return tm_off = timeout, *this;
  }

  template <bool SSL>
  auto  MakeResponse( uWS::HttpResponse<SSL>* response ) -> mtc::api<Responder::Response>
  {
    return new ResponseImpl<uWS::HttpResponse<SSL>>(
      response->writeHeader( "Access-Control-Allow-Origin", "*" ) );
  }

  auto  JsonError( int status, const char* type, const char* reason ) -> mtc::zmap
  {
    return {
      { "error", mtc::zmap{
        { "root_cause", mtc::array_zmap{
           {
             { "type", type },
             { "reason", reason }
           } } },
        { "type", type },
        { "reason", reason } } },
      { "status", status } };
  }

  template  auto  MakeResponse( uWS::HttpResponse<true>* ) -> mtc::api<Responder::Response>;
  template  auto  MakeResponse( uWS::HttpResponse<false>* ) -> mtc::api<Responder::Response>;

}
