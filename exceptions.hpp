# if !defined( __palmira_exceptions_hpp__ )
# define __palmira_exceptions_hpp__
# include <stdexcept>

namespace palmira {

  class index_overflow: public std::runtime_error {  using runtime_error::runtime_error;  };

}

# endif   // __palmira_exceptions_hpp__
