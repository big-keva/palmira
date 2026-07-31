# if !defined( PALMIRA_SRC_SERVICE_QUOTATION_TOOL_H )
# define PALMIRA_SRC_SERVICE_QUOTATION_TOOL_H
# include <structo/fields.hpp>
# include "collect.hpp"

namespace palmira {
namespace enquote {

  using FieldHandler = structo::FieldHandler;
  using FieldOptions = structo::FieldOptions;

  using IContentsIndex = structo::IContentsIndex;

  enum class Scheme: unsigned
  {
    Absent  = 0,
    Sketch  = 1,
    Struct  = 3,
    Source  = 4
  };

  static constexpr struct include_t {} include = {};
  static constexpr struct exclude_t {} exclude = {};
  static constexpr struct fnmatch_t {} fnmatch = {};

  auto  Function( mtc::api<IContentsIndex>, const FieldHandler&, Scheme ) -> collect::QuotesFn;
  auto  Function( mtc::api<IContentsIndex>, const FieldHandler&, Scheme, const include_t&, const mtc::array_charstr& ) -> collect::QuotesFn;
  auto  Function( mtc::api<IContentsIndex>, const FieldHandler&, Scheme, const exclude_t&, const mtc::array_charstr& ) -> collect::QuotesFn;
  auto  Function( mtc::api<IContentsIndex>, const FieldHandler&, Scheme, const fnmatch_t&, const mtc::array_charstr& ) -> collect::QuotesFn;

  auto  StringToScheme( std::string_view ) -> Scheme;
  auto  SchemeToString( Scheme ) -> std::string_view;

}}
# endif   // !PALMIRA_SRC_SERVICE_QUOTATION_TOOL_H
