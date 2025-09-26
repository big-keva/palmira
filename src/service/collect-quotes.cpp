# include "collect-quotes.hpp"

#include <stdexcept>

namespace palmira {
namespace collect {

  enum: size_t
  {
    max_BM25Term_count = 0x100,

    max_EntrySet_count = 0x40,
    max_EntryPos_count = 0x100,

    max_BM25_Abstract_size = sizeof(Abstract) + max_BM25Term_count * sizeof(Abstract::BM25Term),
    max_Rich_Abstract_size = sizeof(Abstract) + max_EntrySet_count * sizeof(Abstract::EntrySet)
                                              + max_EntryPos_count * sizeof(Abstract::EntrySet::EntryPos),

    max_Rand_Abstract_size = std::max( max_BM25_Abstract_size, max_Rich_Abstract_size )
  };

 /*
  * Abstracts::abstracts
  *
  * Хранилище описаний найденных документов - цитат релевантных фрагментов, терминов и весов.
  */
  struct Abstracts::abstracts
  {
    using   abstract_data = std::aligned_storage<max_Rand_Abstract_size, alignof(Abstract)>::type;

    struct  abstract_item
    {
      uint32_t      udocid;
      abstract_data stored;
    };

    unsigned        limit;
    unsigned        count = 0;
    abstract_item   items[1];

  public:
    abstracts( unsigned ulimit ): limit( ulimit ) {}

  public:
    static
    auto  Create( unsigned maxcount ) -> abstracts*;

  };

  auto  CopyAbstract( Abstract* output, const Abstract& source ) -> Abstract*
  {
    output->nWords = source.nWords;

    switch ( output->dwMode = source.dwMode )
    {
      case Abstract::BM25:
      {
        auto  nterms = std::min( source.factors.size(), size_t(max_BM25Term_count) );

        output->factors.beg = (Abstract::BM25Term*)(output + 1);
        output->factors.end = output->factors.beg + nterms;

        std::copy( source.factors.beg, source.factors.beg + nterms, (Abstract::BM25Term*)output->factors.beg );
        break;
      }
      case Abstract::Rich:
      {
        auto  entptr = (Abstract::EntrySet*)(output + 1);
        auto  entend = entptr + max_EntrySet_count;
        auto  posptr = (Abstract::EntrySet::EntryPos*)entend;
        auto  posend = posptr + max_EntryPos_count;

        output->entries.beg = entptr;

        for ( auto beg = source.entries.beg; beg < source.entries.end && entptr != entend && posptr != posend; ++beg )
        {
          *entptr = { beg->limits, beg->weight, beg->center, { posptr, posptr } };

          for ( auto pos = beg->spread.pbeg; pos != beg->spread.pend && posptr != posend; ++pos )
            *posptr++ = *pos++;

          (*entptr++).spread.pend = posptr;
        }

        output->entries.end = entptr;
        break;
      }
      default:  break;
    }
    return output;
  }

  // Abstracts implementation

  Abstracts::Abstracts( unsigned maxcount ): storage( abstracts::Create( maxcount ) )
  {
  }

  Abstracts::~Abstracts()
  {
    if ( storage != nullptr )
    {
      storage->~abstracts();
      std::allocator<abstracts>().deallocate( storage, 0 );
    }
  }

  void  Abstracts::Set( uint32_t newid, const Abstract& abstr, uint32_t oldid )
  {
    if ( storage != nullptr )
    {
      for ( auto p = storage->items; p != storage->items + storage->count; ++p )
        if ( p->udocid == oldid )
        {
          CopyAbstract( (Abstract*)&p->stored, abstr );
          return void(p->udocid = newid);
        }
      if ( storage->count < storage->limit )
      {
        CopyAbstract( (Abstract*)&storage->items[storage->count], abstr );
        return void(storage->items[storage->count++].udocid = newid);
      }
      throw std::logic_error( "Abstracts storage overflow" );
    }
    throw std::logic_error( "Abstracts has no data storage" );
  }

  auto  Abstracts::Get( uint32_t getid ) const -> const Abstract*
  {
    if ( storage != nullptr )
      for ( auto p = storage->items; p != storage->items + storage->count; ++p )
        if ( p->udocid == getid )
          return (const Abstract*)&p->stored;
    return nullptr;
  }

  // Abstracts::abstracts implementation

  auto  Abstracts::abstracts::Create( unsigned maxcount ) -> abstracts*
  {
    auto  nalloc = sizeof(abstracts) + (maxcount - 1) * sizeof(abstracts::abstract_data);
    auto  nitems = (nalloc + sizeof(abstracts) - 1) / sizeof(abstracts);
    auto  palloc = std::allocator<abstracts>().allocate( nitems );

    return new( palloc ) abstracts( maxcount );
  }

}}
