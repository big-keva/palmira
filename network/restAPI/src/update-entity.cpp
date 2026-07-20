# include "update-entity.hpp"
# include <reports.hpp>

namespace restAPI
{

  void  UpdateEntity::ready()
  {
    auto  deldoc = [functor = *this]()
    {
      auto  update = palmira::UpdateReport();
      auto  parsed = mtc::zmap();

      if ( *functor.cancel )
        return;

      try
      {
       /*
        * object body contains:
        *   - metadata as structure,
        *   - version as uint64_t, and
        *   - update condition as serialized zvalue
        */
        mtc::json::Parse( (const char*)functor.buffer->data(), parsed, mtc::zmap{
          { "version", "word64" } } );

        update = functor.search->Update( { functor.docId,
          parsed.get_zmap   ( "metadata",   {} ),
          parsed.get_word64 ( "version",    0ll ),
          parsed.get        ( "condition",  {} ) } )
        ->Wait( functor.tm_off );

        if ( *functor.cancel )
          return;

        functor.evLoop->defer( [functor, update]()
        {
          auto  resp = decltype(functor.answer->WriteStatus( "200 OK" ))();

          if ( update.status().code() == 0 )
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

          if ( !*functor.cancel )
          {
            auto  base = mtc::json::print::decorated();
            auto  deco = mtc::json::print::decorated( base );

            base.Break( ::Serialize( resp, '{' ) );

            PrintKey( resp, "elapced", deco );
            {
              base.Break( ::Serialize( mtc::json::Print( base.Space( ::Serialize( resp, ':' ) ),
                update.timing().elapced(), deco ), ',' ) );
            }

            PrintKey( resp, "metadata", deco );
            {
              base.Break( ::Serialize( mtc::json::Print( base.Space( ::Serialize( resp, ':' ) ),
                update.metadata(), deco ), ',' ) );
            }

            *functor.cancel = true, ::Serialize( resp, '}' )->FinishWrite();
          }
        } );
      }
      catch ( ... )
      {
        // !!!
      }
    };

    return thPool != nullptr ? (void)thPool->Insert( deldoc ) : (void)deldoc();
  }

}
