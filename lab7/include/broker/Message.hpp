#pragma once

#include <string>

#include <nlohmann/json.hpp>

/*
Производители передают сообщения брокеру. Брокер обрабатывает сообщения от производителей согласно заданным правилам очередей.
*/
struct Message {
    std::string id;                         // id сообщения которое хранит брокер
    std::string producerId;                 // id производителя из входящего в брокер конверта
    nlohmann::json payload = nullptr;       // содержимое сообщения
};
