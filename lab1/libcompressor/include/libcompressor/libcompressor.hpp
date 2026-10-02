/**
 * @file libcompressor.hpp
 * @brief Статическая библиотека для сжатия буферов алгоритмами zlib и bzip2
 */

#pragma once

/**
 * @brief Алгоритм сжатия
 */
enum libcompressor_CompressionAlgorithm {
  libcompressor_Zlib,  ///< Сжатие zlib (Z_DEFAULT_COMPRESSION)
  libcompressor_Bzip,  ///< Сжатие bzip2 (blockSize 1, verbosity 0, workFactor 0)
};

/**
 * @brief Буфер байтов
 */
struct libcompressor_Buffer {
  char* data;  ///< Указатель на данные
  int size;    ///< Размер данных в байтах
};

/**
 * @brief Сжимает входной буфер выбранным алгоритмом
 *
 * @param algo Алгоритм сжатия
 * @param input Входной буфер
 * @return Сжатые данные. Память выделена через std::malloc(), освобождать ее должен вызывающий через std::free().
 *         При ошибке возвращается буфер с data == nullptr и size == 0.
 */
libcompressor_Buffer libcompressor_compress(libcompressor_CompressionAlgorithm algo, libcompressor_Buffer input);