#include "elastic/http/codes.h"
//-------------------------------------------------------------------------//
namespace elastic::http {
//-------------------------------------------------------------------------//
  auto to_string(const codes code) -> std::string_view {
    switch (code) {
      case codes::OK: { return "OK"; }
      case codes::Created: { return " Created"; }
      case codes::Accepted: { return "Accepted"; }
      case codes::NonAuthoritativeInformation: { return "Non authoritative information"; }
      case codes::NoContent: { return "No content"; }
      case codes::ResetContent: { return "Reset content"; }

      case codes::BadRequest: { return "Bad request"; }
      case codes::Unauthorized: { return "Unauthorized"; }
      case codes::Forbidden: { return "Forbidden"; }
      case codes::NotFound: { return "Not found"; }
      case codes::MethodNotAllowed: { return "Method not allowed"; }
      case codes::NotAcceptable: { return "Not acceptable"; }
      case codes::RequestTimeout: { return "Request timeout"; }

      case codes::InternalServerError: { return "Internal server error"; }
      case codes::NotImplemented: { return "Not implemented"; }
      case codes::BadGateway: { return "Bad gGateway"; }
      case codes::ServiceUnavailable: { return "Service unavailable"; }
      case codes::GatewayTimeout: { return "Gateway timeout"; }
      case codes::HTTPVersionNotSupported: { return "HTTP version not supported"; }
      case codes::Continue: { return "Continue"; }
      case codes::SwitchingProtocols: { return "Switching protocols"; }
      case codes::Processing: { return "Processing"; }
      case codes::EarlyHints: { return "Early hints"; }
      case codes::PartialContent: { return "PartialContent"; }
      case codes::MultiStatus: { return "MultiStatus"; }
      case codes::AlreadyReported: { return "AlreadyReported"; }
      case codes::IMUsed: { return "IMUsed"; }
      case codes::MultipleChoices: { return "MultipleChoices"; }
      case codes::MovedPermanently: { return "MovedPermanently"; }
      case codes::Found: { return "Found"; }
      case codes::SeeOther: { return "SeeOther"; }
      case codes::NotModified: { return "NotModified"; }
      case codes::UseProxy: { return "UseProxy"; }
      case codes::TemporaryRedirect: { return "TemporaryRedirect"; }
      case codes::PermanentRedirect: { return "PermanentRedirect"; }
      case codes::PaymentRequired: { return "PaymentRequired"; }
      case codes::ProxyAuthenticationRequired: { return "ProxyAuthenticationRequired"; }
      case codes::Conflict: { return "Conflict"; }
      case codes::Gone: { return "Gone"; }
      case codes::LengthRequired: { return "LengthRequired"; }
      case codes::PreconditionFailed: { return "PreconditionFailed"; }
      case codes::ContentTooLarge: { return "ContentTooLarge"; }
      case codes::URITooLong: { return "URITooLong"; }
      case codes::UnsupportedMediaType: { return "UnsupportedMediaType"; }
      case codes::RangeNotSatisfiable: { return "RangeNotSatisfiable"; }
      case codes::ExpectationFailed: { return "ExpectationFailed"; }
      case codes::ImATeapot: { return "ImATeapot"; }
      case codes::MisdirectedRequest: { return "MisdirectedRequest"; }
      case codes::UnprocessableContent: { return "UnprocessableContent"; }
      case codes::Locked: { return "Locked"; }
      case codes::FailedDependency: { return "FailedDependency"; }
      case codes::TooEarly: { return "TooEarly"; }
      case codes::UpgradeRequired: { return "UpgradeRequired"; }
      case codes::PreconditionRequired: { return "PreconditionRequired"; }
      case codes::TooManyRequests: { return "TooManyRequests"; }
      case codes::RequestHeaderFieldsTooLarge: { return "RequestHeaderFieldsTooLarge"; }
      case codes::UnavailableForLegalReasons: { return "UnavailableForLegalReasons"; }
      case codes::VariantAlsoNegotiates: { return "VariantAlsoNegotiates"; }
      case codes::InsufficientStorage: { return "InsufficientStorage"; }
      case codes::LoopDetected: { return "LoopDetected"; }
      case codes::NotExtended: { return "NotExtended"; }
      case codes::NetworkAuthenticationRequired: { return "NetworkAuthenticationRequired"; }
    }
    return "unknown";
  }
//-------------------------------------------------------------------------//
}; // namespace elastic::http
