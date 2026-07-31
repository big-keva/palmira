# include "search.hpp"
# include <reports.hpp>
# include <structo/queries.hpp>
# include <structo/queries/parser.hpp>

namespace
{
  void  Dump( restAPI::Responder::Response*, const palmira::SearchReport& );

  void  GetIntParam( palmira::SearchArgs&, const mtc::zmap&, const char* );
  void  GetStrQuery( palmira::SearchArgs&, const mtc::zmap& );
}

namespace restAPI
{
  void  Search::ready()
  {
    try
    {
      auto  search = palmira::SearchArgs();
      auto  buforg = buffer->data();
      auto  parsed = mtc::zmap();

    // вычитать body
      if ( bufptr > buffer->data() )
        mtc::json::Parse( mtc::sourcebuf( buforg, bufptr - buforg ).getptr(), parsed );

     /*
      * Если есть route params, использовать их
      */
      switch ( zroute.size() )
      {
        case 1:   zquery["space"] = zroute[0];
        case 0:   break;
        default:  throw std::logic_error( "Invalid route arguments count" );
      }

     /*
      * Запрос и пределы могут быть явно заданы аргументами ?first=1&count=10&query=ubuntu+24
      */
      GetIntParam( search, zquery, "first" );
      GetIntParam( search, zquery, "count" );
      GetStrQuery( search, zquery );

    // Границы и строка запроса могут быть задалы в body
    // - first
      if ( search.order.get( "first" ) == nullptr )
        GetIntParam( search, parsed, "first" );
      if ( search.order.get( "count" ) == nullptr )
        GetIntParam( search, parsed, "count" );
      if ( search.order.get( "query" ) == nullptr )
        GetStrQuery( search, parsed );

    // Если запрос так и не задан, сообщить об ошибке
      if ( search.query.empty() )
        throw std::invalid_argument( "'query' not defined" );

    // Если не определён thread Pool, выполнить запрос непосредственно здесь, в обработчике,
    // и, если время не истекло, вернуть его результат
      if ( thPool == nullptr )
      {
        auto  report = palmira::SearchReport(
          this->search->Search( search )->Wait( tm_off ) );

        return *cancel ? (void)NULL : Dump( answer.ptr(), report );
      }

    // Иначе зарегистрировать асинхронный обработчик запроса, который сделает то же самое,
    // но только отложенным образом.
      thPool->Insert( [functor = *this, search]()
      {
        if ( !*functor.cancel )
        {
          auto  report = palmira::UpdateReport(
            functor.search->Search( search )->Wait( functor.tm_off ) );

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

namespace
{
  using namespace restAPI;

  void  Dump( Responder::Response* to, const palmira::SearchReport& report )
  {
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

    mtc::json::Print( to, report, mtc::json::print::decorated() )
      ->FinishWrite();
  }

  void  GetIntParam( palmira::SearchArgs& search, const mtc::zmap& params, const char* key )
  {
    if ( auto pvalue = params.get( key ); pvalue != nullptr )
    {
      if ( pvalue->get_type() < mtc::zval::z_byte || pvalue->get_type() > mtc::zval::z_word64 )
        throw std::invalid_argument( mtc::strprintf( "query '%s' argument has to be integer" ) );
      search.order[key] = pvalue->cast_to_word32( -1 );
    }
  }

  void  GetStrQuery( palmira::SearchArgs& search, const mtc::zmap& params )
  {
    if ( auto pquery = params.get( "query" ); pquery != nullptr )
    {
      switch ( pquery->get_type() )
      {
        case mtc::zval::z_charstr:
          search.query = structo::queries::ParseQuery( *pquery->get_charstr() );
          break;
        case mtc::zval::z_widestr:
          search.query = structo::queries::ParseQuery( *pquery->get_widestr() );
          break;
        default:
          throw std::invalid_argument( "'query' argument has to be string" );
      }
    }
  }

}
