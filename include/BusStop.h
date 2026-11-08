#ifndef BUS_STOP_H // защита срещу многократно включване
#define BUS_STOP_H
#include <string>
#include <iostream>
using namespace std;

/**
 * Спирка от градския транспорт.
 *
 * Всяка спирка се идентифицира с уникален буквено-цифров код (например "S001",
 * позволява се само букви от латинската азбука и цифра).
 * Освен код има име, адрес и географски координати. Всички данни се валидират
 * при създаване и при промяна през сетърите, така че обектът никога не държи
 * невалидно състояние.
 *
 * Ако координатите не са зададени, и двете са 0.0.
 */

class BusStop
{
private:
    string code;      // уникален код: само латински букви и цифри
    string name;      // име: латиница, кирилица, цифри, интервал, '.' и '-'
    string address;   // адрес: като името, плюс ',' и '/'
    double latitude;  // географска ширина, от -90 до 90
    double longitude; // географска дължина, от -180 до 180

    // Валидация. Всеки метод хвърля invalid_argument със съобщение за проблема.
    void validateBasicData(const string &code, const string &name, const string &address) const;
    void validateName(const string &name) const;
    void validateAddress(const string &address) const;
    void validateCode(const string &code) const;
    void validateLatitude(double latitude) const;
    void validateLongitude(double longitude) const;

public:
    // Създава спирка без координати (ширина и дължина остават 0.0).
    BusStop(const string &code,
            const string &name,
            const string &address);

    // Създава спирка с пълни данни, включително координати.
    BusStop(const string &code,
            const string &name,
            const string &address,
            double latitude,
            double longitude);

    // Четене на полетата.
    string getCode() const;
    string getName() const;
    string getAddress() const;
    double getLatitude() const;
    double getLongitude() const;

    // Промяна на полетата. Новата стойност се валидира преди да се запише,
    // така че при грешка обектът остава непроменен.
    void setCode(const string &code);
    void setName(const string &name);
    void setAddress(const string &address);
    void setLatitude(double latitude);
    void setLongitude(double longitude);
};

// Извежда спирката на един ред, например:
// Код на спирка: S001 | Име: ... | Адрес: ... | координати: Ширина: ..., Дължина: ...
ostream &operator<<(ostream &out, const BusStop &stop);

#endif