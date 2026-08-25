# if !defined( __elastic_responder_hpp__ )
# define __elastic_responder_hpp__
# include <service.hpp>
# include <uWebSockets/src/App.h>
# include <mtc/threadPool.hpp>
# include <mtc/iStream.h>
# include <mtc/json.h>

template <>
inline  std::string* Serialize( std::string* o, const void* p, size_t l )
{
  return o->append( (const char*)p, l ), o;
}

namespace restAPI
{
  using IService = palmira::IService;

  class Responder
  {
    Responder( Responder&& ) = delete;
  public:

    struct Response: mtc::Iface
    {
      virtual Response* WriteStatus( std::string_view ) = 0;
      virtual Response* WriteHeader( std::string_view, std::string_view ) = 0;
      virtual Response* WriteBuffer( std::string_view ) = 0;
      virtual void      FinishWrite( std::string_view = {} ) = 0;

      void  Instant( std::string_view, std::string_view );
      void  Instant( std::string_view, const mtc::zmap& );
    };

  protected:
    using shared_buffer = std::shared_ptr<std::vector<char>>;
    using shared_cancel = std::shared_ptr<bool>;
    using answer_object = mtc::api<Response>;

    uWS::Loop*          evLoop = uWS::Loop::get();
    mtc::ThreadPool*    thPool = nullptr;
    double              tm_off = -1.0;

    mtc::api<IService>  search;

    shared_cancel       cancel = std::make_shared<bool>( false );
    answer_object       answer;     // abstract response pointer
    shared_buffer       buffer;
    char*               bufptr = nullptr;

  public:
    Responder( mtc::api<IService>, mtc::api<Response> );
    Responder( const Responder& ) = default;

    void  abort();
    void  chunk( std::string_view, bool );

    virtual void  ready() = 0;

    void  Instant( std::string_view, std::string_view ) const;
    void  Instant( std::string_view, const mtc::zmap& ) const;

    void  Delayed( std::string_view, std::string_view ) const;
    void  Delayed( std::string_view, const mtc::zmap& ) const;

    auto  SetThreads( mtc::ThreadPool& threads ) -> Responder&;
    auto  SetTimeout( double           timeout ) -> Responder&;
  };

  template <class O>
  O*  PrintKey( O* o, const mtc::zmap::key& k )
  {
    switch ( k.type() )
    {
      case mtc::zmap::key::uint:
        return ::Serialize( mtc::json::Print( ::Serialize( o, '"' ), (unsigned)k ), '"' );
      case mtc::zmap::key::cstr:
        return mtc::json::print::charstr( o, (const char*)k, k.size() );
      case mtc::zmap::key::wstr: default:
        return mtc::json::print::widestr( o, (const widechar*)k, k.size() / sizeof(widechar) );
    }
  }

  template <bool SSL>
  auto  MakeResponse( uWS::HttpResponse<SSL>* ) -> mtc::api<Responder::Response>;
  auto  JsonError( int status, const char* type, const char* reason ) -> mtc::zmap;

}

template <> inline
restAPI::Responder::Response* Serialize( restAPI::Responder::Response* o, const void* p, size_t l )
{
  o->WriteBuffer( std::string_view( (const char*)p, l ) );
  return o;
}

# endif // !__elastic_responder_hpp__
