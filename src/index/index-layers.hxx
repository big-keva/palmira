# if !defined( __palmira_index_index_layers_hpp__ )
# define __palmira_index_index_layers_hpp__
# include "../../api/contents-index.hxx"
# include "dynamic-bitmap.hxx"

namespace palmira {
namespace index {

 /*
  * IndexLayers обеспечивает хранение фиксированного массива элементов
  * с индекесами и базовые примитивы работы с ними.
  *
  * Сами индексы хранятся как атомарные переменные и работа с ними
  * ведётся на уровне забрать-положить.
  *
  * Это даёт возможность подменять интерфейсы индексов на лету (commit,
  * merge), а также добавлять новые до достижения предела размерности.
  *
  * Ротацию таких массивов при переполнении будет обеспечивать другой
  * компонент.
  */
  class IndexLayers
  {
    class Entities;

  public:
    IndexLayers() = default;
    IndexLayers( const mtc::api<IContentsIndex>*, size_t );

    auto  getEntity( EntityId ) const -> mtc::api<const IEntity>;
    auto  getEntity( uint32_t ) const -> mtc::api<const IEntity>;
    bool  delEntity( EntityId id );

    auto  getMaxIndex() const -> uint32_t;
    auto  getKeyBlock( const void*, size_t, const mtc::Iface* = nullptr ) const -> mtc::api<IContentsIndex::IEntities>;
    auto  getKeyStats( const void*, size_t ) const -> IContentsIndex::BlockInfo;

    void  addContents( mtc::api<IContentsIndex> pindex );

    void  commitItems();

    void  hideClashes();

  protected:
    struct IndexEntry
    {
      uint32_t                  uLower;
      uint32_t                  uUpper;
      mtc::api<IContentsIndex>  pIndex;
      std::vector<IndexEntry>   backup;

    public:
      IndexEntry( uint32_t uLower, mtc::api<IContentsIndex> pindex );
      IndexEntry( const IndexEntry& );
      IndexEntry( IndexEntry&& );
      IndexEntry& operator=( const IndexEntry& );
      IndexEntry& operator=( IndexEntry&& );

    public:
      auto  Override( mtc::api<const IEntity> ) const -> mtc::api<const IEntity>;

    };

  protected:
    std::vector<IndexEntry> layers;

  };

}}

# endif   // !__palmira_index_index_layers_hpp__
