# include "../../service.hpp"
# include "object-zmap.hpp"
# include <DeliriX/DOM-dump.hpp>
# include "DeliriX/DOM-load.hpp"

template <> inline
mtc::array_char*  Serialize( mtc::array_char* to, const void* p, size_t l )
{
  if ( to != nullptr )
    to->insert( to->end(), (const char*)p, l + (const char*)p );
  return to;
}

namespace palmira
{
  using Document = DeliriX::Text;

  // TimingArgs

  TimingArgs::TimingArgs( const mtc::zmap& args ):
    fTimeout( args.get_double( "timeout", -1.0 ) )
  {
  }

  mtc::zmap&  TimingArgs::Serialize( mtc::zmap& to ) const
  {
    if ( fTimeout >= 0.0 )
      to["timeout"] = fTimeout;
    return to;
  }

  // AccessArgs

  AccessArgs::AccessArgs( const mtc::zmap& args ): TimingArgs( args ),
    objectId( args.get_charstr( "id", "" ) )
  {
    if ( objectId.empty() )
      throw std::invalid_argument( "absent object 'id'" );
  }

  mtc::zmap&  AccessArgs::Serialize( mtc::zmap& to ) const
  {
    if ( !objectId.empty() )
      to["id"] = objectId;
    return TimingArgs::Serialize( to );
  }

  // RemoveArgs implementation

  RemoveArgs::RemoveArgs( const mtc::zmap& args ): AccessArgs( args ),
    uVersion( args.get_word64 ( "version", 0 ) )
  {
    if ( args.get( "claim" ) != nullptr )
      ifClause = *args.get( "claim" );
  };

  mtc::zmap&  RemoveArgs::Serialize( mtc::zmap& to ) const
  {
    if ( !ifClause.empty() )
      to["claim"] = ifClause;
    return AccessArgs::Serialize( to );
  }

  // UpdateArgs implementation

  UpdateArgs::UpdateArgs( const mtc::zmap& args ): RemoveArgs( args ),
    metadata( args.get_zmap( "metadata", {} ) )
  {
  }

  mtc::zmap&  UpdateArgs::Serialize( mtc::zmap& to ) const
  {
    if ( !metadata.empty() )
      to["metadata"] = metadata;
    return RemoveArgs::Serialize( to );
  }

  // InsertArgs implementation

  InsertArgs::InsertArgs( InsertArgs&& args ): UpdateArgs( std::move( args ) ),
    document( std::move( args.document ) ),
    textview( &args.textview == &args.document ? document : args.textview )
  {
  }

  InsertArgs::InsertArgs( const mtc::zmap& args ): UpdateArgs( args ),
    textview( document )
  {
    LoadBody( args );
  }

  mtc::zmap&  InsertArgs::Serialize( mtc::zmap& to, const text::as::json_t& ) const
  {
    textview.Serialize( DeliriX::dump_as::Json( [dump = to.set_charstr( "json" )]( const char* s, size_t l )
      {  dump->append( s, l );  } ) );

    return UpdateArgs::Serialize( to );
  }

  mtc::zmap&  InsertArgs::Serialize( mtc::zmap& to, const text::as::tags_t& ) const
  {
    textview.Serialize( DeliriX::dump_as::Tags( [dump = to.set_charstr( "tags" )]( const char* s, size_t l )
      {  dump->append( s, l );  } ) );

    return UpdateArgs::Serialize( to );
  }

  mtc::zmap&  InsertArgs::Serialize( mtc::zmap& to, const text::as::zmap_t& ) const
  {
    palmira::Serialize( *to.set_zmap( "zmap" ), textview );

    return UpdateArgs::Serialize( to );
  }

  mtc::zmap&  InsertArgs::Serialize( mtc::zmap& to, const text::as::dump_t& ) const
  {
    textview.Serialize( to.set_array_char( "dump" ) );

    return UpdateArgs::Serialize( to );
  }

  void  InsertArgs::LoadBody( const mtc::zmap& args )
  {
    if ( args.get_charstr( "json" ) != nullptr )
      return DeliriX::load_as::Json( &document, DeliriX::load_as::MakeSource( args.get_charstr( "json" )->c_str() ) );

    if ( args.get_charstr( "tags" ) != nullptr )
      return DeliriX::load_as::Tags( &document, DeliriX::load_as::MakeSource( args.get_charstr( "tags" )->c_str() ) );

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

  // SearchArgs implementation

  mtc::zmap& SearchArgs::Serialize( mtc::zmap& to ) const
  {
    return TimingArgs::Serialize( to );
  }

}
