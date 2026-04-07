# include <structo/lang-api.hpp>
# include <moonycode/codes.h>

extern "C"  int   CreateLemmatizer( structo::ILemmatizer**, const char* );

struct DumpWord: structo::ILemmatizer::IWord
{
  implement_lifetime_stub

  void  AddTerm( uint32_t lex,
    float flp, const uint8_t* forms, size_t count ) override
  {
    auto  prefix = "";

    (void)flp;

    fprintf( stdout, "lexid: %-6u forms: ", lex );

    for ( size_t i = 0; i < count; ++i, prefix = ", " )
      fprintf( stdout, "%s%d", prefix, forms[i] );

    fputs( "\n", stdout );
  }
  void  AddStem( const widechar* pws, size_t len, uint32_t cls,
    float flp, const uint8_t* forms, size_t count ) override
  {
    auto  prefix = "";

    (void)len;
    (void)cls;

    fprintf( stdout, "range: %6.4f  lemma: %-15s forms: ", flp, codepages::widetombcs( codepages::codepage_utf8, pws ).c_str() );

    for ( size_t i = 0; i < count; ++i, prefix = ", " )
      fprintf( stdout, "%s%d", prefix, forms[i] );

    fputs( "\n", stdout );
  }
};

int main()
{
  mtc::api<structo::ILemmatizer>  morpho;

  if ( CreateLemmatizer( morpho, "" ) == 0 )
  {
    DumpWord  dumpws;

    morpho->Lemmatize( &dumpws, structo::lex_lemma + structo::lex_fuzzy, u"командировочка" );
  }
  return 0;
}
