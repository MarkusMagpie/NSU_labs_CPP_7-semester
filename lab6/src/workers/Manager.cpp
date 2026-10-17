#include <workers/Manager.h>

Manager::Manager(int id) : id_(id) {}

int Manager::id() const {
    return id_;
}
