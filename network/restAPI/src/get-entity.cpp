# include "get-entity.hpp"
# include <reports.hpp>

namespace restAPI
{
  template <class O, class Z, class D>
  O*  PrintVal( O*, const Z&, const D& );

  template <class O, class D>
  O*  PrintVal( O*, const mtc::zmap&, const D& );

  template <class O, class D>
  O*  PrintVal( O*, const mtc::array_zval&, const D&, const char* braces = "[]" );

  template <class O, class D>
  O*  PrintVal( O* o, const mtc::zval& z, const D& d, const char* braces = "[]" )
  {
    switch ( z.get_type() )
    {
      case mtc::zval::z_char:    return mtc::json::Print( o, *z.get_char() );
      case mtc::zval::z_byte:    return mtc::json::Print( o, *z.get_byte() );
      case mtc::zval::z_int16:   return mtc::json::Print( o, *z.get_int16() );
      case mtc::zval::z_word16:  return mtc::json::Print( o, *z.get_word16() );
      case mtc::zval::z_int32:   return mtc::json::Print( o, *z.get_int32() );
      case mtc::zval::z_word32:  return mtc::json::Print( o, *z.get_word32() );
      case mtc::zval::z_int64:   return mtc::json::Print( o, *z.get_int64() );
      case mtc::zval::z_word64:  return mtc::json::Print( o, *z.get_word64() );
      case mtc::zval::z_float:   return mtc::json::Print( o, *z.get_float() );
      case mtc::zval::z_double:  return mtc::json::Print( o, *z.get_double() );
      case mtc::zval::z_bool:    return mtc::json::Print( o, *z.get_bool() );
      case mtc::zval::z_uuid:    return mtc::json::Print( o, *z.get_uuid() );

      case mtc::zval::z_charstr: return mtc::json::Print( o, *z.get_charstr() );
      case mtc::zval::z_widestr: return mtc::json::Print( o, *z.get_widestr() );

      case mtc::zval::z_zmap:       return PrintVal( o, *z.get_zmap(), d );
      case mtc::zval::z_array_zval: return PrintVal( o, *z.get_array_zval(), d, braces );

      default:  assert( false );  abort();  return o;
    }
  }

  template <class O, class D>
  O*  PrintVal( O* o, const mtc::array_zval& a, const D& decorate, const char* braces )
  {
    auto  ptop = a.begin();
    auto  pend = a.end();
    auto  deco = D( decorate );

  // chekc if list is realy zmap
    if ( auto pmap = a.size() == 1 ? ptop->get_zmap() : nullptr; pmap != nullptr )
      return PrintVal( o, *pmap, decorate );

    for ( o = ::Serialize( o, braces[0] ); o != nullptr && ptop != pend; ++ptop )
    {
      if ( ptop != a.begin() )
        o = ::Serialize( o, ',' );
      o = PrintVal( deco.Shift( deco.Break( o ) ), *ptop, deco );
    }
    return ::Serialize( decorate.Shift( decorate.Break( o ) ), braces[1] );
  }

  template <class O, class D>
  O*  PrintVal( O* o, const mtc::zmap& a, const D& decorate )
  {
    auto  pbeg = a.begin();

    if ( pbeg == a.end() )
      return o;

    if ( pbeg->first == mtc::zmap::key{ "\x1", 1 } )
      return PrintVal( o, pbeg->second, decorate, "{}" );

    return PrintVal( decorate.Space( ::Serialize( PrintKey( o, pbeg->first ), ':' ) ),
      pbeg->second, decorate );
  }

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

      mtc::json::Print( stdout, docset.front().get_array_zval( "quote", {} ), mtc::json::print::decorated() );

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
