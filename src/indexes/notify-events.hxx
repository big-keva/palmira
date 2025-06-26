# if !defined( __palmira_src_index_notify_events_hxx__ )
# define __palmira_src_index_notify_events_hxx__
# include <functional>

namespace palmira {

  struct Notify final
  {
    enum class Event: unsigned
    {
      OK = 1,
      Empty = 2,
      Canceled = 3,
      Failed = 4
    };

    using Func = std::function<void(void*, Event)>;

  };

}

# endif   // !__palmira_src_index_notify_events_hxx__
