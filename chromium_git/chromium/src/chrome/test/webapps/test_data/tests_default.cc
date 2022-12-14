// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if BUILDFLAG(IS_WIN)
#define MAYBE_WebAppIntegration_3Chicken_1Chicken_2ChickenGreen \
  DISABLED_WebAppIntegration_3Chicken_1Chicken_2ChickenGreen
#else
#define MAYBE_WebAppIntegration_3Chicken_1Chicken_2ChickenGreen \
  WebAppIntegration_3Chicken_1Chicken_2ChickenGreen
#endif
IN_PROC_BROWSER_TEST_F(
    TestName,
    MAYBE_WebAppIntegration_3Chicken_1Chicken_2ChickenGreen) {
  // Test contents are generated by script. Please do not modify!
  // See `chrome/test/webapps/README.md` for more info.
  // Sheriffs: Disabling this test is supported.
  helper_.StateChangeA(Animal::kChicken);
  helper_.CheckA(Animal::kChicken);
  helper_.CheckB(Animal::kChicken, Color::kGreen);
}
