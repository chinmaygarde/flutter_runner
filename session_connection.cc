// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "session_connection.h"

#include "lib/ui/scenic/fidl_helpers.h"

namespace flutter {

SessionConnection::SessionConnection(
    const scenic::SceneManagerPtr& scene_manager,
    zx::eventpair import_token,
    fxl::Closure session_metrics_did_change_callback)
    : session_(scene_manager.get()),
      root_node_(&session_),
      surface_producer_(std::make_unique<VulkanSurfaceProducer>(&session_)),
      scene_update_context_(&session_, surface_producer_.get()),
      metrics_changed_callback_(
          std::move(session_metrics_did_change_callback)) {
  session_.set_error_handler(
      std::bind(&SessionConnection::OnSessionError, this));
  session_.set_event_handler(std::bind(&SessionConnection::OnSessionEvents,
                                       this, std::placeholders::_1));

  root_node_.Bind(std::move(import_token));
  root_node_.SetEventMask(scenic::kMetricsEventMask);
  session_.Present(0, [](scenic::PresentationInfoPtr info) {});
}

SessionConnection::~SessionConnection() = default;

void SessionConnection::OnSessionError() {
  // TODO: Not this.
  FXL_CHECK(false) << "Session connection was terminated.";
}

void SessionConnection::OnSessionEvents(fidl::Array<scenic::EventPtr> events) {
  scenic::MetricsPtr new_metrics;
  for (const auto& event : events) {
    if (event->is_metrics() &&
        event->get_metrics()->node_id == root_node_.id()) {
      new_metrics = std::move(event->get_metrics()->metrics);
    }
  }
  if (!new_metrics)
    return;

  scene_update_context_.set_metrics(std::move(new_metrics));

  if (metrics_changed_callback_) {
    metrics_changed_callback_();
  }
}

void SessionConnection::Present(flow::CompositorContext::ScopedFrame& frame) {
  // Flush all session ops. Paint tasks have not yet executed but those are
  // fenced. The compositor can start processing ops while we finalize paint
  // tasks.
  session_.Present(0,  // presentation_time. (placeholder).
                   [](scenic::PresentationInfoPtr) {}  // callback
  );

  // Execute paint tasks and signal fences.
  auto surfaces_to_submit = scene_update_context_.ExecutePaintTasks(frame);

  // Tell the surface producer that a present has occurred so it can perform
  // book-keeping on buffer caches.
  surface_producer_->OnSurfacesPresented(std::move(surfaces_to_submit));

  // Prepare for the next frame. These ops won't be processed till the next
  // present.
  EnqueueClearOps();
}

void SessionConnection::EnqueueClearOps() {
  // We are going to be sending down a fresh node hierarchy every frame. So just
  // enqueue a detach op on the imported root node.
  session_.Enqueue(scenic_lib::NewDetachChildrenOp(root_node_.id()));
}

}  // namespace flutter
