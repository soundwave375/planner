-- 1. Наполнение техпроцессов
INSERT INTO process (name) VALUES 
('Техпроцесс изготовления вала'),
('Техпроцесс изготовления кронштейна'),
('Техпроцесс изготовления корпуса');

-- 2. Наполнение операций
-- Для вала (process_id = 1)
INSERT INTO operation (process_id, name, sequence_number, duration) VALUES 
(1, 'Токарная обработка вала', 1, 30),
(1, 'Фрезерование зубьев', 2, 45),
(1, 'Контроль ОТК вала', 3, 15);

-- Для кронштейна (process_id = 2)
INSERT INTO operation (process_id, name, sequence_number, duration) VALUES 
(2, 'Фрезерование кронштейна', 1, 40),
(2, 'Сверление отверстий', 2, 20),
(2, 'Покраска кронштейна', 3, 60);

-- Для корпуса (process_id = 3)
INSERT INTO operation (process_id, name, sequence_number, duration) VALUES 
(3, 'Расточка корпуса', 1, 90),
(3, 'Шлифование поверхностей', 2, 50);

-- 3. Наполнение ресурсов (станки и люди)
INSERT INTO resource (name, type) VALUES 
('Токарный станок ЧПУ Alpha', 'machine'),   -- id: 1
('Фрезерный станок ЧПУ Beta', 'machine'),   -- id: 2
('Сверлильный станок Gamma', 'machine'),    -- id: 3
('Шлифовальный станок Delta', 'machine'),   -- id: 4
('Иванов И.И. (Токарь)', 'employee'),       -- id: 5
('Петров П.П. (Фрезеровщик)', 'employee'),  -- id: 6
('Сидоров С.С. (Контролер)', 'employee'),   -- id: 7
('Васильев В.В. (Мастер)', 'employee');     -- id: 8

-- 4. Наполнение заказов
-- Заказ 1: Высокий штраф и большой вес
INSERT INTO "order" (process_id, quantity, date_start, deadline, fine, weight) VALUES 
(1, 10, '2026-11-03 08:00:00', '2026-11-04 18:00:00', 500.0, 3.5);

-- Заказ 2: Обычный заказ
INSERT INTO "order" (process_id, quantity, date_start, deadline, fine, weight) VALUES 
(2, 5, '2026-11-03 08:00:00', '2026-11-06 12:00:00', 150.0, 1.0);

-- Заказ 3: Низкий штраф, но высокая важность
INSERT INTO "order" (process_id, quantity, date_start, deadline, fine, weight) VALUES 
(3, 2, '2026-11-03 08:00:00', '2026-11-05 17:00:00', 50.0, 2.0);
