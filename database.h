#pragma once
#include <vector>
#include <string>
#include "planner.h"

class Database {
private:
    std::string connection_string;

    // Вспомогательные утилиты для строк и дат
    std::wstring stringToWstring(const std::string& str);
    std::string wstringToString(const std::wstring& wstr);
    DateTime parseDateTime(const std::string& datetime_str);
    std::string dateTimeToString(const DateTime& dt);

public:
    Database(const std::string& conn_str);

    // Инициализация базы данных и проверка CSV
    void initFromCSV();

    // Считывание данных для планировщика
    std::vector<resource> getResources();
    std::vector<order> getOrders();
    std::vector<operation> getAllOperations();
    std::vector<technical_process> getAllProcesses();

    // Сохранение и очистка расписания в транзакции
    void saveSchedule(const std::vector<ScheduleItem>& schedule);

    // Экспорт в CSV
    void exportScheduleToCSV(const std::string& filename);
};
