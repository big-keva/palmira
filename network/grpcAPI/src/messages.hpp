# include "../service.hpp"
# include <grpcttp.pb.h>
# include <grpcttp.grpc.pb.h>
# include <mtc/zmap.h>

namespace grpcapi
{
  auto  Convert( mtc::zmap& out, const grpcttp::Metadata& src ) -> mtc::zmap&;
  auto  Convert( mtc::zval& out, const grpcttp::MetaValue& src ) -> mtc::zval&;

  auto  Convert( grpcttp::Metadata& out, const mtc::zmap& src ) -> grpcttp::Metadata&;
  auto  Convert( grpcttp::MetaValue& out, const mtc::zval& src ) -> grpcttp::MetaValue&;

  auto  Convert( grpcttp::Document& out, const DeliriX::ITextView& src ) -> grpcttp::Document&;
  auto  Convert( DeliriX::IText& out, const grpcttp::Document& src ) -> DeliriX::IText&;

  auto  Convert( palmira::RemoveArgs& out, const grpcttp::RemoveArgs& src ) -> palmira::RemoveArgs&;
  auto  Convert( palmira::UpdateArgs& out, const grpcttp::UpdateArgs& src ) -> palmira::UpdateArgs&;
  auto  Convert( palmira::InsertArgs& out, const grpcttp::InsertArgs& src ) -> palmira::InsertArgs&;

  auto  Convert( grpcttp::RemoveArgs& out, const palmira::RemoveArgs& src ) -> grpcttp::RemoveArgs&;
  auto  Convert( grpcttp::UpdateArgs& out, const palmira::UpdateArgs& src ) -> grpcttp::UpdateArgs&;
  auto  Convert( grpcttp::InsertArgs& out, const palmira::InsertArgs& src ) -> grpcttp::InsertArgs&;

  auto  Convert( grpcttp::UpdateReport& out, const mtc::zmap& src ) -> grpcttp::UpdateReport&;
  auto  Convert( mtc::zmap& out, grpcttp::UpdateReport& src ) -> mtc::zmap&;
}
