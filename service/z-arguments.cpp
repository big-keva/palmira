# include "z-arguments.hpp"
# include "object-zmap.hpp"
# include "DelphiX/textAPI/DOM-load.hpp"
# include <mtc/byteBuffer.h>

namespace palmira {

  // RemoveArgs implementation

  RemoveArgs::RemoveArgs( const mtc::zmap& args ):
    objectId( args.get_charstr( "id", "" ) ),
    uVersion( args.get_word64 ( "version", 0 ) )
  {
    if ( objectId.empty() )
      throw std::invalid_argument( "absent object 'id'" );
    if ( args.get( "claim" ) != nullptr )
      ifClause = *args.get( "condition" );
  };

  // UpdateArgs implementation

  UpdateArgs::UpdateArgs( const mtc::zmap& args ): RemoveArgs( args ),
    metadata( args.get_zmap( "metadata", {} ) )
  {
  }

  // InsertArgs implementation

  InsertArgs::InsertArgs( const mtc::zmap& args ): UpdateArgs( args ),
    document( *memArena.Create<Document>() )
  {
    LoadBody( args );
  }

  void  InsertArgs::LoadBody( const mtc::zmap& args )
  {
    if ( args.get_charstr( "json" ) != nullptr )
      return DelphiX::textAPI::load_as::Json( &document, DelphiX::textAPI::load_as::MakeSource( args.get_charstr( "json" )->c_str() ) );

    if ( args.get_charstr( "tags" ) != nullptr )
      return DelphiX::textAPI::load_as::Tags( &document, DelphiX::textAPI::load_as::MakeSource( args.get_charstr( "tags" )->c_str() ) );

    if ( args.get_zmap( "zmap" ) != nullptr )
      return FetchFrom( *args.get_zmap( "zmap" ), document );

    if ( args.get( "dump" ) != nullptr )
    {
      if ( args.get_array_char( "dump" ) != nullptr )
        return (void)document.FetchFrom( mtc::sourcebuf( *args.get_array_char( "dump" ) ).ptr() );

      if ( args.get_array_byte( "dump" ) != nullptr )
        return (void)document.FetchFrom( mtc::sourcebuf( *args.get_array_byte( "dump" ) ).ptr() );

      throw std::invalid_argument( "document 'dump' has to be array of char or byte" );
    }
    throw std::invalid_argument( "no document data as 'json', 'tags', 'dump' or 'zmap'" );
  }

}
