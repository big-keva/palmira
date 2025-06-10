# include "../../api/dynamic-contents.hxx"
# include "dynamic-bitmap.hxx"
# include "../../api/static-contents.hxx"
# include <mtc/ptrpatch.h>

#include "dynamic-entities.hxx"

namespace palmira {
namespace index {
namespace flaked {

  namespace overriden
  {
    class base: public mtc::api<const IEntity>
    {
      using mtc::api<const IEntity>::api;

    public:
      base( const mtc::api<const IEntity>& entity ): api( entity ) {}
      base( const mtc::api<IEntity>& entity ): api( entity ) {}

    public:
      auto  GetId() const -> IEntity::Attribute         {  return ptr()->GetId();  }
      auto  GetIndex() const -> uint32_t                {  return ptr()->GetIndex();  }
      auto  GetAttributes() const -> IEntity::Attribute {  return ptr()->GetAttributes();  }
    };

    template <class based_on>
    class index_value: public based_on
    {
      using based_on::based_on;

    public:
      index_value( uint32_t i, const based_on& b ): based_on( b ), alternate_index( i ) {}

    public:
      auto  GetIndex() const -> uint32_t {  return alternate_index;  }

    protected:
      uint32_t alternate_index;

    };

    template <class based_on>
    class attributes_value: public based_on
    {
      using based_on::based_on;

    public:
      attributes_value( const IEntity::Attribute& a, const based_on& b ): based_on( b ), alternate_attributes( a ) {}

    public:
      auto  GetAttributes() const -> IEntity::Attribute {  return alternate_attributes;  }

    protected:
      IEntity::Attribute  alternate_attributes;

    };

    template <class based_on>
    class api_object final: public IEntity, protected based_on
    {
      implement_lifetime_control

    public:
      api_object( const based_on& b ): based_on( b ) {}

    protected:
      auto  GetId() const -> Attribute override {  return based_on::GetId();  }
      auto  GetIndex() const -> uint32_t override {  return based_on::GetIndex();  }
      auto  GetAttributes() const -> Attribute override {  return based_on::GetAttributes();  }

    };

    template <class based_on = base>
    auto  index( uint32_t i, const based_on& b )
      {  return index_value<based_on>( i, b );  }

    template <class based_on = base>
    auto  attributes( const IEntity::Attribute& a, const based_on& b )
      {  return index_value<based_on>( a, b );  }

    template <class based_on = base>
    auto  api( const based_on& b ) -> mtc::api<const IEntity>
      {  return new api_object<based_on>( b );  }

  }

  class ContentsIndex final: public IContentsIndex
  {
    struct IndexEntry
    {
      uint32_t                      uLower;
      uint32_t                      uUpper;
      std::atomic<IContentsIndex*>  pindex;
      Bitmap<>                      banned;
    };

    struct Flakes
    {
      using IndexPlace = typename std::aligned_storage<sizeof(IndexEntry), alignof(IndexEntry)>::type;

      auto  begin() const -> IndexEntry*;
      auto  end() const -> IndexEntry*;
      auto  back() const -> IndexEntry&;

    public:
      const size_t  ncount;
      IndexPlace    nested[1];

    };

  public:
    auto  GetEntity( EntityId ) const -> mtc::api<const IEntity> override;
    auto  GetEntity( uint32_t ) const -> mtc::api<const IEntity> override;
    bool  DelEntity( EntityId id ) override;
    auto  SetEntity( EntityId id,
      mtc::api<const IContents>         index = nullptr,
      mtc::api<const mtc::IByteBuffer>  attrs = nullptr ) -> mtc::api<const IEntity> override;
    auto  Commit() -> mtc::api<IStorage::ISerialized> override;
    auto  Reduce() -> mtc::api<IContentsIndex> override;
    auto  GetMaxIndex() const -> uint32_t override;
    auto  GetKeyBlock( const void*, size_t ) const -> mtc::api<IEntities> override;
    auto  GetKeyStats( const void*, size_t ) const -> BlockInfo override;
    
  protected:
    std::atomic<Flakes*>  indices = nullptr;

  };

  // ContentsIndex implementation

  auto  ContentsIndex::GetEntity( EntityId id ) const -> mtc::api<const IEntity>
  {
    auto  pitems = mtc::ptr::clean( indices.load() );

    if ( pitems != nullptr )
    {
      for ( auto& next: *pitems )
      {
        auto      getdoc = mtc::ptr::clean( next.pindex.load() )->GetEntity( id );
        uint32_t  uindex;

        if ( getdoc == nullptr || next.banned.Get( uindex = getdoc->GetIndex() ) )
          continue;

        return next.uLower <= 1 ? getdoc :
          overriden::api(
          overriden::index( next.uLower + uindex - 1, overriden::base( getdoc ) ) );
      }
    }
    return {};
  }

  auto  ContentsIndex::GetEntity( uint32_t id ) const -> mtc::api<const IEntity>
  {
    auto  pitems = mtc::ptr::clean( indices.load() );

    for ( auto& next: *pitems )
      if ( next.uLower <= id && next.uUpper >= id && !next.banned.Get( id - next.uLower + 1 ) )
        return mtc::ptr::clean( next.pindex.load() )->GetEntity( id - next.uLower + 1 );

    return {};
  }

  bool  ContentsIndex::DelEntity( EntityId id )
  {
  }

  auto  ContentsIndex::SetEntity( EntityId id,
    mtc::api<const IContents>         index,
    mtc::api<const mtc::IByteBuffer>  attrs ) -> mtc::api<const IEntity>
  {

    for ( ; ; )
    {
      auto  pitems = mtc::ptr::clean( indices.load() );

      if ( pitems == nullptr || pitems->ncount == 0 )
        throw std::logic_error( "index flakes are not initialized" );

    // try Set the entity to the last index in the chain
      try
      {
        auto& xEntry = pitems->back();
        auto  newdoc = mtc::ptr::clean( xEntry.pindex.load() )->SetEntity(
          id, index, attrs );

        return xEntry.uLower <= 1 ? overriden::api( overriden::index( xEntry.uLower + newdoc->GetIndex() - 1,
          overriden::base( newdoc ) ) ) : newdoc;
      }
    // on dynamic index overflow, rotate the last index by creating new one in a new flakes,
    // and continue Setting attempts
      catch ( const palmira::index::dynamic::index_overflow& xo )
      {

      }
    }
  }

  auto  ContentsIndex::Commit() -> mtc::api<IStorage::ISerialized>
  {
  }

  auto  ContentsIndex::Reduce() -> mtc::api<IContentsIndex>
  {
  }

  auto  ContentsIndex::GetMaxIndex() const -> uint32_t
  {
  }

  auto  ContentsIndex::GetKeyBlock( const void*, size_t ) const -> mtc::api<IEntities>
  {
  }

  auto  ContentsIndex::GetKeyStats( const void*, size_t ) const -> BlockInfo
  {
  }

}}}
