/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright 2011-2019 Dominik Charousset                                     *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#include "caf/io/basp/worker.hpp"

#include "caf/actor_system.hpp"
#include "caf/io/basp/message_queue.hpp"
#include "caf/io/basp/worker_hub.hpp"
#include "caf/proxy_registry.hpp"
#include "caf/scheduler/abstract_coordinator.hpp"

namespace caf {
namespace io {
namespace basp {

// -- constructors, destructors, and assignment operators ----------------------

worker::worker(worker_hub& hub, message_queue& queue, proxy_registry& proxies)
    : next_(nullptr),
      hub_(&hub),
      queue_(&queue),
      proxies_(&proxies),
      system_(&proxies.system()) {
  CAF_IGNORE_UNUSED(pad_);
}

worker::~worker() {
  // nop
}

// -- management ---------------------------------------------------------------

void worker::launch(const node_id& last_hop, const basp::header& hdr,
                    const buffer_type& payload) {
  CAF_ASSERT(hdr.dest_actor != 0);
  CAF_ASSERT(hdr.operation == basp::message_type::direct_message
             || hdr.operation == basp::message_type::routed_message);
  msg_id_ = queue_->new_id();
  last_hop_ = last_hop;
  memcpy(&hdr_, &hdr, sizeof(basp::header));
  payload_.assign(payload.begin(), payload.end());
  system_->scheduler().enqueue(this);
}

// -- implementation of resumable ----------------------------------------------

resumable::subtype_t worker::subtype() const {
  return resumable::function_object;
}

resumable::resume_result worker::resume(execution_unit* ctx, size_t) {
  ctx->proxy_registry_ptr(proxies_);
  handle_remote_message(ctx);
  hub_->push(this);
  return resumable::awaiting_message;
}

void worker::intrusive_ptr_add_ref_impl() {
  // The basp::instance owns the hub (which owns this object) and must make
  // sure to wait for pending workers at exit.
}

void worker::intrusive_ptr_release_impl() {
  // nop
}

} // namespace basp
} // namespace io
} // namespace caf