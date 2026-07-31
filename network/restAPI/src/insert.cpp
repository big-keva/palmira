# include "insert.hpp"
# include <reports.hpp>
# include <simdjson.h>
#include <moonycode/codes.h>

namespace restAPI
{

  void  Load( simdjson::ondemand::value, mtc::charstr& );
  void  Load( simdjson::ondemand::value, mtc::zmap& );
  void  Load( simdjson::ondemand::value, mtc::zval& );
  void  Load( simdjson::ondemand::value, uint64_t& );
  void  Load( simdjson::ondemand::value, std::function<mtc::api<DeliriX::IText>()> );

  extern
  void  Dump( Responder::Response*, const palmira::UpdateReport& );

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
        if ( key == "id" )        Load( field.value(), out.objectId );
          else
        throw std::invalid_argument( "unexpected field '" + std::string( key ) + "'" );
      }
      return out;
    }
    throw std::invalid_argument( "Invalid JSON object" );
  }

  template <class S>
  auto  Load( S* s ) -> palmira::InsertArgs
  {
    throw std::runtime_error( "Not implemented" );
  }

 /*
  * При получении запроса:
  *   - быстрый парсинг;
  *   - формирование входных данных;
  *   - либо выполнение сразу, либо постановка в очередь на обработку.
  */
  void  Insert::ready()
  {
    if ( *cancel )
      return;

   /*
    * object body contains:
    *   - 'metadata' as structure,
    *   - 'document' as structure
    *   - 'version' as uint64_t, and
    *   - update 'condition' as serialized zvalue
    */
    try
    {
      auto  parser = simdjson::ondemand::parser();
      auto  buforg = buffer->data();
      auto  params = mtc::zmap();
      auto  insert = std::make_shared<palmira::InsertArgs>();
      auto  xspace = mtc::charstr();

    // Проверить тип входящих данных:
    //  - json
    //  - zmap
    //  - tags
      if ( cttype.find( "application/json" ) != cttype.npos )
      {
        insert = std::make_shared<palmira::InsertArgs>(
          Load( parser.iterate( buforg, bufptr - buforg, buffer->size() ).value() ) );
      }
        else
      if ( cttype.find( "application/octet-stream" ) != cttype.npos )
      {
        insert = std::make_shared<palmira::InsertArgs>(
          Load( mtc::sourcebuf( buforg, bufptr - buforg ).ptr() ) );
      }

    // Проверить, что хотя бы тело документа или метаданные заданы
      if ( insert->metadata.empty() && insert->textview.GetLength() == 0 )
        throw std::invalid_argument( "Neither 'document' nor 'metadata' provided" );

     /*
      * Если есть route params, использовать их, должно быть два
      */
      switch ( zroute.size() )
      {
        case 2:   insert->objectId = zroute[1]; [[fallthrough]];
        case 1:   xspace = zroute[0];           [[fallthrough]];
        case 0:   break;
        default:  throw std::logic_error( "Invalid route arguments count" );
      }

     /*
      * Если идентификатор не задан, проверить в body
      */
      if ( insert->objectId.empty() && zquery.get_charstr( "id" ) != nullptr )
        insert->objectId = *zquery.get_charstr( "id" );

    // если всё ещё не определён, это ошибка
      if ( insert->objectId.empty() )
        throw std::invalid_argument( "No document id provided" );

     /*
      * Если не определён thread Pool, выполнить запрос непосредственно здесь, в обработчике,
      * и, если время не истекло, вернуть его результат
      */
      if ( thPool == nullptr )
      {
        auto  report = palmira::UpdateReport(
          search->Insert( *insert )->Wait( tm_off ) );

        return *cancel ? (void)NULL : Dump( answer.ptr(), report );
      }

      /*
       * Иначе зарегистрировать асинхронный обработчик запроса, который сделает то же самое,
       * но только отложенным образом.
       */
      thPool->Insert( [functor = *this, insert]()
      {
        if ( !*functor.cancel )
        {
          auto  report = palmira::UpdateReport(
            functor.search->Insert( *insert )->Wait( functor.tm_off ) );

          functor.evLoop->defer( [answer = functor.answer, cancel = functor.cancel, report]()
            {  return *cancel ? (void)NULL : Dump( answer.ptr(), report );  } );
        }
      } );
    }
    catch ( const simdjson::simdjson_error& jx )
    {
      return Instant( "401 Bad Request", {
        { "status", palmira::Status( EINVAL, "Invalid request format!" ) },
        { "reason", mtc::strprintf( "Json error: %s", jx.what() ) } } );
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

  void  Load( simdjson::ondemand::value val, mtc::charstr& str )
  {
    switch ( val.type() )
    {
      case simdjson::ondemand::json_type::string:
        return void(str = mtc::charstr( val.get_string().value() ));
      case simdjson::ondemand::json_type::number:
        return void(str = val.raw_json_token());
      default:
        throw std::invalid_argument( "Invalid 'condition' value format" );
    }
  }

  void  Load( simdjson::ondemand::value val, std::function<mtc::api<DeliriX::IText>()> add )
  {
    switch ( val.type() )
    {
      case simdjson::ondemand::json_type::array:
      {
        auto  arr = val.get_array().value();
        auto  tag = mtc::api<DeliriX::IText>();

        for ( auto element: arr )
        {
          if ( element.value().type() == simdjson::ondemand::json_type::object )
          {
            Load( element.value(), [&]()
            {
              if ( tag == nullptr )
                tag = add();
              return tag->AddMarkupTag( { "\x1", 1 } );
            } );
          }
            else
          {
            Load( element.value(), [&]()
              {  return tag != nullptr ? tag : tag = add();  } );
          }
        }
        break;
      }
      case simdjson::ondemand::json_type::object:
      {
        auto  obj = val.get_object().value();
        auto  tag = mtc::api<DeliriX::IText>();
        auto  mkx = mtc::api<DeliriX::IText>();

        for ( auto field: obj )
        {
          auto  key = field.unescaped_key().value();

          Load( field.value(), [&]()
            {
              if ( tag == nullptr )
                tag = add();
              if ( mkx == nullptr )
                mkx = tag->AddMarkupTag( { "\x1", 1 } );
              return mkx->AddMarkupTag( key );
            } );
        }
        break;
      }
      case simdjson::ondemand::json_type::string:
      {
        auto  str = val.get_string().value();

        if ( !str.empty() )
          add()->AddString( codepages::mbcstowide( codepages::codepage_utf8, str ) );
        break;
      }
      case simdjson::ondemand::json_type::number:
      {
        simdjson::ondemand::number  num = val.get_number();

        if ( num.is_uint64() ) add()->AddNumber( num.get_uint64() );
          else
        if ( num.is_int64() )  add()->AddNumber( num.get_int64() );
          else
        if ( num.is_double() ) add()->AddNumber( num.get_double() );
          else
        throw std::invalid_argument( "unexpected number type" );
        break;
      }
      case simdjson::ondemand::json_type::boolean:
      {
        add()->AddString( DeliriX::IText::persistent, val.get_bool().value() ? u"true" : u"false" );
        break;
      }
      case simdjson::ondemand::json_type::null:
      {
        add()->AddString( u"NULL" );
        break;
      }
    }
  }

}
