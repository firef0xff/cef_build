// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import './menu_container.js';
import './page_favicon.js';
import './shared_style.js';
import '../../cr_elements/cr_action_menu/cr_action_menu.js';
import '../../cr_elements/cr_lazy_render/cr_lazy_render.m.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {loadTimeData} from '../../js/load_time_data.m.js';

import {Annotation, URLVisit} from './history_clusters.mojom-webui.js';
import {OpenWindowProxyImpl} from './open_window_proxy.js';
import {getTemplate} from './url_visit.html.js';
import {insertHighlightedTextWithMatchesIntoElement} from './utils.js';

/**
 * @fileoverview This file provides a custom element displaying a visit to a
 * page within a cluster. A visit features the page favicon, title, a timestamp,
 * as well as an action menu.
 */

/**
 * Maps supported annotations to localized string identifiers.
 */
const annotationToStringId: Map<number, string> = new Map([
  [Annotation.kBookmarked, 'bookmarked'],
  [Annotation.kTabGrouped, 'savedInTabGroup'],
]);

declare global {
  interface HTMLElementTagNameMap {
    'url-visit': VisitRowElement;
  }
}

interface VisitRowElement {
  $: {
    title: HTMLElement,
    url: HTMLElement,
  };
}

class VisitRowElement extends PolymerElement {
  static get is() {
    return 'url-visit';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      /**
       * Whether this is a top visit.
       */
      isTopVisit: {
        type: Boolean,
        reflectToAttribute: true,
        value: false,
      },

      /**
       * The current query for which related clusters are requested and shown.
       */
      query: String,

      /**
       * The visit to display.
       */
      visit: Object,

      /**
       * Annotations to show for the visit (e.g., whether page was bookmarked).
       */
      annotations_: {
        type: Object,
        computed: 'computeAnnotations_(visit)',
      },

      /**
       * Debug info for the visit.
       */
      debugInfo_: {
        type: String,
        computed: 'computeDebugInfo_(visit)',
      },

      /**
       * Page title for the visit. This property is actually unused. The side
       * effect of the compute function is used to insert the HTML elements for
       * highlighting into this.$.title element.
       */
      unusedTitle_: {
        type: String,
        computed: 'computeTitle_(visit)',
      },

      /**
       * This property is actually unused. The side effect of the compute
       * function is used to insert HTML elements for the highlighted
       * `this.visit.urlForDisplay` URL into the `this.$.url` element.
       */
      unusedUrlForDisplay_: {
        type: String,
        computed: 'computeUrlForDisplay_(visit)',
      }
    };
  }

  //============================================================================
  // Properties
  //============================================================================

  isTopVisit: boolean;
  query: string;
  visit: URLVisit;
  private annotations_: Array<string>;
  private debugInfo_: string;
  private unusedTitle_: string;
  private unusedVisibleUrl_: string;

  //============================================================================
  // Event handlers
  //============================================================================

  private onAuxClick_() {
    // Notify the parent <history-cluster> element of this event.
    this.dispatchEvent(new CustomEvent('visit-clicked', {
      bubbles: true,
      composed: true,
      detail: this.visit,
    }));
  }

  private onClick_(event: MouseEvent) {
    // Ignore previously handled events.
    if (event.defaultPrevented) {
      return;
    }

    event.preventDefault();  // Prevent default browser action (navigation).

    // To record metrics.
    this.onAuxClick_();

    OpenWindowProxyImpl.getInstance().open(this.visit.normalizedUrl.url);
  }

  private onKeydown_(e: KeyboardEvent) {
    // To be consistent with <history-list>, only handle Enter, and not Space.
    if (e.key !== 'Enter') {
      return;
    }

    // To record metrics.
    this.onAuxClick_();

    OpenWindowProxyImpl.getInstance().open(this.visit.normalizedUrl.url);
  }

  //============================================================================
  // Helper methods
  //============================================================================

  private computeAnnotations_(): Array<string> {
    return this.visit.annotations
        .map((annotation: number) => annotationToStringId.get(annotation))
        .filter(
            (id: string|undefined):
                id is string => {
                  return !!id;
                })
        .map((id: string) => loadTimeData.getString(id));
  }

  private computeDebugInfo_(): string {
    if (!loadTimeData.getBoolean('isHistoryClustersDebug')) {
      return '';
    }

    return JSON.stringify(this.visit.debugInfo);
  }

  private computeTitle_(_visit: URLVisit): string {
    insertHighlightedTextWithMatchesIntoElement(
        this.$.title, this.visit.pageTitle, this.visit.titleMatchPositions);
    return this.visit.pageTitle;
  }

  private computeUrlForDisplay_(_visit: URLVisit): string {
    insertHighlightedTextWithMatchesIntoElement(
        this.$.url, this.visit.urlForDisplay,
        this.visit.urlForDisplayMatchPositions);
    return this.visit.urlForDisplay;
  }
}

customElements.define(VisitRowElement.is, VisitRowElement);
