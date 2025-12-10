# include "../../reports.hpp"
# include "../client.hpp"
# include "messages.hpp"
# include <mtc/recursive_shared_mutex.hpp>
# include <grpcpp/grpcpp.h>

namespace grpcttp
{

  class Client: public palmira::IService
  {
    class Waiter;

  public:
    auto  Insert( const palmira::InsertArgs& ) -> mtc::api<IPending> override;
    auto  Update( const palmira::UpdateArgs& ) -> mtc::api<IPending> override;
    auto  Remove( const palmira::RemoveArgs& ) -> mtc::api<IPending> override;
    auto  Search( const palmira::SearchArgs& ) -> mtc::api<IPending> override;
    void  Commit() override {}

    Client( std::shared_ptr<grpc::Channel> channel );

  protected:
    std::unique_ptr<Search::Stub> callStub;

    implement_lifetime_control
  };

  class Client::Waiter: public IPending
  {
    friend class Client;

    UpdateReport            respond;
    grpc::ClientContext     context;

    mtc::zmap               zoutmap;
    std::mutex              waitMtx;
    std::condition_variable wait_cv;
    volatile bool           isReady = false;

  protected:
    void  Done( grpc::Status );
    auto  Wait( double ) -> mtc::zmap override;

  public:
    implement_lifetime_control

  };

  // Client implementation

  auto  Client::Remove( const palmira::RemoveArgs& args ) -> mtc::api<IPending>
  {
    auto  remove = RemoveArgs();
    auto  waiter = mtc::api( new Waiter() );

  // create grpc request
    Convert( remove, args );

  // call async service
    callStub->async()->Remove( &waiter->context, &remove, &waiter->respond, [waiter]( grpc::Status status )
      {  waiter->Done( status );  } );

    return waiter.ptr();
  }

  Client::Client( std::shared_ptr<grpc::Channel> channel ):
    callStub( Search::NewStub( channel ) )
  {
  }

  // Client::Waiter implementation

  void  Client::Waiter::Done( grpc::Status status )
  {
    auto  locker = mtc::make_unique_lock( waitMtx );

    if ( status.ok() )  Convert( zoutmap, respond );
      else zoutmap = palmira::UpdateReport( status.error_code(), status.error_message() );

    isReady = true;
      wait_cv.notify_all();
  }

  auto  Client::Waiter::Wait( double seconds ) -> mtc::zmap
  {
    auto  tpoint = std::chrono::steady_clock::time_point();
    auto  exlock = mtc::make_unique_lock( waitMtx, std::defer_lock );

    if ( seconds >= 0 )
    {
      tpoint = std::chrono::steady_clock::now() + std::chrono::seconds( unsigned(floor( seconds )) )
        + std::chrono::milliseconds( unsigned(1000 * (seconds - floor( seconds ))) );
    }
      else
    tpoint = std::chrono::steady_clock::time_point::max();

    exlock.lock();

    if ( !wait_cv.wait_until( exlock, tpoint, [this](){  return isReady;  } ) )
      zoutmap = palmira::UpdateReport( ETIMEDOUT, "request timed out" );

    return std::move( zoutmap );
  }

  auto  CreateClient( const char* addr ) -> mtc::api<palmira::IService>
  {
    return new Client( grpc::CreateChannel( addr, grpc::InsecureChannelCredentials() ) );
  }

}
