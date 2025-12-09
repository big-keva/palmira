# include "../messages.hpp"
# include <moonycode/codes.h>

namespace grpcttp {

  using namespace palmira;

  template <class S>
  auto  Element( const S& src ) -> const S& {  return src;  }

  auto  Element( const Metadata& src ) -> mtc::zmap
  {
    mtc::zmap out;
    return Convert( out, src );
  }

  auto  Element( const MetaValue& src ) -> mtc::zval
  {
    mtc::zval out;
    return Convert( out, src );
  }

  auto  Element( const mtc::widestr& src ) -> std::string
  {
    return codepages::widetombcs( codepages::codepage_utf8, src );
  }

  auto  Element( const mtc::zmap& src ) -> Metadata
  {
    Metadata out;
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

  void  Convert( array_Metadata& out, const std::vector<mtc::zmap>& src )
  {
    for ( auto& element: src )
      Convert( *out.add_value(), element );
  }

  void  Convert( array_Metavalue& out, const std::vector<mtc::zval>& src )
  {
    for ( auto& element: src )
      Convert( *out.add_value(), element );
  }

  // zval from Metavalue

  auto  Convert( mtc::zval& out, const MetaValue& src ) -> mtc::zval&
  {
    switch ( src.value_type_case() )
    {
      case MetaValue::ValueTypeCase::kAsBool:
        return out = src.as_bool();
      case MetaValue::ValueTypeCase::kAsInt32:
        return out = src.as_int32();
      case MetaValue::ValueTypeCase::kAsInt64:
        return out = src.as_int64();
      case MetaValue::ValueTypeCase::kAsUint32:
        return out = src.as_uint32();
      case MetaValue::ValueTypeCase::kAsUint64:
        return out = src.as_uint64();
      case MetaValue::ValueTypeCase::kAsFloat:
        return out = src.as_float();
      case MetaValue::ValueTypeCase::kAsDouble:
        return out = src.as_double();
      case MetaValue::ValueTypeCase::kAsString:
        return out = src.as_string();
      case MetaValue::ValueTypeCase::kAsMetadata:
        return Convert( *out.set_zmap(), src.as_metadata() ), out;

      case MetaValue::ValueTypeCase::kAsArrayInt32:
        return Convert( *out.set_array_int32(), src.as_array_int32().value() ), out;
      case MetaValue::ValueTypeCase::kAsArrayInt64:
        return Convert( *out.set_array_int64(), src.as_array_int64().value() ), out;
      case MetaValue::ValueTypeCase::kAsArrayUint32:
        return Convert( *out.set_array_word32(), src.as_array_uint32().value() ), out;
      case MetaValue::ValueTypeCase::kAsArrayUint64:
        return Convert( *out.set_array_word64(), src.as_array_uint64().value() ), out;
      case MetaValue::ValueTypeCase::kAsArrayFloat:
        return Convert( *out.set_array_float(), src.as_array_float().value() ), out;
      case MetaValue::ValueTypeCase::kAsArrayDouble:
        return Convert( *out.set_array_double(), src.as_array_double().value() ), out;
      case MetaValue::ValueTypeCase::kAsArrayString:
        return Convert( *out.set_array_charstr(), src.as_array_string().value() ), out;
      case MetaValue::ValueTypeCase::kAsArrayMetadata:
        return Convert( *out.set_array_zmap(), src.as_array_metadata().value() ), out;
      case MetaValue::ValueTypeCase::kAsArrayMetavalue:
        return Convert( *out.set_array_zval(), src.as_array_metavalue().value() ), out;

      default:
        throw std::invalid_argument( "Unknown value type" );
    }
  }

  // Metavalue from zval

  auto  Convert( MetaValue& out, const mtc::zval& src ) -> MetaValue&
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

  auto  Convert( Metadata& out, const mtc::zmap& src ) -> Metadata&
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

  auto  Convert( mtc::zmap& out, const Metadata& src ) -> mtc::zmap&
  {
    for ( auto next: src.data() )
      Convert( *out.put( next.first ), next.second );
    return out;
  }

  // Document form ITextView

  class TextOnDocument final: public DeliriX::IText
  {
    implement_lifetime_control

    Document& doc;

  public:
    TextOnDocument( Document& out ): doc( out ) {}

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

  auto  Convert( Document& out, const DeliriX::ITextView& src ) -> Document&
  {
    auto  doc = mtc::api( new TextOnDocument( out ) );

    return DeliriX::Serialize( doc, src ), out;
  }

  // IText from Document

  auto  Convert( DeliriX::IText& out, const Document& src ) -> DeliriX::IText&
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

  template <class O, class S>
  auto  GetObjectId( O& out, const S& src ) -> O&
  {
    if ( (out.objectId = src.objectid()).empty() )
      throw std::invalid_argument( "Undefined object_id" );
     return out;
  }

  template <class O, class S>
  auto  GetMetadata( O& out, const S& src ) -> O&
  {
    if ( src.has_metadata() )
      Convert( out.metadata, src.metadata() );
    return out;
  }

  auto  GetDocument( palmira::InsertArgs& out, const InsertArgs& src ) -> palmira::InsertArgs&
  {
    if ( src.has_document() )
      Convert( out.GetTextAPI(), src.document() );
    return out;
  }

  auto  Convert( palmira::RemoveArgs& out, const RemoveArgs& src ) -> palmira::RemoveArgs&
  {
    return GetObjectId( out, src );
  }

  auto  Convert( palmira::UpdateArgs& out, const UpdateArgs& src ) -> palmira::UpdateArgs&
  {
    return GetMetadata(
           GetObjectId( out, src ), src );
  }

  auto  Convert( palmira::InsertArgs& out, const InsertArgs& src ) -> palmira::InsertArgs&
  {
    return GetDocument(
           GetMetadata(
           GetObjectId( out, src ), src ), src );
  }

}
