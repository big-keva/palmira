# include "messages.hpp"
# include "../../../reports.hpp"
# include <structo/compat.hpp>
# include <moonycode/codes.h>

namespace grpcapi
{

  using namespace palmira;

  template <class S>
  auto  Element( const S& src ) -> const S& {  return src;  }

  auto  Element( const grpcttp::Metadata& src ) -> mtc::zmap
  {
    mtc::zmap out;
    return Convert( out, src );
  }

  auto  Element( const grpcttp::MetaValue& src ) -> mtc::zval
  {
    mtc::zval out;
    return Convert( out, src );
  }

  auto  Element( const mtc::widestr& src ) -> std::string
  {
    return codepages::widetombcs( codepages::codepage_utf8, src );
  }

  auto  Element( const mtc::zmap& src ) -> grpcttp::Metadata
  {
    grpcttp::Metadata out;
    return std::move( Convert( out, src ) );
  }

  template <class T, class Iterable>
  void  Convert( std::vector<T>& out, const Iterable& src )
  {
    out.reserve( src.size() );

    for ( auto& element: src )
      out.emplace_back( Element( element ) );
  }

  template <class Iterable, class T>
  void  Convert( Iterable& out, const std::vector<T>& src )
  {
    for ( auto& element: src )
      out.add_value( Element( element ) );
  }

  void  Convert( grpcttp::array_Metadata& out, const std::vector<mtc::zmap>& src )
  {
    for ( auto& element: src )
      Convert( *out.add_value(), element );
  }

  void  Convert( grpcttp::array_Metavalue& out, const std::vector<mtc::zval>& src )
  {
    for ( auto& element: src )
      Convert( *out.add_value(), element );
  }

  // zval from Metavalue

  auto  Convert( mtc::zval& out, const grpcttp::MetaValue& src ) -> mtc::zval&
  {
    switch ( src.value_type_case() )
    {
      case grpcttp::MetaValue::ValueTypeCase::kAsBool:
        return out = src.as_bool();
      case grpcttp::MetaValue::ValueTypeCase::kAsInt32:
        return out = src.as_int32();
      case grpcttp::MetaValue::ValueTypeCase::kAsInt64:
        return out = src.as_int64();
      case grpcttp::MetaValue::ValueTypeCase::kAsUint32:
        return out = src.as_uint32();
      case grpcttp::MetaValue::ValueTypeCase::kAsUint64:
        return out = src.as_uint64();
      case grpcttp::MetaValue::ValueTypeCase::kAsFloat:
        return out = src.as_float();
      case grpcttp::MetaValue::ValueTypeCase::kAsDouble:
        return out = src.as_double();
      case grpcttp::MetaValue::ValueTypeCase::kAsString:
        return out = src.as_string();
      case grpcttp::MetaValue::ValueTypeCase::kAsMetadata:
        return Convert( *out.set_zmap(), src.as_metadata() ), out;

      case grpcttp::MetaValue::ValueTypeCase::kAsArrayInt32:
        return Convert( *out.set_array_int32(), src.as_array_int32().value() ), out;
      case grpcttp::MetaValue::ValueTypeCase::kAsArrayInt64:
        return Convert( *out.set_array_int64(), src.as_array_int64().value() ), out;
      case grpcttp::MetaValue::ValueTypeCase::kAsArrayUint32:
        return Convert( *out.set_array_word32(), src.as_array_uint32().value() ), out;
      case grpcttp::MetaValue::ValueTypeCase::kAsArrayUint64:
        return Convert( *out.set_array_word64(), src.as_array_uint64().value() ), out;
      case grpcttp::MetaValue::ValueTypeCase::kAsArrayFloat:
        return Convert( *out.set_array_float(), src.as_array_float().value() ), out;
      case grpcttp::MetaValue::ValueTypeCase::kAsArrayDouble:
        return Convert( *out.set_array_double(), src.as_array_double().value() ), out;
      case grpcttp::MetaValue::ValueTypeCase::kAsArrayString:
        return Convert( *out.set_array_charstr(), src.as_array_string().value() ), out;
      case grpcttp::MetaValue::ValueTypeCase::kAsArrayMetadata:
        return Convert( *out.set_array_zmap(), src.as_array_metadata().value() ), out;
      case grpcttp::MetaValue::ValueTypeCase::kAsArrayMetavalue:
        return Convert( *out.set_array_zval(), src.as_array_metavalue().value() ), out;

      default:
        throw std::invalid_argument( "Unknown value type" );
    }
  }

  // Metavalue from zval

  auto  Convert( grpcttp::MetaValue& out, const mtc::zval& src ) -> grpcttp::MetaValue&
  {
    switch ( src.get_type() )
    {
      case mtc::zval::z_type::z_char:
        return out.set_as_int32( *src.get_char() ), out;
      case mtc::zval::z_type::z_byte:
        return out.set_as_uint32( *src.get_byte() ), out;
      case mtc::zval::z_type::z_int16:
        return out.set_as_int32( *src.get_int16() ), out;
      case mtc::zval::z_type::z_word16:
        return out.set_as_uint32( *src.get_word16() ), out;
      case mtc::zval::z_type::z_int32:
        return out.set_as_int32( *src.get_int32() ), out;
      case mtc::zval::z_type::z_word32:
        return out.set_as_uint32( *src.get_word32() ), out;
      case mtc::zval::z_type::z_int64:
        return out.set_as_int64( *src.get_int64() ), out;
      case mtc::zval::z_type::z_word64:
        return out.set_as_uint32( *src.get_word64() ), out;
      case mtc::zval::z_type::z_float:
        return out.set_as_float( *src.get_float() ), out;
      case mtc::zval::z_type::z_double:
        return out.set_as_double( *src.get_double() ), out;
      case mtc::zval::z_type::z_bool:
        return out.set_as_bool( *src.get_bool() ), out;
      case mtc::zval::z_type::z_charstr:
        return *out.mutable_as_string() = *src.get_charstr() , out;
      case mtc::zval::z_type::z_widestr:
        return *out.mutable_as_string() = codepages::widetombcs( codepages::codepage_utf8, *src.get_widestr() ), out;
      case mtc::zval::z_type::z_zmap:
        return Convert( *out.mutable_as_metadata(), *src.get_zmap() ) , out;

      case mtc::zval::z_type::z_array_char:
        return Convert( *out.mutable_as_array_int32(), *src.get_array_char() ), out;
      case mtc::zval::z_type::z_array_byte:
        return Convert( *out.mutable_as_array_uint32(), *src.get_array_byte() ), out;
      case mtc::zval::z_type::z_array_int16:
        return Convert( *out.mutable_as_array_int32(), *src.get_array_int16() ), out;
      case mtc::zval::z_type::z_array_word16:
        return Convert( *out.mutable_as_array_uint32(), *src.get_array_word16() ), out;
      case mtc::zval::z_type::z_array_int32:
        return Convert( *out.mutable_as_array_int32(), *src.get_array_int32() ), out;
      case mtc::zval::z_type::z_array_word32:
        return Convert( *out.mutable_as_array_uint32(), *src.get_array_word32() ), out;
      case mtc::zval::z_type::z_array_int64:
        return Convert( *out.mutable_as_array_int32(), *src.get_array_int64() ), out;
      case mtc::zval::z_type::z_array_word64:
        return Convert( *out.mutable_as_array_uint32(), *src.get_array_word64() ), out;
      case mtc::zval::z_type::z_array_float:
        return Convert( *out.mutable_as_array_int32(), *src.get_array_float() ), out;
      case mtc::zval::z_type::z_array_double:
        return Convert( *out.mutable_as_array_int32(), *src.get_array_double() ), out;
      case mtc::zval::z_type::z_array_charstr:
        return Convert( *out.mutable_as_array_string(), *src.get_array_charstr() ), out;
      case mtc::zval::z_type::z_array_widestr:
        return Convert( *out.mutable_as_array_string(), *src.get_array_widestr() ), out;
      case mtc::zval::z_type::z_array_zmap:
        return Convert( *out.mutable_as_array_metadata(), *src.get_array_zmap() ), out;
      case mtc::zval::z_type::z_array_zval:
        return Convert( *out.mutable_as_array_metavalue(), *src.get_array_zval() ), out;

      default:
        throw std::invalid_argument( "Unknown value type" );
    }
  }

  // Metadata from zmap

  auto  Convert( grpcttp::Metadata& out, const mtc::zmap& src ) -> grpcttp::Metadata&
  {
    for ( auto next: src )
    {
      if ( next.first.is_charstr() )
        Convert( (*out.mutable_data())[next.first.to_charstr()], next.second );
      else throw std::invalid_argument( "Invalid value type" );
    }
    return out;
  }

  // zmap from Metadata

  auto  Convert( mtc::zmap& out, const grpcttp::Metadata& src ) -> mtc::zmap&
  {
    for ( auto next: src.data() )
      Convert( *out.put( next.first ), next.second );
    return out;
  }

  // Document form ITextView

  class TextOnDocument final: public DeliriX::IText
  {
    implement_lifetime_control

    grpcttp::Document& doc;

  public:
    TextOnDocument( grpcttp::Document& out ): doc( out ) {}

    auto  AddMarkupTag( const std::string_view& tag, const markup_attribute& ) -> mtc::api<IText> override
    {
      auto  mkup = doc.add_elements()->mutable_mkup();
        mkup->set_tag( tag );
      return new TextOnDocument( *mkup->mutable_items() );
    }
    auto  AddParagraph( const DeliriX::Paragraph& txt ) -> DeliriX::Paragraph override
    {
      switch ( txt.GetEncoding() )
      {
        case uint32_t(-1):
          *doc.add_elements()->mutable_text() = codepages::widetombcs( codepages::codepage_utf8, txt.GetWideStr() );
          return {};
        case codepages::codepage_1251:
        case codepages::codepage_1252:
        case codepages::codepage_1254:
        case codepages::codepage_koi8:
        case codepages::codepage_866 :
        case codepages::codepage_iso :
        case codepages::codepage_mac :
          *doc.add_elements()->mutable_text() = codepages::mbcstombcs( codepages::codepage_utf8, txt.GetEncoding(), txt.GetCharStr() );
          return {};
        case codepages::codepage_utf8:
          *doc.add_elements()->mutable_text() = txt.GetCharStr();
          return {};
        default:
          throw std::invalid_argument( "Unknown encoding" );
      }
    }
  };

  auto  Convert( grpcttp::Document& out, const DeliriX::ITextView& src ) -> grpcttp::Document&
  {
    auto  doc = mtc::api( new TextOnDocument( out ) );

    return (void)::Serialize( (DeliriX::IText*)doc, src ), out;
  }

  // IText from Document

  auto  Convert( DeliriX::IText& out, const grpcttp::Document& src ) -> DeliriX::IText&
  {
    for ( auto& next: src.elements() )
    {
      if ( next.has_text() )
      {
        out.AddBlock( codepages::codepage_utf8, next.text() );
      }
        else
      if ( next.has_mkup() && next.mkup().has_items() )
      {
        Convert( *out.AddMarkupTag( next.mkup().tag() ).ptr(), next.mkup().items() );
      }
    }
    return out;
  }

  // Request arguments

  auto  Convert( palmira::RemoveArgs& out, const grpcttp::RemoveArgs& src ) -> palmira::RemoveArgs&
  {
    if ( (out.objectId = src.objectid()).empty() )
      throw std::invalid_argument( "objectId not defined @" __FILE__ ":" LINE_STRING );
    return out;
  }

  auto  Convert( palmira::UpdateArgs& out, const grpcttp::UpdateArgs& src ) -> palmira::UpdateArgs&
  {
    if ( src.has_metadata() )
      Convert( out.metadata, src.metadata() );
    if ( (out.objectId = src.objectid()).empty() )
      throw std::invalid_argument( "objectId not defined @" __FILE__ ":" LINE_STRING );
    return out;
  }

  auto  Convert( palmira::InsertArgs& out, const grpcttp::InsertArgs& src ) -> palmira::InsertArgs&
  {
    if ( src.has_document() )
      Convert( out.GetTextAPI(), src.document() );
    if ( src.has_metadata() )
      Convert( out.metadata, src.metadata() );
    if ( (out.objectId = src.objectid()).empty() )
      throw std::invalid_argument( "objectId not defined @" __FILE__ ":" LINE_STRING );
    return out;
  }

  auto  Convert( grpcttp::RemoveArgs& out, const palmira::RemoveArgs& src ) -> grpcttp::RemoveArgs&
  {
    if ( src.objectId.empty() )
      throw std::invalid_argument( "objectId not defined @" __FILE__ ":" LINE_STRING );
    out.set_objectid( src.objectId );
    return out;
  }

  auto  Convert( grpcttp::UpdateArgs& out, const palmira::UpdateArgs& src ) -> grpcttp::UpdateArgs&
  {
    if ( src.objectId.empty() )
      throw std::invalid_argument( "objectId not defined @" __FILE__ ":" LINE_STRING );
    if ( !src.metadata.empty() )
      Convert( *out.mutable_metadata(), src.metadata );
    out.set_objectid( src.objectId );
    return out;
  }

  auto  Convert( grpcttp::InsertArgs& out, const palmira::InsertArgs& src ) -> grpcttp::InsertArgs&
  {
    if ( src.objectId.empty() )
      throw std::invalid_argument( "objectId not defined @" __FILE__ ":" LINE_STRING );
    if ( !src.metadata.empty() )
      Convert( *out.mutable_metadata(), src.metadata );
    if ( src.textview.GetLength() != 0 )
      Convert( *out.mutable_document(), src.textview );
    out.set_objectid( src.objectId );
    return out;
  }

  // Reports

  auto  Convert( grpcttp::UpdateReport& out, const mtc::zmap& src ) -> grpcttp::UpdateReport&
  {
    auto  rep = palmira::UpdateReport( src );

    out.mutable_status()->set_code( rep.status().code() );
    out.mutable_status()->set_info( rep.status().info() );

    if ( rep.get_zmap( "metadata" ) != nullptr )
      Convert( *out.mutable_metadata(), *rep.get_zmap( "metadata" ) );

    return out;
  }

  auto  Convert( mtc::zmap& out, grpcttp::UpdateReport& src ) -> mtc::zmap&
  {
    if ( src.has_status() )
    {
      out["status"] = mtc::zmap{
        { "code", int32_t(src.status().code()) },
        { "info", src.status().info() } };
    }

    if ( src.has_metadata() )
      Convert( *out.set_zmap( "metadata" ), src.metadata() );

    return out;
  }

}
