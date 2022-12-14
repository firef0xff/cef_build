// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEBID_FEDERATED_IDENTITY_SHARING_PERMISSION_CONTEXT_H_
#define CHROME_BROWSER_WEBID_FEDERATED_IDENTITY_SHARING_PERMISSION_CONTEXT_H_

#include <string>

#include "components/permissions/object_permission_context_base.h"
#include "content/public/browser/federated_identity_sharing_permission_context_delegate.h"

namespace base {
class Value;
}

namespace content {
class BrowserContext;
}

// Context for storing permissions associated with the ability for a relying
// party site to pass an identity request to an identity provider through a
// Javascript API.
class FederatedIdentitySharingPermissionContext
    : public content::FederatedIdentitySharingPermissionContextDelegate,
      public permissions::ObjectPermissionContextBase {
 public:
  explicit FederatedIdentitySharingPermissionContext(
      content::BrowserContext* browser_context);

  ~FederatedIdentitySharingPermissionContext() override;

  FederatedIdentitySharingPermissionContext(
      const FederatedIdentitySharingPermissionContext&) = delete;
  FederatedIdentitySharingPermissionContext& operator=(
      const FederatedIdentitySharingPermissionContext&) = delete;

  // content::FederatedIdentitySharingPermissionContextDelegate:
  bool HasSharingPermissionForAnyAccount(
      const url::Origin& relying_party,
      const url::Origin& identity_provider) override;
  bool HasSharingPermission(const url::Origin& relying_party,
                            const url::Origin& identity_provider,
                            const std::string& account_id) override;
  void GrantSharingPermission(const url::Origin& relying_party,
                              const url::Origin& identity_provider,
                              const std::string& account_id) override;
  void RevokeSharingPermission(const url::Origin& relying_party,
                               const url::Origin& identity_provider,
                               const std::string& account_id) override;

 private:
  // permissions::ObjectPermissionContextBase:
  bool IsValidObject(const base::Value& object) override;
  std::u16string GetObjectDisplayName(const base::Value& object) override;
  std::string GetKeyForObject(const base::Value& object) override;
};

#endif  // CHROME_BROWSER_WEBID_FEDERATED_IDENTITY_SHARING_PERMISSION_CONTEXT_H_
