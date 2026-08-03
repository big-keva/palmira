# include "get-entity.hpp"
# include "quote-printer.hpp"
# include <reports.hpp>

namespace restAPI
{
//
// filling request from the source
//
  void  FillEmpty( mtc::zmap&               zout,
    std::initializer_list<const char*>      keys,
    std::initializer_list<const mtc::zmap>  maps )
  {
    for ( auto key: keys )
    {
      if ( zout.get( key ) == nullptr )
      {
        bool  line = false;

        for ( auto& map: maps )
          if ( auto pval = map.get( key ); pval != nullptr )
          {
            zout.put( key, *pval );
            line = true;
            break;
          }
        if ( !line )
          throw std::invalid_argument( mtc::strprintf( "paramenter '%s' not found", key ) );
      }
    }
  }

  void  Dump( Responder::Response* to, const palmira::SearchReport& report )
  {
    auto  decorg = mtc::json::print::decorated();
    auto  decres = mtc::json::print::decorated( decorg );
    auto  docset = report.items();

    if ( report.status().code() == 0 && report.found() != 0 )
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

    mtc::json::Print( decres.Space( ::Serialize( PrintKey( decres.Shift( to ), "elapced" ), ':' ) ),
      report.timing().elapced(), decres );

    if ( docset.size() != 0 )
    {
      decres.Break( ::Serialize( to, ':' ) );

      decres.Break( ::Serialize( mtc::json::Print( decres.Space( ::Serialize( PrintKey( decres.Shift( to ), "id" ), ':' ) ),
        docset.front().get_charstr( "id", "?" ), decres ), ',' ) );

      decres.Break( ::Serialize( mtc::json::Print( decres.Space( ::Serialize( PrintKey( decres.Shift( to ), "metadata" ), ':' ) ),
        docset.front().get_zmap( "extra", {} ), decres ), ',' ) );

      PrintVal( decres.Space( ::Serialize( PrintKey( decres.Shift( to ), "document" ), ':' ) ),
        docset.front().get_array_zval( "quote", {} ), decres, "{}" );
    }

    ::Serialize( decres.Break( to ), '}' )->FinishWrite();
  }

  void  GetEntity::ready()
  {
    try
    {
      auto  getone = palmira::SearchArgs();
      auto  getarg = mtc::zmap();
      auto  buforg = buffer->data();
      auto  parsed = mtc::zmap();

    // вычитать body
      if ( bufptr > buffer->data() )
        mtc::json::Parse( mtc::sourcebuf( buforg, bufptr - buforg ).getptr(), parsed );

    // проверить, задан ли идентификатор документа в пути запроса
      switch ( zroute.size() )
      {
        case 2:   getarg = { { "space", zroute[0] } };                      [[fallthrough]];
        case 1:   getarg = { { "space", zroute[0] }, { "id", zroute[1] } }; [[fallthrough]];
        case 0:   break;
        default:  throw std::logic_error( "Invalid route arguments count" );
      }

    // Если идентификатор не задан, проверить в body и zquery
    // если всё ещё не определён, это ошибка
      FillEmpty( getarg, { "id", "space" }, { parsed, zquery } );

    // сформировать запрос документа
      getone.query = mtc::zmap{
        { "get", getarg } };
      getone.order = mtc::zmap{
        { "first", 1U },
        { "count", 1U },
        { "order", "score" },
        { "quote", mtc::zmap{ { "mode", "source" } } } };

    // далее либо выполнение здесь, либо асинхронная задача
      if ( thPool == nullptr )
      {
        auto  report = palmira::SearchReport(
          search->Search( getone )->Wait( tm_off ) );

        return *cancel ? (void)NULL : Dump( answer.ptr(), report );
      }
      /*
       * Иначе зарегистрировать асинхронный обработчик запроса, который сделает то же самое,
       * но только отложенным образом.
       */
      thPool->Insert( [functor = *this, getone]()
      {
        if ( !*functor.cancel )
        {
          auto  report = palmira::UpdateReport(
            functor.search->Search( getone )->Wait( functor.tm_off ) );

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
