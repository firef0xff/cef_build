// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/headless/headless_mode_browsertest.h"

#include "build/build_config.h"

// Native headless is currently available on Linux, Windows and Mac platforms.
// More platforms will be added later, so avoid function level clutter by
// providing a compile time condition over the entire file.
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/multiprocess_test.h"
#include "base/test/task_environment.h"
#include "base/test/test_timeouts.h"
#include "chrome/browser/chrome_process_singleton.h"
#include "chrome/browser/headless/headless_mode_util.h"
#include "chrome/browser/process_singleton.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/multiprocess_func_list.h"
#include "ui/gfx/switches.h"

#if BUILDFLAG(IS_LINUX)
#include "ui/ozone/public/ozone_platform.h"
#endif  // BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_WIN)
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_win.h"
#endif  // BUILDFLAG(IS_WIN)

namespace {
const int kErrorResultCode = -1;
}  // namespace

void HeadlessModeBrowserTest::SetUpCommandLine(
    base::CommandLine* command_line) {
  InProcessBrowserTest::SetUpCommandLine(command_line);

  command_line->AppendSwitchASCII(switches::kHeadless, kHeadlessSwitchValue);
  headless::SetUpCommandLine(command_line);
}

void HeadlessModeBrowserTest::SetUpOnMainThread() {
  InProcessBrowserTest::SetUpOnMainThread();

  ASSERT_TRUE(headless::IsChromeNativeHeadless());
}

#if BUILDFLAG(IS_LINUX)
IN_PROC_BROWSER_TEST_F(HeadlessModeBrowserTest, OzonePlatformHeadless) {
  // On Linux, the Native Headless Chrome uses Ozone/Headless.
  ASSERT_NE(ui::OzonePlatform::GetInstance(), nullptr);
  EXPECT_EQ(ui::OzonePlatform::GetPlatformNameForTest(), "headless");
}
#endif  // BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_WIN)
// A class to expose a protected method for testing purposes.
class DesktopWindowTreeHostWinWrapper : public views::DesktopWindowTreeHostWin {
 public:
  HWND GetHWND() const { return DesktopWindowTreeHostWin::GetHWND(); }
};

IN_PROC_BROWSER_TEST_F(HeadlessModeBrowserTest, BrowserDesktopWindowHidden) {
  // On Windows, the Native Headless Chrome browser window exists and is
  // visible, while the underlying platform window is hidden.
  EXPECT_TRUE(browser()->window()->IsVisible());

  DesktopWindowTreeHostWinWrapper* desktop_window_tree_host =
      static_cast<DesktopWindowTreeHostWinWrapper*>(
          browser()->window()->GetNativeWindow()->GetHost());
  EXPECT_FALSE(::IsWindowVisible(desktop_window_tree_host->GetHWND()));
}
#endif  // BUILDFLAG(IS_WIN)

class HeadlessModeBrowserTestWithUserDataDir : public HeadlessModeBrowserTest {
 public:
  HeadlessModeBrowserTestWithUserDataDir() = default;

  HeadlessModeBrowserTestWithUserDataDir(
      const HeadlessModeBrowserTestWithUserDataDir&) = delete;
  HeadlessModeBrowserTestWithUserDataDir& operator=(
      const HeadlessModeBrowserTestWithUserDataDir&) = delete;

  ~HeadlessModeBrowserTestWithUserDataDir() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    HeadlessModeBrowserTest::SetUpCommandLine(command_line);

    ASSERT_TRUE(user_data_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(base::IsDirectoryEmpty(user_data_dir()));

    command_line->AppendSwitchPath(switches::kUserDataDir, user_data_dir());
  }

  const base::FilePath& user_data_dir() const {
    return user_data_dir_.GetPath();
  }

 private:
  base::ScopedTempDir user_data_dir_;
};

class MockChromeProcessSingleton : public ChromeProcessSingleton {
 public:
  explicit MockChromeProcessSingleton(const base::FilePath& user_data_dir)
      : ChromeProcessSingleton(
            user_data_dir,
            base::BindRepeating(
                &MockChromeProcessSingleton::NotificationCallback,
                base::Unretained(this))) {}

 private:
  bool NotificationCallback(const base::CommandLine& command_line,
                            const base::FilePath& current_directory) {
    NOTREACHED();
    return true;
  }
};

IN_PROC_BROWSER_TEST_F(HeadlessModeBrowserTestWithUserDataDir,
                       ChromeProcessSingletonExists) {
  // Pass the user data dir to the child process which will try
  // to create a mock ChromeProcessSingleton in it that is
  // expected to fail.
  base::CommandLine command_line(
      base::GetMultiProcessTestChildBaseCommandLine());
  command_line.AppendSwitchPath(switches::kUserDataDir, user_data_dir());

  base::Process child_process =
      base::SpawnMultiProcessTestChild("ChromeProcessSingletonChildProcessMain",
                                       command_line, base::LaunchOptions());

  int result = kErrorResultCode;
  ASSERT_TRUE(base::WaitForMultiprocessTestChildExit(
      child_process, TestTimeouts::action_timeout(), &result));

  EXPECT_EQ(static_cast<ProcessSingleton::NotifyResult>(result),
            ProcessSingleton::PROFILE_IN_USE);
}

MULTIPROCESS_TEST_MAIN(ChromeProcessSingletonChildProcessMain) {
  content::BrowserTaskEnvironment task_environment;

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  const base::FilePath user_data_dir =
      command_line->GetSwitchValuePath(switches::kUserDataDir);
  if (user_data_dir.empty())
    return kErrorResultCode;

  MockChromeProcessSingleton chrome_process_singleton(user_data_dir);
  ProcessSingleton::NotifyResult notify_result =
      chrome_process_singleton.NotifyOtherProcessOrCreate();

  return static_cast<int>(notify_result);
}

#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
