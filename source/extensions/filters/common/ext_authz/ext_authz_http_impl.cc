#include "extensions/filters/common/ext_authz/ext_authz_http_impl.h"

#include "common/common/enum_to_int.h"
#include "common/http/async_client_impl.h"
#include "common/http/codes.h"

#include "absl/strings/str_cat.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace ExtAuthz {

namespace {
const Http::HeaderMap& getZeroContentLengthHeader() {
  static const Http::HeaderMap* header_map =
      new Http::HeaderMapImpl{{Http::Headers::get().ContentLength, std::to_string(0)}};
  return *header_map;
}

const Response& getErrorResponse() {
  static const Response* response =
      new Response{CheckStatus::Error, Http::HeaderVector{}, Http::HeaderVector{}, std::string{},
                   Http::Code::Forbidden};
  return *response;
}

const Response& getDeniedResponse() {
  static const Response* response =
      new Response{CheckStatus::Denied, Http::HeaderVector{}, Http::HeaderVector{}, std::string{}};
  return *response;
}

const Response& getOkResponse() {
  static const Response* response = new Response{
      CheckStatus::OK, Http::HeaderVector{}, Http::HeaderVector{}, std::string{}, Http::Code::OK};
  return *response;
}
} // namespace

RawHttpClientImpl::RawHttpClientImpl(
    const std::string& cluster_name, Upstream::ClusterManager& cluster_manager,
    const absl::optional<std::chrono::milliseconds>& timeout, const std::string& path_prefix,
    const Http::LowerCaseStrUnorderedSet& allowed_authorization_headers,
    const Http::LowerCaseStrUnorderedSet& allowed_request_headers,
    const HeaderKeyValueVector& authorization_headers_to_add)
    : cluster_name_(cluster_name), path_prefix_(path_prefix),
      allowed_authorization_headers_(allowed_authorization_headers),
      allowed_request_headers_(allowed_request_headers),
      authorization_headers_to_add_(authorization_headers_to_add), timeout_(timeout),
      cm_(cluster_manager) {}

RawHttpClientImpl::~RawHttpClientImpl() { ASSERT(!callbacks_); }

void RawHttpClientImpl::cancel() {
  ASSERT(callbacks_ != nullptr);
  request_->cancel();
  callbacks_ = nullptr;
}

void RawHttpClientImpl::check(RequestCallbacks& callbacks,
                              const envoy::service::auth::v2alpha::CheckRequest& request,
                              Tracing::Span& span) {
  ASSERT(callbacks_ == nullptr);
  ASSERT(span_ == nullptr);
  callbacks_ = &callbacks;
  span_ = &span;
  
  span_->setSampled(false);

  Http::HeaderMapPtr headers_ptr{};
  const uint64_t request_length =
      request.attributes().request().http().body().inline_bytes().length();
  if (request_length > 0) {
    const Http::HeaderMap& header_map =
        Http::HeaderMapImpl{{Http::Headers::get().ContentLength, std::to_string(request_length)}};
    headers_ptr = std::make_unique<Http::HeaderMapImpl>(header_map);
  } else {
    headers_ptr = std::make_unique<Http::HeaderMapImpl>(getZeroContentLengthHeader());
  }

  for (const auto& allowed_header : allowed_request_headers_) {
    const auto& request_header =
        request.attributes().request().http().headers().find(allowed_header.get());
    if (request_header != request.attributes().request().http().headers().cend()) {
      if (allowed_header == Http::Headers::get().Path && !path_prefix_.empty()) {
        std::string value;
        absl::StrAppend(&value, path_prefix_, request_header->second);
        headers_ptr->addCopy(allowed_header, value);
      } else {
        headers_ptr->addCopy(allowed_header, request_header->second);
      }
    }
  }

  for (const auto& kv : authorization_headers_to_add_) {
    headers_ptr->setReference(kv.first, kv.second);
  }

  Http::MessagePtr message_ptr =
      std::make_unique<Envoy::Http::RequestMessageImpl>(std::move(headers_ptr));
  if (request_length > 0) {
    message_ptr->body() = std::make_unique<Buffer::OwnedImpl>(
        request.attributes().request().http().body().inline_bytes());
  }

  request_ =
      cm_.httpAsyncClientForCluster(cluster_name_).send(std::move(message_ptr), *this, timeout_);
}

void RawHttpClientImpl::onSuccess(Http::MessagePtr&& message) {
  callbacks_->onComplete(messageToResponse(std::move(message)));
  callbacks_ = nullptr;
}

void RawHttpClientImpl::onFailure(Http::AsyncClient::FailureReason reason) {
  ASSERT(reason == Http::AsyncClient::FailureReason::Reset);
  callbacks_->onComplete(std::make_unique<Response>(getErrorResponse()));
  callbacks_ = nullptr;
}

ResponsePtr RawHttpClientImpl::messageToResponse(Http::MessagePtr message) {
  // Set an error status if parsing status code fails. A Forbidden response is sent to the client
  // if the filter has not been configured with failure_mode_allow.
  uint64_t status_code{};
  if (!StringUtil::atoul(message->headers().Status()->value().c_str(), status_code)) {
    ENVOY_LOG(warn, "ext_authz HTTP client failed to parse the HTTP status code.");
    return std::make_unique<Response>(getErrorResponse());
  }

  // Set an error status if the call to the authorization server returns any of the 5xx HTTP error
  // codes. A Forbidden response is sent to the client if the filter has not been configured with
  // failure_mode_allow.
  if (Http::CodeUtility::is5xx(status_code)) {
    return std::make_unique<Response>(getErrorResponse());
  }

  ResponsePtr response;
  // Set an accepted or a denied authorization response.
  if (status_code == enumToInt(Http::Code::OK)) {
    span_->setTag(Constants::get().TraceStatus, Constants::get().TraceOk);
    response = std::make_unique<Response>(getOkResponse());
  } else {
    span_->setTag(Constants::get().TraceStatus, Constants::get().TraceUnauthz);
    response = std::make_unique<Response>(getDeniedResponse());
    response->status_code = static_cast<Http::Code>(status_code);
    response->body = message->bodyAsString();
  }

  struct Data {
    const Http::LowerCaseStrUnorderedSet* s_;
    Response* r_;
  };

  Data data;
  data.s_ = &allowed_authorization_headers_;
  data.r_ = response.get();

  message->headers().iterate(
      [](const Envoy::Http::HeaderEntry& e, void* ctx) {
        Data* data = static_cast<Data*>(ctx);
        const auto key = Http::LowerCaseString(e.key().c_str());
        if (data->s_->find(key) != data->s_->end()) {
          data->r_->headers_to_add.emplace_back(key, std::string{e.value().getStringView()});
        }
        return Envoy::Http::HeaderMap::Iterate::Continue;
      },
      &data);

  return response;
}

} // namespace ExtAuthz
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
