# if !defined( __palmira_src_index_override_entities_hxx__ )
# define __palmira_src_index_override_entities_hxx__
# include "../../api/contents-index.hxx"

namespace palmira {
namespace index {

  class Override
  {
    mtc::api<const IEntity> entity;

  public:
    Override( mtc::api<const IEntity> );

    auto  Index( uint32_t ix ) -> mtc::api<const IEntity>;
    auto  Extras( const mtc::api<const mtc::IByteBuffer>& ) -> mtc::api<const IEntity>;

  };

}}

# endif   // !__palmira_src_index_override_entities_hxx__
