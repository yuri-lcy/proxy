/* Copyright Istio Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/envoy/http/baggage_handler/baggage_handler.h"

#include "absl/strings/str_cat.h"
#include "source/common/http/header_utility.h"
#include "source/common/router/string_accessor_impl.h"
#include "src/envoy/common/metadata_object.h"
#include "src/envoy/http/baggage_handler/config/baggage_handler.pb.h"

namespace Envoy {
namespace Http {
namespace BaggageHandler {

Http::FilterHeadersStatus BaggageHandlerFilter::decodeHeaders(
    Http::RequestHeaderMap& headers, bool) {
  const auto header_value = Http::HeaderUtility::getAllOfHeaderAsString(
                                headers, LowerCaseString("baggage"))
                                .result();

  if (header_value.has_value()) {
    auto source_meta = Common::WorkloadMetadataObject::fromBaggage(
        header_value.value(), decoder_callbacks_->connection()->ssl());

    auto filter_state = decoder_callbacks_->streamInfo().filterState();

    filter_state->setData(
        Common::WorkloadMetadataObject::kSourceMetadataObjectKey, source_meta,
        StreamInfo::FilterState::StateType::ReadOnly,
        StreamInfo::FilterState::LifeSpan::Request,
        StreamInfo::StreamSharingMayImpactPooling::SharedWithUpstreamConnectionOnce);

    ENVOY_LOG(
        trace,
        absl::StrCat("baggage header found. filter state set: ",
                     Common::WorkloadMetadataObject::kSourceMetadataObjectKey));

    // Setting a StringAccessor filter state which can be assigned to custom
    // header with PER_REQUEST_STATE
    auto accessor = std::make_shared<Envoy::Router::StringAccessorImpl>(
        header_value.value());
    filter_state->setData(
        Common::WorkloadMetadataObject::kSourceMetadataBaggageKey, accessor,
        StreamInfo::FilterState::StateType::ReadOnly,
        StreamInfo::FilterState::LifeSpan::Request,
        StreamInfo::StreamSharingMayImpactPooling::SharedWithUpstreamConnectionOnce);
  } else {
    ENVOY_LOG(trace, "no baggage header found.");
  }
  return Http::FilterHeadersStatus::Continue;
}

void BaggageHandlerFilter::setDecoderFilterCallbacks(
    Http::StreamDecoderFilterCallbacks& callbacks) {
  decoder_callbacks_ = &callbacks;
}

Http::FilterHeadersStatus BaggageHandlerFilter::encodeHeaders(
    Http::ResponseHeaderMap&, bool) {
  return Http::FilterHeadersStatus::Continue;
}

void BaggageHandlerFilter::setEncoderFilterCallbacks(
    Http::StreamEncoderFilterCallbacks& callbacks) {
  encoder_callbacks_ = &callbacks;
}

}  // namespace BaggageHandler
}  // namespace Http
}  // namespace Envoy
