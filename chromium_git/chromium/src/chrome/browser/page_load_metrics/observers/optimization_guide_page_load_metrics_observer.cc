// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/optimization_guide_page_load_metrics_observer.h"

#include "content/public/browser/navigation_handle.h"
#include "url/gurl.h"

OptimizationGuidePageLoadMetricsObserver::
    OptimizationGuidePageLoadMetricsObserver() = default;

OptimizationGuidePageLoadMetricsObserver::
    ~OptimizationGuidePageLoadMetricsObserver() = default;

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
OptimizationGuidePageLoadMetricsObserver::OnStart(
    content::NavigationHandle* navigation_handle,
    const GURL& currently_committed_url,
    bool started_in_foreground) {
  if (!started_in_foreground)
    return STOP_OBSERVING;
  if (!navigation_handle->GetURL().SchemeIsHTTPOrHTTPS())
    return STOP_OBSERVING;
  optimization_guide_web_contents_observer_ =
      OptimizationGuideWebContentsObserver::FromWebContents(
          navigation_handle->GetWebContents());
  if (!optimization_guide_web_contents_observer_)
    return STOP_OBSERVING;
  return CONTINUE_OBSERVING;
}

// TODO(https://crbug.com/1317494): Audit and use appropriate policy.
page_load_metrics::PageLoadMetricsObserver::ObservePolicy
OptimizationGuidePageLoadMetricsObserver::OnFencedFramesStart(
    content::NavigationHandle* navigation_handle,
    const GURL& currently_committed_url) {
  return STOP_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
OptimizationGuidePageLoadMetricsObserver::OnHidden(
    const page_load_metrics::mojom::PageLoadTiming& timing) {
  if (optimization_guide_web_contents_observer_)
    optimization_guide_web_contents_observer_->FlushLastNavigationData();
  return STOP_OBSERVING;
}

void OptimizationGuidePageLoadMetricsObserver::OnComplete(
    const page_load_metrics::mojom::PageLoadTiming& timing) {
  if (optimization_guide_web_contents_observer_)
    optimization_guide_web_contents_observer_->FlushLastNavigationData();
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
OptimizationGuidePageLoadMetricsObserver::FlushMetricsOnAppEnterBackground(
    const page_load_metrics::mojom::PageLoadTiming& timing) {
  if (optimization_guide_web_contents_observer_)
    optimization_guide_web_contents_observer_->FlushLastNavigationData();
  return STOP_OBSERVING;
}
