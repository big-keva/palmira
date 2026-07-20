# include "insert-entity.hpp"
# include <reports.hpp>
# include <simdjson.h>

namespace restAPI
{

  auto  LoadMetadata( simdjson::ondemand::value val ) -> mtc::zmap;
  auto  LoadCondition( simdjson::ondemand::value val ) -> mtc::charstr;
  auto  LoadVersion( simdjson::ondemand::value val ) -> uint64_t;
  void  LoadDocument( simdjson::ondemand::value val, std::function<mtc::api<DeliriX::IText>()> );

  auto  LoadArguments( simdjson::ondemand::value val ) -> palmira::InsertArgs
  {

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
        update = functor.search->Insert( SetId( LoadArguments( source.value() ), functor.docId ) )
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

          if ( !functor.cancel )
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

    return thPool != nullptr ? (void)thPool->Insert( insert ) : (void)insert();
  }

  auto  LoadMetadata( simdjson::ondemand::value val ) -> mtc::zmap
  {
  }

  auto  LoadCondition( simdjson::ondemand::value val ) -> mtc::charstr
  {
  }

  auto  LoadVersion( simdjson::ondemand::value val ) -> uint64_t
  {
  }

  void  LoadDocument( simdjson::ondemand::value val, std::function<mtc::api<DeliriX::IText>()> add )
  {
    switch ( val.type() )
    {
      case simdjson::ondemand::json_type::array:
      {
        auto  arr = val.get_array().value();

        for ( auto element: arr )
          LoadDocument( element.value(), add );
        break;
      }
      case simdjson::ondemand::json_type::object:
      {
        auto  obj = val.get_object().value();
        auto  tag = mtc::api<DeliriX::IText>();

        for ( auto field: obj )
        {
          auto  key = field.unescaped_key().value();

          LoadDocument( field.value(), [&]()
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
