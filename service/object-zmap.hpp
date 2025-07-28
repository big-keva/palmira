# if !defined( __palmira_service_object_zmap_hpp__ )
# define __palmira_service_object_zmap_hpp__
# include "DelphiX/textAPI/DOM-text.hpp"
# include <mtc/zmap.h>

namespace palmira {

  template <class Allocator>
  void  Serialize( mtc::zmap& to, const DelphiX::textAPI::BaseDocument<Allocator>& doc )
  {
    auto& blocks = doc.GetBlocks();
    auto& format = doc.GetMarkup();

    if ( !blocks.empty() )
    {
      auto& output = *to.set_array_zval( "lines" );

      for ( auto& next: blocks )
      {
        if ( next.encode != unsigned(-1) )
        {
          output.push_back( mtc::zmap{
            { 0U, next.GetCharStr() },
            { 1U, next.encode } } );
        }
          else
        output.push_back( next.GetWideStr() );
      }
    }

    if ( !format.empty() )
    {
      auto& output = *to.set_array_zmap( "spans" );

      for ( auto& next: blocks )
        output.emplace_back( {
          { "f", next.format },
          { "l", next.uLower },
          { "h", next.uUpper } } );
    }
  }

  template <class Allocator>
  void  FetchFrom( const mtc::zmap& from, DelphiX::textAPI::BaseDocument<Allocator>& doc )
  {
    auto  plines = from.get_array_zval( "lines" );
    auto  pspans = from.get_array_zmap( "spans" );

    doc.clear();

    if ( plines != nullptr )
      for ( auto& next: *plines )
      {
        switch ( next.get_type() )
        {
          case mtc::zval::z_widestr:
            doc.AddString( *next.get_widestr() );
            break;
          case mtc::zval::z_zmap:
            doc.AddString(
              next.get_zmap()->get_charstr( 0U, "" ),
              next.get_zmap()->get_word32( 1U, 0 ) );
            break;
          default:
            throw std::invalid_argument( "invalid line struct passed" );
        }
      }
    if ( pspans != nullptr )
      for ( auto& next: *pspans )
      {
        doc.AddFormat(
          next.get_charstr( "f", "" ),
          next.get_word32 ( "l", 0 ),
          next.get_word32 ( "h", 0 ) );
      }
  }

}

# endif   // !__palmira_service_object_zmap_hpp__
