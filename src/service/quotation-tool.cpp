# include "quotation-tool.hpp"
# include "../toolset/object-zmap.hpp"
# include "../toolset/zip-unzip.hpp"
# include <structo/enquote/quotations.hpp>
# include <mtc/bitset.h>
# include <fnmatch.h>

namespace palmira {
namespace enquote {

  class QuoteOpts
  {
    class FieldAccessor;
    class IncludeFields;
    class ExcludeFields;
    class FnMatchFields;

    struct Bundle: protected std::vector<char>, protected mtc::api<const mtc::IByteBuffer>
    {
      mtc::span<const char> mkup;
      mtc::span<const char> body;

      Bundle( const api& src ): api( src )  {}
      Bundle() = default;

      auto  packed() -> std::vector<char>&  {  return *this;  }
      bool  empty() const {  return body.empty();  }
    };

  public:
    QuoteOpts( const FieldHandler&, Scheme );
    QuoteOpts( const FieldHandler&, Scheme, const include_t&, const mtc::array_charstr& );
    QuoteOpts( const FieldHandler&, Scheme, const exclude_t&, const mtc::array_charstr& );
    QuoteOpts( const FieldHandler&, Scheme, const fnmatch_t&, const mtc::array_charstr& );

    auto  Get( mtc::api<IContentsIndex> ) const -> collect::QuotesFn;

    static  auto  LoadBundle( const mtc::api<const mtc::IByteBuffer>& ) -> Bundle;

  protected:
    Scheme                          q_mode;
    std::shared_ptr<FieldAccessor>  filter;
    const FieldHandler*             fields;

  };

  class QuoteOpts::FieldAccessor: public FieldHandler
  {
  public:
    FieldAccessor( const FieldHandler* pfd ): fields( pfd ) {}

    using FieldHandler::Get;

    auto  Add( const std::string_view& ) -> FieldOptions* override              {  throw std::logic_error( "not implemented" );  }
    auto  Get( const std::string_view& ) const -> const FieldOptions* override  {  throw std::logic_error( "not implemented" );  }

  protected:
    const FieldHandler*     fields;
    std::vector<uint64_t>   filter;

  };

  struct QuoteOpts::IncludeFields: FieldAccessor
  {
    IncludeFields( const FieldHandler* fds, const mtc::array_charstr& inc ): FieldAccessor( fds )
    {
      for ( auto& next: inc )
        if ( auto pf = fds->Get( next ); pf != nullptr )
          mtc::bitset_set( filter, pf->id );
    }
    auto  Get( uint32_t id ) const -> const FieldOptions* override
    {
      return mtc::bitset_get( filter, id ) ? fields->Get( id ) : nullptr;
    }
    using FieldAccessor::Get;
  };

  struct QuoteOpts::ExcludeFields: FieldAccessor
  {
    ExcludeFields( const FieldHandler* fds, const mtc::array_charstr& exc ): FieldAccessor( fds )
    {
      for ( auto& next: exc )
        if ( auto pf = fds->Get( next ); pf != nullptr )
          mtc::bitset_set( filter, pf->id );
    }
    auto  Get( uint32_t id ) const -> const FieldOptions* override
    {
      return !mtc::bitset_get( filter, id ) ? fields->Get( id ) : nullptr;
    }
    using FieldAccessor::Get;
  };

  struct QuoteOpts::FnMatchFields: FieldAccessor
  {
    FnMatchFields( const FieldHandler* fds, const mtc::array_charstr& matches ): FieldAccessor( fds )
    {
      const FieldOptions* pf;

      for ( uint32_t id = 0; id == 0 || (pf = fields->Get( id )) != nullptr; ++id )
      {
        if ( pf == nullptr )
          continue;

        for ( auto& match: matches )
          if ( ::fnmatch( match.c_str(), pf->name.data(), 0 ) == 0 )
          {
            mtc::bitset_set( filter, pf->id );
            break;
          }
      }
    }
    auto  Get( uint32_t id ) const -> const FieldOptions* override
    {
      return mtc::bitset_get( filter, id ) ? fields->Get( id ) : nullptr;
    }
    using FieldAccessor::Get;
  };

  // QuoteOpts implementation

  QuoteOpts::QuoteOpts( const FieldHandler& fds, Scheme mode ):
    q_mode( mode ),
    fields( mode != Scheme::Absent ? &fds : nullptr )
  {
  }

  QuoteOpts::QuoteOpts( const FieldHandler& fds, Scheme type, const include_t&, const mtc::array_charstr& inc ): q_mode( type )
  {
    fields = (filter = std::shared_ptr<FieldAccessor>( new IncludeFields( &fds, inc ))).get();
  }

  QuoteOpts::QuoteOpts( const FieldHandler& fds, Scheme type, const exclude_t&, const mtc::array_charstr& exc ): q_mode( type )
  {
    fields = (filter = std::shared_ptr<FieldAccessor>( new ExcludeFields( &fds, exc ))).get();
  }

  QuoteOpts::QuoteOpts( const FieldHandler& fds, Scheme type, const fnmatch_t&, const mtc::array_charstr& fnm ): q_mode( type )
  {
    fields = (filter = std::shared_ptr<FieldAccessor>( new FnMatchFields( &fds, fnm ))).get();
  }

  auto  QuoteOpts::Get( mtc::api<IContentsIndex> ctxIndex ) const -> collect::QuotesFn
  {
    switch ( q_mode )
    {
      case Scheme::Absent:
        return []( uint32_t, const structo::queries::Abstract& ) {  return mtc::array_zval();  };
      case Scheme::Sketch:
        return []( uint32_t, const structo::queries::Abstract& /*abstr*/ ) -> mtc::array_zval
        {
          return {};
        };
      case Scheme::Source:
        return [ctxIndex, fieldMan = filter, fieldPtr = fields]( uint32_t id, const structo::queries::Abstract& abstr )
        {
          auto  entity = ctxIndex->GetEntity( id );
          auto  bundle = entity != nullptr ? LoadBundle( entity->GetBundle() ) : Bundle();
          auto  output = mtc::array_zval();

          if ( !bundle.empty() )
            structo::enquote::QuoteMachine( *fieldPtr ).TextSource()( ZmapAsText( output ), bundle.body, bundle.mkup, abstr );

          return output;
        };
      case Scheme::Struct: default:
        return [ctxIndex, fieldMan = filter, fieldPtr = fields]( uint32_t id, const structo::queries::Abstract& abstr )
        {
          auto  entity = ctxIndex->GetEntity( id );
          auto  bundle = entity != nullptr ? LoadBundle( entity->GetBundle() ) : Bundle();
          auto  output = mtc::array_zval();

          if ( !bundle.empty() )
            structo::enquote::QuoteMachine( *fieldPtr ).Structured()( ZmapAsText( output ), bundle.body, bundle.mkup, abstr );

          return output;
        };
    }
  }

  auto  QuoteOpts::LoadBundle( const mtc::api<const mtc::IByteBuffer>& src ) -> Bundle
  {
    Bundle      bundle( src );
    const char* source;
    size_t      srclen;

    if ( src == nullptr || src->GetPtr() == nullptr )
      return bundle;

    // check for formats
    if ( (source = mtc::zmap::serial::find( src->GetPtr(), "ft" )) != nullptr )
    {
      if ( *source++ != mtc::zval::z_array_char )
        throw std::runtime_error( "invalid object package format" );
      source = ::FetchFrom( source, srclen );
      bundle.mkup = { source, srclen };
    }
    // check compressed image
    if ( (source = mtc::zmap::serial::find( src->GetPtr(), "ip" )) != nullptr )
    {
      if ( *source++ != mtc::zval::z_array_char )
        throw std::runtime_error( "invalid object package format" );
      source = ::FetchFrom( source, srclen );
      bundle.body = (bundle.packed() = Unpack( { source, srclen } ));
    }
      else
  // check uncompressed image
    if ( (source = mtc::zmap::serial::find( src->GetPtr(), "im" )) != nullptr )
    {
      if ( *source++ != mtc::zval::z_array_char )
        throw std::runtime_error( "invalid object package format" );
      source = ::FetchFrom( source, srclen );
      bundle.body = { source, srclen };
    }

    return bundle;
  }

  auto  Function( mtc::api<IContentsIndex> index, const FieldHandler& fdset, Scheme quote ) -> collect::QuotesFn
  {
    return QuoteOpts( fdset, quote ).Get( index );
  }

  auto  Function( mtc::api<IContentsIndex> index, const FieldHandler& fdset, Scheme quote,
    const include_t&, const mtc::array_charstr& patch ) -> collect::QuotesFn
  {
    return QuoteOpts( fdset, quote, include, patch ).Get( index );
  }

  auto  Function( mtc::api<IContentsIndex> index, const FieldHandler& fdset, Scheme quote,
    const exclude_t&, const mtc::array_charstr& patch ) -> collect::QuotesFn
  {
    return QuoteOpts( fdset, quote, exclude, patch ).Get( index );
  }

  auto  Function( mtc::api<IContentsIndex> index, const FieldHandler& fdset, Scheme quote,
    const fnmatch_t&, const mtc::array_charstr& patch ) -> collect::QuotesFn
  {
    return QuoteOpts( fdset, quote, fnmatch, patch ).Get( index );
  }

  auto  StringToScheme( std::string_view type ) -> Scheme
  {
    return
      type == "absent" ? Scheme::Absent :
      type == "scheme" ? Scheme::Sketch :
      type == "source" ? Scheme::Source : Scheme::Struct;
  }

  auto  SchemeToString( Scheme type ) -> std::string_view
  {
    switch ( type )
    {
      case Scheme::Absent:  return "absent";
      case Scheme::Sketch:  return "scheme";
      case Scheme::Source:  return "source";
      default:              return "struct";
    }
  }

}}
