// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Catalin Popescu
//

#include <whisperstreamlib/base/tag_localizer.h>

namespace streaming {

// Tags returned by this guy should be released *only* through
// ReleaseLocalizedTag and else, used *only* in the thread of the
// provided selector
const Tag* TagLocalizer::LocalizeTag(const Tag* tag, net::Selector* selector) {
  DCHECK(media_selector_->IsInSelectThread());
  synch::MutexLocker l(&mutex_);
  SelectorMap::iterator it_sel = selector_map_.find(selector);
  if ( it_sel == selector_map_.end() ) {
    TagMap* new_map = new TagMap();
    tag->Retain();
    const Tag* new_tag = tag->CopyClone();
    new_map->insert(make_pair(tag, new_tag));
    selector_map_.insert(make_pair(selector, new_map));
    media_selector_->RegisterAlarm(
        NewCallback(this, &TagLocalizer::CloseLocalizedTag,
                    new_map, tag, new_tag),
        linger_timeout_ms_);
    return new_tag->Retain();
  } else {
    TagMap::const_iterator it_tag = it_sel->second->find(tag);
    if ( it_tag == it_sel->second->end() ) {
      tag->Retain();
      const Tag* new_tag = tag->CopyClone();
      it_sel->second->insert(make_pair(tag, new_tag));
      media_selector_->RegisterAlarm(
          NewCallback(this, &TagLocalizer::CloseLocalizedTag,
                      it_sel->second, tag, new_tag),
          linger_timeout_ms_);
      return new_tag->Retain();
    } else {
      return it_tag->second->Retain();
    }
  }
}

}
