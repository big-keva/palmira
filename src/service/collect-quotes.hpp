#include <memory>

# include "DelphiX/queries.hpp"

namespace palmira {
namespace collect {

  using Abstract = DelphiX::queries::IQuery::Abstract;

  class Abstracts
  {
    struct abstracts;

    abstracts*  storage;

  public:
    Abstracts( unsigned maxcount );
   ~Abstracts();

    void  Set( uint32_t new_id, const Abstract&, uint32_t old_id = -1 );
    auto  Get( uint32_t get_id ) const -> const Abstract*;
  };

}}
