-- 1. Наполнение продуктов
INSERT INTO product (name) VALUES 
('Вал-шестерня'),
('Кронштейн'),
('Корпус редуктора');

-- 2. Наполнение техпроцессов
INSERT INTO process (product_id, name) VALUES 
(1, 'Техпроцесс изготовления вала'),
(2, 'Техпроцесс изготовления кронштейна'),
(3, 'Техпроцесс изготовления корпуса');

-- 3. Наполнение операций
INSERT INTO operation (process_id, name, sequence_number, duration) VALUES 
(1, 'Токарная обработка', 1, 30),
(1, 'Фрезерование зубьев', 2, 45),
(1, 'Контроль ОТК', 3, 15);

-- 4. Наполнение ресурсов (станки и люди)
INSERT INTO resource (name, type) VALUES 
('ЧПУ Alpha', 'Станок'),
('ЧПУ Beta', 'Станок'),
('Иванов И.И. (Токарь)', 'Персонал'),
('Петров П.П. (Фрезеровщик)', 'Персонал'),
('Сидоров С.С. (Контролер)', 'Персонал');

-- 5. Привязка ресурсов к операциям (Вариант: Станок + Человек на операцию)
-- Для операции №1 (Токарная): нужен станок ЧПУ Alpha (1) и Токарь Иванов (3)
INSERT INTO operation_resource (operation_id, resource_id) VALUES (1, 1), (1, 3);
-- Для операции №2 (Фрезерование): нужен станок ЧПУ Beta (2) и Фрезеровщик Петров (4)
INSERT INTO operation_resource (operation_id, resource_id) VALUES (2, 2), (2, 4);
-- Для операции №3 (Контроль): нужен только Контролер Сидоров (5)
INSERT INTO operation_resource (operation_id, resource_id) VALUES (3, 5);

-- 6. Наполнение заказов
INSERT INTO "order" (product_id, quantity, deadline) VALUES 
(1, 10, '2026-10-10 18:00:00'),
(2, 5, '2026-10-12 12:00:00');
