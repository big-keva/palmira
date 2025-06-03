# if !defined( __palmira_api_contents_index_hxx__ )
# define __palmira_api_contents_index_hxx__
# include <mtc/iBuffer.h>
# include <mtc/iStream.h>
# include <string_view>

namespace palmira
{
  using EntityId = std::string_view;

  struct IEntity;             // common object properties
  struct IContents;           // indexable entity properties interface

  struct IEntity: public mtc::Iface
  {
    struct Attribute;

    virtual auto  GetId() const -> Attribute = 0;
    virtual auto  GetIndex() const -> uint32_t = 0;
    virtual auto  GetAttributes() const -> Attribute = 0;
    virtual auto  GetImage() const -> mtc::api<const mtc::IByteBuffer> = 0;
  };

  struct IEntity::Attribute: public std::string_view, protected mtc::api<Iface>
  {
    Attribute( const Attribute& ) = default;
    Attribute( const std::string_view& s, api i = nullptr ): std::string_view( s ), api( i ) {}
  };

  struct IStorage: public mtc::Iface
  {
    struct IImageStore;
    struct IIndexStore;       // interface to write indices
    struct ISerialized;       // interface to read indices
  };

  struct IStorage::IImageStore: public mtc::Iface
  {
    virtual auto  Put( mtc::api<const mtc::IByteBuffer> ) -> uint64_t = 0;
  };

  struct IStorage::IIndexStore: public mtc::Iface
  {
    virtual auto  Entities() -> mtc::api<mtc::IByteStream> = 0;
    virtual auto  Index() -> mtc::api<mtc::IByteStream> = 0;
    virtual auto  Chains() -> mtc::api<mtc::IByteStream> = 0;
    virtual auto  Images() -> mtc::api<IImageStore> = 0;

    virtual auto  Commit() -> mtc::api<ISerialized> = 0;
    virtual void  Remove() = 0;
  };

  struct IStorage::ISerialized: public mtc::Iface
  {
    struct IPatch;

    virtual auto  Entities() -> mtc::api<const mtc::IByteBuffer> = 0;
    virtual auto  Contents() -> mtc::api<const mtc::IByteBuffer> = 0;
    virtual auto  Blocks() -> mtc::api<const mtc::IFlatStream> = 0;
    virtual auto  Images() -> mtc::api<IImageStore> = 0;

    virtual auto  Commit() -> mtc::api<ISerialized> = 0;
    virtual void  Remove() = 0;

    virtual auto  NewPatch() -> mtc::api<IPatch> = 0;
  };

  struct IStorage::ISerialized::IPatch: public mtc::Iface
  {
    virtual void  Delete( EntityId ) = 0;
    virtual auto  Update( EntityId, const void*, size_t ) -> mtc::api<const IEntity> = 0;

    virtual void  Commit() = 0;
  };

  struct IContentsIndex: public mtc::Iface
  {
    struct IKeyValue;
    struct IEntities;

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
    virtual auto  SetEntity( EntityId id,
      mtc::api<const IContents>         index = nullptr,
      mtc::api<const mtc::IByteBuffer>  attrs = nullptr,
      mtc::api<const mtc::IByteBuffer>  image = nullptr ) -> mtc::api<const IEntity> = 0;

   /*
    * Commit()
    *
    * Writes all index data to storage held inside and return access to stored data.
    */
    virtual auto  Commit() -> mtc::api<IStorage::ISerialized> = 0;

   /*
    * Reduce()
    *
    * Return pointer to simplified version of index being optimized.
    */
    virtual auto  Reduce() -> mtc::api<IContentsIndex> = 0;

   /*
    * Index statistics and service information
    */
    virtual auto  GetMaxIndex() const -> uint32_t = 0;
//    virtual auto  CountEntities() const -> uint32_t = 0;

   /*
    * Blocks search api
    */
  };

 /*
  * IContentsIndex::IKeyValue
  *
  * Internal indexer interface to set key -> value pairs for object being indexed.
  */
  struct IContentsIndex::IKeyValue
  {
    virtual void  Insert( std::string_view key, std::string_view value ) = 0;
  };

  struct IContentsIndex::IEntities: public mtc::Iface
  {
    struct Reference
    {
      uint32_t    entity;
      const char* details;
      uint32_t    detsize;
    };
    virtual auto  Find( uint32_t ) -> Reference = 0;
  };

 /*
  * IContents - keys indexing API provided to IContentsIndex::SetEntity()
  *
  * Method is called with sing interface argument to send (key; value)
  * pairs to contents index
  */
  struct IContents: public mtc::Iface
  {
    virtual void  Enumerate( IContentsIndex::IKeyValue* ) const = 0;
  };

}

# endif   // __palmira_api_contents_index_hxx__
