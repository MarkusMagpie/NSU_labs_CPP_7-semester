#include <fstream>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

#include <broker/DiskStorage.hpp>

nlohmann::json DiskStorage::readJson(const std::filesystem::path& path) {
    std::ifstream input(path); // открыл файл дл чтения
    if (!input) {
        throw std::runtime_error("Не удалось открыть файл: " + path.string());
    }

    try {
        // десериализация
        return nlohmann::json::parse(input);
    } catch (const nlohmann::json::exception& error) {
        throw std::runtime_error("Некорректный JSON в " + path.string() + ": " + error.what());
    }
}

void DiskStorage::writeJson(const std::filesystem::path& path, const nlohmann::json& value) {
    // сначала создаю временный файл path + ".tmp." + i
    std::filesystem::path temporary;
    for (std::uint64_t i = 0;; ++i) {
        temporary = path.string() + ".tmp." + std::to_string(i);

        if (!std::filesystem::exists(temporary)) break;
        
        if (i == std::numeric_limits<std::uint64_t>::max()) {
            throw std::runtime_error("Не удалось выбрать имя временного файла");
        }
    }

    // создаю объект файлового потока открывающий файл для записи
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Не удалось создать файл: " + temporary.string());
    }
    output << value.dump() << '\n';
    output.close();
    if (!output) {
        throw std::runtime_error("Ошибка записи файла: " + temporary.string());
    }

    // успешная запись + закрытие temporary -> переименую временный temporary в настоящее имя path
    std::filesystem::rename(temporary, path);
}
