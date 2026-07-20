# include "insert-entity.hpp"
# include <reports.hpp>
# include <simdjson.h>

namespace restAPI
{

  void  Load( simdjson::ondemand::value, mtc::zmap& );
  void  Load( simdjson::ondemand::value, mtc::zval& );
  void  Load( simdjson::ondemand::value, uint64_t& );
  void  Load( simdjson::ondemand::value, std::function<mtc::api<DeliriX::IText>()> );

  auto  Load( simdjson::ondemand::value val ) -> palmira::InsertArgs
  {
    if ( val.type() == simdjson::ondemand::json_type::object )
    {
      auto  obj = val.get_object().value();
      auto  out = palmira::InsertArgs();

      for ( auto field: obj )
      {
        auto  key = field.unescaped_key().value();

        if ( key == "document" )  Load( field.value(), [&]() -> mtc::api<DeliriX::IText> {  return &out.GetTextAPI();  } );
          else
        if ( key == "metadata" )  Load( field.value(), out.metadata );
          else
        if ( key == "condition" ) Load( field.value(), out.ifClause );
          else
        if ( key == "version" )   Load( field.value(), out.uVersion );
          else
        throw std::invalid_argument( "unexpected field '" + std::string( key ) + "'" );
      }
      return out;
    }
    throw std::invalid_argument( "Invalid JSON object" );
  }

  void  InsertEntity::ready()
  {
    auto  insert = [functor = *this]()
    {
      auto  parser = simdjson::ondemand::parser();
      auto  buforg = functor.buffer->data();
      auto  source = parser.iterate( buforg, functor.bufptr - buforg, functor.buffer->size() );
      auto  inText = DeliriX::Text();
      auto  update = palmira::UpdateReport();
      auto  errmsg = std::string();

      if ( *functor.cancel )
        return;

      try
      {
       /*
        * object body contains:
        *   - 'metadata' as structure,
        *   - 'document' as structure
        *   - 'version' as uint64_t, and
        *   - update 'condition' as serialized zvalue
        */
        update = functor.search->Insert( SetId( Load( source.value() ), functor.docId ) )
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
      catch ( const std::exception& xp )  {  errmsg = xp.what();  }
      catch ( ... )                       {  errmsg = "unexpected exception";  }

      functor.evLoop->defer( [functor, errmsg]()
      {
        if ( !*functor.cancel )
        {
          auto  resp = functor.answer
            ->WriteStatus( "400 Bad Request" )
            ->WriteHeader( "Content-Type", "application/json; charset=utf-8" );
          auto  base = mtc::json::print::decorated();
          auto  deco = mtc::json::print::decorated( base );

          base.Break( ::Serialize( resp, '{' ) );

          PrintKey( resp, "error", deco );

          base.Break( mtc::json::Print( base.Space( ::Serialize( resp, ':' ) ),
            errmsg, deco ) );

          *functor.cancel = true, ::Serialize( resp, '}' )->FinishWrite();
        }
      } );
    };

    return thPool != nullptr ? (void)thPool->Insert( insert ) : (void)insert();
  }

  namespace metadata
  {
    auto  Load( simdjson::ondemand::value val ) -> mtc::zval
    {
      mtc::zval get;

      switch ( val.type() )
      {
        case simdjson::ondemand::json_type::array:
        {
          auto& out = *get.set_array_zval();
          auto  arr = val.get_array().value();

          for ( auto element: arr )
            out.push_back( Load( element.value() ) );
          break;
        }
        case simdjson::ondemand::json_type::object:
        {
          auto& out = *get.set_zmap();
          auto  obj = val.get_object().value();

          for ( auto field: obj )
            out.put( field.unescaped_key().value(), Load( field.value() ) );
          break;
        }
        case simdjson::ondemand::json_type::string:
        {
          get.set_charstr( std::string( val.get_string().value() ) );
          break;
        }
        case simdjson::ondemand::json_type::number:
        {
          simdjson::ondemand::number  number = val.get_number();

          if ( number.is_uint64() ) get.set_word64( number.get_uint64() );
            else
          if ( number.is_int64() )  get.set_int64( number.get_int64() );
            else
          if ( number.is_double() ) get.set_double( number.get_double() );
            else
          throw std::invalid_argument( "unexpected number type" );
          break;
        }
        case simdjson::ondemand::json_type::boolean:
        {
          get.set_bool( val.get_bool().value() );
          break;
        }
        default:
          throw std::invalid_argument( "unexpected json type" );
      }
      return get;
    }
  }

  void  Load( simdjson::ondemand::value val, mtc::zmap& out )
  {
    if ( val.type() == simdjson::ondemand::json_type::object )
    {
      auto  obj = val.get_object().value();

      for ( auto field: obj )
        out.put( field.unescaped_key().value(), metadata::Load( field.value() ) );
    }
      else
    throw std::invalid_argument( "unexpected 'metadata' json type" );
  }

  void  Load( simdjson::ondemand::value val, mtc::zval& con )
  {
    switch ( val.type() )
    {
      case simdjson::ondemand::json_type::string:
        return void(con = mtc::charstr( val.get_string().value() ));
      case simdjson::ondemand::json_type::boolean:
        return void(con = val.get_bool().value());
      case simdjson::ondemand::json_type::null:
        return void(con.clear());
      default:
        throw std::invalid_argument( "Invalid 'condition' value format" );
    }
  }

  void  Load( simdjson::ondemand::value val, uint64_t& ver )
  {
    switch ( val.type() )
    {
      case simdjson::ondemand::json_type::null:
        return void(ver = 0);

      case simdjson::ondemand::json_type::number:
      {
        simdjson::ondemand::number  number = val.get_number();

        if ( number.is_uint64() )
          return (void)(ver = number.get_uint64());
        if ( number.is_int64() )
          return void(ver = number.get_int64());
      }
      [[fallthrough]];
      default:
        throw std::invalid_argument( "Invalid 'version' number format" );
    }
  }

  void  Load( simdjson::ondemand::value val, std::function<mtc::api<DeliriX::IText>()> add )
  {
    switch ( val.type() )
    {
      case simdjson::ondemand::json_type::array:
      {
        auto  arr = val.get_array().value();

        for ( auto element: arr )
          Load( element.value(), add );
        break;
      }
      case simdjson::ondemand::json_type::object:
      {
        auto  obj = val.get_object().value();
        auto  tag = mtc::api<DeliriX::IText>();

        for ( auto field: obj )
        {
          auto  key = field.unescaped_key().value();

          Load( field.value(), [&]()
            {  return (tag != nullptr ? tag : add())->AddMarkupTag( key );  });
        }
        break;
      }
      case simdjson::ondemand::json_type::string:
      {
        auto  str = val.get_string().value();

        if ( !str.empty() )
          add()->AddBlock( DeliriX::IText::persistent, str );
        break;
      }
      case simdjson::ondemand::json_type::number:
      {
        auto str = val.raw_json_token();

        if ( !str.empty() )
          add()->AddBlock( DeliriX::IText::persistent, str );
        break;
      }
      case simdjson::ondemand::json_type::boolean:
      {
        add()->AddBlock( DeliriX::IText::persistent, val.get_bool().value() ? "true" : "false" );
        break;
      }
      case simdjson::ondemand::json_type::null:
      {
        add()->AddBlock( DeliriX::IText::persistent, "NULL" );
        break;
      }
    }
  }

}
