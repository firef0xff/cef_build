// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SCROLLING_SCROLLING_COORDINATOR_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SCROLLING_SCROLLING_COORDINATOR_CONTEXT_H_

#include "base/memory/scoped_refptr.h"
#include "cc/animation/animation_timeline.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"

namespace cc {
class AnimationHost;
}

namespace blink {

// This enscapsulates ScrollingCoordinator state for each local frame root.
// TODO(kenrb): This class could be temporary depending on how
// https://crbug.com/680606 is resolved.
class CORE_EXPORT ScrollingCoordinatorContext final {
  USING_FAST_MALLOC(ScrollingCoordinatorContext);

 public:
  ScrollingCoordinatorContext() = default;
  ScrollingCoordinatorContext(const ScrollingCoordinatorContext&) = delete;
  ScrollingCoordinatorContext& operator=(const ScrollingCoordinatorContext&) =
      delete;
  virtual ~ScrollingCoordinatorContext() = default;

  void SetAnimationTimeline(scoped_refptr<cc::AnimationTimeline> timeline) {
    animation_timeline_ = timeline;
  }
  void SetAnimationHost(cc::AnimationHost* host) { animation_host_ = host; }

  cc::AnimationTimeline* GetCompositorAnimationTimeline() {
    return animation_timeline_.get();
  }
  cc::AnimationHost* GetCompositorAnimationHost() { return animation_host_; }

 private:
  scoped_refptr<cc::AnimationTimeline> animation_timeline_;
  cc::AnimationHost* animation_host_ = nullptr;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SCROLLING_SCROLLING_COORDINATOR_CONTEXT_H_
