#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <libcompressor/libcompressor.hpp>
#include <string_view>

int main(int argc, char* argv[]) {
  // ТЗ: ошибки через spdlog с уровнем логгирования ERROR в stderr
  auto logger = spdlog::stderr_color_mt("compressor");
  logger->set_level(spdlog::level::err);

  if (argc < 3) {
    logger->error("Недостаточно аргументов передано");
    return EXIT_FAILURE;
  }

  // ТЗ: ошибка, если первый аргумент не "zlib" и не "bzip"
  std::string_view algos_name = argv[1];
  libcompressor_CompressionAlgorithm algo{};
  if (algos_name == "zlib") {
    algo = libcompressor_Zlib;
  } else if (algos_name == "bzip") {
    algo = libcompressor_Bzip;
  } else {
    logger->error("Ожидались значения алгоритмов zlib или bzip. Получен: {}", algos_name);

    return EXIT_FAILURE;
  }

  // ТЗ: сжимает строку из второго аргумента
  libcompressor_Buffer input{argv[2], static_cast<int>(std::strlen(argv[2]))};
  libcompressor_Buffer output = libcompressor_compress(algo, input);

  // ТЗ: ошибка если libcompressor_compress вернула пустой буфер
  if (output.data == nullptr || output.size == 0) {
    logger->error("Сжатие сработало неправильно");
    return EXIT_FAILURE;
  }

  // в ТЗ сказано как выводить я так и сделал
  for (int i = 0; i < output.size; ++i) {
    std::printf("%.2hhx", output.data[i]);
  }
  std::printf("\n");

  std::free(output.data);

  return EXIT_SUCCESS;
}