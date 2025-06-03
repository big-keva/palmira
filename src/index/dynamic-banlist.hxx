# if !defined( __palmira_src_index_dynamic_banlist_hxx__ )
# define __palmira_src_index_dynamic_banlist_hxx__
# include <stdexcept>
# include <climits>
# include <atomic>

namespace palmira {
namespace index   {
namespace dynamic {

  template <class Allocator>
  class BanList: protected std::vector<std::atomic<uint32_t>, Allocator>
  {
    using vector_type = std::vector<std::atomic<uint32_t>, Allocator>;

    enum: size_t
    {
      element_size = sizeof(uint32_t),
      element_bits = element_size * CHAR_BIT
    };

  public:
    BanList( Allocator alloc = Allocator() ):
      vector_type( alloc ) {}
    BanList( size_t maxSize, Allocator alloc = Allocator() ):
      vector_type( (maxSize + element_bits - 1) / element_bits, alloc ) {}

  public:
    void  Set( uint32_t );
    bool  Get( uint32_t ) const;
  };

  template <class Allocator>
  void  BanList<Allocator>::Set( uint32_t uvalue )
  {
    auto  uindex = size_t(uvalue / element_bits);
    auto  ushift = uindex % element_bits;
    auto  ddmask = (1 << ushift);

    if ( uindex < vector_type::size() )
    {
      auto& item = vector_type::operator[]( uindex );

      for ( auto uval = item.load(); !item.compare_exchange_weak( uval, uval | ddmask ); )
        (void)NULL;
    } else throw std::range_error( "entity index exceeds deleted map capacity" );
  }

  template <class Allocator>
  bool  BanList<Allocator>::Get( uint32_t uvalue ) const
  {
    auto  uindex = size_t(uvalue / element_bits);
    auto  ushift = uindex % element_bits;
    auto  ddmask = (1 << ushift);

    if ( uindex < vector_type::size() )
      return (vector_type::operator[]( uindex ).load() & ddmask) != 0;
    return false;
  }

}}}

# endif   // __palmira_src_index_dynamic_banlist_hxx__
