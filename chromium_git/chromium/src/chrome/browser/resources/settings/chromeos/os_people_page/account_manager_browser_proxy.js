// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// clang-format off
import {sendWithPromise} from 'chrome://resources/js/cr.m.js';
// clang-format on

/**
 * Information for an account managed by Chrome OS AccountManager.
 * @typedef {{
 *   id: string,
 *   accountType: number,
 *   isDeviceAccount: boolean,
 *   isSignedIn: boolean,
 *   unmigrated: boolean,
 *   isManaged: boolean,
 *   fullName: string,
 *   email: string,
 *   pic: string,
 *   organization: (string|undefined),
 *   isAvailableInArc: boolean,
 * }}
 */
export let Account;

/** @interface */
export class AccountManagerBrowserProxy {
  /**
   * Returns a Promise for the list of GAIA accounts held in AccountManager.
   * @return {!Promise<!Array<Account>>}
   */
  getAccounts() {}

  /**
   * Triggers the 'Add account' flow.
   */
  addAccount() {}

  /**
   * Triggers the re-authentication flow for the account pointed to by
   * |accountEmail|.
   * @param {string} accountEmail
   */
  reauthenticateAccount(accountEmail) {}

  /**
   * Triggers the migration dialog for the account pointed to by
   * |accountEmail|.
   * @param {string} accountEmail
   */
  migrateAccount(accountEmail) {}

  /**
   * Removes |account| from Account Manager.
   * @param {?Account} account
   */
  removeAccount(account) {}

  /**
   * Changes ARC availability for |account|.
   * @param {?Account} account
   * @param {?boolean} isAvailableInArc new ARC availability value
   */
  changeArcAvailability(account, isAvailableInArc) {}
}

/**
 * @implements {AccountManagerBrowserProxy}
 */
export class AccountManagerBrowserProxyImpl {
  /** @override */
  getAccounts() {
    return sendWithPromise('getAccounts');
  }

  /** @override */
  addAccount() {
    chrome.send('addAccount');
  }

  /** @override */
  reauthenticateAccount(accountEmail) {
    chrome.send('reauthenticateAccount', [accountEmail]);
  }

  /** @override */
  migrateAccount(accountEmail) {
    chrome.send('migrateAccount', [accountEmail]);
  }

  /** @override */
  removeAccount(account) {
    chrome.send('removeAccount', [account]);
  }

  /** @override */
  changeArcAvailability(account, isAvailableInArc) {
    chrome.send('changeArcAvailability', [account, isAvailableInArc]);
  }

  /** @return {!AccountManagerBrowserProxy} */
  static getInstance() {
    return instance || (instance = new AccountManagerBrowserProxyImpl());
  }

  /** @param {!AccountManagerBrowserProxy} obj */
  static setInstance(obj) {
    instance = obj;
  }
}

/** @type {?AccountManagerBrowserProxy} */
let instance = null;
