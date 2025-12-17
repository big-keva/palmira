# include "../../../reports.hpp"
# include "../../../grpcapi/client.hpp"
# include "messages.hpp"
# include <mtc/recursive_shared_mutex.hpp>
# include <grpcpp/grpcpp.h>

namespace grpcapi
{

  class Client final: public palmira::IService
  {
    class Waiter;

  public:
    auto  Insert( const palmira::InsertArgs&, NotifyFn ) -> mtc::api<IPending> override;
    auto  Update( const palmira::UpdateArgs&, NotifyFn ) -> mtc::api<IPending> override;
    auto  Remove( const palmira::RemoveArgs&, NotifyFn ) -> mtc::api<IPending> override;
    auto  Search( const palmira::SearchArgs&, NotifyFn ) -> mtc::api<IPending> override;
    void  Commit() override {}

    Client( std::shared_ptr<grpc::Channel> channel );

  protected:
    std::unique_ptr<grpcttp::Search::Stub> callStub;

    implement_lifetime_control
  };

  class Client::Waiter final: public IPending
  {
    friend class Client;

    grpcttp::UpdateReport   respond;
    grpc::ClientContext     context;

    const NotifyFn          fNotify;
    mtc::zmap               zoutmap;
    std::mutex              waitMtx;
    std::condition_variable wait_cv;
    volatile bool           isReady = false;

  public:
    Waiter( NotifyFn notifyFn ): fNotify( notifyFn )  {}

  protected:
    void  Done( grpc::Status );
    auto  Wait( double ) -> mtc::zmap override;

  public:
    implement_lifetime_control

  };

  // Client implementation

  auto  Client::Insert( const palmira::InsertArgs& args, NotifyFn notf ) -> mtc::api<IPending>
  {
    auto  insert = grpcttp::InsertArgs();
    auto  waiter = mtc::api( new Waiter( notf ) );

    // create grpc request
    Convert( insert, args );

    // call async service
    callStub->async()->Insert( &waiter->context, &insert, &waiter->respond, [waiter]( grpc::Status status )
      {  waiter->Done( status );  } );

    return waiter.ptr();
  }

  auto  Client::Update( const palmira::UpdateArgs& args, NotifyFn notf ) -> mtc::api<IPending>
  {
    auto  update = grpcttp::UpdateArgs();
    auto  waiter = mtc::api( new Waiter( notf ) );

    // create grpc request
    Convert( update, args );

    // call async service
    callStub->async()->Update( &waiter->context, &update, &waiter->respond, [waiter]( grpc::Status status )
      {  waiter->Done( status );  } );

    return waiter.ptr();
  }

  auto  Client::Remove( const palmira::RemoveArgs& args, NotifyFn notf ) -> mtc::api<IPending>
  {
    auto  remove = grpcttp::RemoveArgs();
    auto  waiter = mtc::api( new Waiter( notf ) );

  // create grpc request
    Convert( remove, args );

  // call async service
    callStub->async()->Remove( &waiter->context, &remove, &waiter->respond, [waiter]( grpc::Status status )
      {  waiter->Done( status );  } );

    return waiter.ptr();
  }

  auto  Client::Search( const palmira::SearchArgs& args, NotifyFn notf ) -> mtc::api<IPending>
  {
    /*
    auto  search = grpcttp::SearchArgs();
    auto  waiter = mtc::api( new Waiter( notf ) );

  // create grpc request
    Convert( search, args );

  // call async service
    callStub->async()->Remove( &waiter->context, &search, &waiter->respond, [waiter]( grpc::Status status )
      {  waiter->Done( status );  } );

    return waiter.ptr();
    */
    return nullptr;
  }

  Client::Client( std::shared_ptr<grpc::Channel> channel ):
    callStub( grpcttp::Search::NewStub( channel ) )
  {
  }

  // Client::Waiter implementation

  void  Client::Waiter::Done( grpc::Status status )
  {
    auto  locker = mtc::make_unique_lock( waitMtx );

    if ( status.ok() )  Convert( zoutmap, respond );
      else zoutmap = palmira::UpdateReport( status.error_code(), status.error_message() );

    isReady = true;

    if ( fNotify != nullptr )
      fNotify( zoutmap );

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
