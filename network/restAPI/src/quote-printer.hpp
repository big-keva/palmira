# if !defined( PALMIRA_RESTAPI_QUOTE_PRINTER_HPP_ )
# define PALMIRA_RESTAPI_QUOTE_PRINTER_HPP_
# include <mtc/json.h>

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

}

# endif   // !PALMIRA_RESTAPI_QUOTE_PRINTER_HPP_
