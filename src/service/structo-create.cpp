# include "../../service/structo-search.hpp"
# include <structo/context/lemmatizer.hpp>
#include <structo/context/x-contents.hpp>
# include <structo/indexer/layered-contents.hpp>
# include <structo/indexer/dynamic-contents.hpp>
# include <structo/storage/posix-fs.hpp>

namespace palmira
{

  auto  InitLanguages( const mtc::config& config ) -> structo::context::Processor
  {
    auto  processor = structo::context::Processor();
    auto  languages = config.to_zmap().get( "languages" );

    if ( languages != nullptr )
    {
      if ( languages->get_array_zmap() == nullptr )
        throw std::invalid_argument( "'languages' has to be array of structures { 'id': int, 'module': path, 'config': path }" );

      for ( auto& next: *languages->get_array_zmap() )
      {
        auto  section = config.get_section( next );
        auto  lang_id = section.get_int32( "id", -1 );
        auto  as_path = section.get_path( "module" );
        auto  lp_conf = section.get_path( "config" );

        if ( lang_id == -1 )
          throw std::invalid_argument( "language 'id' must be defined and has to be non-negative integer" );
        if ( as_path == "" )
          throw std::invalid_argument( "language 'module' must point to existing shared library" );

        processor.AddModule( lang_id,
          structo::context::LoadLemmatizer( as_path, lp_conf ) );
      }
    }
    return processor;
  }

  auto  OpenStorage( const mtc::config& config ) -> mtc::api<structo::IStorage>
  {
    auto  ixpath = config.get_path( "generic_name" );

    if ( ixpath != "" )
      return Open( storage::posixFS::StoragePolicies::Open( ixpath ) );

    throw std::invalid_argument( "neither generic index name nor index policy was found" );
  }

  auto  OpenIndex( const mtc::config& config ) -> mtc::api<structo::IContentsIndex>
  {
    if ( config.empty() )
      throw std::invalid_argument( "section 'index' not found in configuration file" );

    return structo::indexer::layered::Index()
      .Set( OpenStorage( config ) )
      .Set( structo::indexer::dynamic::Settings()
        .SetMaxEntities( 1024 )
        .SetMaxAllocate( 1024 * 1024 * 1024 ) )
      .Create();
  }

  auto  GetContentsType( const mtc::config& config ) -> FnContents
  {
    auto  ctType = config.get_charstr( "contents" );

    if ( ctType == "Rich" )
      return context::GetRichContents;

    if ( ctType == "BM25" )
      return context::GetBM25Contents;

    if ( ctType == "Mini" )
      return context::GetMiniContents;

    throw std::invalid_argument( mtc::strprintf( "unknown index contents type '%s'", ctType.c_str() ) );
  }

  auto  CreateStructo( const mtc::config& config ) -> mtc::api<IService>
  {
    auto  create = StructoService();

    return create
      .Set( InitLanguages( config ) )
      .Set( OpenIndex( config.get_section( "index" ) ) )
      .Set( GetContentsType( config ) )
      .Create();
  }

}
