# if !defined( PALMIRA_RESTAPI_ARRAY_REPORTS_HPP_ )
# define PALMIRA_RESTAPI_ARRAY_REPORTS_HPP_
# include "responder.hpp"
# include <mutex>

namespace restAPI
{

  class ArrayReport final: public mtc::Iface
  {
    mtc::api<Responder::Response> output;
    std::shared_ptr<bool>         cancel;
    uWS::Loop*                    evLoop;
    const unsigned                utotal;
    std::atomic<unsigned>         uready = 0U;
    std::mutex                    locker;
    mtc::array_zmap               report;

  public:
    ArrayReport(
      mtc::api<Responder::Response>,
      std::shared_ptr<bool>,
      uWS::Loop*,
      unsigned );
    void  Append( const mtc::zmap& );

    implement_lifetime_control
  };

}

# endif   // !PALMIRA_RESTAPI_ARRAY_REPORTS_HPP_
