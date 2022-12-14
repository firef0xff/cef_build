// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/webui/projector_app/public/cpp/projector_app_constants.h"

namespace ash {

const char kChromeUIProjectorAppHost[] = "projector";
const char kChromeUIProjectorAnnotatorHost[] = "projector-annotator";

// content::WebUIDataSource::Create() requires trailing slash.
const char kChromeUIUntrustedProjectorAppUrl[] =
    "chrome-untrusted://projector/";
const char kChromeUIUntrustedProjectorPwaUrl[] =
    "https://screencast.apps.chrome";

const char kChromeUITrustedProjectorUrl[] = "chrome://projector/";
const char kChromeUITrustedProjectorAppUrl[] = "chrome://projector/app/";

const char kChromeUITrustedAnnotatorUrl[] = "chrome://projector-annotator/";
const char kChromeUIUntrustedAnnotatorUrl[] =
    "chrome-untrusted://projector-annotator/";
const char kChromeUITrustedAnnotatorAppUrl[] =
    "chrome://projector-annotator/annotator/annotator_embedder.html";
const char kChromeUIUntrustedAnnotatorAppUrl[] =
    "chrome-untrusted://projector-annotator/annotator/annotator.html";

const char kChromeUITrustedProjectorSwaAppId[] =
    "fgnpbdobngpkonkajbmelfhjkemaddhp";

}  // namespace ash
