# include "../../api/contents-index.hxx"
# include "dynamic-entities.hxx"
# include <stdexcept>

namespace palmira {
namespace index   {
namespace fusion {

  class EntityIterator
  {
    mtc::api<IContentsIndex::IIterator> iterator;
    mtc::api<const IEntity>             curValue;
    EntityId                            entityId;

  public:
    EntityIterator( const mtc::api<IContentsIndex>& contentsIndex ):
      iterator( contentsIndex->GetIterator( "" ) ),
      curValue( iterator->Curr() ),
      entityId( curValue != nullptr ? curValue->GetId() : EntityId() ) {}

    auto  GetId() const -> const EntityId& {  return entityId;  }
    auto  operator -> () const -> mtc::api<const IEntity> {  return curValue;  }

  };

  void  MergeEntities(
    mtc::api<IStorage::IIndexStore>               output,
    const std::vector<mtc::api<IContentsIndex>>&  source )
  {
//    using Entity = dynamic::EntityTable<std::allocator<char>>::Entity;

    auto  iterators = std::vector<EntityIterator>();
    auto  outStream = output->Entities();

  // create iterators list
    for ( auto& next: source )
      iterators.emplace_back( next );

  // set zero document
    /*
    if ( Entity( std::allocator<char>() ).Serialize( outStream.ptr() ) == nullptr )
      throw std::runtime_error( "Failed to serialize entities" );
     */

    for ( ; ; )
    {
      auto  select = iterators.end();

    // find entity with minimal id and maximal version
      for ( auto it = iterators.begin(); it != iterators.end(); ++it )
        if ( !it->GetId().empty() )
          if ( select == iterators.end() || it->GetId() < select->GetId() )
            select = it;

    // check if the minimal entity is found
      if ( select != iterators.end() )
      {/*
        auto  id = std::string_view( select->GetId() );
        auto  index = (*select)->GetIndex();
        auto  version = (*select)->GetVersion();
        auto  extras = (*select)->GetExtra();
        auto  extraPtr = extras != nullptr ? extras->GetPtr() : nullptr;
        auto  extraLen = extras != nullptr ? extras->GetLen() : 0;

      // copy selected entity to output table
        if ( ::Serialize( ::Serialize( ::Serialize( ::Serialize( ::Serialize( outStream.ptr(),
          index ), version ), id ), extraLen ), extraPtr, extraLen ) == nullptr )
        {
          throw std::runtime_error( "Failed to serialize entities" );
        }*/
      } else break;
    }
  }

}}}
