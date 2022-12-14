// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_HTTPS_UPGRADES_HTTPS_ONLY_MODE_UPGRADE_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_HTTPS_UPGRADES_HTTPS_ONLY_MODE_UPGRADE_TAB_HELPER_H_

#include "base/time/time.h"
#include "base/timer/timer.h"
#import "ios/web/public/navigation/web_state_policy_decider.h"
#import "ios/web/public/web_state.h"
#import "ios/web/public/web_state_observer.h"
#import "ios/web/public/web_state_user_data.h"
#include "url/gurl.h"

class PrefService;
class PrerenderService;

// This tab helper handles HTTP main frame navigation upgrades to HTTPS.
// When it encounters an eligible HTTP navigation, it cancels the navigation,
// starts a new navigation to the HTTPS version of the URL and observes the
// response.
// If the response is error free, it considers the upgrade successful.
// Otherwise, it shows the HTTPS-Only Mode interstitial which asks the user to
// proceed to the HTTP URL or go back to the previous page.
class HttpsOnlyModeUpgradeTabHelper
    : public web::WebStateObserver,
      public web::WebStatePolicyDecider,
      public web::WebStateUserData<HttpsOnlyModeUpgradeTabHelper> {
 public:
  // Creates TabHelper. |web_state| and |model| must not be null.
  static void CreateForWebState(web::WebState* web_state, PrefService* prefs);

  // Returns the upgraded HTTPS URL for the give HTTP URL.
  // In tests, we can't use a real HTTPS server to serve good HTTPS, so this
  // returns an HTTP URL that uses the port of the fake HTTPS server.
  // In production, it replaces the scheme with HTTPS.
  static GURL GetUpgradedHttpsUrl(const GURL& http_url,
                                  int https_port_for_testing,
                                  bool use_fake_https_for_testing);

  ~HttpsOnlyModeUpgradeTabHelper() override;
  HttpsOnlyModeUpgradeTabHelper(const HttpsOnlyModeUpgradeTabHelper&) = delete;
  HttpsOnlyModeUpgradeTabHelper& operator=(
      const HttpsOnlyModeUpgradeTabHelper&) = delete;

  // Sets the port used by the embedded https server. This is used to determine
  // the correct port while upgrading URLs to https if the original URL has a
  // non-default port.
  void SetHttpsPortForTesting(int https_port_for_testing);
  // Sets the port used by the embedded http server. This is used to determine
  // the correct port while falling back to http if the upgraded https URL has a
  // non-default port.
  void SetHttpPortForTesting(int http_port_for_testing);
  // Configures tests to use an HTTP server to simulate a good HTTPS response.
  void UseFakeHTTPSForTesting(bool use_fake_https_for_testing);
  // Sets the fallback delay for tests.
  void SetFallbackDelayForTesting(base::TimeDelta delay);
  // Returns true if the upgrade timer is running.
  bool IsTimerRunningForTesting() const;
  // Clears the allowlist that contains domains allowed over HTTP.
  void ClearAllowlistForTesting();

 private:
  HttpsOnlyModeUpgradeTabHelper(web::WebState* web_state,
                                PrefService* prefs,
                                PrerenderService* prerender_service);
  friend class web::WebStateUserData<HttpsOnlyModeUpgradeTabHelper>;

  // Returns true if url is a fake HTTPS URL used in tests. Tests use a fake
  // HTTPS server that actually serves HTTP but on a different port from the
  // test HTTP server. We shouldn't upgrade HTTP URLs from from the fake HTTPS
  // server.
  bool IsFakeHTTPSForTesting(const GURL& url) const;
  bool IsHttpAllowedForUrl(const GURL& url) const;

  // Called when the upgrade timer times out.
  void OnHttpsLoadTimeout();
  // Initiates a fallback navigation to the original HTTP URL. This will be
  // cancelled in ShouldAllowResponse() with an HTTP interstitial, unless the
  // HTTP URL was previously allowlisted.
  void FallbackToHttp();

  // web::WebStatePolicyDecider implementation
  void ShouldAllowRequest(
      NSURLRequest* request,
      WebStatePolicyDecider::RequestInfo request_info,
      WebStatePolicyDecider::PolicyDecisionCallback callback) override;
  void ShouldAllowResponse(
      NSURLResponse* response,
      WebStatePolicyDecider::ResponseInfo response_info,
      web::WebStatePolicyDecider::PolicyDecisionCallback callback) override;
  void WebStateDestroyed() override;

  // web::WebStateObserver implementation.
  void DidStartNavigation(web::WebState* web_state,
                          web::NavigationContext* context) override;
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;
  void WebStateDestroyed(web::WebState* web_state) override;

  // True if the navigation was upgraded.
  bool was_upgraded_ = false;
  // The original HTTP URL that was navigated to.
  GURL http_url_;

  // True if we know if the current navigation has an SSL error.
  bool is_http_fallback_navigation_ = false;

  // True if the HTTP navigation was stopped to initiate an upgrade.
  bool stopped_loading_to_upgrade_ = false;
  bool stopped_with_timeout_ = false;

  // Parameters for the upgraded navigation.
  GURL upgraded_https_url_;
  ui::PageTransition navigation_transition_type_ = ui::PAGE_TRANSITION_FIRST;
  bool navigation_is_renderer_initiated_ = false;
  web::Referrer referrer_;

  int https_port_for_testing_ = 0;
  int http_port_for_testing_ = 0;
  bool use_fake_https_for_testing_ = false;

  base::TimeDelta fallback_delay_ = base::Seconds(3);
  base::OneShotTimer timer_;

  PrefService* prefs_;
  PrerenderService* prerender_service_;

  WEB_STATE_USER_DATA_KEY_DECL();
};

#endif  // IOS_CHROME_BROWSER_HTTPS_UPGRADES_HTTPS_ONLY_MODE_UPGRADE_TAB_HELPER_H_
