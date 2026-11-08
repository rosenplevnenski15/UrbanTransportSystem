#ifndef TRANSPORT_TYPE_H
#define TRANSPORT_TYPE_H
#include <string>

// Видове градски транспорт, които системата поддържа.
enum class TransportType
{
    BUS,
    TRAM,
    TROLLEYBUS
};
// Превръща текст в тип транспорт.
TransportType parseTransportType(const std::string &input);

// Връща българското име на типа ("Автобус", "Трамвай", "Тролейбус").
// Така се записва и в CSV файла.
std::string transportTypeToString(TransportType type);

#endif
