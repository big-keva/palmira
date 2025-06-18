# if !defined( __palmira_api_exceptions_hxx__ )
# define __palmira_api_exceptions_hxx__
# include <stdexcept>

namespace palmira {

  class index_overflow: public std::runtime_error {  using runtime_error::runtime_error;  };

}

# endif   // __palmira_api_exceptions_hxx__
