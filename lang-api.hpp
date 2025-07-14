# if !defined( __palmira_lang_api_hpp__ )
# define __palmira_lang_api_hpp__
#include <contents-index.hpp>

# include "text-api.hpp"

namespace palmira {

  struct IImage: public IContents
  {
    virtual void  AddTerm(
      unsigned        pos,
      unsigned        idl,
      uint32_t        lex, const uint8_t*, size_t ) = 0;
    virtual void  AddStem(
      unsigned        pos,
      unsigned        idl,
      const widechar* str,
      size_t          len,
      uint32_t        cls, const uint8_t*, size_t ) = 0;
  };

  struct DecomposeHolder;

  typedef int (*DecomposeInit)( DecomposeHolder**, const char* );
  typedef int (*DecomposeFree)( DecomposeHolder* );

 /*
  * DecomposeText
  *
  * Creates text decomposition to lexemes for each word passed.
  * Has to be thread-safe, i.e. contain no read-write internal data.
  *
  */
  typedef int (*DecomposeText)( DecomposeHolder*, palmira::IImage*,
    const TextToken*, size_t,
    const MarkupTag*, size_t );

};

# endif // !__palmira_lang_api_hpp__
