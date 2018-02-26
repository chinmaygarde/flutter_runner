// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdlib>

#include "application_runner.h"
#include "lib/fsl/tasks/message_loop.h"

int main(int argc, char const* argv[]) {
  fsl::MessageLoop loop;
  FXL_LOG(INFO) << "Flutter application services initialized.";
  flutter::ApplicationRunner runner([&loop]() {
    loop.PostQuitTask();
    FXL_LOG(INFO) << "Flutter application services terminated. Good bye...";
  });
  loop.Run();
  return EXIT_SUCCESS;
}
