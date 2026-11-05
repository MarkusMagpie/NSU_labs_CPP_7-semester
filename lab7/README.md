# lab7 — Умная очередь сообщений (брокер + клиентская библиотека)

## Зависимости

```bash
sudo apt install libzmq3-dev cppzmq-dev libyaml-cpp-dev nlohmann-json3-dev
```

## Сборка

```bash
cd ~/Desktop/NSU_labs_CPP_7-semester/lab7
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j"$(nproc)"
```

## Запуск

```bash
./broker ../config/broker.yaml
```

## Структура

```
include/   публичные заголовки (protocol/, broker/, client/, common/)
src/       реализации + исполняемые файлы
config/    YAML-конфиг брокера
data/      рантайм-хранилище disk-очередей (не коммитится)
```
