// #include "../include/libcompressor/libcompressor.hpp"
#include <bzlib.h>
#include <zlib.h>  // uLongf, Bytef, compress, Z_OK

#include <cstdlib>
#include <libcompressor/libcompressor.hpp>  // по сравнению со строкой 1 подтянется благодаря target_include_directories

libcompressor_Buffer libcompressor_compress(libcompressor_CompressionAlgorithm algo, libcompressor_Buffer input) {
  libcompressor_Buffer result{nullptr, 0};
  if (input.data == nullptr || input.size <= 0) {
    return result;
  }

  std::size_t output_capacity = static_cast<std::size_t>(input.size) + 1024;
  void* output_ptr = std::malloc(output_capacity);
  if (output_ptr == nullptr) {
    return result;
  }

  switch (algo) {
    case libcompressor_Zlib: {
      auto dest_len = static_cast<uLongf>(output_capacity);

      int status = compress2(static_cast<Bytef*>(output_ptr),  // указатель на буфер куда будет записан сжатый результат
                             &dest_len,  // указатель на переменную содержащую размер выходного буфера
                             reinterpret_cast<const Bytef*>(input.data),  // указатель на исходные (несжатые) данные
                             static_cast<uLong>(input.size),  // размер исходных данных в байтах
                             Z_DEFAULT_COMPRESSION            // уровень сжатия
      );

      if (status == Z_OK) {
        result.data = static_cast<char*>(output_ptr);
        result.size = static_cast<int>(dest_len);

        return result;
      }

      break;
    }
    case libcompressor_Bzip: {
      auto dest_len = static_cast<unsigned int>(output_capacity);

      int status = BZ2_bzBuffToBuffCompress(static_cast<char*>(output_ptr), &dest_len, input.data,
                                            static_cast<unsigned int>(input.size), 1, 0, 0);

      if (status == BZ_OK) {
        result.data = static_cast<char*>(output_ptr);
        result.size = static_cast<int>(dest_len);

        return result;
      }

      break;
    }
  }

  std::free(output_ptr);

  return result;
}