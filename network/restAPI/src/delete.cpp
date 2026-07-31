# include "delete.hpp"
# include "array-reports.hpp"
# include <reports.hpp>

namespace restAPI
{

  extern
  void  Dump( Responder::Response*, const palmira::UpdateReport& );

  void  Delete::ready()
  {
    try
    {
      auto  remove = palmira::RemoveArgs();
      auto  buforg = buffer->data();
      auto  parsed = mtc::zmap();

    // вычитать body
      if ( bufptr > buffer->data() )
        mtc::json::Parse( mtc::sourcebuf( buforg, bufptr - buforg ).getptr(), parsed );

     /*
      * Если есть route params, использовать их, должно быть два
      */
      switch ( zroute.size() )
      {
        case 2:   remove.objectId = zroute[1];
        case 0:   break;
        default:  throw std::logic_error( "Invalid route arguments count" );
      }

    // Если идентификатор не задан, проверить в body
      if ( remove.objectId.empty() && parsed.get_charstr( "id" ) != nullptr )
        remove.objectId = *parsed.get_charstr( "id" );

    // Если и там нет, проверить в запросе
      if ( remove.objectId.empty() && zquery.get_charstr( "id" ) != nullptr )
        remove.objectId = *zquery.get_charstr( "id" );

    // если всё ещё не определён, это ошибка
      if ( remove.objectId.empty() )
        throw std::invalid_argument( "No document id provided" );

      remove.ifClause = parsed.get( "condition",  {} );

     /*
      * Если не определён thread Pool, выполнить запрос непосредственно здесь, в обработчике,
      * и, если время не истекло, вернуть его результат
      */
      if ( thPool == nullptr )
      {
        auto  report = palmira::UpdateReport(
          search->Remove( remove )->Wait( tm_off ) );

        return *cancel ? (void)NULL : Dump( answer.ptr(), report );
      }

     /*
      * Иначе зарегистрировать асинхронный обработчик запроса, который сделает то же самое,
      * но только отложенным образом.
      */
      thPool->Insert( [functor = *this, remove]()
      {
        if ( !*functor.cancel )
        {
          auto  report = palmira::UpdateReport(
            functor.search->Remove( remove )->Wait( functor.tm_off ) );

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
