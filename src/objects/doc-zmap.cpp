# include "../toolset/object-zmap.hpp"
# include <moonycode/codes.h>

namespace palmira {

  class ZmapTag final: public DeliriX::IText
  {
    mtc::zval&  val;

    implement_lifetime_control

  public:
    ZmapTag( mtc::zval& ref ): val( ref ) {}

    auto  AddMarkupTag( const std::string_view&, const markup_attribute& ) -> mtc::api<IText> override;
    auto  AddParagraph( const DeliriX::Paragraph& ) -> DeliriX::Paragraph override;

  };

  class ZmapArr final: public DeliriX::IText
  {
    mtc::array_zval&  output;

    implement_lifetime_control

  public:
    ZmapArr( mtc::array_zval& zva ): output( zva ) {}

  public:
    auto  AddMarkupTag( const std::string_view&, const markup_attribute& ) -> mtc::api<IText> override;
    auto  AddParagraph( const DeliriX::Paragraph& ) -> DeliriX::Paragraph override;

  };

  auto  UtfStr( const DeliriX::Paragraph& para ) -> mtc::charstr
  {
    auto  coding = para.GetEncoding();

    switch ( coding )
    {
      case uint32_t(-1):
        return codepages::widetombcs( codepages::codepage_utf8, para.GetWideStr() );
      case codepages::codepage_utf8:
        return { para.GetCharStr().data(), para.GetCharStr().size() };
      default:
        return codepages::mbcstombcs( codepages::codepage_utf8, coding, para.GetCharStr() );
    }
  }

  // ZmapTag implemenation

  auto  ZmapTag::AddMarkupTag( const std::string_view& tag, const markup_attribute& ) -> mtc::api<IText>
  {
    auto  target = mtc::zmap();
    auto  valptr = target.put( { tag.data(), tag.length() } );

    switch ( val.get_type() )
    {
      case mtc::zval::z_array_zval:
        return val.get_array_zval()->push_back( std::move( target ) ), new ZmapTag( *valptr );
      case mtc::zval::z_charstr:
      case mtc::zval::z_widestr:
      case mtc::zval::z_zmap:
        return val = mtc::array_zval{ std::move( val ), std::move( target ) }, new ZmapTag( *valptr );
      default:
        return val = std::move( target ), new ZmapTag( *valptr );
    }
  }

  auto  ZmapTag::AddParagraph( const DeliriX::Paragraph& para ) -> DeliriX::Paragraph
  {
    switch ( val.get_type() )
    {
      case mtc::zval::z_array_zval:
        val.get_array_zval()->push_back( std::move( UtfStr( para ) ) );
        break;
      case mtc::zval::z_charstr:
      case mtc::zval::z_widestr:
      case mtc::zval::z_zmap:
        val = mtc::array_zval{ std::move( val ), std::move( UtfStr( para ) ) };
        break;
      default:
        val = std::move( UtfStr( para ) );
    }
    return {};
  }

  // ZmapArr implementation

  auto  ZmapArr::AddMarkupTag( const std::string_view& tag, const markup_attribute& ) -> mtc::api<IText>
  {
    output.push_back( mtc::zmap{ { tag, mtc::zval() } } );

    return new ZmapTag( *output.back().get_zmap()->get( tag ) );
  }

  auto  ZmapArr::AddParagraph( const DeliriX::Paragraph& para ) -> DeliriX::Paragraph
  {
    output.push_back( UtfStr( para ) );
    return {};
  }

  // functions

  void  Serialize( mtc::zmap& to, const DeliriX::ITextView& doc )
  {
    auto  blocks = doc.GetBlocks();
    auto  format = doc.GetMarkup();

    if ( !blocks.empty() )
    {
      auto& output = *to.set_array_zval( "lines" );

      for ( auto& next: blocks )
      {
        if ( next.GetEncoding() != uint32_t(-1) )
        {
          output.push_back( mtc::zmap{
            { 0U, next.GetCharStr() },
            { 1U, next.GetEncoding() } } );
        }
          else
        output.push_back( next.GetWideStr() );
      }
    }

    if ( !format.empty() )
    {
      auto& output = *to.set_array_zmap( "spans" );

      for ( auto& next: format )
        output.push_back( {
          { "f", next.tagKey },
          { "l", next.uLower },
          { "h", next.uUpper } } );
    }
  }

  void  FetchFrom( const mtc::zmap& from, DeliriX::Text& doc )
  {
    auto  plines = from.get_array_zval( "lines" );
    auto  pspans = from.get_array_zmap( "spans" );

    if ( plines != nullptr )
      for ( auto& next: *plines )
      {
        switch ( next.get_type() )
        {
          case mtc::zval::z_widestr:
            doc.AddBlock( *next.get_widestr() );
            break;
          case mtc::zval::z_zmap:
            doc.AddBlock(
              next.get_zmap()->get_word32( 1U, 0 ),
              next.get_zmap()->get_charstr( 0U, "" ) );
            break;
          default:
            throw std::invalid_argument( "invalid line struct passed" );
        }
      }
    if ( pspans != nullptr )
    {
      auto& mkup = doc.GetMarkup();

      for ( auto& next: *pspans )
      {
        mkup.push_back( {
          next.get_charstr( "f", "" ),
          next.get_word32 ( "l", 0 ),
          next.get_word32 ( "h", 0 ) } );
      }
    }
  }

  auto  ZmapAsText( mtc::array_zval& out ) -> mtc::api<DeliriX::IText>
  {
    return new ZmapArr( out );
  }

}
