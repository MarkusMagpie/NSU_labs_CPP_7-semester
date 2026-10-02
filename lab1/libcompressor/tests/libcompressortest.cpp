#include <gtest/gtest.h>

#include <cstdlib>
#include <libcompressor/libcompressor.hpp>
#include <vector>

// Если входной буфер не пустой то и выходной не пустой ----------------------------------------------------------------
TEST(ZlibTest, NonEmptyInputGivesNonEmptyOutput) {
  char data[] = "matveysorokin";
  libcompressor_Buffer input{data, sizeof(data) - 1};

  libcompressor_Buffer output = libcompressor_compress(libcompressor_Zlib, input);
  // проверяю непустоту вывода
  EXPECT_NE(output.data, nullptr);
  EXPECT_GT(output.size, 0);

  std::free(output.data);
}

TEST(BzipTest, NonEmptyInputGivesNonEmptyOutput) {
  char data[] = "matveysorokin";
  libcompressor_Buffer input{data, sizeof(data) - 1};

  libcompressor_Buffer output = libcompressor_compress(libcompressor_Bzip, input);
  EXPECT_NE(output.data, nullptr);
  EXPECT_GT(output.size, 0);

  std::free(output.data);
}

// Если входной буфер пустой или имеет нулевой размер, то выходной буфер пустой ----------------------------------------
TEST(ZlibTest, NullInputGivesEmptyOutput) {
  libcompressor_Buffer input{nullptr, 0};

  libcompressor_Buffer output = libcompressor_compress(libcompressor_Zlib, input);
  EXPECT_EQ(output.data, nullptr);
  EXPECT_EQ(output.size, 0);
}

TEST(ZlibTest, ZeroSizeInputGivesEmptyOutput) {
  char data[] = "matveysorokin";
  libcompressor_Buffer input{data, 0};

  libcompressor_Buffer output = libcompressor_compress(libcompressor_Zlib, input);
  EXPECT_EQ(output.data, nullptr);
  EXPECT_EQ(output.size, 0);
}

TEST(BzipTest, NullInputGivesEmptyOutput) {
  libcompressor_Buffer input{nullptr, 0};

  libcompressor_Buffer output = libcompressor_compress(libcompressor_Bzip, input);
  EXPECT_EQ(output.data, nullptr);
  EXPECT_EQ(output.size, 0);
}

TEST(BzipTest, ZeroSizeInputGivesEmptyOutput) {
  char data[] = "matveysorokin";
  libcompressor_Buffer input{data, 0};

  libcompressor_Buffer output = libcompressor_compress(libcompressor_Bzip, input);
  EXPECT_EQ(output.data, nullptr);
  EXPECT_EQ(output.size, 0);
}

// Если на вход алгоритму libcompressor_Zlib подается "test_string", то выходной буфер содержит байты из ТЗ ------------
TEST(ZlibTest, TestStringGivesExpectedBytes) {
  char data[] = "test_string";
  libcompressor_Buffer input{data, sizeof(data) - 1};  // без учета завершающего \0

  std::vector<unsigned char> expected{0x78, 0x9c, 0x2b, 0x49, 0x2d, 0x2e, 0x89, 0x2f, 0x2e, 0x29,
                                      0xca, 0xcc, 0x4b, 0x07, 0x00, 0x1c, 0x79, 0x04, 0xb7};

  libcompressor_Buffer output = libcompressor_compress(libcompressor_Zlib, input);
  std::vector<unsigned char> actual(output.data, output.data + output.size);

  EXPECT_EQ(actual, expected);

  std::free(output.data);
}

// Если на вход алгоритму libcompressor_Bzip подается "test_string", то выходной буфер содержит байты из ТЗ ------------
TEST(BzipTest, TestStringGivesExpectedBytes) {
  char data[] = "test_string";
  libcompressor_Buffer input{data, sizeof(data) - 1};

  std::vector<unsigned char> expected{0x42, 0x5a, 0x68, 0x31, 0x31, 0x41, 0x59, 0x26, 0x53, 0x59, 0x4a, 0x7c,
                                      0x69, 0x05, 0x00, 0x00, 0x04, 0x83, 0x80, 0x00, 0x00, 0x82, 0xa1, 0x1c,
                                      0x00, 0x20, 0x00, 0x22, 0x03, 0x68, 0x84, 0x30, 0x22, 0x50, 0xdf, 0x04,
                                      0x99, 0xe2, 0xee, 0x48, 0xa7, 0x0a, 0x12, 0x09, 0x4f, 0x8d, 0x20, 0xa0};

  libcompressor_Buffer output = libcompressor_compress(libcompressor_Bzip, input);
  std::vector<unsigned char> actual(output.data, output.data + output.size);

  EXPECT_EQ(actual, expected);

  std::free(output.data);
}