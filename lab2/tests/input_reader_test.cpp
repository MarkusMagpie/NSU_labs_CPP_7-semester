#include <cstdio>
#include <iterator>
#include <memory>
#include <string_view>
#include <unistd.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <InputReader.hpp>

namespace {
class ScopedStdin {
private:
    int saved_;
public:
    explicit ScopedStdin(std::FILE* input) : saved_(dup(STDIN_FILENO)) {
        REQUIRE(saved_ != -1);
        const int result = dup2(fileno(input), STDIN_FILENO);
        if (result == -1) {
            close(saved_);
        }
        REQUIRE(result != -1);
        std::clearerr(stdin);
    }

    ScopedStdin(const ScopedStdin&) = delete;
    ScopedStdin& operator=(const ScopedStdin&) = delete;

    ~ScopedStdin() {
        // POSIX очищает буфер ввода для временного файла перед восстановлением stdin.
        std::fflush(stdin);
        dup2(saved_, STDIN_FILENO);
        close(saved_);
        std::clearerr(stdin);
    }
};
} // namespace

TEST_CASE("Чтение всех начальных таблиц из stdin") {
    constexpr char input[] = R"(Accounts 6
101 # deposit # 1500.005 # RUB
102 # credit # 50.020 # USD
201 # debit # 10000.000 # RUB
202 # debit # 20000.000 # USD
203 # debit # 30000.000 # EUR
204 # debit # 40000.000 # YUAN
Debits 1
301 # 6.32E0 # Compounded Semi-Annually Min # 7 # 365
Credits 1
401 # 12.5 # 2500.750 # Charged Semi-annually
Clients 1
501 # Hitchcock Alfred Joseph # VIP Individual Client
Client Debit 1
501 # 101 # 301
Bank Accounts 4
201
202
203
204
Client Credit 1
501 # 102 # 401
Work Places 4
Client Manager # 2
Cash Desk # 3
Currency Exchange # 1
VIP Client Manager # 4
Exchange Rates 2
RUB # USD # 1.25E-2
USD # RUB # 7.75E1
)";

    const auto closeFile = [](std::FILE* file) { std::fclose(file); };
    const std::unique_ptr<std::FILE, decltype(closeFile)> fixture(std::tmpfile(), closeFile);
    REQUIRE(fixture != nullptr);
    REQUIRE(std::fputs(input, fixture.get()) != EOF);
    REQUIRE(std::fseek(fixture.get(), 0, SEEK_SET) == 0);
    const ScopedStdin stdinRedirect(fixture.get());

    AccountTable accounts;
    DepositTable deposits;
    CreditTable credits;
    ClientTable clients;
    ClientAccountTable clientAccounts;
    BankAccountTable bankAccounts;
    ClientCreditTable clientCredits;
    WorkPlaceTable workPlaces;
    ExchangeRateTable exchangeRates;

    readAccounts(accounts);
    readDeposits(deposits);
    readCredits(credits);
    readClients(clients);
    readClientAccounts(clientAccounts);
    readBankAccounts(bankAccounts);
    readClientCredits(clientCredits);
    readWorkPlaces(workPlaces);
    readExchangeRates(exchangeRates);

    const Account expectedAccounts[] = {
        {101, AccountType::Deposit, 1500.005, Currency::RUB},
        {102, AccountType::Credit, 50.020, Currency::USD},
        {201, AccountType::Debit, 10000.000, Currency::RUB},
        {202, AccountType::Debit, 20000.000, Currency::USD},
        {203, AccountType::Debit, 30000.000, Currency::EUR},
        {204, AccountType::Debit, 40000.000, Currency::YUAN}
    };
    REQUIRE(accounts.size() == std::size(expectedAccounts));
    for (const Account& expected : expectedAccounts) {
        CAPTURE(expected.id);
        const Account* account = accounts.findById(expected.id);
        REQUIRE(account != nullptr);
        CHECK(account->type == expected.type);
        CHECK(account->balance == expected.balance);
        CHECK(account->currency == expected.currency);
    }

    REQUIRE(deposits.size() == 1);
    const Deposit* deposit = deposits.findById(301);
    REQUIRE(deposit != nullptr);
    CHECK(deposit->annualRate == Catch::Approx(6.32));
    CHECK(deposit->type == DepositType::SemiAnnuallyMin);
    CHECK(deposit->creationDate == 7);
    CHECK(deposit->termDays == 365);

    REQUIRE(credits.size() == 1);
    const Credit* credit = credits.findById(401);
    REQUIRE(credit != nullptr);
    CHECK(credit->annualRate == Catch::Approx(12.5));
    CHECK(credit->loanAmount == 2500.750);
    CHECK(credit->type == CreditType::SemiAnnually);

    REQUIRE(clients.size() == 1);
    const Client* client = clients.findById(501);
    REQUIRE(client != nullptr);
    CHECK(std::string_view(client->name) == "Hitchcock Alfred Joseph");
    CHECK(client->type == ClientType::VipIndividualClient);

    REQUIRE(clientAccounts.size() == 1);
    const ClientAccount* clientAccount = clientAccounts.get(0);
    REQUIRE(clientAccount != nullptr);
    CHECK(clientAccount->clientId == 501);
    CHECK(clientAccount->accountId == 101);
    CHECK(clientAccount->depositId == 301);

    REQUIRE(bankAccounts.size() == 4);
    for (std::size_t i = 0; i < bankAccounts.size(); ++i) {
        CAPTURE(i);
        const BankAccount* bankAccount = bankAccounts.get(i);
        REQUIRE(bankAccount != nullptr);
        CHECK(bankAccount->accountId == 201 + i);
    }

    REQUIRE(clientCredits.size() == 1);
    const ClientCredit* clientCredit = clientCredits.get(0);
    REQUIRE(clientCredit != nullptr);
    CHECK(clientCredit->clientId == 501);
    CHECK(clientCredit->accountId == 102);
    CHECK(clientCredit->creditId == 401);

    const WorkPlace expectedWorkPlaces[] = {
        {WorkPlaceType::ClientManager, 2},
        {WorkPlaceType::CashDesk, 3},
        {WorkPlaceType::CurrencyExchange, 1},
        {WorkPlaceType::VipClientManager, 4}
    };
    REQUIRE(workPlaces.size() == std::size(expectedWorkPlaces));
    for (std::size_t i = 0; i < workPlaces.size(); ++i) {
        CAPTURE(i);
        const WorkPlace* workPlace = workPlaces.get(i);
        REQUIRE(workPlace != nullptr);
        CHECK(workPlace->type == expectedWorkPlaces[i].type);
        CHECK(workPlace->count == expectedWorkPlaces[i].count);
    }

    const ExchangeRate expectedExchangeRates[] = {
        {Currency::RUB, Currency::USD, 0.0125},
        {Currency::USD, Currency::RUB, 77.5}
    };
    REQUIRE(exchangeRates.size() == std::size(expectedExchangeRates));
    for (std::size_t i = 0; i < exchangeRates.size(); ++i) {
        CAPTURE(i);
        const ExchangeRate* rate = exchangeRates.get(i);
        REQUIRE(rate != nullptr);
        CHECK(rate->soldCurrency == expectedExchangeRates[i].soldCurrency);
        CHECK(rate->boughtCurrency == expectedExchangeRates[i].boughtCurrency);
        CHECK(rate->rate == Catch::Approx(expectedExchangeRates[i].rate));
    }
}
