#include "serializer.h"
//-------------------------------------------------------------------------//
#include <DeliriX/DOM-dump.hpp>
#include <DeliriX/DOM-text.hpp>
//-------------------------------------------------------------------------//
namespace elastic::json
{
//-------------------------------------------------------------------------//
    auto serialize(std::ostream &stream, const mtc::array_zval_t *array) -> std::ostream &
    {
      //<???> stream << '[';

      if (array != nullptr)
      {
        auto need_comma = false;
        for (const auto &val : *array)
        {
          if (need_comma)
          {
            stream << ',';
          }

          switch (val.get_type())
          {
          case mtc::zval::z_int16:
            stream << val.get_int16();
            break;
          case mtc::zval::z_int32:
            stream << val.get_int32();
            break;
          case mtc::zval::z_int64:
            stream << val.get_int64();
            break;
          case mtc::zval::z_bool:
            stream << std::boolalpha << val.get_bool();
            break;
          case mtc::zval::z_charstr:
            stream << '"' << val.get_charstr() << '"';
            break;
          case mtc::zval::z_array_int16:
            serialize_array_values(stream, val.get_array_int16());
            break;
          case mtc::zval::z_array_int32:
            serialize_array_values(stream, val.get_array_int32());
            break;
          case mtc::zval::z_array_int64:
            serialize_array_values(stream, val.get_array_int64());
            break;
          case mtc::zval::z_array_charstr:
            serialize_array_values(stream, val.get_array_charstr());
            break;
          case mtc::zval::z_zmap:
            serialize(stream, val.get_zmap());
            break;
          case mtc::zval::z_array_zval:
            serialize(stream, val.get_array_zval());
            break;
          default:
            break;
          }

          need_comma = true;
        }
      }

      //<???> stream << ']';
      return stream;
    }

    auto serialize(std::ostream &stream, const mtc::zmap *zmap) -> std::ostream &
    {
      // mtc::json::Print(stdout, zmap, decorate);
      auto need_comma = false;
      //<???> stream << '{';

      if (zmap != nullptr)
      {
        for (auto iter = zmap->begin(), end = zmap->end(); iter != end; ++iter)
        {
          if (need_comma)
          {
            stream << ',';
          }

          stream << '"' << iter->first.to_charstr() << R"(":)";
          switch (iter->second.get_type())
          {
          case mtc::zval::z_int16:
            stream << iter->second.get_int16();
            break;
          case mtc::zval::z_int32:
            stream << iter->second.get_int32();
            break;
          case mtc::zval::z_int64:
            stream << iter->second.get_int64();
            break;
          case mtc::zval::z_bool:
            stream << std::boolalpha << iter->second.get_bool();
            break;
          case mtc::zval::z_charstr:
            stream << '"' << iter->second.to_string() << '"';
            break;
          case mtc::zval::z_array_int16:
            serialize_array_values(stream, iter->second.get_array_int16());
            break;
          case mtc::zval::z_array_int32:
            serialize_array_values(stream, iter->second.get_array_int32());
            break;
          case mtc::zval::z_array_int64:
            serialize_array_values(stream, iter->second.get_array_int64());
            break;
          case mtc::zval::z_array_charstr:
            serialize_array_values(stream, iter->second.get_array_charstr());
            break;
          case mtc::zval::z_zmap:
            serialize(stream, iter->second.get_zmap());
            break;
          case mtc::zval::z_array_zval:
            serialize(stream, iter->second.get_array_zval());
            break;
          default:
            break;
          }

          need_comma = true;
        }
      }

      //<???> stream << '}';
      return stream;
    }

    auto serialize(std::ostream &stream, const mtc::array_zval &array) -> std::ostream &
    {
      //<!!!> mtc::json::Print(buffer, mtc::zval::dump(array), mtc::json::print::decorated());
/*
      DeliriX::Text text;
      text.Serialize(DeliriX::dump_as::Json(DeliriX::dump_as::MakeOutput(buffer)));
*/
      return serialize(stream, &array);
    }
//-------------------------------------------------------------------------//
} // namespace elastic::json
