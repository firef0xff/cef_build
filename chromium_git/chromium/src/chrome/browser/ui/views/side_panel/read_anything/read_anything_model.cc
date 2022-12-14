// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/side_panel/read_anything/read_anything_model.h"

#include <memory>

#include "base/check.h"
#include "base/strings/utf_string_conversions.h"

ReadAnythingModel::ReadAnythingModel()
    : font_model_(std::make_unique<ReadAnythingFontModel>()) {}
ReadAnythingModel::~ReadAnythingModel() = default;

void ReadAnythingModel::AddObserver(Observer* obs) {
  observers_.AddObserver(obs);
}

void ReadAnythingModel::RemoveObserver(Observer* obs) {
  observers_.RemoveObserver(obs);
}

void ReadAnythingModel::SetSelectedFontIndex(int new_index) {
  font_name_ = font_model_->GetCurrentFontName(new_index);
  NotifyFontNameUpdated();
}

void ReadAnythingModel::SetContent(std::vector<ContentNodePtr> content_nodes) {
  content_nodes_ = std::move(content_nodes);
  NotifyContentUpdated();
}

void ReadAnythingModel::NotifyFontNameUpdated() {
  for (Observer& obs : observers_) {
    obs.OnFontNameUpdated(font_name_);
  }
}

void ReadAnythingModel::NotifyContentUpdated() {
  for (Observer& obs : observers_) {
    obs.OnContentUpdated(content_nodes_);
  }
}

ReadAnythingFontModel::ReadAnythingFontModel() {
  // TODO(1266555): Replace these with proper versions once finalized.
  font_choices_.emplace_back(u"Standard font");
  font_choices_.emplace_back(u"Sans-serif");
  font_choices_.emplace_back(u"Serif");
  font_choices_.emplace_back(u"Arial");
  font_choices_.emplace_back(u"Open Sans");
  font_choices_.emplace_back(u"Calibri");
  font_choices_.shrink_to_fit();
}

int ReadAnythingFontModel::GetDefaultIndex() const {
  // TODO(1266555): This should be set on initialization based on Prefs.
  return 0;
}

int ReadAnythingFontModel::GetItemCount() const {
  return font_choices_.size();
}

std::u16string ReadAnythingFontModel::GetItemAt(int index) const {
  // TODO(1266555): Placeholder text, replace when finalized.
  return u"Default font";
}

std::u16string ReadAnythingFontModel::GetDropDownTextAt(int index) const {
  DCHECK(index >= 0 && index < GetItemCount());
  return font_choices_.at(index);
}

std::string ReadAnythingFontModel::GetCurrentFontName(int index) {
  DCHECK(index >= 0 && index < GetItemCount());
  return base::UTF16ToUTF8(font_choices_.at(index));
}

ReadAnythingFontModel::~ReadAnythingFontModel() = default;
