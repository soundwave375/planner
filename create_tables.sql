CREATE TABLE resources (
	id SERIAL PRIMARY KEY,
	name VARCHAR(100) NOT NULL,
	type VARCHAR(50) NOT NULL
);

CREATE TABLE products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL
);

CREATE TABLE operations (
    id SERIAL PRIMARY KEY,
    product_id INT NOT NULL REFERENCES products(id) ON DELETE CASCADE,
    resource_id INT NOT NULL REFERENCES resources(id),
    name VARCHAR(100) NOT NULL,
    sequence_number INT NOT NULL,
    duration INT NOT NULL CHECK (duration > 0)
);

CREATE TABLE orders (
    id SERIAL PRIMARY KEY,
    product_id INT NOT NULL REFERENCES products(id),
    quantity INT NOT NULL CHECK (quantity > 0),
    deadline TIMESTAMP NOT NULL
);

CREATE TABLE schedule (
    id SERIAL PRIMARY KEY,
    order_id INT NOT NULL REFERENCES orders(id) ON DELETE CASCADE,
    operation_id INT NOT NULL REFERENCES operations(id),
    start_time TIMESTAMP NOT NULL,
    end_time TIMESTAMP NOT NULL,
    CONSTRAINT check_dates CHECK (end_time > start_time)
);