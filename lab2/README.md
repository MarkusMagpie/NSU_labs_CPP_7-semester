## Сборка

```bash
cd ~/Desktop/NSU_labs_CPP_7-semester/lab2
rm -rf build && mkdir build && cd build
cmake .. -DWITH_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . -j"$(nproc)"
```

Из каталога `build` запуск программы и тестов:

```bash
./bank
ctest --output-on-failure
```
