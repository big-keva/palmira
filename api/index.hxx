# if !defined( __palmira_api_index_hxx__ )
# define __palmira_api_index_hxx__
# include <mtc/interfaces.h>
# include <string_view>
# include <mtc/zmap.h>

namespace palmira
{

  struct IEntity: public mtc::Iface
  {
    class Id;
    class Attributes;

    virtual auto  GetId() const -> Id = 0;
    virtual auto  GetIndex() const -> uint32_t = 0;
    virtual auto  GetAttributes() const -> Attributes = 0;
  };

  struct IEntity::Id: public std::string_view, protected mtc::api<Iface>
  {
    Id( const Id& ) = default;
    Id( const std::string_view& s, api i = nullptr ): std::string_view( s ), api( i ) {}
  };

  struct IEntity::Attributes: public mtc::zmap::dump, protected mtc::api<Iface>
  {
    Attributes( const Attributes& ) = default;
    Attributes( const dump& d, api i = nullptr ): dump( d ), api( i ) {}
  };

}

# endif   // __palmira_api_index_hxx__
