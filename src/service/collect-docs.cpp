# include "collect.hpp"
# include "collect-quotes.hpp"
# include "DelphiX/compat.hpp"
# include <stdexcept>
# include <cmath>

namespace palmira {
namespace collect {

  class Documents::data
  {
    static
    auto  GetRange( uint32_t, const Abstract& ) -> double;

  public:
    uint32_t  first = 1;
    uint32_t  count = 10;
    IsLessFn  order = []( uint32_t, double w1, uint32_t, double w2 ) {  return w1 > w2;  };
    RankerFn  range = &GetRange;
    QuotesFn  quote;
  };

  class Documents::impl final: public ICollector
  {
    struct Entity
    {
      uint32_t  id;
      double    weight;
    };

    void  operator delete( void* p )
    {
      std::allocator<impl>().deallocate( (impl*)p, 0 );
    }

    implement_lifetime_control

  protected:  // construction
    impl( const data& params ):
      nFirst( params.first ),
      nLimit( params.first + params.count - 1 ),
      isLess( params.order ),
      ranker( params.range ),
      quoter( params.quote ),
      quoBox( nLimit ) {}

  public:     // creation
    static
    auto  Create( const data& ) -> impl*;

  public:     // overridables
    void  Search( mtc::api<IQuery> ) override;
    auto  Finish( mtc::api<IContentsIndex> ) -> mtc::zmap override;

  protected:
    auto  Entities() const -> Entity*
      {  return (Entity*)(this + 1);  }

  private:
    const unsigned            nFirst;
    const unsigned            nLimit;
    const IsLessFn            isLess;
    const RankerFn            ranker;
    const QuotesFn            quoter;

    mtc::api<IQuery>          pQuery;
    Abstracts                 quoBox;
    unsigned                  nCount = 0;
    unsigned                  nFound = 0;
    Entity*                   pWorst = nullptr;
  };

  auto  Documents::impl::Create( const data& params ) -> impl*
  {
    auto  nalloc = sizeof(impl) + (params.first + params.count - 1) * sizeof(Entity);
    auto  nitems = (sizeof(impl) + nalloc - 1) / sizeof(impl);
    auto  palloc = std::allocator<impl>().allocate( nitems );

    return new( palloc ) impl( params );
  }

  void  Documents::impl::Search( mtc::api<IQuery> query )
  {
    uint32_t  id = uint32_t(-1);

    for ( pQuery = query; (id = query->SearchDoc( id + 1 )) != uint32_t(-1); )
    {
      auto    tuples = query->GetTuples( id );

      if ( tuples.dwMode != Abstract::None )
      {
        ++nFound;

        if ( nCount < nLimit )
        {
          new ( Entities() + nCount++ ) Entity{ id, ranker( id, tuples ) };
            quoBox.Set( id, tuples );
        }
          else
        {
          auto  weight = ranker( id, tuples );

          // check if worst is defined; detect worst
          if ( pWorst == nullptr )
            for ( auto beg = (pWorst = Entities()) + 1, end = Entities() + nCount; beg < end; ++beg )
              if ( isLess( pWorst->id, pWorst->weight, id, weight ) )
                pWorst = beg;

          // если лучше худшего, то заместить
          if ( isLess( id, weight, pWorst->id, pWorst->weight ) )
          {
            quoBox.Set( id, tuples, pWorst->id );
              *pWorst = { id, weight };
            pWorst = nullptr;
          }
        }
      }
    }
  }

  auto  Documents::impl::Finish( mtc::api<IContentsIndex> pIndex ) -> mtc::zmap
  {
    auto  report = mtc::zmap{
      { "first", uint32_t(nFirst) },
      { "found", uint32_t(nFound) } };

    if ( nCount >= nFirst )
    {
      auto  pitems = report.set_array_zmap( "items" );

      report["count"] = uint32_t(nCount + 1 - nFirst);

      std::sort( Entities(), Entities() + nCount, [this]( const Entity& l, const Entity& r )
        {  return isLess( l.id, l.weight, r.id, r.weight );  } );

      for ( auto beg = Entities() + nFirst - 1, end = Entities() + nCount; beg != end; ++beg )
      {
        auto  entity = pIndex->GetEntity( beg->id );
        auto  pExtra = entity != nullptr ? entity->GetExtra() : nullptr;
        auto  zExtra = mtc::zmap();

        if ( entity == nullptr )
          throw std::logic_error( "index has no entity found by index @" __FILE__ ":" LINE_STRING );

        if ( pExtra != nullptr && pExtra->GetLen() != 0 )
          (void)::FetchFrom( mtc::sourcebuf( pExtra->GetPtr(), pExtra->GetLen() ).ptr(), zExtra );

        pitems->push_back( {
          { "id",     std::string( entity->GetId() ) },
          { "index",  beg->id },
          { "extra",  zExtra },
          { "range",  beg->weight } } );

      // if quotation enabled, set the found element quote
        if ( quoter != nullptr && quoBox.Get( beg->id ) != nullptr )
          pitems->back().set_array_zval( "quote", std::move( quoter( beg->id, *quoBox.Get( beg->id ) ) ) );
      }
    }
    return report;
  }

  auto  Documents::data::GetRange( uint32_t, const Abstract& tuples ) -> double
  {
    double  weight;

    switch ( tuples.dwMode )
    {
      case Abstract::Rich:
        weight = 0.0;

        for ( auto p = tuples.entries.pbeg; p < tuples.entries.pend; ++p )
          weight = std::max( weight, p->weight );
        return weight / log( 2 + tuples.nWords );

      case Abstract::BM25:
        weight = 1.0;

        for ( auto p = tuples.factors.beg; p < tuples.factors.end; ++p )
          weight *= (1.0 - p->dblIDF);
        return tuples.nWords != 0 ? (1.0 - weight) / log( 2 + tuples.nWords ) : 1.0 - weight;

      default:
        throw std::logic_error( "invalid Abstract mode passed @" __FILE__ ":" LINE_STRING );
    }
  }

  auto  Documents::SetFirst( uint32_t first ) -> Documents&
  {
    if ( params == nullptr )
      params = std::make_shared<data>();
    if ( first == 0 )
      throw std::invalid_argument( "'first' has to be 1-based @" __FILE__ ":" LINE_STRING );
    return params->first = first, *this;
  }

  auto  Documents::SetCount( uint32_t count ) -> Documents&
  {
    if ( params == nullptr )
      params = std::make_shared<data>();
    return params->count = count, *this;
  }

  auto  Documents::SetOrder( IsLessFn fless ) -> Documents&
  {
    if ( params == nullptr )
      params = std::make_shared<data>();
    if ( fless == nullptr )
      throw std::invalid_argument( "'fless' has to be a valid IsLessFn @" __FILE__ ":" LINE_STRING );
    return params->order = fless, *this;
  }

  auto  Documents::SetRange( RankerFn scale ) -> Documents&
  {
    if ( params == nullptr )
      params = std::make_shared<data>();
    if ( scale == nullptr )
      throw std::invalid_argument( "'range' has to be a valid RankerFn @" __FILE__ ":" LINE_STRING );
    return params->range = scale, *this;
  }

  auto  Documents::SetQuote( QuotesFn quote ) -> Documents&
  {
    if ( params == nullptr )
      params = std::make_shared<data>();
    if ( quote == nullptr )
      throw std::invalid_argument( "'quote' has to be a valid IQuotation object @" __FILE__ ":" LINE_STRING );
    return params->quote = quote, *this;
  }

  auto  Documents::Create() -> mtc::api<ICollector>
  {
    if ( params == nullptr )
      params = std::make_shared<data>();
    return impl::Create( *params );
  }

}}
