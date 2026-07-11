# include "loader.hpp"
# include <structo/queries/parser.hpp>
# include <remottp/src/server/rest.hpp>
# include <DeliriX/DOM-load.hpp>

namespace remoapi {
namespace json    {

  template <class Args>
  static  auto  Access( Args&, const http::Request&, const mtc::zmap& ) -> Args&;
  template <class Args>
  static  auto  Remove( Args&, const http::Request&, const mtc::zmap& ) -> Args&;
  template <class Args>
  static  auto  Update( Args&, const http::Request&, const mtc::zmap& ) -> Args&;
  static  auto  Search( palmira::SearchArgs&, const http::Request&, const mtc::zmap& ) -> palmira::SearchArgs&;
  static  auto  LoadJs( mtc::IByteStream* ) -> mtc::zmap;

  auto  Load( palmira::AccessArgs& arg, const http::Request& req, mtc::IByteStream* src ) -> palmira::AccessArgs&
  {
    return Access( arg, req, LoadJs( src ) );
  }

  auto  Load( palmira::RemoveArgs& arg, const http::Request& req, mtc::IByteStream* src ) -> palmira::RemoveArgs&
  {
    return Remove( arg, req, LoadJs( src ) );
  }

  auto  Load( palmira::UpdateArgs& arg, const http::Request& req, mtc::IByteStream* src ) -> palmira::UpdateArgs&
  {
    return Update( arg, req, LoadJs( src ) );
  }

  auto  Load( palmira::InsertArgs& arg, const http::Request& req, mtc::IByteStream* src ) -> palmira::InsertArgs&
  {
    auto  jinput = mtc::json::parse::make_source( src );
    auto  reader = mtc::json::parse::reader( jinput );
    auto  jsData = REST::JSON::ParseInput( reader, {    // parse json arguments
      { "document", [&]( mtc::json::parse::reader& in )
        {  DeliriX::load_as::Json( &arg.GetTextAPI(), [&](){  return in.getnext();  } );  } } } );

    return Update( arg, req, jsData );
  }

  auto  Load( palmira::SearchArgs& arg, const http::Request& req, mtc::IByteStream* src ) -> palmira::SearchArgs&
  {
    return Search( arg, req, LoadJs( src ) );
  }

  template <class Args>
  auto  Access( Args& arg, const http::Request& req, const mtc::zmap& jsn ) -> Args&
  {
    auto  psrcid = jsn.get_charstr( "id" );
    auto  sreqid = req.GetHeaders().get( "id" );

    if ( sreqid != "" )
      return arg.objectId = http::UriDecode( sreqid ), arg;

    if ( psrcid != nullptr )
      return arg.objectId = *psrcid, arg;

    throw std::invalid_argument( "undefined document 'id', string expected @" __FILE__ ":" LINE_STRING );
  }

  template <class Args>
  auto  Remove( Args& arg, const http::Request& req, const mtc::zmap& jsn ) -> Args&
  {
/*    auto  ifexpr = jsn.get("condition");

      if (ifexpr != nullptr)
      {
        result.check = IfExpr(*ifexpr);
      }*/
    return Access( arg, req, jsn );
  }

  template <class Args>
  auto  Update( Args& arg, const http::Request& req, const mtc::zmap& jsn ) -> Args&
  {
    auto  pmdata = jsn.get_zmap( "metadata" );

    if ( pmdata != nullptr )
      arg.metadata = *pmdata;

    return Remove( arg, req, jsn );
  }

  auto  Search( palmira::SearchArgs& search, const http::Request& req, const mtc::zmap& jsn ) -> palmira::SearchArgs&
  {
    auto  get_id = req.GetUri().parameters().get( "id", "undefined" );
    auto  sz_req = jsn.get_charstr( "string" );
    auto  ws_req = jsn.get_widestr( "string" );
    auto  zm_req = jsn.get_zmap   ( "query" );

    if ( get_id != "undefined" )
      return search.query = mtc::zmap{ { "id", get_id } }, search;

    search.order["first"] = jsn.get_int32( "first", 1 );
    search.order["count"] = jsn.get_int32( "count", 10 );

    if ( sz_req != nullptr )  search.query = structo::queries::ParseQuery( *sz_req );
      else
    if ( ws_req != nullptr )  search.query = structo::queries::ParseQuery( *ws_req );
      else
    if ( zm_req != nullptr )  search.query = *zm_req;
      else
    throw std::invalid_argument( "request contains neither 'query' nor 'string'" );

    return search;
  }

  auto  LoadJs( mtc::IByteStream* src ) -> mtc::zmap
  {
    auto  output = mtc::zmap();
    auto  revive = mtc::zmap{
      { "id",       "charstr" },
      { "metadata", mtc::zmap{ { "ctime", "word64" } } } };

    return mtc::json::Parse( src, output, revive ) , output;
  }

}}
