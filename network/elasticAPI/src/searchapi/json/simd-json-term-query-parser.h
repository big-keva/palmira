#pragma once
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <string>
#include <vector>
//-------------------------------------------------------------------------//
namespace elastic::searchapi::json
{
//-------------------------------------------------------------------------//
  struct term_query
  {
    //!< Keeps a name of field.
    std::string field;

    //!< Keeps a value of field.
    std::string value;

    double boost = 1.0;
  };

  struct terms_query
  {
    //!< Keeps a name of field.
    std::string field;

    //!< Keeps a list of field values.
    std::vector<std::string> values;
  };
//-------------------------------------------------------------------------//
  /**
   * Parses a term query.
   * @param term_object [in] - A term object.
   * @return A term query.
   */
  auto parse_term_query(simdjson::ondemand::object term_object) -> term_query;

  /**
   * Parses a terms query.
   * @param terms_object [in] - A terms object.
   * @return A terms query.
   */
  auto parse_terms_query(simdjson::ondemand::object terms_object) -> terms_query;
//-------------------------------------------------------------------------//
} // namespace elastic::searchapi::json
