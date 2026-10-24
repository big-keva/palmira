#pragma once
//-------------------------------------------------------------------------//
#include <simdjson.h>
//-------------------------------------------------------------------------//
#include <string>
#include <string_view>
#include <vector>
//-------------------------------------------------------------------------//
namespace elastic::searchapi::json
{
//-------------------------------------------------------------------------//
  struct match_query
  {
    //!< Keeps a name of field, which executes a match.
    std::string field;

    //!< Keeps a query.
    std::string query;

    //!< operator: "or" / "and".
    std::string operator_value;

    //!< fuzziness: например "AUTO".
    std::string fuzziness;

    //!< analyzer.
    std::string analyzer;

    //!< Keeps a minimum_should_match.
    std::string minimum_should_match;

    //!< Keeps zero terms query.
    std::string zero_terms_query; // "none" / "all"

    bool lenient = false;
    bool auto_generate_synonyms_phrase_query = true;

    //!< Keeps a boost.
    double boost = 1.0;
  };
//-------------------------------------------------------------------------//
  /**
   * Parses a match query.
   * @param match_object  [in] - A match object.
   * @return A match query.
   * @note
   *
   * 1. simple:
   *    { "match": { "name": "brave" } }
   *
   * 2. full:
   *    {
   *      "match": {
   *        "name": {
   *          "query": "brave",
   *          "operator": "and",
   *          "analyzer": "standard",
   *          "fuzziness": "AUTO",
   *          "minimum_should_match": "75%",
   *          "zero_terms_query": "none",
   *          "lenient": false,
   *          "auto_generate_synonyms_phrase_query": true,
   *          "boost": 1.5
   *        }
   *      }
   *    }
   */
  auto parse_match_query(simdjson::ondemand::object match_object) -> match_query;
//-------------------------------------------------------------------------//
} // namespace elastic::searchapi::json
