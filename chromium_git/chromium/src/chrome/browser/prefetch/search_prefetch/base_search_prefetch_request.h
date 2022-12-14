// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFETCH_SEARCH_PREFETCH_BASE_SEARCH_PREFETCH_REQUEST_H_
#define CHROME_BROWSER_PREFETCH_SEARCH_PREFETCH_BASE_SEARCH_PREFETCH_REQUEST_H_

#include <memory>

#include "base/callback.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

class Profile;
class SearchPrefetchURLLoader;

// Any updates to this class need to be propagated to enums.xml.
enum class SearchPrefetchStatus {
  // The request has not started yet. This status should ideally never be
  // recorded as Start() should be called on the same stack as creating the
  // fetcher (as of now).
  kNotStarted = 0,
  // The request is on the network and may move to any other state.
  kInFlight = 1,
  // The request can be served to the navigation stack, but may still encounter
  // errors and move to |kRequestFailed| or it may complete and move to
  // |kComplete|. It may also move to |kCanBeServedAndUserClicked| when the user
  // navigates to the result in omnibox or |kRequestCancelled| if the user
  // closes omnibox.
  kCanBeServed = 2,
  // The request can be served to the navigation stack, and is marked as being
  // clicked by the user. At this point, it may move to |kComplete| or
  // |kRequestFailed|.
  kCanBeServedAndUserClicked = 3,
  // The request can be served to the navigation stack, and has fully streamed
  // the response with no errors.
  kComplete = 4,
  // The request hit an error and cannot be served.
  kRequestFailed = 5,
  // The request was cancelled before completion. This is terminal state.
  kRequestCancelled = 6,
  // The request was served to the navigation stack. This is terminal state.
  kServed = 7,
  kMaxValue = kServed,
};

// A class representing a prefetch used by the Search Prefetch Service.
// Implementors should provide the fetch and storage functionality as well as
// updating |current_status_|.
class BaseSearchPrefetchRequest {
 public:
  BaseSearchPrefetchRequest(
      const GURL& prefetch_url,
      bool navigation_prefetch,
      base::OnceCallback<void(bool)> report_error_callback);
  virtual ~BaseSearchPrefetchRequest();

  BaseSearchPrefetchRequest(const BaseSearchPrefetchRequest&) = delete;
  BaseSearchPrefetchRequest& operator=(const BaseSearchPrefetchRequest&) =
      delete;

  // The NTA for any search prefetch request.
  static net::NetworkTrafficAnnotationTag NetworkAnnotationForPrefetch();

  // Starts the network request to prefetch |prefetch_url_|. Sets various fields
  // on a resource request and calls |StartPrefetchRequestInternal()|. Returns
  // |false| if the request is not started (i.e., it would be deferred by
  // throttles).
  bool StartPrefetchRequest(Profile* profile);

  // Marks a prefetch as canceled and stops any ongoing fetch.
  void CancelPrefetch();

  // Called when the prefetch encounters an error.
  void ErrorEncountered();

  // Called when the prefetch encounters an error.
  void ErrorEncounteredUsingFallback();

  // Update the status when the request is serveable.
  void MarkPrefetchAsServable();

  // Update the status when the request is complete.
  void MarkPrefetchAsComplete();

  // Update the status when the relevant search item is clicked in omnibox.
  void MarkPrefetchAsClicked();

  // Update the status when the request is actually served to the navigation
  // stack.
  void MarkPrefetchAsServed();

  // Whether the request was started as a navigation prefetch (as opposed to a
  // suggestion prefetch).
  bool navigation_prefetch() const { return navigation_prefetch_; }

  // Starts and begins processing |resource_request|.
  virtual void StartPrefetchRequestInternal(
      Profile* profile,
      std::unique_ptr<network::ResourceRequest> resource_request,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      base::OnceCallback<void(bool)> report_error_callback) = 0;

  // Stops the on-going prefetch and should mark |current_status_|
  // appropriately.
  virtual void StopPrefetch() = 0;

  SearchPrefetchStatus current_status() const { return current_status_; }

  const GURL& prefetch_url() const { return prefetch_url_; }

  // Takes ownership of underlying data/objects needed to serve the response.
  virtual std::unique_ptr<SearchPrefetchURLLoader>
  TakeSearchPrefetchURLLoader() = 0;

 protected:
  SearchPrefetchStatus current_status_ = SearchPrefetchStatus::kNotStarted;

  // The URL to prefetch the search terms from.
  GURL prefetch_url_;

  // Whether this is for a navigation-time prefetch.
  bool navigation_prefetch_;

  // Called when there is a network/server error on the prefetch request.
  base::OnceCallback<void(bool)> report_error_callback_;

  // The time at which the prefetched URL was clicked in the Omnibox.
  base::TimeTicks time_clicked_;
};

#endif  // CHROME_BROWSER_PREFETCH_SEARCH_PREFETCH_BASE_SEARCH_PREFETCH_REQUEST_H_
