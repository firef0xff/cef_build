// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_EXTENSIONS_CHROMEOS_SYSTEM_EXTENSIONS_WINDOW_MANAGEMENT_CROS_WINDOW_H_
#define THIRD_PARTY_BLINK_RENDERER_EXTENSIONS_CHROMEOS_SYSTEM_EXTENSIONS_WINDOW_MANAGEMENT_CROS_WINDOW_H_

#include <cstdint>
#include "third_party/blink/public/mojom/chromeos/system_extensions/window_management/cros_window_management.mojom-blink.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/member.h"

namespace blink {
class CrosWindowManagement;
class ScriptPromise;

class CrosWindow : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  CrosWindow(CrosWindowManagement* manager,
             mojom::blink::CrosWindowInfoPtr window);

  void Trace(Visitor*) const override;

  String id();

  String title();
  String appId();
  String windowState();
  bool isFocused();
  String visibilityState();
  int32_t screenLeft();
  int32_t screenTop();
  int32_t width();
  int32_t height();

  ScriptPromise setOrigin(ScriptState* script_state, int32_t x, int32_t y);
  ScriptPromise setBounds(ScriptState* script_state,
                          int32_t x,
                          int32_t y,
                          int32_t width,
                          int32_t height);
  ScriptPromise setFullscreen(ScriptState* script_state, bool fullscreen);
  ScriptPromise maximize(ScriptState* script_state);
  ScriptPromise minimize(ScriptState* script_state);
  ScriptPromise focus(ScriptState* script_state);
  void close();

 private:
  Member<CrosWindowManagement> window_management_;

  mojom::blink::CrosWindowInfoPtr window_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_EXTENSIONS_CHROMEOS_SYSTEM_EXTENSIONS_WINDOW_MANAGEMENT_CROS_WINDOW_H_
