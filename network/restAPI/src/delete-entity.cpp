# include "delete-entity.hpp"
# include <reports.hpp>

namespace restAPI
{

  void  DeleteEntity::ready()
  {
    auto  deldoc = [functor = *this]()
    {

      if ( !*functor.cancel )
      {
        auto  report = palmira::UpdateReport( functor.search->Remove( { functor.docId } )
          ->Wait( functor.tm_off ) );

        if ( *functor.cancel )
          return;

        functor.evLoop->defer( [functor, report]()
        {
          auto  resp = decltype(functor.answer->WriteStatus( "200 OK" ))();

          if ( report.status().code() == 0 )
          {
            resp = functor.answer
              ->WriteStatus( "200 OK" )
              ->WriteHeader( "Content-Type", "application/json; charset=utf-8" );
          }
            else
          {
            resp = functor.answer
              ->WriteStatus( "404 Not Found" )
              ->WriteHeader( "Content-Type", "application/json; charset=utf-8" );
          }

          if ( !functor.cancel )
          {
            auto  base = mtc::json::print::decorated();
            auto  deco = mtc::json::print::decorated( base );

            base.Break( ::Serialize( resp, '{' ) );

            PrintKey( resp, "elapced", deco );
              base.Break( ::Serialize( mtc::json::Print( base.Space( ::Serialize( resp, ':' ) ),
                report.timing().elapced(), deco ), ',' ) );

            *functor.cancel = true;

            ::Serialize( resp, '}' )->FinishWrite();
          }
        } );
      }
    };

    return thPool != nullptr ? (void)thPool->Insert( deldoc ) : (void)deldoc();
  }

}
