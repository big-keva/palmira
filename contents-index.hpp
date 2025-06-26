# if !defined( __palmira_contents_index_hpp__ )
# define __palmira_contents_index_hpp__
# include "span.hpp"
# include <mtc/iStream.h>
# include <string_view>

namespace palmira
{
  class EntityId: public std::string_view, protected mtc::api<const mtc::Iface>
  {
    using std::string_view::string_view;

  public:
    EntityId( const EntityId& ) = default;
    EntityId( const std::string_view& s, api i = nullptr ): std::string_view( s ), api( i ) {}
  };

  struct IEntity;             // common object properties
  struct IContents;           // indexable entity properties interface

  struct IEntity: public mtc::Iface
  {
    virtual auto  GetId() const -> EntityId = 0;
    virtual auto  GetIndex() const -> uint32_t = 0;
    virtual auto  GetExtra() const -> mtc::api<const mtc::IByteBuffer> = 0;
    virtual auto  GetVersion() const -> uint64_t = 0;
  };

  struct IStorage: public mtc::Iface
  {
    struct IIndexStore;       // interface to write indices
    struct ISerialized;       // interface to read indices
    struct ISourceList;

    virtual auto  ListIndices() -> mtc::api<ISourceList> = 0;
    virtual auto  CreateStore() -> mtc::api<IIndexStore> = 0;
  };

  struct IStorage::ISourceList: public mtc::Iface
  {
    virtual auto  Get() -> mtc::api<ISerialized> = 0;
  };

  struct IStorage::IIndexStore: public mtc::Iface
  {
    virtual auto  Entities() -> mtc::api<mtc::IByteStream> = 0;
    virtual auto  Contents() -> mtc::api<mtc::IByteStream> = 0;
    virtual auto  Chains() -> mtc::api<mtc::IByteStream> = 0;

    virtual auto  Commit() -> mtc::api<ISerialized> = 0;
    virtual void  Remove() = 0;
  };

  struct IStorage::ISerialized: public mtc::Iface
  {
    struct IPatch;

    virtual auto  Entities() -> mtc::api<const mtc::IByteBuffer> = 0;
    virtual auto  Contents() -> mtc::api<const mtc::IByteBuffer> = 0;
    virtual auto  Blocks() -> mtc::api<mtc::IFlatStream> = 0;

    virtual auto  Commit() -> mtc::api<ISerialized> = 0;
    virtual void  Remove() = 0;

    virtual auto  NewPatch() -> mtc::api<IPatch> = 0;
  };

  struct IStorage::ISerialized::IPatch: public mtc::Iface
  {
    virtual void  Delete( EntityId ) = 0;
    virtual void  Update( EntityId, const void*, size_t ) = 0;
    virtual void  Commit() = 0;
  };

  struct IContentsIndex: public mtc::Iface
  {
    struct IEntities;
    struct IIndexAPI;
    struct IEntityIterator;
    struct IRecordIterator;

   /*
    * entity details block statistics
    */
    struct BlockInfo
    {
      uint32_t    bkType;
      uint32_t    nCount;
    };

   /*
    * GetEntity()
    *
    * Provide access to entity by entiry id or index.
    */
    virtual auto  GetEntity( EntityId ) const -> mtc::api<const IEntity> = 0;
    virtual auto  GetEntity( uint32_t ) const -> mtc::api<const IEntity> = 0;

   /*
    * DelEntity()
    *
    * Remove entity by id.
    */
    virtual bool  DelEntity( EntityId id ) = 0;

   /*
    * SetEntity()
    *
    * Insert object with id to the index, sets dynamic untyped attributes
    * and indexable properties
    */
    virtual auto  SetEntity( EntityId,
      mtc::api<const IContents> = {}, const Span& = {} ) -> mtc::api<const IEntity> = 0;

   /*
    * SetExtras()
    *
    * Changes the value of extras block for the entity identified by id
    */
    virtual auto  SetExtras( EntityId, const Span& ) -> mtc::api<const IEntity> = 0;

    /*
    * Index statistics and service information
    */
    virtual auto  GetMaxIndex() const -> uint32_t = 0;
//    virtual auto  CountEntities() const -> uint32_t = 0;

   /*
    * Blocks search api
    */
    virtual auto  GetKeyBlock( const Span& ) const -> mtc::api<IEntities> = 0;
    virtual auto  GetKeyStats( const Span& ) const -> BlockInfo = 0;

   /*
    * Iterators
    */
    virtual auto  GetEntityIterator( EntityId ) -> mtc::api<IEntityIterator> = 0;
    virtual auto  GetEntityIterator( uint32_t ) -> mtc::api<IEntityIterator> = 0;

    virtual auto  GetRecordIterator( const Span& = {} ) -> mtc::api<IRecordIterator> = 0;
   /*
    * Commit()
    *
    * Writes all index data to storage held inside and returns
    * serialized interface.
    */
    virtual auto  Commit() -> mtc::api<IStorage::ISerialized> = 0;

   /*
    * Reduce()
    *
    * Return pointer to simplified version of index being optimized.
    */
    virtual auto  Reduce() -> mtc::api<IContentsIndex> = 0;

   /*
    * Stash( id )
    *
    * Stashes entity wothout any modifications to index.
    *
    * Defined for static indices.
    */
    virtual void  Stash( EntityId ) = 0;
  };

 /*
  * IContentsIndex::IKeyValue
  *
  * Internal indexer interface to set key -> value pairs for object being indexed.
  */
  struct IContentsIndex::IIndexAPI
  {
    virtual void  Insert( const Span& key, const Span& block, unsigned bkType = unsigned(-1) ) = 0;
  };

  struct IContentsIndex::IEntities: public mtc::Iface
  {
    struct Reference
    {
      uint32_t    uEntity;
      Span        details;
    };

    virtual auto  Find( uint32_t ) -> Reference = 0;
    virtual auto  Size() const -> uint32_t = 0;
    virtual auto  Type() const -> uint32_t = 0;
  };

  struct IContentsIndex::IEntityIterator: public mtc::Iface
  {
    virtual auto  Curr() -> mtc::api<const IEntity> = 0;
    virtual auto  Next() -> mtc::api<const IEntity> = 0;
  };

  struct IContentsIndex::IRecordIterator: public mtc::Iface
  {
    virtual auto  Curr() -> std::string = 0;
    virtual auto  Next() -> std::string = 0;
  };

 /*
  * IContents - keys indexing API provided to IContentsIndex::SetEntity()
  *
  * Method is called with single interface argument to send (key; value)
  * pairs to contents index
  */
  struct IContents: public mtc::Iface
  {
    virtual void  Enumerate( IContentsIndex::IIndexAPI* ) const = 0;
  };

}

# endif   // __palmira_contents_index_hpp__
