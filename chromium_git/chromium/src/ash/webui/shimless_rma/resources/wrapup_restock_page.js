// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://resources/cr_elements/icons.m.js';
import 'chrome://resources/polymer/v3_0/iron-icon/iron-icon.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import './shimless_rma_shared_css.js';
import './base_page.js';
import './icons.js';

import {I18nBehavior, I18nBehaviorInterface} from 'chrome://resources/js/i18n_behavior.m.js';
import {html, mixinBehaviors, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {getShimlessRmaService} from './mojo_interface_provider.js';
import {ShimlessRmaServiceInterface, StateResult} from './shimless_rma_types.js';
import {enableNextButton, executeThenTransitionState} from './shimless_rma_util.js';

/**
 * @fileoverview
 * 'wrapup-restock-page' is the page that informs the repair technician they
 * can shut down the device and restock the mainboard or continue to finalize
 * the repair if the board is being used to repair another device.
 */

/**
 * @constructor
 * @extends {PolymerElement}
 * @implements {I18nBehaviorInterface}
 */
const WrapupRestockPageBase = mixinBehaviors([I18nBehavior], PolymerElement);

/** @polymer */
export class WrapupRestockPage extends WrapupRestockPageBase {
  static get is() {
    return 'wrapup-restock-page';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      /**
       * Set by shimless_rma.js.
       * @type {boolean}
       */
      allButtonsDisabled: Boolean,
    };
  }

  constructor() {
    super();
    /** @private {ShimlessRmaServiceInterface} */
    this.shimlessRmaService_ = getShimlessRmaService();
  }

  /** @override */
  ready() {
    super.ready();
  }

  /** @protected */
  onShutdownButtonClicked_(e) {
    e.preventDefault();
    const dialog = /** @type {!CrDialogElement} */ (
        this.shadowRoot.querySelector('#powerwashDialog'));
    if (!dialog.open) {
      dialog.showModal();
    }
  }

  /** @protected */
  onCancelClick_() {
    const dialog = /** @type {!CrDialogElement} */ (
        this.shadowRoot.querySelector('#powerwashDialog'));
    dialog.close();
  }

  /** @protected */
  onPowerwashButtonClick_(e) {
    e.preventDefault();
    const dialog = /** @type {!CrDialogElement} */ (
      this.shadowRoot.querySelector('#powerwashDialog'));
    dialog.close();
    executeThenTransitionState(
        this, () => this.shimlessRmaService_.shutdownForRestock());
  }

  /** @protected */
  onRestockContinueButtonClicked_() {
    executeThenTransitionState(
        this,
        () => this.shimlessRmaService_.continueFinalizationAfterRestock());
  }
}

customElements.define(WrapupRestockPage.is, WrapupRestockPage);
