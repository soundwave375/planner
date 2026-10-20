import csv
import pg8000
from datetime import datetime

def get_connection():
    # pg8000 принимает параметры как отдельные именованные аргументы
    return pg8000.connect(
        database="planner",
        user="postgres",
        password="123",
        host="127.0.0.1",
        port=5432
    )

def init_db():
    queries = [
        "CREATE TABLE IF NOT EXISTS resource (id SERIAL PRIMARY KEY, name VARCHAR(100) NOT NULL, type VARCHAR(50) NOT NULL);",
        "CREATE TABLE IF NOT EXISTS process (id SERIAL PRIMARY KEY, name VARCHAR(100) NOT NULL);",
        "CREATE TABLE IF NOT EXISTS operation (id SERIAL PRIMARY KEY, process_id INT NOT NULL REFERENCES process(id) ON DELETE CASCADE, name VARCHAR(100) NOT NULL, sequence_number INT NOT NULL, duration INT NOT NULL);",
        'CREATE TABLE IF NOT EXISTS "order" (id SERIAL PRIMARY KEY, process_id INT NOT NULL REFERENCES process(id), quantity INT NOT NULL, date_start TIMESTAMP NOT NULL, deadline TIMESTAMP NOT NULL, fine REAL NOT NULL, weight REAL NOT NULL DEFAULT 1.0);',
        "CREATE TABLE IF NOT EXISTS schedule_item (id SERIAL PRIMARY KEY, order_id INT NOT NULL REFERENCES \"order\"(id) ON DELETE CASCADE, operation_id INT NOT NULL REFERENCES operation(id) ON DELETE CASCADE, employee_id INT NOT NULL REFERENCES resource(id), machine_id INT REFERENCES resource(id), start_time TIMESTAMP NOT NULL, end_time TIMESTAMP NOT NULL);"
    ]
    conn = get_connection()
    with conn.cursor() as cursor:
        for q in queries:
            cursor.execute(q)
    conn.commit()
    conn.close()

def load_csv_data():
    conn = get_connection()
    with conn.cursor() as cursor:
        cursor.execute("SELECT COUNT(*) FROM resource;")
        if cursor.fetchone()[0] > 0:
            conn.close()
            return
    try:
        with open('resources.csv', mode='r', encoding='utf-8') as f:
            for row in csv.DictReader(f):
                with conn.cursor() as cursor:
                    cursor.execute("INSERT INTO resource (id, name, type) VALUES (%s, %s, %s);", (int(row['id']), row['name'], row['type']))
    except: pass
    try:
        with open('products.csv', mode='r', encoding='utf-8') as f:
            for row in csv.DictReader(f):
                with conn.cursor() as cursor:
                    cursor.execute("INSERT INTO process (id, name) VALUES (%s, %s);", (int(row['id']), row['name']))
    except: pass
    try:
        with open('operations.csv', mode='r', encoding='utf-8') as f:
            for row in csv.DictReader(f):
                with conn.cursor() as cursor:
                    cursor.execute("INSERT INTO operation (id, process_id, name, sequence_number, duration) VALUES (%s, %s, %s, %s, %s);", (int(row['id']), int(row['process_id']), row['name'], int(row['sequence_number']), int(row['duration'])))
    except: pass
    try:
        with open('orders.csv', mode='r', encoding='utf-8') as f:
            for row in csv.DictReader(f):
                d_start = datetime.strptime(row['date_start'], '%Y-%m-%d %H:%M')
                d_dead = datetime.strptime(row['deadline'], '%Y-%m-%d %H:%M')
                with conn.cursor() as cursor:
                    cursor.execute('INSERT INTO "order" (id, process_id, quantity, date_start, deadline, fine, weight) VALUES (%s, %s, %s, %s, %s, %s, %s);', (int(row['id']), int(row['process_id']), int(row['quantity']), d_start, d_dead, float(row['fine']), float(row['weight'])))
    except: pass
    conn.commit()
    conn.close()

def load_data_from_db():
    from python_try import Resource as DBResource, Order as DBOrder, Operation as DBOp, TechnicalProcess as DBTP
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT id, name, type FROM resource;")
    machines, workers = [], []
    for r_id, name, r_type in cursor.fetchall():
        if r_type == 'machine': machines.append(DBResource(r_id, name, is_machine=True))
        else: workers.append(DBResource(r_id, name, is_machine=False))
    cursor.execute("SELECT id, process_id, name, duration FROM operation ORDER BY sequence_number;")
    all_ops, all_tps = {}, {}
    for op_id, proc_id, op_name, duration in cursor.fetchall():
        all_ops[op_id] = DBOp(op_id, op_name, duration)
        if proc_id not in all_tps: all_tps[proc_id] = DBTP(proc_id, [])
        all_tps[proc_id].operation_ids.append(op_id)
    cursor.execute("SELECT id, date_start, deadline, fine, weight, process_id FROM \"order\";")
    orders = []
    for o_id, start_dt, deadline_dt, fine, weight, proc_id in cursor.fetchall():
        # Обеспечиваем совместимость типов дат для pg8000
        s_dt = start_dt if isinstance(start_dt, datetime) else datetime.strptime(str(start_dt)[:16], '%Y-%m-%d %H:%M')
        d_dt = deadline_dt if isinstance(deadline_dt, datetime) else datetime.strptime(str(deadline_dt)[:16], '%Y-%m-%d %H:%M')
        orders.append(DBOrder(o_id, s_dt, d_dt, fine, weight, proc_id))
    cursor.close()
    conn.close()
    return machines, workers, all_ops, all_tps, orders

def save_schedule_transaction(schedule_items_list):
    conn = get_connection()
    try:
        with conn:
            with conn.cursor() as cursor:
                cursor.execute("TRUNCATE schedule_item CASCADE;")
                for item in schedule_items_list:
                    cursor.execute(
                        "INSERT INTO schedule_item (order_id, operation_id, employee_id, machine_id, start_time, end_time) VALUES (%s, %s, %s, %s, %s, %s);", 
                        (item['order_id'], item['operation_id'], item['employee_id'], item['machine_id'], item['start_time'], item['end_time'])
                    )
                print("Успешно! Оптимальное расписание сохранено в PostgreSQL.")
    except Exception as e:
        print("Ошибка транзакции:", e)
