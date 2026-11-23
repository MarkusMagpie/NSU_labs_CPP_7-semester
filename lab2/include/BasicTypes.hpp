#pragma once

using Date = unsigned long long;

struct Time {
    unsigned long long hour;
    unsigned long long minute;
};

struct DateTime {
    Date date;
    Time time;
};
