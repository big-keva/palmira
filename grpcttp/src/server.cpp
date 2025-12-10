# include "../reports.hpp"
# include "../server.hpp"
# include "messages.hpp"
# include <condition_variable>
# include <grpcpp/health_check_service_interface.h>
# include <grpcpp/ext/proto_server_reflection_plugin.h>
# include <grpcpp/grpcpp.h>
# include <mtc/recursive_shared_mutex.hpp>

namespace grpcttp {

  class Server final: public palmira::IServer
  {
    class GrpcServer: public Search::CallbackService
    {
      mtc::api<palmira::IService>   search;

    public:
      auto  Insert(
        grpc::CallbackServerContext* context,
        const InsertArgs*            request,
        UpdateReport*                respond ) -> grpc::ServerUnaryReactor* override;
      auto  Update(
        grpc::CallbackServerContext* context,
        const UpdateArgs*            request,
        UpdateReport*                respond ) -> grpc::ServerUnaryReactor* override;
      auto  Remove(
        grpc::CallbackServerContext* context,
        const RemoveArgs*            request,
        UpdateReport*                respond ) -> grpc::ServerUnaryReactor* override;

      GrpcServer( mtc::api<palmira::IService> service ): search( service )  {}

    protected:
      template <class IntRequest, mtc::api<palmira::IService::IPending> (palmira::IService::*Method)( const IntRequest& ), class PubRequest>
      auto  Change(
        grpc::CallbackServerContext* context,
        const PubRequest*            request,
        grpcttp::UpdateReport*       respond ) -> grpc::ServerUnaryReactor*;
    };

    GrpcServer                    custom;
    std::unique_ptr<grpc::Server> server;
    uint16_t                      dwport;
    std::atomic_long              rcount = 0;

    volatile bool                 waiter = false;
    std::mutex                    mxwait;
    std::condition_variable       cvwait;

  public:     // Server::CallbackService overridables

    Server( mtc::api<palmira::IService> service, uint16_t port ):
      custom( service ), dwport( port ) {}

  public:     // mtc::Iface overridables
    long  Attach() override;
    long  Detach() override;

  public:     // IServer overridables
    void  Start() override;
    void  Stop() override;
    void  Wait() override;

  };

  // Server::GrpcServer implementation

  template <class IntRequest, mtc::api<palmira::IService::IPending> (palmira::IService::*Method)( const IntRequest& ), class PubRequest>
  auto  Server::GrpcServer::Change(
    grpc::CallbackServerContext* context,
    const PubRequest*            request,
    UpdateReport*                respond ) -> grpc::ServerUnaryReactor*
  {
    IntRequest                thearg;
    grpc::ServerUnaryReactor* writer = context->DefaultReactor();

    try
    {
      // Проверка на отмену в начале
      if ( context->IsCancelled() )
        return writer->Finish( grpc::Status::CANCELLED ), writer;
      Convert( *respond, (search->*Method)( Convert( thearg, *request ) )
        ->Wait() );
    }
    catch ( ... )
    {
      Convert( *respond, palmira::UpdateReport( EFAULT, "unknown exception" ) );
    }

    return writer->Finish( grpc::Status::OK ), writer;
  }

  auto  Server::GrpcServer::Insert(
    grpc::CallbackServerContext* context,
    const InsertArgs*            request,
    UpdateReport*                respond ) -> grpc::ServerUnaryReactor*
  {
    return Change<palmira::InsertArgs, &palmira::IService::Insert>( context, request, respond );
  }

  auto  Server::GrpcServer::Update(
    grpc::CallbackServerContext* context,
    const UpdateArgs*            request,
    UpdateReport*                respond ) -> grpc::ServerUnaryReactor*
  {
    return Change<palmira::UpdateArgs, &palmira::IService::Update>( context, request, respond );
  }

  auto  Server::GrpcServer::Remove(
    grpc::CallbackServerContext* context,
    const RemoveArgs*            request,
    UpdateReport*                respond ) -> grpc::ServerUnaryReactor*
  {
    return Change<palmira::RemoveArgs, &palmira::IService::Remove>( context, request, respond );
  }

  // Server::GrpcServer implementation

  long  Server::Attach()
  {
    return ++rcount;
  }

  long  Server::Detach()
  {
    auto  refers = --rcount;

    if ( refers == 0 )
    {
      auto  locker = mtc::make_unique_lock( mxwait, std::defer_lock );

      if ( server != nullptr )
        server->Shutdown();

      locker.lock();
      cvwait.wait( locker, [this](){  return !waiter;  } );

      delete this;
    }
    return refers;
  }

  void  Server::Start()
  {
    grpc::ServerBuilder builder;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    builder.AddListeningPort( mtc::strprintf( "0.0.0.0:%d", dwport ), grpc::InsecureServerCredentials());
    builder.RegisterService( &custom );

    server = std::move( builder.BuildAndStart() );
  }

  void  Server::Wait()
  {
    if ( server == nullptr )
      throw std::logic_error( "Wait() called on server which was not started" );

    waiter = true,
      server->Wait(),
        waiter = false;
    cvwait.notify_one();
  }

  auto  CreateServer( mtc::api<palmira::IService> serv, uint16_t port ) -> mtc::api<palmira::IServer>
  {
    return new Server( serv, port );
  }

}
