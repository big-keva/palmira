# pragma once
# if !defined( PALMIRA_SRC_TOOLSET_ZIP_UNZIP_HPP_ )
# define PALMIRA_SRC_TOOLSET_ZIP_UNZIP_HPP_
# include <mtc/span.hpp>
# include <vector>

namespace palmira
{
  auto  ZipBuf( const mtc::span<const char>& src ) -> std::vector<char>;
  auto  Unpack( const mtc::span<const char>& src ) -> std::vector<char>;
}

# endif   // !PALMIRA_SRC_TOOLSET_ZIP_UNZIP_HPP_
