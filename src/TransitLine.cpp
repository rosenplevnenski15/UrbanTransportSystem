#include "TransitLine.h"
#include "TransportType.h"
#include <stdexcept>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <cctype>

using namespace std;

// Проверява код на спирка. Правилото е същото като в BusStop: непразен низ
// само от латински букви и цифри. Използва се за кодовете в маршрута.
void TransitLine::validateCode(const string &code) const
{
    if (code.empty())
    {
        throw invalid_argument("Кодът на линията не може да бъде празен");
    }

    for (char ch : code)
    {
        if (!isalnum(static_cast<unsigned char>(ch)))
        {
            throw invalid_argument("Кодът на линията трябва да съдържа само букви и цифри");
        }
    }
}

// Проверява дали номерът на линията е положително цяло число.
void TransitLine::validateLineNumber(int lineNumber) const
{
    if (lineNumber <= 0)
    {
        throw invalid_argument("Номерът на линията трябва да бъде положително цяло число");
    }
}

// Проверява дали интервалът на движение е положително цяло число.
void TransitLine::validateMovementInterval(int movementInterval) const
{
    if (movementInterval <= 0)
    {
        throw invalid_argument("Интервалът на движение трябва да бъде положително цяло число");
    }
}

// Проверява дали типът на транспорта е валиден (BUS, TRAM или TROLLEYBUS).
void TransitLine::validateTransportType(TransportType transportType) const
{
    switch (transportType)
    {
    case TransportType::BUS:
    case TransportType::TRAM:
    case TransportType::TROLLEYBUS:

        break;

    default:
        throw invalid_argument("Невалиден тип транспорт");
    }
}

// Всеки код трябва да е валиден и да се среща най-много веднъж.
// unordered_set::insert връща второ поле false, ако елементът вече е бил там.
void TransitLine::validateStopCodes(const vector<string> &stopCodes) const
{
    unordered_set<string> uniqueCodes;

    for (const string &stopCode : stopCodes)
    {
        validateCode(stopCode);

        if (!uniqueCodes.insert(stopCode).second)
        {
            throw invalid_argument("Дублиран код на спирка");
        }
    }
}

// Линия без спирки: stopCodes се инициализира празен.
TransitLine::TransitLine(int lineNumber,
                         TransportType transportType,
                         int movementInterval)
    : lineNumber(lineNumber),
      transportType(transportType),
      stopCodes(),
      movementInterval(movementInterval)
{
    validateLineNumber(lineNumber);
    validateTransportType(transportType);
    validateMovementInterval(movementInterval);
}

// Линия със зададен маршрут. Кодовете не могат да се повтарят.
TransitLine::TransitLine(int lineNumber,
                         TransportType transportType,
                         const vector<string> &stopCodes,
                         int movementInterval)
    : lineNumber(lineNumber),
      transportType(transportType),
      stopCodes(stopCodes),
      movementInterval(movementInterval)
{
    validateLineNumber(lineNumber);
    validateTransportType(transportType);
    validateStopCodes(stopCodes);
    validateMovementInterval(movementInterval);
}

// Гетъри. Връщат стойността на полетата.
int TransitLine::getLineNumber() const
{
    return lineNumber;
}

TransportType TransitLine::getTransportType() const
{
    return transportType;
}

const vector<string> &TransitLine::getStopCodes() const
{
    return stopCodes;
}

int TransitLine::getMovementInterval() const
{
    return movementInterval;
}

// Сетъри. Проверяват валидността на новите стойности и ги присвояват.
void TransitLine::setLineNumber(int lineNumber)
{
    validateLineNumber(lineNumber);
    this->lineNumber = lineNumber;
}

void TransitLine::setTransportType(TransportType transportType)
{
    validateTransportType(transportType);
    this->transportType = transportType;
}

void TransitLine::setStopCodes(const vector<string> &stopCodes)
{
    validateStopCodes(stopCodes);
    this->stopCodes = stopCodes;
}

void TransitLine::setMovementInterval(int movementInterval)
{
    validateMovementInterval(movementInterval);
    this->movementInterval = movementInterval;
}

// Добавя спирка в края на маршрута, ако кодът е валиден и още не присъства.
void TransitLine::addStop(const string &stopCode)
{
    validateCode(stopCode);

    if (find(stopCodes.begin(), stopCodes.end(), stopCode) != stopCodes.end())
    {
        throw invalid_argument("Кодът на спирката вече съществува в тази линия");
    }

    stopCodes.push_back(stopCode);
}

// Премахва спирката, като запазва реда на останалите.
void TransitLine::removeStop(const string &stopCode)
{
    auto it = find(stopCodes.begin(), stopCodes.end(), stopCode);

    if (it == stopCodes.end())
    {
        throw invalid_argument("Кодът на спирката не съществува в тази линия");
    }

    stopCodes.erase(it);
}

// Извеждане
ostream &operator<<(ostream &out, const TransitLine &line)
{
    out << "Номер на линията: " << line.getLineNumber() << " | ";
    out << "Тип на транспорта: " << transportTypeToString(line.getTransportType()) << " | ";
    out << "Интервал на движение: " << line.getMovementInterval() << " минути | ";
    out << "Кодове на спирки: ";

    const vector<string> &stopCodes = line.getStopCodes();

    if (stopCodes.empty())
    {
        out << "Няма добавени спирки";
    }
    else
    {
        // Кодовете се разделят със запетая, без запетая след последния.
        for (size_t i = 0; i < stopCodes.size(); ++i)
        {
            out << stopCodes[i];

            if (i + 1 < stopCodes.size())
            {
                out << ", ";
            }
        }
    }

    return out;
}
