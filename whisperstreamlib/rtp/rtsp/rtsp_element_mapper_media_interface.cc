#include <whisperlib/common/base/timer.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_element_mapper_media_interface.h>

namespace streaming {
namespace rtsp {

ElementMapperMediaInterface::ElementMapperMediaInterface(
    ElementMapper* element_mapper)
  : MediaInterface(),
    element_mapper_(element_mapper) {
}
ElementMapperMediaInterface::~ElementMapperMediaInterface() {
}

void ElementMapperMediaInterface::ProcessTag(rtp::Broadcaster* broadcaster,
    BData* bdata, const Tag* tag) {
  uint32 outbuf_size = broadcaster->sender()->OutQueueSpace();

  broadcaster->HandleTag(tag);
  if ( bdata->state_ == BData::PAUSE ) {
    return;
  }

  ElementController* controller = (bdata->req_ == NULL) ?
      NULL : bdata->req_->controller();

  if ( controller != NULL ) {
    if ( outbuf_size == 0 ) {
      RTSP_LOG_DEBUG << "ProcessTag: OutQueueSpace() = 0"
                        ", pausing stream";
      broadcaster->sender()->SetCallOnOutQueueSpace(NewCallback(this,
          &ElementMapperMediaInterface::RestartFlow));
    }
  }
}
void ElementMapperMediaInterface::RestartFlow() {
  RTSP_LOG_DEBUG << "RestartFlow";
}

void ElementMapperMediaInterface::AddBroadcaster(
    rtp::Broadcaster* broadcaster, BData* bdata) {
  CHECK_NULL(GetBroadcaster(broadcaster));
  broadcasters_[broadcaster] = bdata;
}
const ElementMapperMediaInterface::BData*
ElementMapperMediaInterface::GetBroadcaster(
    rtp::Broadcaster* broadcaster) const {
  BroadcasterMap::const_iterator it = broadcasters_.find(broadcaster);
  return it == broadcasters_.end() ? NULL : it->second;
}
ElementMapperMediaInterface::BData*
ElementMapperMediaInterface::GetBroadcaster(rtp::Broadcaster* broadcaster) {
  BroadcasterMap::iterator it = broadcasters_.find(broadcaster);
  return it == broadcasters_.end() ? NULL : it->second;
}
void ElementMapperMediaInterface::DelBroadcaster(
    rtp::Broadcaster* broadcaster) {
  BroadcasterMap::iterator it = broadcasters_.find(broadcaster);
  if ( it == broadcasters_.end() ) {
    return;
  }
  BData* bdata = it->second;
  delete bdata;
  broadcasters_.erase(it);
}

bool ElementMapperMediaInterface::Describe(const string& media,
    MediaInfoCallback* callback) {
  return element_mapper_->DescribeMedia(media, callback);
}
bool ElementMapperMediaInterface::Attach(rtp::Broadcaster* broadcaster,
    const string& media) {
  CHECK_NULL(GetBroadcaster(broadcaster));
  RTSP_LOG_INFO << "Attach broadcaster to " << media;
  BData* bdata = new BData(new streaming::Request(), NULL, BData::PAUSE);
  bdata->callback_ = NewPermanentCallback(this,
      &ElementMapperMediaInterface::ProcessTag, broadcaster, bdata);
  if ( !element_mapper_->AddRequest(media.c_str(), bdata->req_,
       bdata->callback_) ) {
    delete bdata->req_;
    delete bdata->callback_;
    delete bdata;
    return false;
  }
  AddBroadcaster(broadcaster, bdata);
  return true;
}
void ElementMapperMediaInterface::Detach(rtp::Broadcaster* broadcaster) {
  const BData* bdata = GetBroadcaster(broadcaster);
  if ( bdata == NULL ) {
    RTSP_LOG_ERROR << "Cannot detach broadcaster, not found";
    return;
  }
  element_mapper_->RemoveRequest(bdata->req_, bdata->callback_);
  DelBroadcaster(broadcaster);
}
void ElementMapperMediaInterface::Play(rtp::Broadcaster* broadcaster,
    bool play) {
  BData* bdata = GetBroadcaster(broadcaster);
  CHECK_NOT_NULL(bdata);
  bdata->state_ = (play ? BData::PLAY : BData::PAUSE);
  if ( bdata->req_ == NULL || bdata->req_->controller() == NULL ||
      !bdata->req_->controller()->SupportsPause() ) {
    RTSP_LOG_ERROR << "Cannot " << (play ? "PLAY" : "PAUSE") << " "
                  << ((bdata->req_ == NULL || bdata->req_->controller() == NULL)
                   ? "controller is NULL" :
                     "controller does not support Pause");
    return;
  }
  if ( !bdata->first_play_ ) {
    bdata->req_->controller()->Pause(!play);
  }
}

} // namespace rtsp
} // namespace streaming

