# if !defined( __palmira_api_contents_index_hxx__ )
# define __palmira_api_contents_index_hxx__
# include <mtc/byteBuffer.h>
# include <string_view>
# include <mtc/zmap.h>

namespace palmira
{

  struct IEntity;             // common object properties
  struct IContents;           // indexable entity properties interface

  struct IEntity: public mtc::Iface
  {
    struct Attribute;

    virtual auto  GetId() const -> Attribute = 0;
    virtual auto  GetIndex() const -> uint32_t = 0;
    virtual auto  GetAttributes() const -> Attribute = 0;
  };

  struct IEntity::Attribute: public std::string_view, protected mtc::api<Iface>
  {
    Attribute( const Attribute& ) = default;
    Attribute( const std::string_view& s, api i = nullptr ): std::string_view( s ), api( i ) {}
  };

  struct IContentsIndex: public mtc::Iface
  {
    struct IKeyValue;

    virtual auto  GetEntity( std::string_view id ) const -> mtc::api<const IEntity> = 0;
    virtual bool  DelEntity( std::string_view id ) = 0;

   /*
    * IContentsIndex::SetEntity()
    *
    * Inserts object with id to the index, sets dynamic untyped attributes
    * and indexable properties
    */
    virtual auto  SetEntity( std::string_view id,
      mtc::api<const IContents>         props = nullptr,
      mtc::api<const mtc::IByteBuffer>  attrs = nullptr,
      mtc::api<const mtc::IByteBuffer>  image = nullptr ) -> mtc::api<const IEntity> = 0;
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
