# include "update.hpp"
# include "array-reports.hpp"
# include <reports.hpp>
#include <toolset.hpp>

namespace restAPI
{

  void  Dump( Responder::Response* to, const palmira::UpdateReport& report )
  {
    auto  decorg = mtc::json::print::decorated();
    auto  decres = mtc::json::print::decorated( decorg );

    if ( report.status().code() == 0 )
    {
      to->WriteStatus( "200 OK" )
        ->WriteHeader( "Content-Type", "application/json; charset=utf-8" );
    }
    else
    {
      to->WriteStatus( "404 Not Found" )
        ->WriteHeader( "Content-Type", "application/json; charset=utf-8" );
    }

    decres.Break( ::Serialize( to, '{' ) );

    decres.Break( ::Serialize( mtc::json::Print( decres.Space( ::Serialize( PrintKey( decres.Shift( to ), "status" ), ':' ) ),
      report.status(), decres ), ',' ) );

    decres.Break( ::Serialize( mtc::json::Print( decres.Space( ::Serialize( PrintKey( decres.Shift( to ), "elapced" ), ':' ) ),
      report.timing().elapced(), decres ), ',' ) );

    decres.Break(              mtc::json::Print( decres.Space( ::Serialize( PrintKey( decres.Shift( to ), "metadata" ), ':' ) ),
      report.extra(), decres ) );

    ::Serialize( to, '}' )->FinishWrite();
  }

  void  Update::ready()
  {
    if ( *cancel )
      return;

    try
    {
      auto  update = palmira::UpdateArgs();
      auto  buforg = buffer->data();
      auto  parsed = mtc::zmap();

     /*
      * object body contains:
      *   - metadata as structure,
      *   - version as uint64_t, and
      *   - update condition as serialized zvalue
      */
      if ( bufptr > buffer->data() )
        mtc::json::Parse( mtc::sourcebuf( buforg, bufptr - buforg ).getptr(), parsed );

     /*
      * Если есть route params, использовать их, должно быть два
      */
      switch ( zroute.size() )
      {
        case 2:   update.objectId = zroute[1];
        case 0:   break;
        default:  throw std::logic_error( "Invalid route arguments count" );
      }

     /*
      * Если идентификатор не задан, проверить в body
      */
      if ( update.objectId.empty() && parsed.get_charstr( "id" ) != nullptr )
        update.objectId = *parsed.get_charstr( "id" );

      // Если и там нет, проверить в запросе
      if ( update.objectId.empty() && zquery.get_charstr( "id" ) != nullptr )
        update.objectId = *zquery.get_charstr( "id" );

      // если всё ещё не определён, это ошибка
      if ( update.objectId.empty() )
        throw std::invalid_argument( "No document id provided" );

      update.metadata = parsed.get_zmap( "metadata",   {} );
      update.ifClause = parsed.get     ( "condition",  {} );

     /*
      * Если не определён thread Pool, выполнить запрос непосредственно здесь, в обработчике,
      * и, если время не истекло, вернуть его результат
      */
      if ( thPool == nullptr )
      {
        auto  report = palmira::UpdateReport(
          search->Update( update )->Wait( tm_off ) );

        return *cancel ? (void)NULL : Dump( answer.ptr(), report );
      }

     /*
      * Иначе зарегистрировать асинхронный обработчик запроса, который сделает то же самое,
      * но только отложенным образом.
      */
      thPool->Insert( [functor = *this, update]()
      {
        if ( !*functor.cancel )
        {
          auto  report = palmira::UpdateReport(
            functor.search->Update( update )->Wait( functor.tm_off ) );

          functor.evLoop->defer( [answer = functor.answer, cancel = functor.cancel, report]()
            {  return *cancel ? (void)NULL : Dump( answer.ptr(), report );  } );
        }
      } );
    }
    catch ( const mtc::json::parse::error& jx )
    {
      return Instant( "401 Bad Request", {
        { "status", palmira::Status( EINVAL, "Invalid request format!" ) },
        { "reason", mtc::strprintf( "Json error (line %u): %s", jx.get_json_lineid(), jx.what() ) } } );
    }
    catch ( const std::invalid_argument& xp )
    {
      return Instant( "401 Bad Request", {
        { "status", palmira::Status( EFAULT, "Invalid request format!" ) },
        { "reason", xp.what() } } );
    }
    catch ( const std::exception& xp )
    {
      return Instant( "500 Internal Server Error", {
        { "status", palmira::Status( EFAULT, "Internal server error!" ) },
        { "reason", xp.what() } } );
    }
    catch ( ... )
    {
      return Instant( "500 Internal Server Error", {
        { "status", palmira::Status( EFAULT, "Internal server error!" ) } } );
    }
  }

}
