// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <vector>

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/fake/fake_display_snapshot.h"
#include "ui/display/manager/configure_displays_task.h"
#include "ui/display/manager/test/action_logger_util.h"
#include "ui/display/manager/test/test_native_display_delegate.h"
#include "ui/display/types/display_constants.h"
#include "ui/display/types/display_mode.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"

namespace display {
namespace test {

namespace {

// Non-zero generic connector IDs.
constexpr uint64_t kEdpConnectorId = 71u;
constexpr uint64_t kSecondConnectorId = kEdpConnectorId + 10u;
constexpr uint64_t kThirdConnectorId = kEdpConnectorId + 20u;

// Invalid PATH topology parse connector ID.
constexpr uint64_t kInvalidConnectorId = 0u;

std::string GetDisableCrtcAction(
    const std::unique_ptr<DisplaySnapshot>& display) {
  return GetCrtcAction({display->display_id(), gfx::Point(), nullptr});
}

class ConfigureDisplaysTaskTest : public testing::Test {
 public:
  ConfigureDisplaysTaskTest()
      : delegate_(&log_),
        small_mode_60hz_(gfx::Size(1366, 768), false, 60.0f),
        small_mode_30hz_(gfx::Size(1366, 768), false, 30.0f),
        medium_mode_60hz_(gfx::Size(1920, 1080), false, 60.0f),
        medium_mode_29_98hz_(gfx::Size(1920, 1080), false, 29.98f),
        big_mode_60hz_(gfx::Size(2560, 1600), false, 60.0f),
        big_mode_29_97hz_(gfx::Size(2560, 1600), false, 29.97f) {}

  ConfigureDisplaysTaskTest(const ConfigureDisplaysTaskTest&) = delete;
  ConfigureDisplaysTaskTest& operator=(const ConfigureDisplaysTaskTest&) =
      delete;

  ~ConfigureDisplaysTaskTest() override = default;

  void SetUp() override {
    displays_.push_back(FakeDisplaySnapshot::Builder()
                            .SetId(123)
                            .SetNativeMode(medium_mode_60hz_.Clone())
                            .SetCurrentMode(medium_mode_60hz_.Clone())
                            .AddMode(medium_mode_29_98hz_.Clone())
                            .AddMode(small_mode_60hz_.Clone())
                            .AddMode(small_mode_30hz_.Clone())
                            .SetType(DISPLAY_CONNECTION_TYPE_INTERNAL)
                            .SetBaseConnectorId(kEdpConnectorId)
                            .Build());
    displays_.push_back(FakeDisplaySnapshot::Builder()
                            .SetId(456)
                            .SetNativeMode(big_mode_60hz_.Clone())
                            .SetCurrentMode(big_mode_60hz_.Clone())
                            .AddMode(big_mode_29_97hz_.Clone())
                            .AddMode(small_mode_60hz_.Clone())
                            .AddMode(small_mode_30hz_.Clone())
                            .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                            .SetBaseConnectorId(kSecondConnectorId)
                            .Build());
  }

  void ConfigureCallback(ConfigureDisplaysTask::Status status) {
    callback_called_ = true;
    status_ = status;
  }

 protected:
  base::test::SingleThreadTaskEnvironment task_environment_;
  ActionLogger log_;
  TestNativeDisplayDelegate delegate_;

  bool callback_called_ = false;
  ConfigureDisplaysTask::Status status_ = ConfigureDisplaysTask::ERROR;

  const DisplayMode small_mode_60hz_;
  const DisplayMode small_mode_30hz_;
  const DisplayMode medium_mode_60hz_;
  const DisplayMode medium_mode_29_98hz_;
  const DisplayMode big_mode_60hz_;
  const DisplayMode big_mode_29_97hz_;
  std::vector<std::unique_ptr<DisplaySnapshot>> displays_;
};

}  // namespace

/**************************************************
 * Cases that report ConfigureDisplaysTask::SUCCESS
 **************************************************/

TEST_F(ConfigureDisplaysTaskTest, ConfigureInternalDisplay) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  std::vector<DisplayConfigureRequest> requests(
      1, DisplayConfigureRequest(displays_[0].get(),
                                 displays_[0]->native_mode(), gfx::Point()));
  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::SUCCESS, status_);
  EXPECT_EQ(GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                           displays_[0]->native_mode()}),
            log_.GetActionsAndClear());
}

// Tests that and an internal + one external display pass modeset. Note that
// this case covers an external display connected via MST as well.
TEST_F(ConfigureDisplaysTaskTest, ConfigureInternalAndOneExternalDisplays) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::SUCCESS, status_);
  EXPECT_EQ(JoinActions(GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                                       displays_[0]->native_mode()})
                            .c_str(),
                        GetCrtcAction({displays_[1]->display_id(), gfx::Point(),
                                       &big_mode_60hz_})
                            .c_str(),
                        nullptr),
            log_.GetActionsAndClear());
}

// Tests that one external display (with no internal display present;
// e.g. chromebox) pass modeset. Note that this case covers an external display
// connected via MST as well.
TEST_F(ConfigureDisplaysTaskTest, ConfigureOneExternalDisplay) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  std::vector<DisplayConfigureRequest> requests(
      1, DisplayConfigureRequest(displays_[1].get(),
                                 displays_[1]->native_mode(), gfx::Point()));
  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::SUCCESS, status_);
  EXPECT_EQ(GetCrtcAction({displays_[1]->display_id(), gfx::Point(),
                           displays_[1]->native_mode()}),
            log_.GetActionsAndClear());
}

// Tests that two external MST displays (with no internal display present; e.g.
// chromebox) pass modeset.
TEST_F(ConfigureDisplaysTaskTest, ConfigureTwoMstDisplays) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  // Two displays sharing the same base connector via MST.
  displays_[0] = FakeDisplaySnapshot::Builder()
                     .SetId(456)
                     .SetNativeMode(big_mode_60hz_.Clone())
                     .SetCurrentMode(big_mode_60hz_.Clone())
                     .AddMode(small_mode_60hz_.Clone())
                     .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                     .SetBaseConnectorId(kSecondConnectorId)
                     .Build();
  displays_[1] = FakeDisplaySnapshot::Builder()
                     .SetId(789)
                     .SetNativeMode(big_mode_60hz_.Clone())
                     .SetCurrentMode(big_mode_60hz_.Clone())
                     .AddMode(small_mode_60hz_.Clone())
                     .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                     .SetBaseConnectorId(kSecondConnectorId)
                     .Build();

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::SUCCESS, status_);
  EXPECT_EQ(JoinActions(GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                                       &big_mode_60hz_})
                            .c_str(),
                        GetCrtcAction({displays_[1]->display_id(), gfx::Point(),
                                       &big_mode_60hz_})
                            .c_str(),
                        nullptr),
            log_.GetActionsAndClear());
}

// Although most devices do not support more than three displays in total
// (including the internal display), this tests that this configuration can pass
// all displays in a single request.
TEST_F(ConfigureDisplaysTaskTest, ConfigureInternalAndTwoMstAndHdmiDisplays) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  // Add an additional display to base connector kSecondConnectorId via MST.
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(789)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(small_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kSecondConnectorId)
                          .Build());

  // Additional independent HDMI display (has its own connector).
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(101112)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(medium_mode_60hz_.Clone())
                          .AddMode(small_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
                          .SetBaseConnectorId(kThirdConnectorId)
                          .Build());

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::SUCCESS, status_);
  EXPECT_EQ(JoinActions(GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                                       displays_[0]->native_mode()})
                            .c_str(),
                        GetCrtcAction({displays_[1]->display_id(), gfx::Point(),
                                       &big_mode_60hz_})
                            .c_str(),
                        GetCrtcAction({displays_[2]->display_id(), gfx::Point(),
                                       &big_mode_60hz_})
                            .c_str(),
                        GetCrtcAction({displays_[3]->display_id(), gfx::Point(),
                                       &big_mode_60hz_})
                            .c_str(),
                        nullptr),
            log_.GetActionsAndClear());
}

/************************************************
 * Cases that report ConfigureDisplaysTask::ERROR
 ************************************************/

TEST_F(ConfigureDisplaysTaskTest, DisableInternalDisplayFails) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  // Force a failed configuration.
  delegate_.set_max_configurable_pixels(-1);

  std::vector<DisplayConfigureRequest> requests(
      1, DisplayConfigureRequest(displays_[0].get(), nullptr, gfx::Point()));
  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(JoinActions(
                // Initial modeset fails. Initiate retry logic.
                GetDisableCrtcAction(displays_[0]).c_str(),
                GetDisableCrtcAction(displays_[0]).c_str(),
                // There is no way to downgrade a disable request. Configuration
                // fails.
                GetDisableCrtcAction(displays_[0]).c_str(), nullptr),
            log_.GetActionsAndClear());
}

// Tests that the internal display does not attempt to fallback to alternative
// modes upon failure to modeset with preferred mode.
TEST_F(ConfigureDisplaysTaskTest, NoModeChangeAttemptWhenInternalDisplayFails) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  delegate_.set_max_configurable_pixels(1);

  std::vector<DisplayConfigureRequest> requests(
      1, DisplayConfigureRequest(displays_[0].get(),
                                 displays_[0]->native_mode(), gfx::Point()));
  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(
      JoinActions(
          // Initial modeset fails. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          // Retry logic fails to modeset internal display. Since internal
          // displays are restricted to their preferred mode, there are no
          // other modes to try. The configuration fails completely.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that an external display (with no internal display present; e.g.
// chromebox) attempts to fallback to alternative modes upon failure to modeset
// to the original request before completely failing. Note that this case
// applies to a single external display over MST as well.
TEST_F(ConfigureDisplaysTaskTest, ConfigureOneExternalNoInternalDisplayFails) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  delegate_.set_max_configurable_pixels(1);

  std::vector<DisplayConfigureRequest> requests(
      1, DisplayConfigureRequest(displays_[1].get(), &big_mode_60hz_,
                                 gfx::Point()));
  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(
      JoinActions(
          // Initial modeset fails. Initiate retry logic.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[1]).c_str(),
          // External display will fail, downgrade twice, and fail completely.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_30hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that two external non-MST displays (with no internal display present;
// e.g. chromebox) attempt to fallback to alternative modes upon failure to
// modeset to the original request before completely failing.
TEST_F(ConfigureDisplaysTaskTest, ConfigureTwoNoneMstDisplaysNoInternalFail) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  displays_[0] = FakeDisplaySnapshot::Builder()
                     .SetId(456)
                     .SetNativeMode(big_mode_60hz_.Clone())
                     .SetCurrentMode(big_mode_60hz_.Clone())
                     .AddMode(small_mode_60hz_.Clone())
                     .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                     .SetBaseConnectorId(kSecondConnectorId)
                     .Build();
  displays_[1] = FakeDisplaySnapshot::Builder()
                     .SetId(789)
                     .SetNativeMode(big_mode_60hz_.Clone())
                     .SetCurrentMode(big_mode_60hz_.Clone())
                     .AddMode(medium_mode_60hz_.Clone())
                     .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                     .SetBaseConnectorId(kThirdConnectorId)
                     .Build();

  delegate_.set_max_configurable_pixels(small_mode_60hz_.size().GetArea());

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction(
              {displays_[0]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          // |displays_[0]| will fail, downgrade once, and pass.
          GetCrtcAction(
              {displays_[0]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[0]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          // |displays_[1]| will fail, downgrade once, and fail completely.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that two external MST displays (with no internal display present; e.g.
// chromebox) attempt to fallback to alternative modes upon failure to modeset
// to the original request before completely failing.
TEST_F(ConfigureDisplaysTaskTest, ConfigureTwoMstDisplaysNoInternalFail) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  // Two displays sharing the same base connector.
  displays_[0] = FakeDisplaySnapshot::Builder()
                     .SetId(456)
                     .SetNativeMode(big_mode_60hz_.Clone())
                     .SetCurrentMode(big_mode_60hz_.Clone())
                     .AddMode(small_mode_60hz_.Clone())
                     .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                     .SetBaseConnectorId(kSecondConnectorId)
                     .Build();
  displays_[1] = FakeDisplaySnapshot::Builder()
                     .SetId(789)
                     .SetNativeMode(big_mode_60hz_.Clone())
                     .SetCurrentMode(big_mode_60hz_.Clone())
                     .AddMode(medium_mode_60hz_.Clone())
                     .AddMode(small_mode_60hz_.Clone())
                     .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                     .SetBaseConnectorId(kSecondConnectorId)
                     .Build();

  delegate_.set_max_configurable_pixels(1);

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction(
              {displays_[0]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          // MST displays will be tested (and fail) together.
          GetCrtcAction(
              {displays_[0]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // |displays_[0]| will downgrade first. Configuration will fail.
          GetCrtcAction(
              {displays_[0]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // |displays_[1] will downgrade next. Configuration still fails.
          GetCrtcAction(
              {displays_[0]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          // Since |displays_[1]| is still the largest and has one more mode, it
          // downgrades again. Configuration fails completely.
          GetCrtcAction(
              {displays_[0]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that the internal display does not attempt to fallback to alternative
// modes upon failure to modeset with preferred mode while an external display
// is present. Note that this case applies for an internal + a single external
// display over MST as well.
TEST_F(ConfigureDisplaysTaskTest,
       ConfigureInternalAndOneExternalDisplaysFailsDueToInternal) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  delegate_.set_max_configurable_pixels(1);

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          // Retry logic fails to modeset internal display. Since internal
          // displays are restricted to their preferred mode, there are no other
          // modes to try. The configuration will fail completely, but the
          // external display will attempt to modeset as well.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // External display will fail, downgrade twice, and fail again.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_30hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that an external display attempts to fallback to alternative modes upon
// failure to modeset to the original request after the internal display modeset
// successfully. Note that this case applies for an internal + a single MST
// display as well.
TEST_F(ConfigureDisplaysTaskTest,
       ConfigureInternalAndOneExternalDisplaysFailsDueToExternal) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  displays_[0] = FakeDisplaySnapshot::Builder()
                     .SetId(123)
                     .SetNativeMode(small_mode_60hz_.Clone())
                     .SetCurrentMode(small_mode_60hz_.Clone())
                     .SetType(DISPLAY_CONNECTION_TYPE_INTERNAL)
                     .SetBaseConnectorId(kEdpConnectorId)
                     .Build();
  displays_[1] = FakeDisplaySnapshot::Builder()
                     .SetId(456)
                     .SetNativeMode(big_mode_60hz_.Clone())
                     .SetCurrentMode(big_mode_60hz_.Clone())
                     .AddMode(medium_mode_60hz_.Clone())
                     .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                     .SetBaseConnectorId(kSecondConnectorId)
                     .Build();

  delegate_.set_max_configurable_pixels(small_mode_60hz_.size().GetArea());

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          // Internal display will succeed to modeset.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // External display fails, downgrades once, and fails completely.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that the internal display does not attempt to fallback to alternative
// modes upon failure to modeset with preferred mode while two external MST
// displays are present.
TEST_F(ConfigureDisplaysTaskTest,
       ConfigureInternalAndTwoMstExternalDisplaysFailsDueToInternal) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  // Add an additional display to base connector kSecondConnectorId via MST.
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(789)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(small_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kSecondConnectorId)
                          .Build());

  delegate_.set_max_configurable_pixels(1);

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          GetDisableCrtcAction(displays_[2]).c_str(),
          // Retry logic fails to modeset internal display. Since internal
          // displays are restricted to their preferred mode, there are no other
          // modes to try. The configuration will fail completely. The external
          // displays will attempt to modeset next.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // MST displays will be tested (and fail) together.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // |displays_[1] & displays_[2] will cycle through all available
          // modes, but configuration will eventually completely fail.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_30hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that two external MST displays attempt to fallback to alternative modes
// upon failure to modeset to the original request after the internal display
// succeeded to modeset
TEST_F(ConfigureDisplaysTaskTest,
       ConfigureInternalAndTwoMstExternalDisplaysFailsDueToExternals) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  displays_[0] = FakeDisplaySnapshot::Builder()
                     .SetId(123)
                     .SetNativeMode(small_mode_60hz_.Clone())
                     .SetCurrentMode(small_mode_60hz_.Clone())
                     .SetType(DISPLAY_CONNECTION_TYPE_INTERNAL)
                     .SetBaseConnectorId(kEdpConnectorId)
                     .Build();
  // Two MST displays sharing the same base connector.
  displays_[1] = FakeDisplaySnapshot::Builder()
                     .SetId(456)
                     .SetNativeMode(big_mode_60hz_.Clone())
                     .SetCurrentMode(big_mode_60hz_.Clone())
                     .AddMode(small_mode_60hz_.Clone())
                     .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                     .SetBaseConnectorId(kSecondConnectorId)
                     .Build();
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(789)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(medium_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kSecondConnectorId)
                          .Build());

  delegate_.set_max_configurable_pixels(small_mode_60hz_.size().GetArea());

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          GetDisableCrtcAction(displays_[2]).c_str(),
          // Internal display will succeed to modeset.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // MST displays will be tested (and fail) together.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // |displays_[1]| will downgrade first. Configuration will fail.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // |displays_[2] will downgrade next and configuration fails
          // completely.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that the internal display does not attempt to fallback to alternative
// modes upon failure to modeset with preferred mode while two MST and one HDMI
// displays are present.
TEST_F(ConfigureDisplaysTaskTest,
       ConfigureInternalAndTwoMstAndHdmiDisplaysFailsDueToInternal) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  // Add an additional display to kSecondConnectorId (via MST).
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(789)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(small_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kSecondConnectorId)
                          .Build());

  // Additional independent HDMI display (has its own connector).
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(101112)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(big_mode_29_97hz_.Clone())
                          .AddMode(medium_mode_60hz_.Clone())
                          .AddMode(medium_mode_29_98hz_.Clone())
                          .AddMode(small_mode_60hz_.Clone())
                          .AddMode(small_mode_30hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
                          .SetBaseConnectorId(kThirdConnectorId)
                          .Build());

  delegate_.set_max_configurable_pixels(1);

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          GetDisableCrtcAction(displays_[2]).c_str(),
          GetDisableCrtcAction(displays_[3]).c_str(),
          // Retry logic fails to modeset internal display. Since internal
          // displays are restricted to their preferred mode, there are no other
          // modes to try. The configuration will fail completely, but the
          // external displays will still attempt to configure.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // MST displays_[1,2] will be tested (and fail) together. displays_[1]
          // downgrades first and fails, Then displays_[2], and the process will
          // repeat once more before the group fails to modeset.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_30hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          // Finally, HDMI display will attempt to modeset and cycle through its
          // six available modes.
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &medium_mode_29_98hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &small_mode_30hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that two external MST displays attempt to fallback to alternative modes
// upon failure to modeset to the original request after the internal display
// succeeded to modeset.
TEST_F(ConfigureDisplaysTaskTest,
       ConfigureInternalAndTwoMstAndHdmiDisplaysFailsDueToMst) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  // Add an additional display to kSecondConnectorId (via MST).
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(789)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kSecondConnectorId)
                          .Build());

  // Additional independent HDMI display (has its own connector).
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(101112)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(big_mode_29_97hz_.Clone())
                          .AddMode(medium_mode_60hz_.Clone())
                          .AddMode(medium_mode_29_98hz_.Clone())
                          .AddMode(small_mode_60hz_.Clone())
                          .AddMode(small_mode_30hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
                          .SetBaseConnectorId(kThirdConnectorId)
                          .Build());

  delegate_.set_max_configurable_pixels(medium_mode_60hz_.size().GetArea());

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          GetDisableCrtcAction(displays_[2]).c_str(),
          GetDisableCrtcAction(displays_[3]).c_str(),
          // Internal display will succeed to modeset.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // MST displays will be tested (and fail) together. displays_[1]
          // downgrade first. Modeset fails.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // displays_[2] downgrade next, but there are no other modes available
          // for displays_[2], so configuration fails completely for the MST
          // group. The HDMI display will attempt to modeset nest.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_30hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // HDMI display attempts to modeset, fails, downgrades twice, and
          // passes modeset.
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that the HDMI display attempts to fallback to alternative modes upon
// failure to modeset to the original request after the internal and two MST
// displays succeeded to modeset.
TEST_F(ConfigureDisplaysTaskTest,
       ConfigureInternalAndTwoMstAndHdmiDisplaysFailsDueToHDMI) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  // Two displays sharing the same base connector.
  displays_[1] = FakeDisplaySnapshot::Builder()
                     .SetId(456)
                     .SetNativeMode(medium_mode_60hz_.Clone())
                     .SetCurrentMode(medium_mode_60hz_.Clone())
                     .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                     .SetBaseConnectorId(kSecondConnectorId)
                     .Build();
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(789)
                          .SetNativeMode(medium_mode_60hz_.Clone())
                          .SetCurrentMode(medium_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kSecondConnectorId)
                          .Build());

  // Additional independent HDMI display (has its own connector).
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(101112)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
                          .SetBaseConnectorId(kThirdConnectorId)
                          .Build());

  delegate_.set_max_configurable_pixels(medium_mode_60hz_.size().GetArea());

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          GetDisableCrtcAction(displays_[2]).c_str(),
          GetDisableCrtcAction(displays_[3]).c_str(),
          // Internal display will succeed modeset.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // MST displays will be tested and pass together.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          // HDMI display will fail modeset, but since there are no other modes
          // available for fallback configuration fails completely.
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that two external displays that share a bad MST hub are tested and fail
// together, since they are grouped under kInvalidConnectorId. Also test that
// this does not affect the internal display's ability configured separately
// during retry and passes modeset.
TEST_F(ConfigureDisplaysTaskTest,
       ConfigureInternalAndOneBadMstHubWithTwoDisplaysFails) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  // Two displays sharing a bad MST Hub that did not report its PATH topology
  // correctly.
  displays_[1] = FakeDisplaySnapshot::Builder()
                     .SetId(456)
                     .SetNativeMode(big_mode_60hz_.Clone())
                     .SetCurrentMode(big_mode_60hz_.Clone())
                     .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                     .SetBaseConnectorId(kInvalidConnectorId)
                     .Build();
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(789)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kInvalidConnectorId)
                          .Build());

  delegate_.set_max_configurable_pixels(medium_mode_60hz_.size().GetArea());

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          GetDisableCrtcAction(displays_[2]).c_str(),
          // Internal display will succeed modeset.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // displays_[1] and displays_[2] will be tested and fail together
          // under connector kInvalidConnectorId. Since neither expose any
          // alternative modes to try, configuration completely fails.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that four external displays that share two separate bad MST hubs are
// tested and fail together, since they are grouped under kInvalidConnectorId.
// Also test that this does not affect the internal display's ability configured
// separately during retry and passes modeset.
TEST_F(ConfigureDisplaysTaskTest,
       ConfigureInternalAndTwoBadMstHubsWithFourDisplaysFails) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  // Four displays sharing two bad MST Hubs that did not report their PATH
  // topology correctly. First two:
  displays_[1] = FakeDisplaySnapshot::Builder()
                     .SetId(456)
                     .SetNativeMode(big_mode_60hz_.Clone())
                     .SetCurrentMode(big_mode_60hz_.Clone())
                     .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                     .SetBaseConnectorId(kInvalidConnectorId)
                     .Build();
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(789)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(medium_mode_60hz_.Clone())
                          .AddMode(small_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kInvalidConnectorId)
                          .Build());

  // Last two:
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(101112)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(medium_mode_60hz_.Clone())
                          .AddMode(small_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kInvalidConnectorId)
                          .Build());
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(131415)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kInvalidConnectorId)
                          .Build());

  delegate_.set_max_configurable_pixels(medium_mode_60hz_.size().GetArea());

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::ERROR, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[4]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          GetDisableCrtcAction(displays_[2]).c_str(),
          GetDisableCrtcAction(displays_[3]).c_str(),
          GetDisableCrtcAction(displays_[4]).c_str(),
          // Internal display will succeed modeset.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // displays_[1-4] will be tested and downgraded as a group, since they
          // share kInvalidConnectorId due to bad MST hubs.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[4]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // displays_[2] will downgrade first, since it is the next largest
          // display with available alternative modes.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[4]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // displays_[3] will downgrade next, and fail.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[4]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Same downgrade process as above will repeat for displays_[2] and
          // displays_[3] before failing completely.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[4]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[4]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

/**********************************************************
 * Cases that report ConfigureDisplaysTask::PARTIAL_SUCCESS
 **********************************************************/

// Tests that the last display (in order of available displays) attempts and
// succeeds to fallback after it fails to modeset the initial request.
TEST_F(ConfigureDisplaysTaskTest, ConfigureLastDisplayPartialSuccess) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  delegate_.set_max_configurable_pixels(medium_mode_60hz_.size().GetArea());

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::PARTIAL_SUCCESS, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          // Internal display will succeed to modeset.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // Last display will fail, downgrade twice, and pass.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that the second display (in order of available displays) attempts and
// succeeds to fallback after it fails to modeset the initial request.
TEST_F(ConfigureDisplaysTaskTest, ConfigureMiddleDisplayPartialSuccess) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(789)
                          .SetNativeMode(small_mode_60hz_.Clone())
                          .SetCurrentMode(small_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
                          .SetBaseConnectorId(kThirdConnectorId)
                          .Build());

  delegate_.set_max_configurable_pixels(medium_mode_60hz_.size().GetArea());

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::PARTIAL_SUCCESS, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          GetDisableCrtcAction(displays_[2]).c_str(),
          // Internal display will succeed to modeset.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // Second display will fail once, downgrade, and pass.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          // Third external display will succeed to modeset on first attempt.
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that both MST displays fail initial configuration and are tested,
// downgraded, and eventually pass modeset as a group and separately from the
// internal display.
TEST_F(ConfigureDisplaysTaskTest, ConfigureTwoMstDisplaysPartialSuccess) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  // Add an additional display to the base connector kSecondConnectorId via MST.
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(789)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(small_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kSecondConnectorId)
                          .Build());

  delegate_.set_max_configurable_pixels(medium_mode_60hz_.size().GetArea());

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::PARTIAL_SUCCESS, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          GetDisableCrtcAction(displays_[2]).c_str(),
          // Internal display will succeed modeset.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // MST displays will be tested (and fail) together.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // |displays_[1]| will downgrade first. Configuration will fail.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // |displays_[2] will downgrade next. Configuration will fail.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          // |displays_[1]| will downgrade again. Configuration succeeds.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests that the two MST displays, and then the HDMI display fail initial
// configuration, are tested, downgraded, and eventually pass modeset as
// separate groups.
TEST_F(ConfigureDisplaysTaskTest,
       ConfigureInternalAndTwoMstAndHdmiDisplaysPartialSuccess) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  // Add an additional display to the base connector kSecondConnectorId via MST.
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(789)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(small_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kSecondConnectorId)
                          .Build());

  // Additional independent HDMI display (has its own connector).
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(101112)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(medium_mode_60hz_.Clone())
                          .AddMode(small_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
                          .SetBaseConnectorId(kThirdConnectorId)
                          .Build());

  delegate_.set_max_configurable_pixels(medium_mode_60hz_.size().GetArea());

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::PARTIAL_SUCCESS, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together. Initiate retry logic.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          GetDisableCrtcAction(displays_[2]).c_str(),
          GetDisableCrtcAction(displays_[3]).c_str(),
          // Internal display will succeed modeset.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // Both MST displays will be tested (and fail) together.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // |displays_[1]| will downgrade first. Configuration will fail.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // |displays_[2] will downgrade next. Configuration still fails.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          // |displays_[1]| will downgrade again. Configuration succeeds.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          // HDMI display will fail modeset and downgrade once. Configuration
          // will then succeed.
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[3]->display_id(), gfx::Point(), &medium_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests a nested MST configuration in which after a successful modset on the
// root branch device (i.e. two external displays connected to a single MST hub)
// one display is removed from the original MST hub, connected to a second MST
// hub together with a third display, and then the second MST hub is connected
// to the first. The tests ensures that the three MST displays are grouped,
// tested, and fallback together appropriately before passing modeset.
TEST_F(ConfigureDisplaysTaskTest,
       ConfigureInternalAndMstThenNestAnotherMstForThreeExternalDisplays) {
  // We now have one internal display + two external displays connected via MST.
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(789)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(small_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kSecondConnectorId)
                          .Build());

  // Initial configuration succeeds modeset.
  {
    ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
        &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

    std::vector<DisplayConfigureRequest> requests;
    for (const auto& display : displays_) {
      requests.emplace_back(display.get(), display->native_mode(),
                            gfx::Point());
    }

    ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
    task.Run();

    EXPECT_TRUE(callback_called_);
    EXPECT_EQ(ConfigureDisplaysTask::SUCCESS, status_);
    EXPECT_EQ(
        JoinActions(GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                                   displays_[0]->native_mode()})
                        .c_str(),
                    GetCrtcAction({displays_[1]->display_id(), gfx::Point(),
                                   &big_mode_60hz_})
                        .c_str(),
                    GetCrtcAction({displays_[2]->display_id(), gfx::Point(),
                                   &big_mode_60hz_})
                        .c_str(),
                    nullptr),
        log_.GetActionsAndClear());
  }

  // Add an additional display to kSecondConnectorId. This is akin to unplugging
  // One display from the first MST hub, attaching it to a second one, together
  // with a third display, and plugging the second MST hub to the first.
  displays_.push_back(FakeDisplaySnapshot::Builder()
                          .SetId(101112)
                          .SetNativeMode(big_mode_60hz_.Clone())
                          .SetCurrentMode(big_mode_60hz_.Clone())
                          .AddMode(small_mode_60hz_.Clone())
                          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
                          .SetBaseConnectorId(kSecondConnectorId)
                          .Build());

  // Simulate bandwidth pressure by reducing configurable pixels.
  delegate_.set_max_configurable_pixels(medium_mode_60hz_.size().GetArea());

  // This configuration requires that all displays connected via the nested
  // MST setup downgrade, so we test that all three displays are grouped,
  // fallback appropriately, and eventually succeed modeset.
  {
    ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
        &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

    std::vector<DisplayConfigureRequest> requests;
    for (const auto& display : displays_) {
      requests.emplace_back(display.get(), display->native_mode(),
                            gfx::Point());
    }

    ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
    task.Run();

    EXPECT_TRUE(callback_called_);
    EXPECT_EQ(ConfigureDisplaysTask::PARTIAL_SUCCESS, status_);
    EXPECT_EQ(
        JoinActions(
            // All displays will fail to modeset together. Initiate retry logic.
            GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                           displays_[0]->native_mode()})
                .c_str(),
            GetCrtcAction(
                {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
                .c_str(),
            // Turn off all displays to reset the system resources allocation.
            GetDisableCrtcAction(displays_[0]).c_str(),
            GetDisableCrtcAction(displays_[1]).c_str(),
            GetDisableCrtcAction(displays_[2]).c_str(),
            GetDisableCrtcAction(displays_[3]).c_str(),
            // Internal display will succeed to modeset.
            GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                           displays_[0]->native_mode()})
                .c_str(),
            // All MST displays will fail modeset together.
            GetCrtcAction(
                {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
                .c_str(),
            // displays_[1] will downgrade first, then displays_[2], followed by
            // displays_[3], and finally displays_[1] will downgrade one last
            // time. Then the configuration will pass modeset.
            GetCrtcAction(
                {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[2]->display_id(), gfx::Point(), &big_mode_60hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[3]->display_id(), gfx::Point(), &big_mode_60hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[3]->display_id(), gfx::Point(), &small_mode_60hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[2]->display_id(), gfx::Point(), &small_mode_60hz_})
                .c_str(),
            GetCrtcAction(
                {displays_[3]->display_id(), gfx::Point(), &small_mode_60hz_})
                .c_str(),
            nullptr),
        log_.GetActionsAndClear());
  }
}

// Tests that an internal display with one external display pass modeset
// asynchronously after the external display fallback once.
TEST_F(ConfigureDisplaysTaskTest, AsyncConfigureWithTwoDisplaysPartialSuccess) {
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));

  delegate_.set_run_async(true);
  delegate_.set_max_configurable_pixels(medium_mode_60hz_.size().GetArea());

  std::vector<DisplayConfigureRequest> requests;
  for (const auto& display : displays_) {
    requests.emplace_back(display.get(), display->native_mode(), gfx::Point());
  }

  ConfigureDisplaysTask task(&delegate_, requests, std::move(callback));
  task.Run();

  EXPECT_FALSE(callback_called_);
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(callback_called_);
  EXPECT_EQ(ConfigureDisplaysTask::PARTIAL_SUCCESS, status_);
  EXPECT_EQ(
      JoinActions(
          // All displays will fail to modeset together.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources allocation.
          GetDisableCrtcAction(displays_[0]).c_str(),
          GetDisableCrtcAction(displays_[1]).c_str(),
          // Internal display will succeed to modeset.
          GetCrtcAction({displays_[0]->display_id(), gfx::Point(),
                         displays_[0]->native_mode()})
              .c_str(),
          // External display will fail twice, downgrade, and pass.
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &big_mode_29_97hz_})
              .c_str(),
          GetCrtcAction(
              {displays_[1]->display_id(), gfx::Point(), &small_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

// Tests requiring a resources cleanup for an internal display to succeed after
// it was closed and the system bandwidth can't handle all displays at big mode.
TEST_F(ConfigureDisplaysTaskTest, CloseLidThenOpenLid) {
  std::unique_ptr<DisplaySnapshot> internal_display =
      FakeDisplaySnapshot::Builder()
          .SetId(100)
          .SetNativeMode(big_mode_60hz_.Clone())
          .SetCurrentMode(big_mode_60hz_.Clone())
          .SetType(DISPLAY_CONNECTION_TYPE_INTERNAL)
          .SetBaseConnectorId(kEdpConnectorId)
          .Build();
  std::unique_ptr<DisplaySnapshot> external_display1 =
      FakeDisplaySnapshot::Builder()
          .SetId(200)
          .SetNativeMode(big_mode_60hz_.Clone())
          .SetCurrentMode(big_mode_60hz_.Clone())
          .AddMode(small_mode_60hz_.Clone())
          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
          .SetBaseConnectorId(kSecondConnectorId)
          .Build();
  std::unique_ptr<DisplaySnapshot> external_display2 =
      FakeDisplaySnapshot::Builder()
          .SetId(external_display1->display_id() + 1)
          .SetNativeMode(big_mode_60hz_.Clone())
          .SetCurrentMode(big_mode_60hz_.Clone())
          .AddMode(small_mode_60hz_.Clone())
          .SetType(DISPLAY_CONNECTION_TYPE_DISPLAYPORT)
          .SetBaseConnectorId(kSecondConnectorId)
          .Build();

  // Turn on all displays at the same time.
  std::vector<DisplayConfigureRequest> requests;
  requests.emplace_back(internal_display.get(), internal_display->native_mode(),
                        gfx::Point());
  requests.emplace_back(external_display1.get(),
                        external_display1->native_mode(), gfx::Point());
  requests.emplace_back(external_display2.get(),
                        external_display2->native_mode(), gfx::Point());
  // Set the bandwidth which the system can handle. it should be based on the
  // area of the mode.
  // For this test, we wanna support either just 2 monitors at big mode or 1
  // internal display + 2 monitors at small mode. This is to mean we can't have
  // all 3 running at max mode.
  int supported_option_1 = big_mode_60hz_.size().GetArea() * 2;
  int supported_option_2 =
      big_mode_60hz_.size().GetArea() + small_mode_60hz_.size().GetArea() * 2;
  // pick the biggest one.
  int supported_bw = std::max(supported_option_1, supported_option_2);
  // but also make sure we can't run all 3 at max mode.
  ASSERT_LT(supported_bw, big_mode_60hz_.size().GetArea() * 3);
  delegate_.set_system_bandwidth_limit(supported_bw);

  // Run Task turning on all displays.
  ConfigureDisplaysTask::ResponseCallback callback = base::BindOnce(
      &ConfigureDisplaysTaskTest::ConfigureCallback, base::Unretained(this));
  ConfigureDisplaysTask turn_on_all_displays_task(&delegate_, requests,
                                                  std::move(callback));
  turn_on_all_displays_task.Run();
  // This case has been checked in all other tests. Just verify and work to move
  // on to the case we're interested in in this test.
  ASSERT_EQ(ConfigureDisplaysTask::PARTIAL_SUCCESS, status_);
  log_.GetActionsAndClear();

  // Simulate closing the lid
  requests.clear();
  requests.emplace_back(internal_display.get(), nullptr, gfx::Point());
  requests.emplace_back(external_display1.get(),
                        external_display1->native_mode(), gfx::Point());
  requests.emplace_back(external_display2.get(),
                        external_display2->native_mode(), gfx::Point());
  callback = base::BindOnce(&ConfigureDisplaysTaskTest::ConfigureCallback,
                            base::Unretained(this));
  ConfigureDisplaysTask close_lid(&delegate_, requests, std::move(callback));
  close_lid.Run();
  EXPECT_EQ(ConfigureDisplaysTask::SUCCESS, status_);
  EXPECT_EQ(JoinActions(GetDisableCrtcAction(internal_display).c_str(),
                        GetCrtcAction({external_display1->display_id(),
                                       gfx::Point(), &big_mode_60hz_})
                            .c_str(),
                        GetCrtcAction({external_display2->display_id(),
                                       gfx::Point(), &big_mode_60hz_})
                            .c_str(),
                        nullptr),
            log_.GetActionsAndClear());

  // Simulate opening the lid as the 2 external displays are already running at
  // big mode.
  requests.clear();
  requests.emplace_back(internal_display.get(), internal_display->native_mode(),
                        gfx::Point());
  requests.emplace_back(external_display1.get(),
                        external_display1->native_mode(), gfx::Point());
  requests.emplace_back(external_display2.get(),
                        external_display2->native_mode(), gfx::Point());
  callback = base::BindOnce(&ConfigureDisplaysTaskTest::ConfigureCallback,
                            base::Unretained(this));
  ConfigureDisplaysTask open_lid(&delegate_, requests, std::move(callback));
  open_lid.Run();
  ASSERT_EQ(ConfigureDisplaysTask::PARTIAL_SUCCESS, status_);
  EXPECT_EQ(
      JoinActions(
          // Attempt to turn everything on with the highest mode.
          GetCrtcAction(
              {internal_display->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {external_display1->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {external_display2->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Turn off all displays to reset the system resources
          GetDisableCrtcAction(internal_display).c_str(),
          GetDisableCrtcAction(external_display1).c_str(),
          GetDisableCrtcAction(external_display2).c_str(),
          // Internal display will be the first to be turned on at big mode.
          GetCrtcAction(
              {internal_display->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // External displays will attempt to be turned on at big mode.
          GetCrtcAction(
              {external_display1->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {external_display2->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          // Fallback until success as small mode.
          GetCrtcAction({external_display1->display_id(), gfx::Point(),
                         &small_mode_60hz_})
              .c_str(),
          GetCrtcAction(
              {external_display2->display_id(), gfx::Point(), &big_mode_60hz_})
              .c_str(),
          GetCrtcAction({external_display1->display_id(), gfx::Point(),
                         &small_mode_60hz_})
              .c_str(),
          GetCrtcAction({external_display2->display_id(), gfx::Point(),
                         &small_mode_60hz_})
              .c_str(),
          nullptr),
      log_.GetActionsAndClear());
}

}  // namespace test
}  // namespace display
