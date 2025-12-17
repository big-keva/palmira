# include "loader.hpp"
# include <DeliriX/DOM-load.hpp>

namespace remoapi {
namespace zmap    {

  template <class Args>
  static  auto  Access( Args&, const http::Request&, const mtc::zmap& ) -> Args&;
  template <class Args>
  static  auto  Remove( Args&, const http::Request&, const mtc::zmap& ) -> Args&;
  template <class Args>
  static  auto  Update( Args&, const http::Request&, const mtc::zmap& ) -> Args&;
  template <class Args>
  static  auto  Search( Args&, const http::Request&, const mtc::zmap& ) -> Args&;
  static  auto  ZmLoad( mtc::IByteStream* ) -> mtc::zmap;

  auto  Load( palmira::AccessArgs& arg, const http::Request& req, mtc::IByteStream* src ) -> palmira::AccessArgs&
  {
    return Access( arg, req, ZmLoad( src ) );
  }

  auto  Load( palmira::RemoveArgs& arg, const http::Request& req, mtc::IByteStream* src ) -> palmira::RemoveArgs&
  {
    return Remove( arg, req, ZmLoad( src ) );
  }

  auto  Load( palmira::UpdateArgs& arg, const http::Request& req, mtc::IByteStream* src ) -> palmira::UpdateArgs&
  {
    return Update( arg, req, ZmLoad( src ) );
  }

  auto  Load( palmira::InsertArgs& arg, const http::Request& req, mtc::IByteStream* src ) -> palmira::InsertArgs&
  {
    auto  zmdata = mtc::zmap();
    auto  intext = DeliriX::Text();
    auto  dockey = decltype(zmdata.get( "document" )){};

    if ( zmdata.FetchFrom( src ) == nullptr )
      throw std::runtime_error( "Failed to load zmap from the stream @" __FILE__ ":" LINE_STRING );

    if ( (dockey = zmdata.get( "document" )) == nullptr )
      return Update( arg, req, zmdata );

    switch ( dockey->get_type() )
    {
      case mtc::zval::z_charstr:
      {
        if ( *dockey->get_charstr() == "after:dump" )
        {
          if ( intext.FetchFrom( src ) == nullptr )
            throw std::runtime_error( "Failed to load text from the stream @" __FILE__ ":" LINE_STRING );
        }
          else
        if ( *dockey->get_charstr() == "after:xml" )
        {
          DeliriX::load_as::Tags( &intext, DeliriX::load_as::MakeSource( src ) );
        }
          else
        if ( *dockey->get_charstr() == "after:json" )
        {
          DeliriX::load_as::Json( &intext, DeliriX::load_as::MakeSource( src ) );
        }
        break;
      }
      case mtc::zval::z_array_char:
      {
        auto  inbuff = mtc::sourcebuf(
          dockey->get_array_charstr()->data(),
          dockey->get_array_charstr()->size() );

        if ( intext.FetchFrom( inbuff.ptr() ) == nullptr )
          throw std::invalid_argument( "Failed to load text from the 'document' buffer @" __FILE__ ":" LINE_STRING );
        break;
      }
      default:
        throw std::invalid_argument( "Invalid 'document' image key type @" __FILE__ ":" LINE_STRING );
    }

    return Update( arg, req, zmdata ).SetDocText( std::move( intext ) );
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

  auto  ZmLoad( mtc::IByteStream* src ) -> mtc::zmap
  {
    auto  output = mtc::zmap();

    if ( output.FetchFrom( src ) == nullptr )
      throw std::invalid_argument( "invalid binary serialization format, zmap expected @" __FILE__ ":" LINE_STRING );
    return output;
  }

}}
