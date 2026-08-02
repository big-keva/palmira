#include "method-type.h"
//-------------------------------------------------------------------------//
namespace elastic::http
{
//-------------------------------------------------------------------------//
  auto to_string(method_type type) -> std::string_view
  {
    switch (type)
    {
      case method_type::post: return "post";
      case method_type::put: return "put";
      case method_type::get: return "get";
      case method_type::del: return "delete";
      case method_type::head: return "head";
    }

    return "unknown";
  }
//-------------------------------------------------------------------------//
} // elastic::http
