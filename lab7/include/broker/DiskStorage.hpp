#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>

// работа с json файлами очередей с durability: disk
namespace DiskStorage {
    nlohmann::json readJson(const std::filesystem::path& path);
    void writeJson(const std::filesystem::path& path, const nlohmann::json& value);
}
