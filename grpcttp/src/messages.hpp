# include "../service.hpp"
# include <grpcttp.pb.h>
# include <grpcttp.grpc.pb.h>
# include <mtc/zmap.h>

namespace grpcttp {

  auto  Convert( mtc::zmap& out, const Metadata& src ) -> mtc::zmap&;
  auto  Convert( mtc::zval& out, const MetaValue& src ) -> mtc::zval&;

  auto  Convert( Metadata& out, const mtc::zmap& src ) -> Metadata&;
  auto  Convert( MetaValue& out, const mtc::zval& src ) -> MetaValue&;

  auto  Convert( Document& out, const DeliriX::ITextView& src ) -> Document&;
  auto  Convert( DeliriX::IText& out, const Document& src ) -> DeliriX::IText&;

  auto  Convert( palmira::RemoveArgs& out, const RemoveArgs& src ) -> palmira::RemoveArgs&;
  auto  Convert( palmira::UpdateArgs& out, const UpdateArgs& src ) -> palmira::UpdateArgs&;
  auto  Convert( palmira::InsertArgs& out, const InsertArgs& src ) -> palmira::InsertArgs&;

  auto  Convert( RemoveArgs& out, const palmira::RemoveArgs& src ) -> RemoveArgs&;
  auto  Convert( UpdateArgs& out, const palmira::UpdateArgs& src ) -> UpdateArgs&;
  auto  Convert( InsertArgs& out, const palmira::InsertArgs& src ) -> InsertArgs&;

  auto  Convert( UpdateReport& out, const mtc::zmap& src ) -> UpdateReport&;
  auto  Convert( mtc::zmap& out, UpdateReport& src ) -> mtc::zmap&;

}
