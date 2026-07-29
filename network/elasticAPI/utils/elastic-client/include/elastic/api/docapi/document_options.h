#pragma once
//-------------------------------------------------------------------------//
namespace elastic::api::docapi
{
//-------------------------------------------------------------------------//
  enum class version_types
  {
      internal,
      external,
      external_gte,
      force
  };

  enum class refresh_policies
  {
      disabled,
      immediate,
      wait_for
  };
//-------------------------------------------------------------------------//
} // namespace elastic::api::docapi
