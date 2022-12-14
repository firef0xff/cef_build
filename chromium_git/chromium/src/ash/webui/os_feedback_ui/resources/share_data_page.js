// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import './os_feedback_shared_css.js';
import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';

import {html, PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {FeedbackFlowState} from './feedback_flow.js';
import {FeedbackContext, Report} from './feedback_types.js';

/**
 * @fileoverview
 * 'share-data-page' is the second page of the feedback tool. It allows users to
 * choose what data to send with the feedback report.
 */
export class ShareDataPageElement extends PolymerElement {
  static get is() {
    return 'share-data-page';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      feedbackContext: {type: FeedbackContext, readOnly: false, notify: true},
      screenshotUrl: {type: String, readOnly: false, notify: true},
    };
  }

  constructor() {
    super();

    /**
     * @type {!FeedbackContext}
     */
    this.feedbackContext;

    /**
     * @type {string}
     */
    this.screenshotUrl;
  }

  /**
   * @return {boolean}
   * @protected
   */
  hasEmail_() {
    return (this.feedbackContext !== null && !!this.feedbackContext.email);
  }

  /**
   * @return {boolean}
   * @protected
   */
  hasScreenshot_() {
    return !!this.screenshotUrl;
  }

  /**
   * @param {!Event} e
   * @protected
   */
  handleBackButtonClicked_(e) {
    e.stopPropagation();

    this.dispatchEvent(new CustomEvent('go-back-click', {
      composed: true,
      bubbles: true,
      detail: {currentState: FeedbackFlowState.SHARE_DATA}
    }));
  }

  /**
   * @param {!Event} e
   * @protected
   */
  handleSendButtonClicked_(e) {
    e.stopPropagation();
    // TODO(xiangdongkong): Disable the button to prevent sending the same
    // report again.
    this.dispatchEvent(new CustomEvent('continue-click', {
      composed: true,
      bubbles: true,
      detail: {
        currentState: FeedbackFlowState.SHARE_DATA,
        report: this.createReport_()
      }
    }));
  }

  /**
   * @param {string} selector
   * @return {Element}
   * @private
   */
  getElement_(selector) {
    return this.shadowRoot.querySelector(selector);
  }

  /**
   * @return {!{
   *  feedbackContext: FeedbackContext,
   *  includeSystemLogsAndHistograms: boolean,
   *  includeScreenshot: boolean,
   * }}
   * @private
   */
  createReport_() {
    const report = {
      feedbackContext: {},
      includeSystemLogsAndHistograms:
          this.getElement_('#sysInfoCheckbox').checked,
      includeScreenshot: this.getElement_('#screenshotCheckbox').checked &&
          !!this.getElement_('#screenshotImage').src
    };

    const email = this.getElement_('#userEmailDropDown').value;
    if (email) {
      report.feedbackContext.email = email;
    }

    if (this.getElement_('#pageUrlCheckbox').checked) {
      report.feedbackContext.pageUrl = {
        url: this.getElement_('#pageUrlText').value
      };
    }
    return report;
  }
}

customElements.define(ShareDataPageElement.is, ShareDataPageElement);
