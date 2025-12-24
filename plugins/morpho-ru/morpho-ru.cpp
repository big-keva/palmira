# include <structo/lang-api.hpp>
# include <libmorph/rus/include/mlma1049.h>
# include <libmorph/rus/include/mlfa1049.h>
# include <moonycode/codes.h>

# define EXPORTED_API __attribute__((__visibility__("default")))

struct Lemmatizer final: structo::ILemmatizer
{
  implement_lifetime_stub

  Lemmatizer()
  {
    mlmaruLoadWcAPI( &mlma );
    mlfaruLoadWcAPI( &mlfa );
  }

  int   Lemmatize( IWord* out, unsigned options, const widechar* str, size_t len ) override
  {
    SGramInfo   gInfos[0x80];

    if ( options & structo::lex_lemma )
    {
      SLemmInfoW  lemmas[0x20];
      auto        nlemma = mlma->Lemmatize( { str, len }, lemmas, {}, gInfos );

      if ( nlemma > 0 )
      {
        std::sort( lemmas, lemmas + nlemma, []( const SLemmInfoW& l, const SLemmInfoW& r )
          {  return l.pgrams->idForm < r.pgrams->idForm;  } );

        if ( lemmas[nlemma - 1].pgrams->idForm == 0xff )
          while ( nlemma > 1 && lemmas[nlemma - 2].pgrams->idForm == 0xff )
            --nlemma;

        for ( int idl = 0; idl < nlemma; ++idl )
        {
          uint8_t  aforms[0x100];

          for ( unsigned idf = 0; idf < lemmas[idl].ngrams; ++idf )
            aforms[idf] = lemmas[idl].pgrams[idf].idForm;

          out->AddTerm( lemmas[idl].nlexid, 1.0 / nlemma, aforms, lemmas[idl].ngrams );
        }
      }
    }
    if ( options & structo::lex_fuzzy )
    {
      SStemInfoW  astems[0x20];
      auto        nstems = mlfa->Lemmatize( { str, len }, astems, {}, gInfos );
      widechar    slower[0x40];
      float       wtotal = 0.0;

      if ( nstems > 0 )
      {
        if ( codepages::strtolower( slower, std::size(slower), str, len ) == size_t(-1) )
          return 0;

        std::sort( astems, astems + nstems, []( const SStemInfoW& l, const SStemInfoW& r )
          {  return l.weight > r.weight;  } );

        for ( auto ids = 0; ids < nstems; ++ids )
          wtotal += astems[ids].weight;

        wtotal *= 0.8;

        for ( auto ids = 0; ids < nstems && wtotal > 0.0 && astems[ids].weight > 0.1; wtotal -= astems[ids++].weight )
        {
          uint8_t  aforms[0x100];

          for ( unsigned idf = 0; idf < astems[ids].ngrams; ++idf )
            aforms[idf] = astems[ids].pgrams[idf].idForm;

          out->AddStem( slower, astems[ids].ccstem, astems[ids].nclass,
            astems[ids].weight, aforms, astems[ids].ngrams );
        }
      }
    }
    return 0;
  }

public:
  IMlmaWc*  mlma;
  IMlfaWc*  mlfa;

};

extern "C"
{
  int   EXPORTED_API  CreateLemmatizer( structo::ILemmatizer** out, const char* /* cfg */ )
  {
    if ( out == nullptr )
      return EINVAL;
    (*out = new Lemmatizer())->Attach();
      return 0;
  }
}
