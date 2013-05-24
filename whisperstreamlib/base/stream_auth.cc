// Copyright (c) 2012, Whispersoft s.r.l.
// All rights reserved.

#include <whisperlib/common/base/timer.h>
#include <whisperstreamlib/base/stream_auth.h>
#include <whisperstreamlib/base/request.h>
#include <whisperstreamlib/base/auto/request_types.h>

// defined in request.cc
DECLARE_string(req_session_key);

namespace streaming {

void AuthorizerRequest::ReadFromUrl(const URL& url) {
  if ( url.has_query() ) {
    vector< pair<string, string> > comp;
    if ( url.GetQueryParameters(&comp, true) ) {
      ReadQueryComponents(comp);
    }
  }
}

void AuthorizerRequest::ReadQueryComponents(
    const vector< pair<string, string> >& comp) {
  for ( int i = 0; i < comp.size(); ++i ) {
    if ( comp[i].first == streaming::kMediaUrlParam_UserName ) {
      user_ = comp[i].second;
    } else if ( comp[i].first == streaming::kMediaUrlParam_UserPass ) {
      passwd_ = comp[i].second;
    } else if ( comp[i].first == streaming::kMediaUrlParam_UserToken ) {
      token_ = comp[i].second;
    }
  }
}

string AuthorizerRequest::GetUrlQuery() const {
  vector<string> ret;
  if ( !user_.empty() ) {
    ret.push_back(FLAGS_req_session_key + "=" +
                  URL::UrlEscape(user_));
  }
  if ( !passwd_.empty() ) {
    ret.push_back(FLAGS_req_session_key + "=" +
                  URL::UrlEscape(passwd_));
  }
  if ( !token_.empty() ) {
    ret.push_back(FLAGS_req_session_key + "=" +
                  URL::UrlEscape(token_));
  }
  return strutil::JoinStrings(ret, "&");
}

void AuthorizerRequest::ToSpec(MediaAuthorizerRequestSpec* spec) const {
  spec->user_ = user_;
  spec->passwd_ = passwd_;
  spec->token_ = token_;
  spec->net_address_ = net_address_;

  spec->resource_ = resource_;

  spec->action_ = action_;
  spec->action_performed_ms_ = action_performed_ms_;
}

void AuthorizerRequest::FromSpec(const MediaAuthorizerRequestSpec& spec) {
  user_ = spec.user_.get();
  passwd_ = spec.passwd_.get();
  token_ = spec.token_.get();
  net_address_ = spec.net_address_.get();

  resource_ = spec.resource_.get();

  action_ = spec.action_.get();
  action_performed_ms_ = spec.action_performed_ms_.get();
}

string AuthorizerRequest::ToString() const {
  ostringstream oss;
  oss << "AuthorizerRequest{user_: " << user_
      << ", passwd_: " << passwd_
      << ", token_: " << token_
      << ", net_address_: " << net_address_
      << ", resource_: " << resource_
      << ", action_: " << action_
      << ", action_performed_ms_: " << action_performed_ms_ << "}";
  return oss.str();
}

string AuthorizerReply::ToString() const {
  ostringstream oss;
  oss << "AuthorizerReply{allowed_: " << strutil::BoolToString(allowed_)
      << ", time_limit_ms_: " << time_limit_ms_ << "}";
  return oss.str();
}

AsyncAuthorize::AsyncAuthorize(net::Selector& selector)
  : auth_(NULL),
    req_(),
    authorization_completed_(NewPermanentCallback(this,
        &AsyncAuthorize::AuthorizationCompleted)),
    first_auth_completion_(NULL),
    reauthorization_failed_(NULL),
    reauthorization_alarm_(selector),
    first_auth_ts_(0) {
  // setup reauthorization_alarm_
  reauthorization_alarm_.Set(NewPermanentCallback(this,
      &AsyncAuthorize::Reauthorize), true,
      0, false, false);
}
AsyncAuthorize::~AsyncAuthorize() {
  Stop();
  CHECK_NULL(auth_);
  CHECK_NULL(first_auth_completion_);
  CHECK_NULL(reauthorization_failed_);
  delete authorization_completed_;
  authorization_completed_ = NULL;
}

void AsyncAuthorize::Start(Authorizer* auth,
                           const AuthorizerRequest& req,
                           Callback1<bool>* completion,
                           Closure* reauthorization_failed) {
  CHECK(!completion->is_permanent());
  CHECK(reauthorization_failed == NULL ||
        !reauthorization_failed->is_permanent());
  Stop();
  CHECK_NULL(auth_);
  CHECK_NULL(first_auth_completion_);
  CHECK_NULL(reauthorization_failed_);
  auth_ = auth;
  auth_->IncRef();
  req_ = req;
  first_auth_completion_ = completion;
  reauthorization_failed_ = reauthorization_failed;
  auth_->Authorize(req_, authorization_completed_);
}

void AsyncAuthorize::Stop() {
  if ( auth_ == NULL ) {
    return;
  }

  auth_->Cancel(authorization_completed_);
  auth_->DecRef();
  auth_ = NULL;

  delete first_auth_completion_;
  first_auth_completion_ = NULL;

  delete reauthorization_failed_;
  reauthorization_failed_ = NULL;

  reauthorization_alarm_.Stop();
}

void AsyncAuthorize::Reauthorize() {
  req_.action_performed_ms_ = timer::TicksMsec() - first_auth_ts_;
  auth_->Authorize(req_, authorization_completed_);
}

void AsyncAuthorize::AuthorizationCompleted(const AuthorizerReply& reply) {
  if ( first_auth_completion_ != NULL ) {
    first_auth_ts_ = timer::TicksMsec();
    Callback1<bool>* c = first_auth_completion_;
    first_auth_completion_ = NULL;
    if ( reauthorization_failed_ != NULL &&
         reply.allowed_ &&
         reply.time_limit_ms_ > 0 ) {
      // schedule reauthorization
      reauthorization_alarm_.ResetTimeout(reply.time_limit_ms_/2);
      reauthorization_alarm_.Start();
    }
    c->Run(reply.allowed_);
    return;
  }
  CHECK_NOT_NULL(reauthorization_failed_);
  if ( !reply.allowed_ ) {
    Closure* c = reauthorization_failed_;
    reauthorization_failed_ = NULL;
    c->Run();
    return;
  }
}

} // namespace streaming
