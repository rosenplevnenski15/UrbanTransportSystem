#include "TransportType.h"
#include <stdexcept>

using namespace std;

// Преобразуване на текстов низ въведен от потребителя в стойност от изброимия
// тип TransportType.
TransportType parseTransportType(const string &input)
{
    if (input == "Автобус" || input == "автобус" ||
        input == "BUS" || input == "bus")
    {
        return TransportType::BUS;
    }
    else if (input == "Трамвай" || input == "трамвай" ||
             input == "TRAM" || input == "tram")
    {
        return TransportType::TRAM;
    }
    else if (input == "Тролейбус" || input == "тролейбус" ||
             input == "TROLLEYBUS" || input == "trolleybus")
    {
        return TransportType::TROLLEYBUS;
    }

    throw invalid_argument("Невалиден тип транспорт");
}

// Обратното преобразуване. default клонът е за стойност извън изброените,
// която може да се получи само с преобразуване от int.
string transportTypeToString(TransportType type)
{
    switch (type)
    {
    case TransportType::BUS:
        return "Автобус";

    case TransportType::TRAM:
        return "Трамвай";

    case TransportType::TROLLEYBUS:
        return "Тролейбус";

    default:
        throw invalid_argument("Невалиден тип транспорт");
    }
}