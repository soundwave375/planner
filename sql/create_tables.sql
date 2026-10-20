-- 1. вид изделия
CREATE TABLE product (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL
);

-- 2. техпроцесс
CREATE TABLE process (
    id SERIAL PRIMARY KEY,
    product_id INT NOT NULL,
    name VARCHAR(100) NOT NULL,
    CONSTRAINT fk_process_product FOREIGN KEY (product_id) REFERENCES product(id) ON DELETE CASCADE
);

-- 3. операции ТП
CREATE TABLE operation (
    id SERIAL PRIMARY KEY,
    process_id INT NOT NULL,
    name VARCHAR(100) NOT NULL,
    sequence_number INT NOT NULL,
    duration INT NOT NULL,
    CONSTRAINT fk_operation_process FOREIGN KEY (process_id) REFERENCES process(id) ON DELETE CASCADE,
    CONSTRAINT chk_operation_duration CHECK (duration > 0)
);

-- 4. ресурс (персонал и оборудование)
CREATE TABLE resource (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    type VARCHAR(50) NOT NULL
);

-- 5. связь операций и необходимых ресурсов
CREATE TABLE operation_resource (
    id SERIAL PRIMARY KEY,
    resource_id INT NOT NULL,
    operation_id INT NOT NULL,
    CONSTRAINT fk_op_res_resource FOREIGN KEY (resource_id) REFERENCES resource(id) ON DELETE CASCADE,
    CONSTRAINT fk_op_res_operation FOREIGN KEY (operation_id) REFERENCES operation(id) ON DELETE CASCADE
);

-- 6. заказы
CREATE TABLE "order" (
    id SERIAL PRIMARY KEY,
    product_id INT NOT NULL,
    quantity INT NOT NULL,
    deadline TIMESTAMP NOT NULL,
    CONSTRAINT fk_order_product FOREIGN KEY (product_id) REFERENCES product(id) ON DELETE RESTRICT,
    CONSTRAINT chk_order_quantity CHECK (quantity > 0)
);
