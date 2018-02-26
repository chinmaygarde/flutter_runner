// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "flutter/flow/compositor_context.h"
#include "flutter/flow/scene_update_context.h"
#include "lib/fidl/cpp/bindings/interface_handle.h"
#include "lib/fxl/functional/closure.h"
#include "lib/fxl/macros.h"
#include "lib/ui/scenic/client/resources.h"
#include "lib/ui/scenic/client/session.h"
#include "vulkan_surface_producer.h"
#include "zx/eventpair.h"

namespace flutter {

// The component residing on the GPU thread that is responsible for maintaining
// the Scenic session connection and presenting node updates.
class SessionConnection final {
 public:
  SessionConnection(const scenic::SceneManagerPtr& scene_manager,
                    zx::eventpair import_token,
                    fxl::Closure session_metrics_did_change_callback);

  ~SessionConnection();

  bool has_metrics() const { return scene_update_context_.has_metrics(); }

  const scenic::MetricsPtr& metrics() const {
    return scene_update_context_.metrics();
  }

  flow::SceneUpdateContext& scene_update_context() {
    return scene_update_context_;
  }

  scenic_lib::ImportNode& root_node() { return root_node_; }

  void Present(flow::CompositorContext::ScopedFrame& frame);

 private:
  scenic_lib::Session session_;
  scenic_lib::ImportNode root_node_;
  std::unique_ptr<VulkanSurfaceProducer> surface_producer_;
  flow::SceneUpdateContext scene_update_context_;
  fxl::Closure metrics_changed_callback_;

  void OnSessionError();

  void OnSessionEvents(fidl::Array<scenic::EventPtr> events);

  void EnqueueClearOps();

  FXL_DISALLOW_COPY_AND_ASSIGN(SessionConnection);
};

}  // namespace flutter
