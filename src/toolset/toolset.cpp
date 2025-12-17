# include "../../toolset.hpp"

namespace palmira
{

  class QuickPending final: public IService::IPending, mtc::zmap
  {
    auto  Wait(double) -> zmap override {  return *this;}

  public:
    QuickPending( const zmap& z, IService::NotifyFn n ): mtc::zmap( z )
    {
      if ( n != nullptr )
        n( *this );
    }

    implement_lifetime_control
  };

  auto  Immediate( const mtc::zmap& rez, IService::NotifyFn nfn ) -> mtc::api<IService::IPending>
  {
    return new QuickPending( rez, nfn );
  }

}
