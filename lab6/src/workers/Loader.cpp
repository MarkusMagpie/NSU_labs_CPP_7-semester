#include <workers/Loader.h>

Loader::Loader(int id, std::chrono::milliseconds actionTime) : id_(id), actionTime_(actionTime) {}

int Loader::id() const {
    return id_;
}

std::chrono::milliseconds Loader::actionTime() const {
    return actionTime_;
}
