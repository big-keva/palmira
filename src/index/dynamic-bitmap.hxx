# if !defined( __palmira_src_index_dynamic_bitmap_hxx__ )
# define __palmira_src_index_dynamic_bitmap_hxx__
# include <stdexcept>
# include <climits>
# include <atomic>

namespace palmira {
namespace index   {

  template <class Allocator = std::allocator<char>>
  class Bitmap: protected std::vector<std::atomic<uint32_t>, Allocator>
  {
    using vector_type = std::vector<std::atomic<uint32_t>, Allocator>;

    enum: size_t
    {
      element_size = sizeof(uint32_t),
      element_bits = element_size * CHAR_BIT
    };

  public:
    Bitmap( Allocator alloc = Allocator() ):
      vector_type( alloc ) {}
    Bitmap( size_t maxSize, Allocator alloc = Allocator() ):
      vector_type( (maxSize + element_bits - 1) / element_bits, alloc ) {}
    Bitmap( Bitmap&& bim ):
      vector_type( std::move( bim ) ) {}
    Bitmap& operator=( Bitmap&& bim )
      {  return vector_type::operator=( std::move( bim ) ), *this;  }

  public:
    void  Set( uint32_t );
    bool  Get( uint32_t ) const;
  };

  template <class Allocator>
  void  Bitmap<Allocator>::Set( uint32_t uvalue )
  {
    auto  uindex = size_t(uvalue / element_bits);
    auto  ushift = uvalue % element_bits;
    auto  ddmask = (1 << ushift);

    if ( uindex < vector_type::size() )
    {
      auto& item = vector_type::operator[]( uindex );

      for ( auto uval = item.load(); !item.compare_exchange_weak( uval, uval | ddmask ); )
        (void)NULL;
    } else throw std::range_error( "entity index exceeds deleted map capacity" );
  }

  template <class Allocator>
  bool  Bitmap<Allocator>::Get( uint32_t uvalue ) const
  {
    auto  uindex = size_t(uvalue / element_bits);
    auto  ushift = uvalue % element_bits;
    auto  ddmask = (1 << ushift);

    if ( uindex < vector_type::size() )
      return (vector_type::operator[]( uindex ).load() & ddmask) != 0;
    return false;
  }

}}

# endif   // __palmira_src_index_dynamic_bitmap_hxx__
