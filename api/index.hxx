# if !defined( __palmira_api_index_hxx__ )
# define __palmira_api_index_hxx__
# include <mtc/interfaces.h>
# include <string_view>
# include <mtc/zmap.h>

namespace palmira
{

  struct IEntity: public mtc::Iface
  {
    struct Attribute;

    virtual auto  GetId() const -> Attribute = 0;
    virtual auto  GetIndex() const -> uint32_t = 0;
    virtual auto  GetAttributes() const -> Attribute = 0;
  };

/*
 * IMapping
 *
 * Mapping object properties to be stored in inverted index and in a plain data storage.
 */
  struct IMapping: public mtc::Iface
  {
  };

  struct IStorage: public mtc::Iface
  {
    virtual auto  GetEntity( std::string_view ) const -> mtc::api<const IEntity> = 0;
    virtual auto  GetEntity( std::string_view )       -> mtc::api<      IEntity> = 0;

    virtual bool  DelEntity( std::string_view ) const = 0;

    virtual auto  SetEntity( std::string_view, mtc::api<const IMapping> ) -> mtc::api<const IEntity> = 0;
  };

  struct IEntity::Attribute: public std::string_view, protected mtc::api<Iface>
  {
    Attribute( const Attribute& ) = default;
    Attribute( const std::string_view& s, api i = nullptr ): std::string_view( s ), api( i ) {}
  };

}

# endif   // __palmira_api_index_hxx__
