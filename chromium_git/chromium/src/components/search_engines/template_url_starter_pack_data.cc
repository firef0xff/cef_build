// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/search_engines/template_url_starter_pack_data.h"

#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/template_url_data.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace TemplateURLStarterPackData {

const int kMaxStarterPackEngineID = 3;
const int kCurrentDataVersion = 1;

const StarterPackEngine bookmarks = {
    IDS_SEARCH_ENGINES_STARTER_PACK_BOOKMARKS_NAME,
    IDS_SEARCH_ENGINES_STARTER_PACK_BOOKMARKS_KEYWORD,
    nullptr,
    "chrome://bookmarks/?q={searchTerms}",
    1,
    SEARCH_ENGINE_STARTER_PACK_BOOKMARKS,
};

const StarterPackEngine history = {
    IDS_SEARCH_ENGINES_STARTER_PACK_HISTORY_NAME,
    IDS_SEARCH_ENGINES_STARTER_PACK_HISTORY_KEYWORD,
    nullptr,
    "chrome://history/?q={searchTerms}",
    2,
    SEARCH_ENGINE_STARTER_PACK_HISTORY,
};

const StarterPackEngine* engines[] = {
    &bookmarks,
    &history,
};

int GetDataVersion() {
  return kCurrentDataVersion;
}

std::vector<std::unique_ptr<TemplateURLData>> GetStarterPackEngines() {
  std::vector<std::unique_ptr<TemplateURLData>> t_urls;

  for (auto* engine : engines) {
    t_urls.push_back(TemplateURLDataFromStarterPackEngine(*engine));
  }
  return t_urls;
}

}  // namespace TemplateURLStarterPackData
