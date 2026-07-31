# include "collect.hpp"
# include "structo/compat.hpp"
# include <stdexcept>
# include <cmath>
#include <condition_variable>
#include <reports.hpp>
#include <mtc/json.h>
#include <mtc/recursive_shared_mutex.hpp>

namespace palmira {
namespace collect {

  using IEntity = structo::IEntity;

  class LoadDumps::impl final: public ICollector
  {
    friend class LoadDumps;

    struct document
    {
      std::string             docid;
      QuotesFn                quote;
      mtc::api<const IEntity> fetch;
    };

  public:
    impl( mtc::api<const IContentsIndex>  ctx ): ctxIndex( ctx ) {}

    void  Search( mtc::api<IQuery> )                      override;
    auto  Finish( mtc::api<IContentsIndex> ) -> mtc::zmap override;

    implement_lifetime_control

  protected:
    mtc::api<const IContentsIndex>  ctxIndex;
    std::vector<document>           docStore;

  };

  // LoadDumps::impl implementation

  void  LoadDumps::impl::Search( mtc::api<IQuery> )
  {
    for ( auto it = docStore.begin(); it != docStore.end(); )
      if ( (it->fetch = ctxIndex->GetEntity( it->docid )) == nullptr )
        it = docStore.erase( it );
      else ++it;
  }

  auto  LoadDumps::impl::Finish( mtc::api<IContentsIndex> ) -> mtc::zmap
  {
    if ( docStore.size() != 0 )
    {
      auto  report = SearchReport( 0, "OK", {
        { "first", uint32_t(1) },
        { "found", uint32_t(docStore.size()) },
        { "items", mtc::array_zmap() } } );
      auto  pcache = report.get_array_zmap( "items" );

      for ( auto& next: docStore )
      {
        auto  uindex = next.fetch->GetIndex();
        auto  pExtra = next.fetch->GetExtra();
        auto  zExtra = mtc::zmap();

        if ( pExtra != nullptr && pExtra->GetLen() != 0 )
          (void)::FetchFrom( mtc::sourcebuf( pExtra->GetPtr(), pExtra->GetLen() ).ptr(), zExtra );

        pcache->push_back( {
          { "id", next.docid },
          { "index", uindex },
          { "extra", zExtra },
          { "quote", next.quote( uindex, {} ) } } );
      }
      return report;
    }
    return SearchReport( ENOENT, "No entities found" );
  }

  // LoadDumps implementation

  LoadDumps::LoadDumps( mtc::api<const IContentsIndex> ctx )
  {
    params = new impl( ctx );
  }

  LoadDumps::~LoadDumps()
  {
    if ( params != nullptr )
      delete params;
  }

  auto  LoadDumps::Insert( std::string_view docid, QuotesFn quote ) -> LoadDumps&
  {
    params->docStore.push_back( { std::string( docid ), quote, {} } );
    return *this;
  }

  auto  LoadDumps::Create() -> mtc::api<ICollector>
  {
    auto  output = params;
      params = nullptr;
    return output;
  }

}}
