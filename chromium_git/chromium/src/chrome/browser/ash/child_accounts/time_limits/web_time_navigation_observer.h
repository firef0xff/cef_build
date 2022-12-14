// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ASH_CHILD_ACCOUNTS_TIME_LIMITS_WEB_TIME_NAVIGATION_OBSERVER_H_
#define CHROME_BROWSER_ASH_CHILD_ACCOUNTS_TIME_LIMITS_WEB_TIME_NAVIGATION_OBSERVER_H_

#include <string>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "chrome/browser/ash/child_accounts/time_limits/app_types.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace content {
class WebContents;
class NavigationEntry;
}  // namespace content

class GURL;

namespace ash {
namespace app_time {

class WebTimeNavigationObserver
    : public content::WebContentsUserData<WebTimeNavigationObserver>,
      public content::WebContentsObserver {
 public:
  struct NavigationInfo {
    base::Time navigation_finish_time;

    // Boolean to specify if the navigation ended in an error page.
    bool is_error;

    // Boolean to specify if the WebContents is hosting a web app.
    bool is_web_app;

    // The url that is being hosted in WebContents.
    GURL url;

    // The WebContent where the navigation has taken place.
    content::WebContents* web_contents;
  };

  class EventListener : public base::CheckedObserver {
   public:
    virtual void OnWebActivityChanged(const NavigationInfo& info) {}
    virtual void WebTimeNavigationObserverDestroyed(
        WebTimeNavigationObserver* observer) {}
  };

  static void MaybeCreateForWebContents(content::WebContents* web_contents);
  static void CreateForWebContents(content::WebContents* web_contents) = delete;

  ~WebTimeNavigationObserver() override;

  void AddObserver(EventListener* listener);
  void RemoveObserver(EventListener* listener);

  bool IsWebApp() const;

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;
  void WebContentsDestroyed() override;
  void TitleWasSet(content::NavigationEntry* entry) override;

  const absl::optional<NavigationInfo>& last_navigation_info() const {
    return last_navigation_info_;
  }

  const absl::optional<std::u16string>& previous_title() const {
    return previous_title_;
  }

 private:
  friend class content::WebContentsUserData<WebTimeNavigationObserver>;

  explicit WebTimeNavigationObserver(content::WebContents* web_contents);
  WebTimeNavigationObserver(const WebTimeNavigationObserver&) = delete;
  WebTimeNavigationObserver& operator=(const WebTimeNavigationObserver&) =
      delete;

  base::ObserverList<EventListener> listeners_;

  absl::optional<NavigationInfo> last_navigation_info_ = absl::nullopt;

  absl::optional<std::u16string> previous_title_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace app_time
}  // namespace ash

#endif  // CHROME_BROWSER_ASH_CHILD_ACCOUNTS_TIME_LIMITS_WEB_TIME_NAVIGATION_OBSERVER_H_
