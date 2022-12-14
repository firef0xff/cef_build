// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/privacy/privacy_safe_browsing_mediator.h"

#include "base/auto_reset.h"
#include "base/mac/foundation_util.h"
#include "base/notreached.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "ios/chrome/browser/application_context.h"
#import "ios/chrome/browser/policy/policy_util.h"
#include "ios/chrome/browser/pref_names.h"
#import "ios/chrome/browser/ui/icons/chrome_symbol.h"
#import "ios/chrome/browser/ui/list_model/list_model.h"
#import "ios/chrome/browser/ui/settings/cells/settings_image_detail_text_item.h"
#import "ios/chrome/browser/ui/settings/privacy/privacy_constants.h"
#import "ios/chrome/browser/ui/settings/privacy/privacy_safe_browsing_consumer.h"
#import "ios/chrome/browser/ui/settings/privacy/privacy_safe_browsing_navigation_commands.h"
#import "ios/chrome/browser/ui/settings/sync/utils/sync_util.h"
#import "ios/chrome/browser/ui/settings/utils/observable_boolean.h"
#import "ios/chrome/browser/ui/settings/utils/pref_backed_boolean.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_info_button_item.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_info_button_item_delegate.h"
#import "ios/chrome/browser/ui/ui_feature_flags.h"
#import "ios/chrome/browser/ui/util/uikit_ui_util.h"
#import "ios/chrome/common/ui/colors/semantic_color_names.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

typedef NSArray<TableViewItem*>* ItemArray;

namespace {

// The size of the symbol image.
NSInteger kSymbolImagePointSize = 20;

// List of item types.
typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeSafeBrowsingStandardProtection = kItemTypeEnumZero,
  ItemTypeSafeBrowsingEnhancedProtection,
  ItemTypeSafeBrowsingNoProtection,
};

}  // namespace

@interface PrivacySafeBrowsingMediator () <BooleanObserver,
                                           TableViewInfoButtonItemDelegate>

// Preference value for the enhanced safe browsing feature.
@property(nonatomic, strong, readonly)
    PrefBackedBoolean* safeBrowsingEnhancedProtectionPreference;

// Preference value for the standard safe browsing feature.
@property(nonatomic, strong, readonly)
    PrefBackedBoolean* safeBrowsingStandardProtectionPreference;

// All the items for the safe browsing section.
@property(nonatomic, strong, readonly) ItemArray safeBrowsingItems;

// User pref service used to check if a specific pref is managed by enterprise
// policies.
@property(nonatomic, assign, readonly) PrefService* userPrefService;

// Local pref service used to check if a specific pref is managed by enterprise
// policies.
@property(nonatomic, assign, readonly) PrefService* localPrefService;

// Boolean to check if safe browsing is controlled by enterprise.
@property(nonatomic, readonly) BOOL enterpriseEnabled;

@end

@implementation PrivacySafeBrowsingMediator

@synthesize safeBrowsingItems = _safeBrowsingItems;

- (instancetype)initWithUserPrefService:(PrefService*)userPrefService
                       localPrefService:(PrefService*)localPrefService {
  self = [super init];
  if (self) {
    DCHECK(userPrefService);
    DCHECK(localPrefService);
    _userPrefService = userPrefService;
    _localPrefService = localPrefService;
    _safeBrowsingEnhancedProtectionPreference = [[PrefBackedBoolean alloc]
        initWithPrefService:userPrefService
                   prefName:prefs::kSafeBrowsingEnhanced];
    _safeBrowsingEnhancedProtectionPreference.observer = self;
    _safeBrowsingStandardProtectionPreference = [[PrefBackedBoolean alloc]
        initWithPrefService:userPrefService
                   prefName:prefs::kSafeBrowsingEnabled];
    _safeBrowsingStandardProtectionPreference.observer = self;
  }
  return self;
}

- (void)selectSettingItem:(TableViewItem*)item {
  // If item is already selected or if we cancel turning off Safe Browsing,
  // don't do anything and keep the current selected choice.
  ItemType type = static_cast<ItemType>(item.type);
  if (item == nil || [self shouldItemTypeHaveCheckmark:type]) {
    [self updatePrivacySafeBrowsingSectionAndNotifyConsumer:YES];
    return;
  }

  // Show checkmark for selected item and update associated preference value.
  switch (type) {
    case ItemTypeSafeBrowsingEnhancedProtection:
      self.safeBrowsingEnhancedProtectionPreference.value = YES;
      self.safeBrowsingStandardProtectionPreference.value = YES;
      break;
    case ItemTypeSafeBrowsingStandardProtection:
      self.safeBrowsingStandardProtectionPreference.value = YES;
      self.safeBrowsingEnhancedProtectionPreference.value = NO;
      break;
    case ItemTypeSafeBrowsingNoProtection:
      self.safeBrowsingStandardProtectionPreference.value = NO;
      self.safeBrowsingEnhancedProtectionPreference.value = NO;
      break;
    default:
      break;
  }
  [self updatePrivacySafeBrowsingSectionAndNotifyConsumer:YES];
}

#pragma mark - Properties

- (ItemArray)safeBrowsingItems {
  if (!_safeBrowsingItems) {
    NSMutableArray* items = [NSMutableArray array];
    TableViewInfoButtonItem* safeBrowsingEnhancedProtectionItem = [self
             infoButtonItemType:ItemTypeSafeBrowsingEnhancedProtection
                        titleId:
                            IDS_IOS_PRIVACY_SAFE_BROWSING_ENHANCED_PROTECTION_TITLE
                     detailText:
                         IDS_IOS_PRIVACY_SAFE_BROWSING_ENHANCED_PROTECTION_SUMMARY
        accessibilityIdentifier:kSettingsSafeBrowsingEnhancedProtectionCellId];
    [items addObject:safeBrowsingEnhancedProtectionItem];

    TableViewInfoButtonItem* safeBrowsingStandardProtectionItem = [self
             infoButtonItemType:ItemTypeSafeBrowsingStandardProtection
                        titleId:
                            IDS_IOS_PRIVACY_SAFE_BROWSING_STANDARD_PROTECTION_TITLE
                     detailText:
                         IDS_IOS_PRIVACY_SAFE_BROWSING_STANDARD_PROTECTION_SUMMARY
        accessibilityIdentifier:kSettingsSafeBrowsingStandardProtectionCellId];
    [items addObject:safeBrowsingStandardProtectionItem];

    if (self.enterpriseEnabled) {
      TableViewInfoButtonItem* safeBrowsingNoProtectionItem = [self
               infoButtonItemType:ItemTypeSafeBrowsingNoProtection
                          titleId:
                              IDS_IOS_PRIVACY_SAFE_BROWSING_NO_PROTECTION_TITLE
                       detailText:
                           IDS_IOS_PRIVACY_SAFE_BROWSING_NO_PROTECTION_SUMMARY
          accessibilityIdentifier:kSettingsSafeBrowsingNoProtectionCellId];
      [items addObject:safeBrowsingNoProtectionItem];
    } else {
      TableViewInfoButtonItem* safeBrowsingNoProtectionItem = [self
               infoButtonItemType:ItemTypeSafeBrowsingNoProtection
                          titleId:
                              IDS_IOS_PRIVACY_SAFE_BROWSING_NO_PROTECTION_TITLE
                       detailText:
                           IDS_IOS_PRIVACY_SAFE_BROWSING_NO_PROTECTION_SUMMARY
          accessibilityIdentifier:kSettingsSafeBrowsingNoProtectionCellId];
      safeBrowsingNoProtectionItem.infoButtonIsHidden = YES;
      [items addObject:safeBrowsingNoProtectionItem];
    }
    _safeBrowsingItems = items;
  }
  return _safeBrowsingItems;
}

- (void)setConsumer:(id<PrivacySafeBrowsingConsumer>)consumer {
  if (_consumer == consumer)
    return;
  _consumer = consumer;
  [_consumer setSafeBrowsingItems:self.safeBrowsingItems];
  [_consumer setEnterpriseEnabled:self.enterpriseEnabled];
}

- (BOOL)enterpriseEnabled {
  return self.userPrefService->IsManagedPreference(prefs::kSafeBrowsingEnabled);
}

#pragma mark - Private

// Creates item with an image checkmark and an info button.
- (TableViewInfoButtonItem*)infoButtonItemType:(NSInteger)type
                                       titleId:(NSInteger)titleId
                                    detailText:(NSInteger)detailText
                       accessibilityIdentifier:
                           (NSString*)accessibilityIdentifier {
  TableViewInfoButtonItem* managedItem =
      [[TableViewInfoButtonItem alloc] initWithType:type];
  managedItem.text = l10n_util::GetNSString(titleId);
  managedItem.detailText = l10n_util::GetNSString(detailText);
  // If Safe Browsing is controlled by enterprise, make non-selected options
  // greyed out.
  if (self.enterpriseEnabled && ![self shouldItemTypeHaveCheckmark:type]) {
    managedItem.textColor =
        [[UIColor colorNamed:kTextPrimaryColor] colorWithAlphaComponent:0.4f];
    managedItem.detailTextColor =
        [[UIColor colorNamed:kTextSecondaryColor] colorWithAlphaComponent:0.4f];
    managedItem.accessibilityHint = l10n_util::GetNSString(
        IDS_IOS_TOGGLE_SETTING_MANAGED_ACCESSIBILITY_HINT);
  }
  managedItem.image =
      DefaultSymbolWithPointSize(kCheckmarkSymbol, kSymbolImagePointSize);
  managedItem.tintColor = [self shouldItemTypeHaveCheckmark:type]
                              ? [UIColor colorNamed:kBlueColor]
                              : [UIColor clearColor];
  managedItem.accessibilityIdentifier = accessibilityIdentifier;
  managedItem.accessibilityDelegate = self;

  return managedItem;
}

// Returns whether an ItemType should have a checkmark based on its related
// preference value.
- (BOOL)shouldItemTypeHaveCheckmark:(NSInteger)itemType {
  ItemType type = static_cast<ItemType>(itemType);
  if (self.safeBrowsingEnhancedProtectionPreference.value) {
    return type == ItemTypeSafeBrowsingEnhancedProtection;
  } else if (self.safeBrowsingStandardProtectionPreference.value) {
    return type == ItemTypeSafeBrowsingStandardProtection;
  }
  return type == ItemTypeSafeBrowsingNoProtection;
}

// Updates the privacy safe browsing section according to the user consent. If
// |notifyConsumer| is YES, the consumer is notified about model changes.
- (void)updatePrivacySafeBrowsingSectionAndNotifyConsumer:(BOOL)notifyConsumer {
  for (TableViewItem* item in self.safeBrowsingItems) {
    TableViewInfoButtonItem* infoButtonItem =
        base::mac::ObjCCast<TableViewInfoButtonItem>(item);
    ItemType type = static_cast<ItemType>(item.type);
    infoButtonItem.tintColor = [self shouldItemTypeHaveCheckmark:type]
                                   ? [UIColor colorNamed:kBlueColor]
                                   : [UIColor clearColor];
  }

  if (notifyConsumer) {
    [self.consumer reloadCellsForItems];
    [self selectItem];
  }
}

// Check if selected row should display enterprise popover.
- (BOOL)shouldEnterprisePopOverDisplay:(TableViewItem*)item {
  ItemType type = static_cast<ItemType>(item.type);
  return self.enterpriseEnabled && (![self shouldItemTypeHaveCheckmark:type] ||
                                    type == ItemTypeSafeBrowsingNoProtection);
}

#pragma mark - SafeBrowsingViewControllerDelegate

- (void)didSelectItem:(TableViewItem*)item {
  ItemType type = static_cast<ItemType>(item.type);
  if (type == ItemTypeSafeBrowsingEnhancedProtection) {
    if ([self shouldItemTypeHaveCheckmark:type]) {
      [self.handler showSafeBrowsingEnhancedProtection];
    } else {
      [self selectSettingItem:item];
    }
  }

  if (type == ItemTypeSafeBrowsingStandardProtection) {
    if ([self shouldItemTypeHaveCheckmark:type]) {
      [self.handler showSafeBrowsingStandardProtection];
    } else {
      [self selectSettingItem:item];
    }
  }

  if (type == ItemTypeSafeBrowsingNoProtection &&
      ![self shouldItemTypeHaveCheckmark:type]) {
    [self.handler showSafeBrowsingNoProtectionPopUp:item];
  }
}

- (void)didTapInfoButton:(UIButton*)button onItem:(TableViewItem*)item {
  if ([self shouldEnterprisePopOverDisplay:item]) {
    [self.consumer showEnterprisePopUp:button];
    return;
  }

  // Info button tap logic when not in enterprise mode.
  ItemType type = static_cast<ItemType>(item.type);
  switch (type) {
    case ItemTypeSafeBrowsingEnhancedProtection:
      [self.handler showSafeBrowsingEnhancedProtection];
      break;
    case ItemTypeSafeBrowsingStandardProtection:
      [self.handler showSafeBrowsingStandardProtection];
      break;
    default:
      NOTREACHED();
      break;
  }
}

- (void)selectItem {
  for (TableViewItem* item in self.safeBrowsingItems) {
    ItemType type = static_cast<ItemType>(item.type);
    if ([self shouldItemTypeHaveCheckmark:type]) {
      [self.consumer selectItem:item];
      break;
    }
  }
}

#pragma mark - TableViewInfoButtonItemDelegate

- (void)handleTapOutsideInfoButtonForItem:(TableViewItem*)item {
  [self didSelectItem:item];
}

#pragma mark - BooleanObserver

- (void)booleanDidChange:(id<ObservableBoolean>)observableBoolean {
  [self updatePrivacySafeBrowsingSectionAndNotifyConsumer:YES];
}

@end
