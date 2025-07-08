# if !defined( __palmira_texts_DOM_body_hpp__ )
# define __palmira_texts_DOM_body_hpp__
# include "DOM-text.hpp"

namespace palmira {
namespace texts {

  struct TextToken
  {
    enum: unsigned
    {
      lt_space = 1,
      is_punct = 2
    };

    unsigned        uFlags;
    const widechar* pwsstr;
    uint32_t        offset;
    uint32_t        length;
  };

  template <class Allocator>
  class Body
  {
  public:
   ~Body();

  public:
    auto  GetMarkup() const -> const std::vector<MarkupTag, Allocator>&  {  return markup;  }
    auto  GetTokens() const -> const std::vector<TextToken, Allocator>&  {  return tokens;  }

    auto  AddBuffer( const widechar*, size_t ) -> const widechar*;
    auto  GetMarkup() -> std::vector<MarkupTag, Allocator>&  {  return markup;  }
    auto  GetTokens() -> std::vector<TextToken, Allocator>&  {  return tokens;  }

    auto  Serialize( IText* ) -> IText*;

  protected:
    std::vector<MarkupTag, Allocator> markup;
    std::vector<TextToken, Allocator> tokens;
    std::vector<widechar*, Allocator> bufbox;

  };

  // Body template implementation

  template <class Allocator>
  Body<Allocator>::~Body()
  {
    auto  deallocator = typename std::allocator_traits<Allocator>::template rebind_alloc<widechar>(
      bufbox.get_allocator() );

    for ( auto next: bufbox )
      deallocator.deallocate( next, 0 );
  }

  template <class Allocator>
  auto  Body<Allocator>::AddBuffer( const widechar* pws, size_t len ) -> const widechar*
  {
    auto  palloc = typename std::allocator_traits<Allocator>::template rebind_alloc<widechar>(
      bufbox.get_allocator() ).allocate( len );

    bufbox.push_back( (widechar*)memcpy(
      palloc, pws, len * sizeof(widechar) ) );

    return bufbox.back();
  }

  template <class Allocator>
  auto  Body<Allocator>::Serialize( IText* text ) -> IText*
  {
    auto  lineIt = tokens.begin();
    auto  markIt = markup.begin();
    auto  fPrint = std::function<void( IText*, uint32_t )>( [&]( IText* to, uint32_t up )
      {
        while ( lineIt != tokens.end() && lineIt - tokens.begin() <= up )
        {
        // check if print next line to current IText*
          if ( markIt == markup.end() || (lineIt - tokens.begin()) < markIt->uLower )
          {
            to->AddWideStr( lineIt->pwsstr, lineIt->length );

            if ( ++lineIt == tokens.end() ) return;
              continue;
          }

        // check if open new span
          if ( lineIt - tokens.begin() >= markIt->uLower )
          {
            auto  new_to = to->AddTextTag( markIt->format );
            auto  uUpper = markIt->uUpper;
              ++markIt;
            fPrint( new_to.ptr(), uUpper );
          }
        }
      } );

    fPrint( text, -1 );

    return text;
  }

}}

# endif   // !__palmira_texts_DOM_body_hpp__
