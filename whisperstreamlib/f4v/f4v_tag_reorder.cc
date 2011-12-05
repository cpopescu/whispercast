#include <whisperstreamlib/f4v/f4v_util.h>
#include <whisperstreamlib/f4v/atoms/base_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/moov_atom.h>
#include "f4v_tag_reorder.h"

namespace streaming {
namespace f4v {

TagReorder::TagReorder(bool order_by_timestamp)
  : order_by_timestamp_(order_by_timestamp),
    moov_(NULL),
    ordered_frames_(),
    ordered_next_(0),
    cache_(),
    output_() {
}
TagReorder::~TagReorder() {
  Clear();
  CHECK_NULL(moov_);
  CHECK(ordered_frames_.empty());
  CHECK(cache_.empty());
  ClearOutput();
  CHECK(output_.empty());
}

void TagReorder::Push(const Tag* tag) {
  //LOG_WARNING << "Push tag: " << tag->ToString();
  if ( tag->is_atom() ) {
    const BaseAtom* atom = tag->atom();
    if ( atom->type() == ATOM_MOOV ) {
      Clear();
      moov_ = new MoovAtom(*static_cast<const MoovAtom*>(atom));
      vector<FrameHeader*> frames;
      util::ExtractFrames(*moov_, true, &frames);
      util::ExtractFrames(*moov_, false, &frames);
      ::stable_sort(frames.begin(), frames.end(), order_by_timestamp_ ?
          util::CompareFramesByTimestamp : util::CompareFramesByOffset);
      ::copy(frames.begin(), frames.end(), ::back_inserter(ordered_frames_));
    }
    output_.push_back(tag);
    return;
  }
  CHECK(tag->is_frame());
  const Frame* frame = tag->frame();

  // send RAW frames directly to output. Nobody is expecting these.
  if ( frame->header().type() == FrameHeader::RAW_FRAME ) {
    output_.push_back(tag);
    return;
  }

  // cache a copy of every frame
  pair<FrameMap::iterator, bool> result = cache_.insert(make_pair(
      frame->header().offset(), new Tag(*tag)));
  CHECK(result.second) << "duplicate frame offset in cache, old: "
                       << result.first->second->ToString()
                       << ", new: " << tag->ToString();

  // pop ordered frames from cache into output_
  PopCache();
}

const Tag* TagReorder::Pop() {
  if ( output_.empty() ) {
    //LOG_WARNING << "Pop() => Nothing";
    return NULL;
  }
  const Tag* tag = output_.front();
  output_.pop_front();
  //LOG_WARNING << "Pop() => tag: " << tag->ToString();
  return tag;
}
void TagReorder::PopCache() {
  while ( true ) {
    if ( ordered_next_ >= ordered_frames_.size() ) {
      // stop when no more frames expected
      //LOG_WARNING << "PopCache() => No more frames expected";
      break;
    }
    const FrameHeader* next = ordered_frames_[ordered_next_];
    FrameMap::iterator it = cache_.find(next->offset());
    if ( it == cache_.end() ) {
      //LOG_WARNING << "PopCache() => expected frame: " << *next
      //            << " not found in cache, cache.size: " << cache_.size();
      // stop when an expected frame is not found in cache
      break;
    }
    // move frame from cache_ into output_
    //LOG_WARNING << "PopCache() => moving expected frame: " << *next
    //            << " into output, output.size: " << output_.size();
    const Tag* tag = it->second;
    output_.push_back(tag);
    // erase frame from cache, advance to the next expected frame
    cache_.erase(it);
    ordered_next_++;
  }
}

void TagReorder::Clear() {
  delete moov_;
  moov_ = NULL;
  // clear ordered_frames
  for ( uint32 i = 0; i < ordered_frames_.size(); i++ ) {
    delete ordered_frames_[i];
  }
  ordered_frames_.clear();
  ordered_next_ = 0;
  // clear cache
  for ( FrameMap::iterator it = cache_.begin(); it != cache_.end(); ++it ) {
    const Tag* tag = it->second;
    delete tag;
  }
  cache_.clear();
}
void TagReorder::ClearOutput() {
  for ( TagList::iterator it = output_.begin(); it != output_.end(); ++it ) {
    const Tag* tag = *it;
    delete tag;
  }
  output_.clear();
}

}
}
