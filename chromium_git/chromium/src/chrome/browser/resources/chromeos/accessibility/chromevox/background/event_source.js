// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Tracks event sources.
 */

goog.provide('EventSourceState');
goog.provide('EventSourceType');

/** @enum {string} */
EventSourceType = {
  NONE: 'none',
  BRAILLE_KEYBOARD: 'brailleKeyboard',
  STANDARD_KEYBOARD: 'standardKeyboard',
  TOUCH_GESTURE: 'touchGesture'
};

/**
 * Sets the current event source.
 * @param {EventSourceType} source
 */
EventSourceState.set = function(source) {
  EventSource.current_ = source;
};

/**
 * Gets the current event source.
 * @return {EventSourceType}
 */
EventSourceState.get = function() {
  return EventSource.current_;
};

/**
 * @private {EventSourceType}
 */
EventSource.current_ =
    chrome.accessibilityPrivate.IS_DEFAULT_EVENT_SOURCE_TOUCH ?
    EventSourceType.TOUCH_GESTURE :
    EventSourceType.NONE;

BridgeHelper.registerHandler(
    BridgeTarget.EVENT_SOURCE_STATE, BridgeAction.GET,
    () => EventSourceState.get());
