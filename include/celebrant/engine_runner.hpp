
#pragma once

#include "celebrant/engine.hpp"
#include "celebrant/inbound_queue.hpp"
#include "celebrant/types.hpp"

namespace celebrant {

class EngineRunner {
  public:
    explicit EngineRunner(InboundQueue& inbound);
    void run();

  private:
    Engine engine_;
    InboundQueue& inbound_;
};
} // namespace celebrant
