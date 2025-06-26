# include "indexes/layered-contents.hpp"
# include "indexes/static-contents.hpp"
# include "indexes/dynamic-contents.hpp"
# include "exceptions.hpp"
# include "commit-contents.hxx"
# include "merger-contents.hxx"
# include "index-layers.hxx"
# include <mtc/recursive_shared_mutex.hpp>
# include <shared_mutex>

namespace palmira {
namespace index {
namespace layered {

  class ContentsIndex final: protected IndexLayers, public IContentsIndex
  {
    std::atomic<size_t> referenceCount = 0;

    long  Attach() override {  return ++referenceCount;  }
    long  Detach() override;

  public:
    ContentsIndex( const mtc::api<IContentsIndex>* indices, size_t count );
    ContentsIndex( const mtc::api<IStorage>&, const dynamic::Settings& );

    auto  StartMonitor( const std::chrono::seconds& mergeMonitorDelay ) -> ContentsIndex*;

  public:
    auto  GetEntity( EntityId ) const -> mtc::api<const IEntity> override;
    auto  GetEntity( uint32_t ) const -> mtc::api<const IEntity> override;

    bool  DelEntity( EntityId ) override;
    auto  SetEntity( EntityId,
      mtc::api<const IContents>, const Span& ) -> mtc::api<const IEntity> override;
    auto  SetExtras( EntityId, const Span& ) -> mtc::api<const IEntity> override;

    auto  GetMaxIndex() const -> uint32_t override;
    auto  GetKeyBlock( const Span& ) const -> mtc::api<IEntities> override;
    auto  GetKeyStats( const Span& ) const -> BlockInfo override;

    auto  GetEntityIterator( EntityId ) -> mtc::api<IEntityIterator> override
      {  throw std::runtime_error( "not implemented" );  }
    auto  GetEntityIterator( uint32_t ) -> mtc::api<IEntityIterator> override
      {  throw std::runtime_error( "not implemented" );  }
    auto  GetRecordIterator( const Span& ) -> mtc::api<IRecordIterator> override
      {  throw std::runtime_error( "not implemented" );  }

    auto  Commit() -> mtc::api<IStorage::ISerialized> override;
    auto  Reduce() -> mtc::api<IContentsIndex> override {  return this;  }

    void  Stash( EntityId ) override  {}

  protected:
    using LayersIterator = decltype(layers)::iterator;

    void  MergeMonitor( const std::chrono::seconds& );
    auto  selectLimits() -> std::pair<LayersIterator, LayersIterator>;

  protected:
    using EventRec = std::pair<void*, Notify::Event>;

    mtc::api<IStorage>          istore;
    dynamic::Settings           dynSet;
    bool                        rdOnly = false;

    volatile bool               canRun = true;    // the continue flag

    mutable std::shared_mutex   ixlock;

  // event manager - the events are processed after the index
  // asyncronous action is performed
    std::list<EventRec>         evQueue;
    std::mutex                  evMutex;
    std::condition_variable     evEvent;
    std::thread                 monitor;
  };

  // ContentsIndex implementation

  ContentsIndex::ContentsIndex( const mtc::api<IContentsIndex>* indices, size_t count ):
    IndexLayers( indices, count )
  {
  }

  ContentsIndex::ContentsIndex( const mtc::api<IStorage>& storage, const dynamic::Settings& dynamicSets ):
    IndexLayers(), istore( storage ), dynSet( dynamicSets )
  {
    auto  sources = istore->ListIndices();
    auto  dynamic = istore->CreateStore();

  // check if has any sources
    if ( sources != nullptr )
      for ( auto serial = sources->Get(); serial != nullptr; serial = sources->Get() )
        addContents( static_::Contents().Create( serial ) );

  // add dynamic index to the end if possible
    if ( dynamic != nullptr )
    {
      addContents( index::dynamic::Contents()
        .Set( dynamic )
        .Set( dynSet ).Create() );
      rdOnly = false;
    } else rdOnly = true;
  }

  auto  ContentsIndex::StartMonitor( const std::chrono::seconds& mergeMonitorDelay ) -> ContentsIndex*
  {
    monitor = std::thread( &ContentsIndex::MergeMonitor, this, mergeMonitorDelay );
    return this;
  }

  long  ContentsIndex::Detach()
  {
    auto  rcount = --referenceCount;

    if ( rcount == 0 )
    {
      if ( monitor.joinable() )
      {
        canRun = false,
          evEvent.notify_one();
        monitor.join();
      }
      commitItems();
      delete this;
    }
    return rcount;
  }


  auto  ContentsIndex::GetEntity( EntityId id ) const -> mtc::api<const IEntity>
  {
    auto  shlock = mtc::make_shared_lock( ixlock );

    return getEntity( id );
  }

  auto  ContentsIndex::GetEntity( uint32_t id ) const -> mtc::api<const IEntity>
  {
    auto  shlock = mtc::make_shared_lock( ixlock );

    return getEntity( id );
  }

  bool  ContentsIndex::DelEntity( EntityId id )
  {
    auto  shlock = mtc::make_shared_lock( ixlock );

    return delEntity( id );
  }

  auto  ContentsIndex::SetEntity( EntityId id,
    mtc::api<const IContents> contents, const Span& extras ) -> mtc::api<const IEntity>
  {
    if ( layers.empty() )
      throw std::logic_error( "index flakes are not initialized" );

    for ( ; ; )
    {
      auto  shlock = mtc::make_shared_lock( ixlock );
      auto  exlock = mtc::make_unique_lock( ixlock, std::defer_lock );
      auto  pindex = layers.back().pIndex.ptr();    // the last index pointer, unchanged in one thread

    // try Set the entity to the last index in the chain
      try
      {
        return layers.back().Override( pindex->SetEntity( id, contents, extras ) );
      }

    // on dynamic index overflow, rotate the last index by creating new one in a new flakes,
    // and continue Setting attempts
      catch ( const index_overflow& xo )
      {
        shlock.unlock();  exlock.lock();

      // received exclusive lock, check if index is already rotated by another
      // SetEntity call; if yes, try again to SetEntity, else rotate index
        if ( layers.back().pIndex.ptr() == pindex )
        {
        // rotate the index by creating the commiter for last (dynamic) index
        // and create the new dynamic index
          layers.back().uUpper = layers.back().uLower
            + pindex->GetMaxIndex() - 1;

          layers.back().pIndex = commit::Contents().Create( layers.back().pIndex, [this]( void* to, Notify::Event event )
            {
              mtc::interlocked( mtc::make_unique_lock( evMutex ), [&]()
                {  evQueue.emplace_back( to, event );  } );
              evEvent.notify_one();
            } );

          layers.emplace_back( layers.back().uUpper + 1, dynamic::Contents()
            .Set( dynamic::Settings()
              .SetMaxEntities( 10000 )
              .SetMaxAllocate( 2 * 1024 * 1024 ) )
            .Set( istore->CreateStore() ).Create() );
          layers.back().uUpper = (uint32_t)-1;
        }
      }
    }
  }

  auto  ContentsIndex::SetExtras( EntityId id, const Span& extras ) -> mtc::api<const IEntity>
  {
    auto  shlock = mtc::make_shared_lock( ixlock );
    return setExtras( id, extras );
  }

  auto  ContentsIndex::Commit() -> mtc::api<IStorage::ISerialized>
  {
    throw std::logic_error( "not implemented" );
  }

  auto  ContentsIndex::GetMaxIndex() const -> uint32_t
  {
    auto  shlock = mtc::make_shared_lock( ixlock );
    return getMaxIndex();
  }

  auto  ContentsIndex::GetKeyBlock( const Span& key ) const -> mtc::api<IEntities>
  {
    auto  shlock = mtc::make_shared_lock( ixlock );
    return getKeyBlock( key, this );
  }

  auto  ContentsIndex::GetKeyStats( const Span& key ) const -> BlockInfo
  {
    auto  shlock = mtc::make_shared_lock( ixlock );
    return getKeyStats( key );
  }

  void  ContentsIndex::MergeMonitor( const std::chrono::seconds& startDelay )
  {
    pthread_setname_np( pthread_self(), "MergeMonitor" );

    for ( std::this_thread::sleep_for( startDelay ); canRun; )
    {
      auto  exwait = mtc::make_unique_lock( evMutex );
      auto  fEvent = [this](){  return !canRun || !evQueue.empty();  };

    // check if any events occured; process events first
    // for each processed event, either reduce the index sent the event,
    // or simply remove the empty index
    // or process the errors
      if ( evEvent.wait_for( exwait, std::chrono::seconds( 30 ), fEvent ) && canRun )
      {
        auto  exlock = mtc::make_unique_lock( ixlock );

      // for each event, search the element in the list of indices to Reduce()
      // and finish index modification
        for ( auto& event: evQueue )
        {
          auto  pfound = std::find_if( layers.begin(), layers.end(), [&]( const IndexEntry& index )
            {  return index.pIndex.ptr() == event.first;  } );

        // if the index with key pointer found, check the type of event occured
          if ( pfound == layers.end() )
            throw std::logic_error( "strange event not attached to any index!" );

          switch ( event.second )
          {
          // On OK, replace the index in the entry to it's reduced version,
          // resort the indices in the size-decreasing order, and renumber
            case Notify::Event::OK:
            {
              uint32_t uLower = 1;

              pfound->pIndex = pfound->pIndex->Reduce();
              pfound->backup.clear();

              std::sort( layers.begin(), layers.end() - 1, []( const IndexEntry& a, const IndexEntry& b )
                {  return a.pIndex->GetMaxIndex() > b.pIndex->GetMaxIndex();  } );

              for ( auto& index: layers )
                uLower = (index.uUpper = (index.uLower = uLower) + index.pIndex->GetMaxIndex() - 1) + 1;

              layers.back().uUpper = uint32_t(-1);
              break;
            }

          // On Empty, simple remove the existing index because its processing
          // result is empty
            case Notify::Event::Empty:
              layers.erase( pfound );
              break;

          // On Cancel, rollback the event record to the previous subset
          // of entries saved in the entry processed
            case Notify::Event::Canceled:
            {
              auto  backup = std::move( pfound->backup );
                pfound = layers.erase( pfound );
              layers.insert( pfound, backup.begin(), backup.end() );
              break;
            }

        // On Failed, commit index and shutdown service if possible
            default:
              break;
          }
        }
        evQueue.clear();
      }

    // try select indices to be merged
      if ( canRun )
      {
        auto  shlock = mtc::make_shared_lock( ixlock );
        auto  exlock = mtc::make_unique_lock( ixlock, std::defer_lock );
        auto  limits = selectLimits();

      // select the limits, check and select again the limits for merger
        if ( limits.first != limits.second )
        {
          shlock.unlock();  exlock.lock();

          if ( (limits = selectLimits()).first != limits.second )
          {
            auto  xMaker = fusion::Contents()
              .Set( [this]( void* to, Notify::Event event )
                {
                  mtc::interlocked( mtc::make_unique_lock( evMutex ), [&]()
                    {  evQueue.emplace_back( to, event );  } );
                  evEvent.notify_one();
                } )
//              .Set( canContinue )
              .Set( istore->CreateStore() );

            for ( auto p =  limits.first; p != limits.second; ++p )
            {
              xMaker.Add( p->pIndex );
              limits.first->backup.push_back( IndexEntry{ p->uLower, p->pIndex } );
            }

            limits.first->uUpper = limits.first->backup.back().uUpper;
            limits.first->pIndex = xMaker.Create();

            layers.erase( limits.first + 1, layers.end() );
          }
        }
      }
    }
  }

  // Contents implementation

  auto Contents::Set( mtc::api<IStorage> ps ) -> Contents&
  {
    return contentsStorage = ps, *this;
  }

  auto Contents::Set( const dynamic::Settings& settings ) -> Contents&
  {
    return dynamicSettings = settings, *this;
  }

  auto Contents::Create() -> mtc::api<IContentsIndex>
  {
    if ( contentsStorage == nullptr )
      throw std::logic_error( "layered index storage is not defined" );
    return (new ContentsIndex( contentsStorage, dynamicSettings ))->StartMonitor( runMonitorDelay );
  }

  auto  Contents::Create( const mtc::api<IContentsIndex>* indices, size_t size ) -> mtc::api<IContentsIndex>
  {
    return new ContentsIndex( indices, size );
  }

  auto  Contents::Create( const std::vector<mtc::api<IContentsIndex>>& indices ) -> mtc::api<IContentsIndex>
  {
    return Create( indices.data(), indices.size() );
  }

}}}
