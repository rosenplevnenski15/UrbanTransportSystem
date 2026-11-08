#include "BusStop.h"
#include <stdexcept>
#include <cctype>
#include <string>
#include <cmath>
#include <iostream>

using namespace std;

// Валидация

// Кодът трябва да е непразен и да съдържа само латински букви и цифри.
// isalnum работи с unsigned char, затова символът се преобразува изрично
// (при отрицателна стойност на char поведението е недефинирано).
void BusStop::validateCode(const string &code) const
{
    if (code.empty())
    {
        throw invalid_argument("Кодът на спирката не може да бъде празен");
    }

    for (char ch : code)
    {
        if (!isalnum(static_cast<unsigned char>(ch)))
        {
            throw invalid_argument("Кодът на спирката трябва да съдържа само букви и цифри");
        }
    }
}

// Името може да съдържа латински букви, цифри, интервал, '.', '-' и кирилица.
//
// Файловете и конзолата работят с UTF-8, където кирилската буква заема два
// байта. Първият е 0xD0 (U+0400..U+043F) или 0xD1 (U+0440..U+047F), вторият е
// продължаващ байт. Приемат се само символите U+0400..U+045F, затова:
//   след 0xD0 вторият байт е в 0x80..0xBF,
//   след 0xD1 вторият байт е в 0x80..0x9F.
void BusStop::validateName(const string &name) const
{
    if (name.empty())
    {
        throw invalid_argument("Името на спирката не може да бъде празно");
    }

    if (isspace(static_cast<unsigned char>(name.front())) ||
        isspace(static_cast<unsigned char>(name.back())))
    {

        throw invalid_argument("Името на спирката не може да започва или свършва с интервал");
    }

    for (size_t i = 0; i < name.length(); i++)
    {
        unsigned char ch = static_cast<unsigned char>(name[i]);

        if (ch < 128)
        {
            // ASCII символ: позволени са букви, цифри, интервал, '.' и '-'.
            if (!isalnum(ch) &&
                ch != ' ' &&
                ch != '.' &&
                ch != '-')
            {
                throw invalid_argument("Името на спирката съдържа невалидни символи");
            }
        }

        else if (ch == 0xD0 || ch == 0xD1)
        {
            // Начало на двубайтов кирилски символ: трябва да има втори байт.
            if (i + 1 >= name.length())
            {
                throw invalid_argument("Невалиден UTF-8 в името на спирката");
            }

            unsigned char next = static_cast<unsigned char>(name[i + 1]);

            if (ch == 0xD0 && (next < 0x80 || next > 0xBF))
            {
                throw invalid_argument("Невалиден кириличен символ в името на спирката");
            }

            if (ch == 0xD1 && (next < 0x80 || next > 0x9F))
            {
                throw invalid_argument("Невалиден кириличен символ в името на спирката");
            }

            i++; // вторият байт вече е проверен, прескачаме го
        }
        else
        {
            // Всеки друг многобайтов символ се отхвърля.
            throw invalid_argument("Името на спирката съдържа неподдържани символи");
        }
    }
}

// Адресът се валидира като името, но допълнително позволява ',' и '/'
// (например "бул. Витоша 15, вх. А" или "ул. Рила 3/5").
void BusStop::validateAddress(const string &address) const
{

    if (address.empty())
    {
        throw invalid_argument("Адресът на спирката не може да бъде празен");
    }

    if (isspace(static_cast<unsigned char>(address.front())) ||
        isspace(static_cast<unsigned char>(address.back())))
    {

        throw invalid_argument("Адресът на спирката не може да започва или свършва с интервал");
    }

    for (size_t i = 0; i < address.length(); i++)
    {
        unsigned char ch = static_cast<unsigned char>(address[i]);

        if (ch < 128)
        {
            if (!isalnum(ch) && ch != ' ' && ch != '.' &&
                ch != ',' && ch != '-' && ch != '/')
            {
                throw invalid_argument("Адресът на спирката съдържа невалидни символи");
            }
        }
        else if (ch == 0xD0 || ch == 0xD1)
        {

            // Същата проверка за кирилица като в validateName.
            if (i + 1 >= address.length())
            {
                throw invalid_argument("Невалиден UTF-8 символ в адреса на спирката");
            }

            unsigned char next = static_cast<unsigned char>(address[i + 1]);

            if (ch == 0xD0 && (next < 0x80 || next > 0xBF))
            {
                throw invalid_argument("Невалиден кириличен символ в адреса на спирката");
            }

            if (ch == 0xD1 && (next < 0x80 || next > 0x9F))
            {
                throw invalid_argument("Невалиден кириличен символ в адреса на спирката");
            }

            i++;
        }
        else
        {
            throw invalid_argument("Адресът на спирката съдържа неподдържани символи");
        }
    }
}

// Ширината трябва да е крайно число в интервала [-90, 90].
// isfinite отхвърля NaN и безкрайност, които иначе минават през сравненията.
void BusStop::validateLatitude(double latitude) const
{
    if (!isfinite(latitude) || latitude < -90.0 || latitude > 90.0)
    {
        throw invalid_argument("Ширината трябва да бъде между -90 и 90 градуса");
    }
}

// Дължината трябва да е крайно число в интервала [-180, 180].
void BusStop::validateLongitude(double longitude) const
{
    if (!isfinite(longitude) || longitude < -180.0 || longitude > 180.0)
    {
        throw invalid_argument("Дължината трябва да бъде между -180 и 180 градуса");
    }
}

// Проверява трите текстови полета, които има всяка спирка.
void BusStop::validateBasicData(const string &code, const string &name, const string &address) const
{
    validateCode(code);
    validateName(name);
    validateAddress(address);
}

// Конструкторът първо запълва полетата, после валидира. Ако валидацията
// хвърли изключение, обектът не се създава, така че невалидно състояние
// никога не излиза навън. Конструкторът задава ширина и дължина 0.0.
BusStop::BusStop(const string &code,
                 const string &name,
                 const string &address)
    : code(code),
      name(name),
      address(address),
      latitude(0.0),
      longitude(0.0)
{
    validateBasicData(code, name, address);
}

// Конструкторът позволява на потребител я да зададе и координати.
BusStop::BusStop(const string &code,
                 const string &name,
                 const string &address,
                 double latitude,
                 double longitude)
    : code(code),
      name(name),
      address(address),
      latitude(latitude),
      longitude(longitude)
{
    validateBasicData(code, name, address);
    validateLatitude(latitude);
    validateLongitude(longitude);
}

// Гетъри: връщат стойността на полетата.
string BusStop::getCode() const
{
    return code;
}

string BusStop::getName() const
{
    return name;
}

string BusStop::getAddress() const
{
    return address;
}

double BusStop::getLatitude() const
{
    return latitude;
}

double BusStop::getLongitude() const
{
    return longitude;
}

// Сетъри: първо валидация, после запис. При грешка стойността не се променя.
void BusStop::setCode(const string &code)
{
    validateCode(code);
    this->code = code;
}

void BusStop::setName(const string &name)
{
    validateName(name);
    this->name = name;
}

void BusStop::setAddress(const string &address)
{
    validateAddress(address);
    this->address = address;
}

void BusStop::setLatitude(double latitude)
{
    validateLatitude(latitude);
    this->latitude = latitude;
}

void BusStop::setLongitude(double longitude)
{
    validateLongitude(longitude);
    this->longitude = longitude;
}

// Извеждане
ostream &operator<<(ostream &out, const BusStop &stop)
{
    out << "Код на спирка: " << stop.getCode() << " | ";
    out << "Име: " << stop.getName() << " | ";
    out << "Адрес: " << stop.getAddress() << " | координати: ";
    out << "Ширина: " << stop.getLatitude() << ", ";
    out << "Дължина: " << stop.getLongitude();

    return out;
}
