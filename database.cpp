#include "database.h"
#include <pqxx/pqxx>
#include <fstream>
#include <iostream>
#include <sstream>
#include <windows.h>

using namespace std;

Database::Database(const string& conn_str) : connection_string(conn_str) {}

wstring Database::stringToWstring(const string& str) {
    if (str.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buf, len);
    wstring r(buf);
    delete[] buf;
    return r;
}

string Database::wstringToString(const wstring& wstr) {
    if (wstr.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    char* buf = new char[len];
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buf, len, NULL, NULL);
    string r(buf);
    delete[] buf;
    return r;
}

// Парсер даты из формата "YYYY-MM-DD HH:MM" в структуру DateTime
DateTime Database::parseDateTime(const string& datetime_str) {
    DateTime dt = { 2026, 1, 1, 0 };
    sscanf_s(datetime_str.c_str(), "%d-%d-%d %d:", &dt.year, &dt.month, &dt.day, &dt.hour);
    return dt;
}

string Database::dateTimeToString(const DateTime& dt) {
    char buf[64];
    sprintf_s(buf, "%04d-%02d-%02d %02d:00:00", dt.year, dt.month, dt.day, dt.hour);
    return string(buf);
}

// Инициализация структуры таблиц и загрузка CSV
void Database::initFromCSV() {
    try {
        pqxx::connection c(connection_string);

        // ШАГ 1: Сначала БЕЗОПАСНО создаем структуру, если таблиц нет
        {
            pqxx::work tx(c);
            tx.exec(R"(
                CREATE TABLE IF NOT EXISTS product (id SERIAL PRIMARY KEY, name VARCHAR(100) NOT NULL);
                CREATE TABLE IF NOT EXISTS process (id SERIAL PRIMARY KEY, product_id INT NOT NULL, name VARCHAR(100));
                CREATE TABLE IF NOT EXISTS operation (id SERIAL PRIMARY KEY, process_id INT NOT NULL, name VARCHAR(100), sequence_number INT, duration INT);
                CREATE TABLE IF NOT EXISTS resource (id SERIAL PRIMARY KEY, name VARCHAR(100) NOT NULL, type VARCHAR(50));
                CREATE TABLE IF NOT EXISTS operation_resource (id SERIAL PRIMARY KEY, resource_id INT, operation_id INT);
                CREATE TABLE IF NOT EXISTS "order" (id SERIAL PRIMARY KEY, product_id INT, quantity INT, deadline TIMESTAMP, weight INT);
                CREATE TABLE IF NOT EXISTS schedule_item (id SERIAL PRIMARY KEY, order_id INT, operation_id INT, resource_id INT, start_time TIMESTAMP, end_time TIMESTAMP);
            )");
            tx.commit();
        }

        // ШАГ 2: Теперь проверяем, пусты ли таблицы
        bool is_empty = true;
        {
            pqxx::work tx_check(c);
            pqxx::result res = tx_check.exec("SELECT COUNT(*) FROM product");
            if (!res.empty() && res[0][0].as<int>() > 0) {
                is_empty = false; // Данные уже есть, ничего импортировать не надо
            }
            tx_check.commit();
        }

        if (!is_empty) return;

        // ШАГ 3: Если база чистая — заливаем CSV-файлы
        string line;

        // 1. Продукты
        ifstream prod_file("product.csv");
        if (prod_file.is_open()) {
            getline(prod_file, line);
            pqxx::work tx_prod(c);
            while (getline(prod_file, line)) {
                stringstream ss(line); string id, name;
                getline(ss, id, ','); getline(ss, name, ',');
                if (!id.empty()) tx_prod.exec_params("INSERT INTO product (id, name) VALUES ($1, $2)", id, name);
            }
            tx_prod.commit();
        }
        else {
            MessageBoxA(NULL, "Не удалось открыть файл product.csv! Проверьте, что он лежит в папке с .exe", "Ошибка CSV", MB_ICONERROR);
        }

        // 2. Ресурсы
        ifstream res_file("resource.csv");
        if (res_file.is_open()) {
            getline(res_file, line);
            pqxx::work tx_res(c);
            while (getline(res_file, line)) {
                stringstream ss(line); string id, name, type;
                getline(ss, id, ','); getline(ss, name, ','); getline(ss, type, ',');
                if (!id.empty()) tx_res.exec_params("INSERT INTO resource (id, name, type) VALUES ($1, $2, $3)", id, name, type);
            }
            tx_res.commit();
        }

        // 3. Заказы
        ifstream order_file("orders.csv");
        if (order_file.is_open()) {
            getline(order_file, line);
            pqxx::work tx_ord(c);
            while (getline(order_file, line)) {
                stringstream ss(line); string id, prod_id, qty, deadline, weight;
                getline(ss, id, ','); getline(ss, prod_id, ','); getline(ss, qty, ','); getline(ss, deadline, ','); getline(ss, weight, ',');
                if (!id.empty()) tx_ord.exec_params("INSERT INTO \"order\" (id, product_id, quantity, deadline, weight) VALUES ($1, $2, $3, $4, $5)", id, prod_id, qty, deadline, weight);
            }
            tx_ord.commit();
        }

        // 4. Процессы
        ifstream proc_file("process.csv");
        if (proc_file.is_open()) {
            getline(proc_file, line);
            pqxx::work tx_proc(c);
            while (getline(proc_file, line)) {
                stringstream ss(line); string id, prod_id, name;
                getline(ss, id, ','); getline(ss, prod_id, ','); getline(ss, name, ',');
                if (!id.empty()) tx_proc.exec_params("INSERT INTO process (id, product_id, name) VALUES ($1, $2, $3)", id, prod_id, name);
            }
            tx_proc.commit();
        }

        // 5. Операции
        ifstream op_file("operation.csv");
        if (op_file.is_open()) {
            getline(op_file, line);
            pqxx::work tx_op(c);
            while (getline(op_file, line)) {
                stringstream ss(line); string id, proc_id, name, seq, dur;
                getline(ss, id, ','); getline(ss, proc_id, ','); getline(ss, name, ','); getline(ss, seq, ','); getline(ss, dur, ',');
                if (!id.empty()) tx_op.exec_params("INSERT INTO operation (id, process_id, name, sequence_number, duration) VALUES ($1, $2, $3, $4, $5)", id, proc_id, name, seq, dur);
            }
            tx_op.commit();
        }

        // 6. Связи операций и ресурсов
        ifstream op_res_file("operation_resource.csv");
        if (op_res_file.is_open()) {
            getline(op_res_file, line);
            pqxx::work tx_op_res(c);
            while (getline(op_res_file, line)) {
                stringstream ss(line); string res_id, op_id;
                getline(ss, res_id, ','); getline(ss, op_id, ',');
                if (!res_id.empty()) tx_op_res.exec_params("INSERT INTO operation_resource (resource_id, operation_id) VALUES ($1, $2)", res_id, op_id);
            }
            tx_op_res.commit();
        }

        MessageBoxA(NULL, "База данных успешно инициализирована из CSV-файлов!", "Успех", MB_OK);
    }
    catch (const exception& e) {
        // Окно покажет реальную причину, если пароль или имя базы не подходят
        MessageBoxA(NULL, e.what(), "Критическая ошибка БД", MB_ICONERROR);
    }
}


// Загрузка ресурсов в структуры С++ приложения
vector<resource> Database::getResources() {
    vector<resource> result;
    try {
        pqxx::connection c(connection_string);
        pqxx::nontransaction tx(c);
        for (auto row : tx.exec("SELECT id, name, type FROM resource")) {
            int id = row["id"].as<int>();
            wstring name = stringToWstring(row["name"].as<string>());
            wstring type = stringToWstring(row["type"].as<string>());
            bool is_mac = (type == L"Станок");
            result.push_back({ id, name, L"", is_mac, {} });
        }
    }
    catch (...) {}
    return result;
}

vector<order> Database::getOrders() {
    vector<order> result;
    try {
        pqxx::connection c(connection_string);
        pqxx::nontransaction tx(c);
        for (auto row : tx.exec("SELECT id, product_id, quantity, deadline, weight FROM \"order\"")) {
            int id = row["id"].as<int>();
            int prod_id = row["product_id"].as<int>();
            int qty = row["quantity"].as<int>();
            int weight = row["weight"].as<int>();
            DateTime dl = parseDateTime(row["deadline"].as<string>());

            // Задаем базовые даты старта для планирования
            result.push_back({ id, {2025,3,1,10}, {2025,3,5,8}, dl, 5000, prod_id, weight });
        }
    }
    catch (...) {}
    return result;
}

// Сохранение расписания внутри одной транзакции
void Database::saveSchedule(const vector<ScheduleItem>& schedule) {
    try {
        pqxx::connection c(connection_string);
        pqxx::work tx(c); // Начало транзакции

        // полностью удаляем старое расписание
        tx.exec("DELETE FROM schedule_item");

        // записываем элементы нового расчёта
        for (const auto& item : schedule) {
            string s_time = dateTimeToString(item.start);
            string e_time = dateTimeToString(item.end);
            tx.exec_params(
                "INSERT INTO schedule_item (order_id, operation_id, resource_id, start_time, end_time) VALUES ($1, $2, $3, $4, $5)",
                item.orderID, item.opID, item.resourceID, s_time, e_time
            );
        }
        tx.commit(); // если упадет раньше то база откатится назад
    }
    catch (const exception& e) {
        cerr << "Ошибка транзакции! Расписание откатано назад: " << e.what() << endl;
    }
}

vector<operation> Database::getAllOperations() {
    vector<operation> result;
    try {
        pqxx::connection c(connection_string);
        pqxx::nontransaction tx(c);
        // загружаем id операции, имя и длительность
        for (auto row : tx.exec("SELECT id, name, duration FROM operation")) {
            int id = row["id"].as<int>();
            wstring name = stringToWstring(row["name"].as<string>());
            int duration = row["duration"].as<int>();
            result.push_back({ id, name, duration });
        }
    }
    catch (...) {}
    return result;
}

vector<technical_process> Database::getAllProcesses() {
    vector<technical_process> result;
    try {
        pqxx::connection c(connection_string);
        pqxx::nontransaction tx(c);

        // Берем все уникальные ID техпроцессов, которые привязаны к продуктам
        for (auto row_tp : tx.exec("SELECT id FROM process")) {
            int tp_id = row_tp["id"].as<int>();
            vector<int> op_ids;

            // Вытаскиваем все операции для этого техпроцесса, сортируя по их порядку
            pqxx::nontransaction tx_ops(c);
            string q = "SELECT id FROM operation WHERE process_id = " + to_string(tp_id) + " ORDER BY sequence_number";
            for (auto row_op : tx_ops.exec(q)) {
                op_ids.push_back(row_op["id"].as<int>());
            }

            result.push_back({ tp_id, op_ids });
        }
    }
    catch (...) {}
    return result;
}

void Database::exportScheduleToCSV(const string& filename) {
    try {
        pqxx::connection c(connection_string);
        pqxx::nontransaction tx(c);

        // открываем файл на запись в кодировке UTF-8 с BOM
        ofstream file(filename);
        if (!file.is_open()) return;

        file << "\xEF\xBB\xBF"; // UTF-8 BOM
        // Заголовки колонок
        file << "Заказ,Операция,Ресурс,Время начала,Время окончания\n";

        // вытаскиваем упорядоченные данные из базы данных
        string query = R"(
            SELECT s.order_id, s.operation_id, r.name as res_name, s.start_time, s.end_time 
            FROM schedule_item s
            JOIN resource r ON s.resource_id = r.id
            ORDER BY s.order_id, s.start_time
        )";

        for (auto row : tx.exec(query)) {
            file << "Заказ #" << row["order_id"].as<int>() << ","
                << "Операция " << row["operation_id"].as<int>() << ","
                << row["res_name"].as<string>() << ","
                << row["start_time"].as<string>() << ","
                << row["end_time"].as<string>() << "\n";
        }
        file.close();
        cout << "Файл schedule.csv успешно сгенерирован!" << endl;
    }
    catch (const exception& e) {
        cerr << "Ошибка экспорта в CSV: " << e.what() << endl;
    }
}
