# include "array-reports.hpp"
# include <reports.hpp>
# include <mtc/recursive_shared_mutex.hpp>

namespace restAPI
{

  ArrayReport::ArrayReport(
      mtc::api<Responder::Response> o,
      std::shared_ptr<bool>         c,
      uWS::Loop*                    l,
      unsigned                      u ):
        output( o ),
        cancel( c ),
        evLoop( l ),
        utotal( u )
    {
    }

  void  ArrayReport::Append( const mtc::zmap& result )
  {
    mtc::interlocked( mtc::make_unique_lock( locker ), [&]()
      {  report.push_back( result );  } );

    if ( ++uready == utotal )
    {
      evLoop->defer( [output = this->output, report = std::move( this->report ), cancel = this->cancel]()
      {
        if ( !*cancel )
        {
          auto  dumped = output
            ->WriteStatus( "200 OK" )
            ->WriteHeader( "Content-Type", "application/json; charset=utf-8" );

          if ( report.size() == 1 )
          {
            mtc::json::Print( dumped, report.front(),
              mtc::json::print::decorated() )->FinishWrite();
          }
            else
          {
            mtc::json::Print( dumped, report,
              mtc::json::print::decorated() )->FinishWrite();
          }
          *cancel = true;
        }
      } );
    }
  }

}
