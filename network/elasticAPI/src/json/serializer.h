/*!==========================================================================
* \file
* - Program:       elasticapi
* - File:          serializer.h
* - Created:       07/21/2026
* - Author:        Vitaly Bulganin
* - Description:
* - Comments:
*
-----------------------------------------------------------------------------
*
* - History:
*
===========================================================================*/
#pragma once
//-------------------------------------------------------------------------//
#ifndef __SERIALIZER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __SERIALIZER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <ostream>
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
namespace elastic::json
{
//-------------------------------------------------------------------------//
  auto serialize(std::ostream &stream, const mtc::zmap *zmap) -> std::ostream &;
//-------------------------------------------------------------------------//
  template<typename array_t>
  auto serialize_array_values(std::ostream &stream, const array_t *array) -> std::ostream &
  {
    stream << '[';

    if (array != nullptr)
    {
      bool add_comma = false;
      for (const auto &val : *array)
      {
        if (add_comma)
        {
          stream << ',';
        }
        stream << val;
        add_comma = true;
      }
    }

    stream << ']';
    return stream;
  }

  /**
   * Serializes array into output stream.
   * @param stream [iin. out] - A stream.
   * @param array [in] - A pointer on array of zvalues.
   * @return A reference on the stream.
   */
  auto serialize(std::ostream &stream, const mtc::array_zval_t *array) -> std::ostream &;

  /**
   * Serializes a zmap into output stream.
   * @param stream [in, out] - A stream.
   * @param zmap [in] - A zmap.
   * @return A reference on the stream.
   */
  auto serialize(std::ostream &stream, const mtc::zmap *zmap) -> std::ostream &;

  /**
   * Serializes array into output stream.
   * @param stream [iin. out] - A stream.
   * @param array [in] - Array of zvalues.
   * @return A reference on the stream.
   */
  auto serialize(std::ostream &stream, const mtc::array_zval &array) -> std::ostream &;
//-------------------------------------------------------------------------//
} // namespace elastic::json
//-------------------------------------------------------------------------//
#endif // __SERIALIZER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
