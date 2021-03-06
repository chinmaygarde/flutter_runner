// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application_runner.h"

#include <utility>

#include "flutter/lib/ui/text/font_collection.h"
#include "fuchsia_font_manager.h"
#include "lib/fonts/fidl/font_provider.fidl.h"
#include "lib/icu_data/cpp/icu_data.h"

namespace flutter {

ApplicationRunner::ApplicationRunner(fxl::Closure on_termination_callback)
    : on_termination_callback_(std::move(on_termination_callback)),
      host_context_(app::ApplicationContext::CreateFromStartupInfo()) {
  SetupICU();

  SetupGlobalFonts();

  host_context_->outgoing_services()->AddService<app::ApplicationRunner>(
      std::bind(&ApplicationRunner::RegisterApplication, this,
                std::placeholders::_1));

  active_applications_bindings_.set_empty_set_handler(
      [this]() { FireTerminationCallbackIfNecessary(); });
}

ApplicationRunner::~ApplicationRunner() {
  host_context_->outgoing_services()->RemoveService<app::ApplicationRunner>();
}

void ApplicationRunner::RegisterApplication(
    fidl::InterfaceRequest<app::ApplicationRunner> request) {
  active_applications_bindings_.AddBinding(this, std::move(request));
}

void ApplicationRunner::StartApplication(
    app::ApplicationPackagePtr package,
    app::ApplicationStartupInfoPtr startup_info,
    fidl::InterfaceRequest<app::ApplicationController> controller) {
  auto thread_application_pair =
      Application::Create(*this,                    // delegate
                          std::move(package),       // application pacakge
                          std::move(startup_info),  // startup info
                          std::move(controller)     // controller request
      );
  active_applications_[thread_application_pair.second.get()] =
      std::move(thread_application_pair);
}

void ApplicationRunner::OnApplicationTerminate(const Application* application) {
  active_applications_.erase(application);
  FireTerminationCallbackIfNecessary();
}

void ApplicationRunner::SetupICU() {
  if (!icu_data::Initialize(host_context_.get())) {
    FXL_LOG(ERROR) << "Could not initialize ICU data.";
  }
}

void ApplicationRunner::SetupGlobalFonts() {
  fonts::FontProviderPtr font_provider(
      host_context_->ConnectToEnvironmentService<fonts::FontProvider>());
  blink::FontCollection::ForProcess().GetFontCollection()->PushFront(
      sk_make_sp<txt::FuchsiaFontManager>(std::move(font_provider)));
}

void ApplicationRunner::FireTerminationCallbackIfNecessary() {
  // We have no reason to exist if:
  // 1: No previously launched applications are running.
  // 2: No bindings exist that may require launching more applications.
  if (on_termination_callback_ && active_applications_.size() == 0 &&
      active_applications_bindings_.size() == 0) {
    on_termination_callback_();
  }
}

}  // namespace flutter
