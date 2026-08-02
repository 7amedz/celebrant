#pragma once

#include <memory>
#include <unordered_map>
#include <utility>

#include "celebrant/connection.hpp"
#include "celebrant/types.hpp"
namespace celebrant {

class ConnectionRegistry {
  public:
    void add(SessionId id, std::shared_ptr<Connection> conn) { registry_[id] = std::move(conn); }
    void remove(SessionId id) { registry_.erase(id); }
    std::shared_ptr<Connection> get(SessionId id) {
        auto it = registry_.find(id);
        if (it != registry_.end()) {
            return it->second;
        }
        return {};
    }

  private:
    std::unordered_map<SessionId, std::shared_ptr<Connection>> registry_;
};
} // namespace celebrant
