# include "collect.hpp"
# include "collect-quotes.hpp"
# include "structo/compat.hpp"
# include <stdexcept>
# include <cmath>
#include <condition_variable>
#include <mtc/json.h>
#include <mtc/recursive_shared_mutex.hpp>

namespace palmira {
namespace collect {

 /*
  * size limit for one section in parallel processing
  */
  constexpr uint32_t  max_thread_section = 0x4000;

 /*
  * Коллекция настроек коллектора
  */
  class Documents::data
  {
  public:
    struct WrapDifferFn
    {
      DifferFn  differ;

      int   operator()( uint32_t d1, double f1, uint32_t d2, double f2 ) const
      {
        int   res = differ( d1, f1, d2, f2 );
        return res != 0 ? res : d1 - d2;
      }
    };
    static int  compareByRange( uint32_t d1, double f1, uint32_t d2, double f2 )
    {
      int   res = (f1 < f2) - (f1 > f2);
      return res != 0 ? res : d1 - d2;
    }

    using Threads = mtc::ThreadPool;

    static
    auto  GetRange( uint32_t, const Abstract& ) -> double;

  public:
    uint32_t  nfirst = 1;
    uint32_t  ncount = 10;
    DifferFn  differ = compareByRange;
    RankerFn  ranker = &GetRange;
    QuotesFn  quoter;
    Threads*  async = nullptr;

  };

 /*
  * Собственно коллектор документов
  */
  class Documents::impl final: public ICollector, protected data
  {
    struct linear_t  {};
    constexpr static linear_t linear = {};

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
    impl( const data& params ): data( params ),
      nFirst( params.nfirst ),
      nLimit( params.nfirst + params.ncount - 1 ),
      quoBox( nLimit ) {}

  public:     // creation
    static
    auto  Create( const data& ) -> impl*;

  public:     // overridables
   /*
    * Insert()s the passed collector documents
    */
    void  Search( mtc::api<IQuery> ) override;
    auto  Finish( mtc::api<IContentsIndex> ) -> mtc::zmap override;

  protected:  // using partial queries
    void  Insert( const impl& );
    bool  Search( linear_t, mtc::api<IQuery> );
    auto  Buffer() const -> Entity* {  return (Entity*)(this + 1);  }

  private:
    const unsigned    nFirst;
    const unsigned    nLimit;

    std::mutex        mxLock;
    mtc::api<IQuery>  pQuery;
    Abstracts         quoBox;
    unsigned          nCount = 0;
    unsigned          nFound = 0;
    Entity*           pWorst = nullptr;
  };

  auto  Documents::impl::Create( const data& params ) -> impl*
  {
    auto  nalloc = sizeof(impl) + (params.nfirst + params.ncount - 1) * sizeof(Entity);
    auto  nitems = (sizeof(impl) + nalloc - 1) / sizeof(impl);
    auto  palloc = std::allocator<impl>().allocate( nitems );

    return new( palloc ) impl( params );
  }

  void  Documents::impl::Search( mtc::api<IQuery> query )
  {
    uint32_t  rBound;

  // check if multikernel processing enabled and needed
    if ( async != nullptr && (rBound = query->LastIndex()) > max_thread_section * 4 )
    {
      std::mutex              mxWait;
      auto                    mxLock = mtc::make_unique_lock( mxWait );
      std::condition_variable cvWait;
      std::atomic_long        nParts = 0;   // executor threads
      int  nloops = 0;
      int  nquery = 0;
      std::atomic_int  nmerge = 0;
      std::atomic nfound = 0;

      //
      // share query execution to parts
      //
      for ( uint32_t uLower = 0; uLower < rBound; uLower += max_thread_section )
      {
        ++nloops;
        auto  subQuery = query->Duplicate( { uLower, uLower + max_thread_section } );

        if ( subQuery != nullptr )
        {
          ++nquery;
          ++nParts;

          async->Insert( [&, this, subQuery, subStore = mtc::api( Create( *this ) )]()
          {
            if ( subStore->Search( linear, subQuery ) )
              this->Insert( *subStore.ptr() );

            ++nmerge;
            nfound += subStore->nFound;
            if ( --nParts == 0 )
              cvWait.notify_all();
          } );
        }
      }

      // wait until the execution finished
      cvWait.wait( mxLock );
    }
      else
    Search( linear, query );
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

      std::sort( Buffer(), Buffer() + nCount, [this]( const Entity& l, const Entity& r )
        {  return differ( l.id, l.weight, r.id, r.weight ) < 0;  } );

      for ( auto beg = Buffer() + nFirst - 1, end = Buffer() + nCount; beg != end; ++beg )
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

 /*
  * Вливание порции найденных документов в результирующий контейнер. Выполняется
  * через пересортировки, чтобы не искать каждый раз "худший", а замещать лучшими
  * из списка найденных
  */
  void  Documents::impl::Insert( const impl& docs )
  {
    auto    exLock = mtc::make_unique_lock( mxLock, std::defer_lock );

  // пересортировать вставляемый контейнер
    std::sort( docs.Buffer(), docs.Buffer() + docs.nCount, [this]( const Entity& l, const Entity& r )
      {  return differ( l.id, l.weight, r.id, r.weight ) < 0;  } );

    exLock.lock();

  // последовательно вставить элементы
    for ( auto
      dstptr = Buffer(),
      dstend = dstptr + nCount,
      dstlim = dstptr + nLimit,
      srcptr = docs.Buffer(),
      srcend = srcptr + docs.nCount; srcptr != srcend; ++srcptr )
    {
      while ( dstptr != dstend && differ( dstptr->id, dstptr->weight, srcptr->id, srcptr->weight ) < 0 )
        ++dstptr;

      if ( dstptr == dstlim )
        break;

      if ( dstend < dstlim )
      {
        quoBox.Set( srcptr->id,
          *docs.quoBox.Get( srcptr->id ) );
        memmove( dstptr + 1, dstptr, sizeof(Entity) * (dstend - dstptr) );
          dstend = ++nCount + Buffer();
      }
        else
      {
        quoBox.Set( srcptr->id,
          *docs.quoBox.Get( srcptr->id ), dstlim[-1].id );
        memmove( dstptr + 1, dstptr, sizeof(Entity) * (dstlim - dstptr - 1) );
      }
      new( dstptr ) Entity( *srcptr );
    }
    nFound += docs.nFound;
  }

 /*
  * Performs real search in a query passed
  */
  bool  Documents::impl::Search( linear_t, mtc::api<IQuery> query )
  {
    uint32_t  id = 0;

    for ( pQuery = query; (id = query->SearchDoc( id + 1 )) != uint32_t(-1); )
    {
      auto  tuples = query->GetTuples( id );

      if ( tuples.dwMode != Abstract::None )
      {
        auto  weight = ranker( id, tuples );

        ++nFound;

        if ( nCount < nLimit )
        {
          new ( Buffer() + nCount++ ) Entity{ id, weight };
          quoBox.Set( id, tuples );
        }
          else
        {
          // check if worst is defined; detect worst
          if ( pWorst == nullptr )
            for ( auto beg = (pWorst = Buffer()) + 1, end = Buffer() + nCount; beg < end; ++beg )
              if ( differ( pWorst->id, pWorst->weight, beg->id, beg->weight ) < 0 )
                pWorst = beg;

          // если лучше худшего, то заместить
          if ( differ( id, weight, pWorst->id, pWorst->weight ) < 0 )
          {
            quoBox.Set( id, tuples, pWorst->id );
            *pWorst = { id, weight };
            pWorst = nullptr;
          }
        }
      }
    }
    return nFound != 0;
  }

  auto  Documents::data::GetRange( uint32_t /*id*/, const Abstract& tuples ) -> double
  {
    double  weight = 0.0;

    switch ( tuples.dwMode )
    {
      case Abstract::Rich:
        {
          const Abstract::EntrySet* best = nullptr;

          for ( auto p = tuples.entries.pbeg; p < tuples.entries.pend; ++p )
            if ( best == nullptr || best->weight < p->weight )
              weight = (best = p)->weight;

          return weight/* / log10( 2 + tuples.nWords )*/;
        }
      case Abstract::BM25:
        weight = 1.0;

        for ( auto p = tuples.factors.pbeg; p < tuples.factors.pend; ++p )
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
    return params->nfirst = first, *this;
  }

  auto  Documents::SetCount( uint32_t count ) -> Documents&
  {
    if ( params == nullptr )
      params = std::make_shared<data>();
    return params->ncount = count, *this;
  }

  auto  Documents::SetOrder( DifferFn differ ) -> Documents&
  {
    if ( params == nullptr )
      params = std::make_shared<data>();
    if ( differ == nullptr )
      throw std::invalid_argument( "'fless' has to be a valid IsLessFn @" __FILE__ ":" LINE_STRING );
    return params->differ = data::WrapDifferFn{ differ }, *this;
  }

  auto  Documents::SetRange( RankerFn scale ) -> Documents&
  {
    if ( params == nullptr )
      params = std::make_shared<data>();
    if ( scale == nullptr )
      throw std::invalid_argument( "'range' has to be a valid RankerFn @" __FILE__ ":" LINE_STRING );
    return params->ranker = scale, *this;
  }

  auto  Documents::SetQuote( QuotesFn quotes ) -> Documents&
  {
    if ( params == nullptr )
      params = std::make_shared<data>();
    if ( quotes == nullptr )
      throw std::invalid_argument( "'quote' has to be a valid IQuotation object @" __FILE__ ":" LINE_STRING );
    return params->quoter = quotes, *this;
  }

  auto  Documents::SetAsync( mtc::ThreadPool* actors ) -> Documents&
  {
    if ( params == nullptr )
      params = std::make_shared<data>();
    if ( actors == nullptr )
      throw std::invalid_argument( "'actor' has to be a valid mtc::ThreadsPool object @" __FILE__ ":" LINE_STRING );
    return params->async = actors, *this;
  }

  auto  Documents::Create() -> mtc::api<ICollector>
  {
    if ( params == nullptr )
      params = std::make_shared<data>();
    return impl::Create( *params );
  }

}}
