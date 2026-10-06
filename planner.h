#pragma once
#include <vector>
#include <string>

// Структуры времени и интервалов
struct DateTime {
    int year, month, day, hour;
};

struct TimeInterval {
    DateTime start, end;
    std::wstring reason;
};

// Изделие
struct product {
    int id;
    std::wstring name;
};

// Техпроцесс
struct technical_process {
    int ID_tp;
    std::vector<int> operation_ids;
};

// Операция
struct operation {
    int ID_operation;
    std::wstring name;
    int duration;
};

// Ресурс
struct resource {
    int ID_resource;
    std::wstring name1;
    std::wstring name2;
    bool is_machine;
    std::vector<TimeInterval> assigned_intervals;
};

// Заказ
struct order {
    int ID_order;
    DateTime order_date;
    DateTime date_start;
    DateTime deadline;
    int fine;
    int TP_ID;
    int weight; // Вес заказа от 1 до 5
};

// Элемент расписания
struct ScheduleItem {
    int orderID, opID, resourceID;
    DateTime start, end;
};
