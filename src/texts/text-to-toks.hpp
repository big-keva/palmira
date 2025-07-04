# include "texts/DOM-text.hpp"
# include "texts/dissection.hpp"
# include <moonycode/chartype.h>

namespace palmira {
namespace texts   {

  template <class ... Classes>
  bool  IsCharClass( widechar c, unsigned cls, Classes ... classes )
  {
    return (codepages::charType[c] & 0xf0) == cls || IsCharClass( c, classes... );
  }

  template <>
  bool  IsCharClass( widechar c, unsigned cls )
  {
    return (codepages::charType[c] & 0xf0) == cls;
  }

  template <class T>
  class BodyBlocks
  {
    template <class Allocator>
    friend void  BreakWords( Body<Allocator>&, const BodyBlocks<T>& );
    template <class Allocator>
    friend void  CopyMarkup( Body<Allocator>&, const BodyBlocks<T>& );

  public:
    template <class Allocator>
    BodyBlocks( const std::vector<T, Allocator>& source ):
      data( source.data() ),
      size( source.size() ) {}

  protected:
    const T*  data;
    size_t    size;
  };

  template <class Allocator>
  void  BreakWords( Body<Allocator>& output, const BodyBlocks<TextChunk>& blocks )
  {
    uint32_t  offset = 0;

    for ( auto beg = blocks.data, end = beg + blocks.size; beg != end; offset += (beg++)->length )
    {
      auto  sblock = beg->GetWideStr();
      auto  buforg = output.AddBuffer( sblock.data(), sblock.size() );
      auto  ptrtop = buforg;
      auto  ptrend = ptrtop + sblock.size();

      while ( ptrtop != ptrend )
      {
        unsigned  uFlags = 0;

      // detect lower space
        if ( codepages::IsBlank( *ptrtop ) )
        {
          uFlags = TextToken::lt_space;

          for ( ++ptrtop; ptrtop != ptrend && codepages::IsBlank( *ptrtop ); ++ptrtop )
            (void)NULL;
        }

      // select next word
        if ( ptrtop != ptrend )
        {
          auto  origin = ptrtop;

        // get substring length
          if ( IsCharClass( *ptrtop, codepages::cat_P, codepages::cat_S ) )  ++ptrtop;
            else
          while ( ptrtop != ptrend && !codepages::IsBlank( *ptrtop ) && !IsCharClass( *ptrtop,
            codepages::cat_P,
            codepages::cat_S ) ) ++ptrtop;

        // create word string
          output.GetTokens().push_back( { uFlags, origin,
            uint32_t(offset + origin - buforg), uint32_t(ptrtop - origin) } );
        }
      }
    }
  }

  template <class Allocator>
  void  CopyMarkup(
    Body<Allocator>&              output,
    const BodyBlocks<MarkupTag>&  markup )
  {
    using vectorType = typename std::remove_reference<decltype(output.GetMarkup())>::type;
    using formatType = typename vectorType::iterator;

    auto  fwdFmt = std::vector<formatType>();
    auto  bckFmt = std::vector<formatType>();

    output.GetMarkup().insert( output.GetMarkup().end(), markup.data, markup.data + markup.size );

  // create formats indices
    for ( auto beg = output.GetMarkup().begin(); beg != output.GetMarkup().end(); ++beg )
    {
      fwdFmt.push_back( beg );
      bckFmt.push_back( beg );
    }
    std::sort( bckFmt.begin(), bckFmt.end(), []( formatType a, formatType b )
    {
      auto  rescmp = a->uUpper - b->uUpper;
      return rescmp != 0 ? rescmp < 0 : a->uLower < b->uLower;
    } );

  // open and close the items
    auto  fwdtop = fwdFmt.begin();
    auto  bcktop = bckFmt.begin();

    for ( auto windex = 0; windex != output.GetTokens().size(); ++windex )
    {
      auto& rfword = output.GetTokens().at( windex );
      auto  uLower = rfword.offset;
      auto  uUpper = rfword.offset + rfword.length - 1;

      while ( fwdtop != fwdFmt.end() && (*fwdtop)->uLower <= uLower )
        (*fwdtop++)->uLower = windex;
      while ( bcktop != bckFmt.end() && (*bcktop)->uUpper <= uUpper )
        (*bcktop++)->uUpper = windex;
    }
  }

}}
