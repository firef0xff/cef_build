// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {InputController} from '/accessibility_common/dictation/input_controller.js';
import {Macro, MacroError} from '/accessibility_common/dictation/macros/macro.js';
import {MacroName} from '/accessibility_common/dictation/macros/macro_names.js';

/**
 * Macro that inputs text at the current cursor position.
 */
export class InputTextViewMacro extends Macro {
  /**
   * @param {string} text
   * @param {InputController} inputController
   * @param {MacroName=} macroName
   */
  constructor(text, inputController, macroName = MacroName.INPUT_TEXT_VIEW) {
    super(macroName);

    /** @private {string} */
    this.text_ = text;

    /** @private {InputController} */
    this.inputController_ = inputController;
  }

  /** @override */
  checkContext() {
    if (!this.inputController_.isActive()) {
      return this.createFailureCheckContextResult_(MacroError.FAILED_ACTUATION);
    }
    return this.createSuccessCheckContextResult_(
        /*willImmediatelyDisambiguate=*/ false);
  }

  /** @override */
  runMacro() {
    if (!this.inputController_.isActive()) {
      return this.createRunMacroResult_(
          /*isSuccess=*/ false, MacroError.FAILED_ACTUATION);
    }
    this.inputController_.commitText(this.text_);
    return this.createRunMacroResult_(/*isSuccess=*/ true);
  }
}

/**
 * Macro to type a new line character.
 */
export class NewLineMacro extends InputTextViewMacro {
  /**
   * @param {InputController} inputController
   */
  constructor(inputController) {
    super('\n', inputController, MacroName.NEW_LINE);
  }
}
