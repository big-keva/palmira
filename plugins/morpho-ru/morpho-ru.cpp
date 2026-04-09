# include <structo/lang-api.hpp>
# include <libmorph/api.hpp>
# include <libmorph/rus.h>
# include <moonycode/codes.h>
# include <mtc/bitset.h>

# define EXPORTED_API __attribute__((__visibility__("default")))

struct Lemmatizer final: structo::ILemmatizer
{
  implement_lifetime_stub

  Lemmatizer()
  {
    mlmaruGetAPI( LIBMORPH_API_4_MAGIC ":utf-16", (void**)&mlma );
    mlfaruGetAPI( LIBFUZZY_API_4_MAGIC ":utf-16", (void**)&mlfa );
  }

  int   Lemmatize( IWord* out, unsigned options, const widechar* str, size_t len ) override
  {
    SGramInfo   gInfos[0x80];

    if ( options & structo::lex_lemma )
    {
      SLemmInfoW  lemmas[0x20];
      auto        nlemma = mlma->Lemmatize( { str, len }, lemmas, {}, gInfos, sfIgnoreCapitals );

      // check for verb imperatives and clean if not only one present
      if ( nlemma > 1 )
      {
        for ( int i = 0; i < nlemma; ++i )
          if ( (lemmas[i].pgrams->wdInfo & 0x3f) <= 6 && lemmas[i].pgrams->idForm == 2 )
          {
            memcpy( lemmas + 1, lemmas + i + 1, (nlemma - i - 1) * sizeof(SLemmInfoW) );
              --nlemma;
            break;
          }
      }

      // check non-flective forms and suppress if flectives are present
      if ( nlemma > 0 )
      {
        std::sort( lemmas, lemmas + nlemma, []( const SLemmInfoW& l, const SLemmInfoW& r )
          {
            int   rescmp = l.pgrams->idForm - r.pgrams->idForm;

            return rescmp != 0 ? rescmp < 0 : l.nlexid < r.nlexid;
          } );

        for ( int idl = 0; idl < nlemma; ++idl )
        {
          uint8_t  aforms[0x100];
          bool     is_inv = false;

          for ( auto idf = 0U; idf < lemmas[idl].ngrams; ++idf )
            is_inv |= (aforms[idf] = lemmas[idl].pgrams[idf].idForm) == 0xff;

          if ( is_inv && idl > 0 )
            break;

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
      float       before = 0.0;

      if ( nstems > 0 )
      {
        if ( codepages::strtolower( slower, std::size(slower), str, len ) == size_t(-1) )
          return 0;

      // get total weight
        for ( auto ids = 0; ids < nstems; ++ids )
          wtotal += astems[ids].weight;

      // limit selection to:
      // - 80% of coverage;
      // - next element is 15% worse than previous
        wtotal *= 0.8;

        for ( auto ids = 0; ids < nstems
          && wtotal > 0.0
          && astems[ids].weight > 0.1
          && before * 0.8 <= astems[ids].weight; wtotal -= (before = astems[ids++].weight) )
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

  int   Wildcards( IWord* out, unsigned /*options*/, const widechar* str, size_t len ) override
  {
    return mlma->FindMatch( { str, len }, [&]( lexeme_t nlexid, int nforms, const SStrMatch* pforms ) -> int
      {
        uint64_t  uforms[4] = { 0, 0, 0, 0 };
        formid_t  fidset[0x100];
        formid_t* fidptr = fidset;

        for ( auto p = pforms, e = pforms + nforms; p != e; ++p )
          mtc::bitset_set( uforms, p->id );

        for ( auto u = 0; u < 0x100; ++u )
          if ( mtc::bitset_get( uforms, u ) )
            *fidptr++ = formid_t(u);

        return out->AddTerm( nlexid, 1.0, fidset, fidptr - fidset ), 0;
      } ) ;
  }

public:
  IMlmaWcXX*  mlma;
  IMlfaWcXX*  mlfa;

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
