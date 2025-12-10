# include "../../toolset.hpp"

namespace palmira
{

  class QuickPending final: public IService::IPending, mtc::zmap
  {
    auto  Wait(double) -> zmap override {  return *this;}

  public:
    QuickPending( const mtc::zmap& z ): mtc::zmap( z ) {}

    implement_lifetime_control
  };

  auto  Quick( const mtc::zmap& rez ) -> mtc::api<IService::IPending> {  return new QuickPending( rez );  }

}
