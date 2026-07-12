# if !defined( PALMIRA_NETWORK_RESTAPI_CLIENT_HPP_ )
# define PALMIRA_NETWORK_RESTAPI_CLIENT_HPP_
# include "../server.hpp"
# include "../service.hpp"

namespace restAPI
{

  enum class CompressionMethod: unsigned
  {
    None = 0,
    Deflate = 1
  };

  class Client
  {
    class impl;

    mtc::api<impl>  internal;

  public:
    Client();
    Client( const char* address );
    Client( const char* host, uint16_t port );
   ~Client();
   /*
    * SetCompressionMethod( method )
    *   - None (default)
    *   - Deflate
    */
    auto  SetCompressionMethod( CompressionMethod method ) -> Client&;
    auto  SetDefaultTimeout( double seconds ) -> Client&;
    auto  SetAddress( const char* address ) -> Client&;
    auto  SetAddress( const char* host, uint16_t port ) -> Client&;
    auto  Create( const char* host, uint16_t port ) -> mtc::api<palmira::IService>;
    auto  Create( const char* address ) -> mtc::api<palmira::IService>;
    auto  Create() -> mtc::api<palmira::IService>;

  };

  auto  CreateClient( const char* addr ) -> mtc::api<palmira::IService>;

}

# endif // !PALMIRA_NETWORK_RESTAPI_CLIENT_HPP_
