-- 1. Таблица техпроцессов
CREATE TABLE process (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL
);

-- 2. Таблица операций внутри техпроцесса
CREATE TABLE operation (
    id SERIAL PRIMARY KEY,
    process_id INT NOT NULL,
    name VARCHAR(100) NOT NULL,
    sequence_number INT NOT NULL,
    duration INT NOT NULL,
    CONSTRAINT fk_operation_process FOREIGN KEY (process_id) REFERENCES process(id) ON DELETE CASCADE,
    CONSTRAINT chk_operation_duration CHECK (duration > 0)
);

-- 3. Таблица ресурсов (станки и сотрудники)
CREATE TABLE resource (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    type VARCHAR(50) NOT NULL -- 'machine' или 'employee'
);

-- 4. Таблица заказов
CREATE TABLE "order" (
    id SERIAL PRIMARY KEY,
    process_id INT NOT NULL,
    quantity INT NOT NULL,
    date_start TIMESTAMP,
    deadline TIMESTAMP NOT NULL,
    fine REAL NOT NULL DEFAULT 0.0,
    weight REAL NOT NULL DEFAULT 1.0,
    CONSTRAINT fk_order_process FOREIGN KEY (process_id) REFERENCES process(id) ON DELETE RESTRICT,
    CONSTRAINT chk_order_quantity CHECK (quantity > 0)
);

-- 5. Таблица элементов расписания (результат планирования)
CREATE TABLE schedule_item (
    id SERIAL PRIMARY KEY,
    order_id INT NOT NULL,
    operation_id INT NOT NULL,
    employee_id INT NOT NULL,
    machine_id INT NOT NULL,
    start_time TIMESTAMP NOT NULL,
    end_time TIMESTAMP NOT NULL,
    CONSTRAINT fk_schedule_order FOREIGN KEY (order_id) REFERENCES "order"(id) ON DELETE CASCADE,
    CONSTRAINT fk_schedule_operation FOREIGN KEY (operation_id) REFERENCES operation(id) ON DELETE CASCADE,
    CONSTRAINT fk_schedule_employee FOREIGN KEY (employee_id) REFERENCES resource(id) ON DELETE RESTRICT,
    CONSTRAINT fk_schedule_machine FOREIGN KEY (machine_id) REFERENCES resource(id) ON DELETE RESTRICT,
    CONSTRAINT chk_schedule_times CHECK (end_time > start_time)
);
