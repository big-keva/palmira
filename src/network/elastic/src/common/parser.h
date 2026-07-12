/*!==========================================================================
* \file
* - Program:       elasticapi
* - File:          parser.h
* - Created:       06/24/2026
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
#ifndef __PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
#define __PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
//-------------------------------------------------------------------------//
#include <string_view>
#include <memory>
//-------------------------------------------------------------------------//
#include <mtc/zmap.h>
//-------------------------------------------------------------------------//
namespace elastic {
//-------------------------------------------------------------------------//
  template<typename parse_object_t>
  struct parser {
    using value_type = parse_object_t;

    /**
     * Destructor.
     */
    virtual ~parser() = default;

    /**
     * Validates a body on valid.
     * @param body [in] - A document body.
     */
    virtual auto validate(std::string_view body) const -> void = 0;

    /**
     * Parses a document.
     * @param body [in] - A document body.
     * @param opts [in] - Options.
     * @return A parsed object.
     */
    virtual auto parse(std::string_view body, const mtc::zmap &opts) const -> std::unique_ptr<parse_object_t> = 0;
  };
//-------------------------------------------------------------------------//
} // namespace elastic
//-------------------------------------------------------------------------//
#endif // __PARSER_H_9A383FB9_69EF_4D2E_8CFD_2640EFA93EE0__
