#pragma once

#include <BasicTypes.hpp>
#include <Enums.hpp>

struct Account {
    unsigned long long id; // Уникальный номер счета
    AccountType type; // Тип счета: debit; deposit; credit
    Money balance; // Объем средств на счете
    Currency currency; // Валюта счета: RUB; YUAN; USD; EUR
};

struct Deposit {
    unsigned long long id; // Уникальный номер вклада
    double annualRate; // Процентная ставка (пример: 6.32 = 6.32%)
    DepositType type; // Тип вклада
    Date creationDate; // Дата создания вклада
    unsigned long long termDays; // Полный срок вклада в днях
};

struct Credit {
    unsigned long long id; // Уникальный номер кредита
    double annualRate; // Процентная ставка (годовая) (пример: 6.32 = 6.32%)
    Money loanAmount; // Сумма займа
    CreditType type; // Тип кредита - периодичность начисления процентов
};

struct Client {
    unsigned long long id; // Индивидуальный номер клиента
    char name[100]; // Фамилия Имя Отчество / Название организации
    ClientType type; // Тип клиента
};
