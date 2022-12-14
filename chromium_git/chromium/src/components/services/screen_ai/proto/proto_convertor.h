// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_SCREEN_AI_PROTO_PROTO_CONVERTOR_H_
#define COMPONENTS_SERVICES_SCREEN_AI_PROTO_PROTO_CONVERTOR_H_

#include <string>

#include "ui/accessibility/ax_tree_update.h"

namespace screen_ai {

// Converts serialized VisualAnnotation proto from ScreenAI to AXTreeUpdate.
ui::AXTreeUpdate ScreenAIVisualAnnotationToAXTreeUpdate(
    const std::string& serialized_proto);

// Converts an AXTreeUpdate snapshot to serialized ViewHierarchy proto for
// Screen2X.
std::string Screen2xSnapshotToViewHierarchy(const ui::AXTreeUpdate& snapshot);

}  // namespace screen_ai
#endif  // COMPONENTS_SERVICES_SCREEN_AI_PROTO_PROTO_CONVERTOR_H_
