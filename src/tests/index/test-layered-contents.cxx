# include "../../../api/layered-contents.hxx"
# include "../../../api/storage-filesystem.hxx"
# include "../../../api/dynamic-contents.hxx"
# include "../../../api/static-contents.hxx"
# include <mtc/test-it-easy.hpp>
# include <mtc/zmap.h>

using namespace palmira;

class KeyValues: public IContents, protected mtc::zmap
{
  implement_lifetime_stub

public:
  KeyValues( const mtc::zmap& keyval ):
    zmap( keyval )  {}

  auto  ptr() const -> const IContents*
  {  return this;  }

  void  Enumerate( IContentsIndex::IIndexAPI* to ) const override
  {
    for ( auto keyvalue: *this )
    {
      auto  val = keyvalue.second.to_string();

      to->Insert( { (const char*)keyvalue.first.data(), keyvalue.first.size() },
        { val.data(), val.size() } );
    }
  }
};

class MockDynamic final: public IContentsIndex
{
  implement_lifetime_control

  class MockEntity final: public IEntity
  {
    std::string id;
    uint32_t index;

    implement_lifetime_control

  protected:
    auto  GetId() const -> Attribute override {  return { id };  }
    auto  GetIndex() const -> uint32_t override {  return index;  }
    auto  GetAttributes() const -> mtc::api<const mtc::IByteBuffer> override  {  return nullptr;  }

  public:
    MockEntity( const std::string& entId, uint32_t uindex ):
      id( entId ), index( uindex )  {}
  };

public:
  auto  GetEntity( EntityId id ) const -> mtc::api<const IEntity> override
    {  return new MockEntity( std::string( id ), 0 );  }
  auto  GetEntity( uint32_t ix ) const -> mtc::api<const IEntity> override
    {  return new MockEntity( mtc::strprintf( "%u", ix ), ix );  }
  bool  DelEntity( EntityId ) override
    {  return false;  }
  auto  SetEntity( EntityId id, mtc::api<const IContents>, mtc::api<const mtc::IByteBuffer> ) -> mtc::api<const IEntity> override
    {  return new MockEntity( std::string( id ), 1 );  }
  auto  GetMaxIndex() const -> uint32_t override
    {  return 1;  }
  auto  GetKeyBlock( const void*, size_t ) const -> mtc::api<IEntities> override
    {  return nullptr;  }
  auto  GetKeyStats( const void*, size_t ) const -> BlockInfo override
    {  return { uint32_t(-1), 0 };  }
  auto  GetIterator( EntityId ) -> mtc::api<IIterator> override
    {  throw std::runtime_error("not implemented");  }
  auto  GetIterator( uint32_t ) -> mtc::api<IIterator> override
    {  throw std::runtime_error("not implemented");  }
  auto  Commit() -> mtc::api<IStorage::ISerialized> override
    {  return nullptr;  }
  auto  Reduce() -> mtc::api<IContentsIndex> override
    {  return this;  }
  void  Stash( EntityId ) override
    {}

};

TestItEasy::RegisterFunc  layered_contents( []()
  {
    TEST_CASE( "index/layered-contents" )
    {
      SECTION( "Layered cintents index may be created directly" )
      {
        mtc::api<IContentsIndex> index;

        SECTION( "created without covered indices, it does nothing" )
        {
          if ( REQUIRE_NOTHROW( index = index::layered::Contents::Create( nullptr, 0 ) ) )
          {
            REQUIRE( index->DelEntity( "id" ) == false );
            REQUIRE( index->GetEntity( "id" ) == nullptr );
            REQUIRE( index->GetEntity( 1U ) == nullptr );
            REQUIRE( index->GetKeyStats( "aaa", 3 ).bkType == uint32_t(-1) );
            REQUIRE( index->GetKeyBlock( "aaa", 3 ) == nullptr );
            REQUIRE( index->GetMaxIndex() == 0 );
            REQUIRE( index->Reduce().ptr() == index.ptr() );
            REQUIRE_EXCEPTION( index->SetEntity( "i1" ), std::logic_error );
            REQUIRE_EXCEPTION( index->Commit(), std::logic_error );
          }
        }
        SECTION( "after adding some index it works transparently" )
        {
          if ( REQUIRE_NOTHROW( index = index::layered::Contents::Create( std::vector<mtc::api<IContentsIndex>>{
            new MockDynamic() } ) ) )
          {
            REQUIRE( index->DelEntity( "id" ) == false );

            if ( REQUIRE( index->GetEntity( "id" ) != nullptr ) )
              REQUIRE( index->GetEntity( "id" )->GetId() == "id" );

            if ( REQUIRE( index->GetEntity( 1U ) != nullptr ) )
              REQUIRE( index->GetEntity( 1U )->GetIndex() == 1U );

            REQUIRE( index->GetMaxIndex() == 1 );
          }
        }
      }
    }
  } );
