import os
import csv
from datetime import datetime, timedelta
# Импортируем функции работы с базой данных из соседнего модуля
import database

# --- СТРУКТУРЫ ДАННЫХ (КЛАССЫ) ---

class Resource:
    def __init__(self, id_resource: int, name: str, is_machine: bool):
        self.id_resource = id_resource
        self.name = name
        self.is_machine = is_machine
        self.assigned_intervals = []

    def is_available(self, start_dt: datetime, end_dt: datetime) -> bool:
        for assigned_start, assigned_end in self.assigned_intervals:
            if start_dt < assigned_end and end_dt > assigned_start:
                return False
        return True


class Order:
    def __init__(self, id_order: int, date_start: datetime, deadline: datetime, fine: float, weight: float, tp_id: int):
        self.id_order = id_order
        self.date_start = date_start
        self.deadline = deadline
        self.fine = fine          # Почасовой штраф
        self.weight = weight      # Вес заказа (важность 1-5)
        self.tp_id = tp_id        # ID техпроцесса (в БД это process_id)


class Operation:
    def __init__(self, id_operation: int, name: str, duration: int):
        self.id_operation = id_operation
        self.name = name
        self.duration = duration  # Длительность в часах


class TechnicalProcess:
    def __init__(self, id_tp: int, operation_ids: list):
        self.id_tp = id_tp
        self.operation_ids = operation_ids


class ScheduleItem:
    def __init__(self, order_id: int, operation_id: int, employee_id: int, machine_id: int, start: datetime, end: datetime):
        self.order_id = order_id
        self.operation_id = operation_id
        self.employee_id = employee_id
        self.machine_id = machine_id
        self.start = start
        self.end = end

# --- ПРАВИЛА ПРИОРИТЕТА И ОЦЕНКА ---

def sort_orders(orders: list, rule: str, all_ops: dict, all_tps: dict) -> list:
    """Пункт 15: Сортировка заказов по 5 правилам приоритета."""
    def get_duration(o):
        tp = all_tps.get(o.tp_id)
        if not tp: return 0
        return sum(all_ops[op_id].duration for op_id in tp.operation_ids if op_id in all_ops)

    if rule == "FIFO":
        orders.sort(key=lambda o: o.id_order)
    elif rule == "EDD":
        orders.sort(key=lambda o: o.deadline)
    elif rule == "SPT":
        orders.sort(key=get_duration)
    elif rule == "LPT":
        orders.sort(key=get_duration, reverse=True)
    elif rule == "WSPT":
        orders.sort(key=lambda o: o.weight / get_duration(o) if get_duration(o) > 0 else 0, reverse=True)
        
    return orders


def evaluate_schedule(orders: list, schedule: list):
    """Пункт 17: Оценка критериев эффективности полученного расписания."""
    if not schedule:
        return 0, 0, 0, None
        
    total_weighted_fine = 0
    delayed_orders_count = 0
    max_delay = 0
    
    # Время окончания самой последней операции в системе
    last_operation_end = max(item.end for item in schedule)
    
    for ord in orders:
        order_items = [item for item in schedule if item.order_id == ord.id_order]
        if not order_items: 
            continue
            
        max_end = max(item.end for item in order_items)
        if max_end > ord.deadline:
            delay_hours = int((max_end - ord.deadline).total_seconds() // 3600)
            if delay_hours > 0:
                delayed_orders_count += 1
                # Расчет взвешенного штрафа
                total_weighted_fine += delay_hours * ord.fine * ord.weight
                if delay_hours > max_delay:
                    max_delay = delay_hours
                    
    return total_weighted_fine, delayed_orders_count, max_delay, last_operation_end

# --- АЛГОРИТМ ПЛАНИРОВАНИЯ ---

def build_schedule(orders: list, machines: list, workers: list, all_ops: dict, all_tps: dict, rule: str) -> list:
    """Строит расписание на основе выбранного правила приоритета."""
    # Очищаем интервалы занятости ресурсов перед расчетом
    for m in machines: m.assigned_intervals.clear()
    for w in workers: w.assigned_intervals.clear()

    # Применяем выбранное правило сортировки
    sorted_orders = sort_orders(orders, rule, all_ops, all_tps)

    result = []
    last_machine_idx = 0

    for ord in sorted_orders:
        tp = all_tps.get(ord.tp_id)
        if not tp: continue

        next_s = ord.date_start

        for op_id in tp.operation_ids:
            op = all_ops.get(op_id)
            if not op: continue

            placed = False
            for t_offset in range(1000): # Окно поиска в часах
                current_start = next_s + timedelta(hours=t_offset)
                current_end = current_start + timedelta(hours=op.duration)

                # Перебор станков (Round Robin)
                start_idx = last_machine_idx
                for i in range(len(machines)):
                    idx = (start_idx + i) % len(machines)
                    m = machines[idx]

                    # Перебираем доступных работников
                    for w in workers:
                        if m.is_available(current_start, current_end) and w.is_available(current_start, current_end):
                            # Создаем один элемент расписания
                            result.append(ScheduleItem(
                                order_id=ord.id_order,
                                operation_id=op.id_operation,
                                employee_id=w.id_resource,
                                machine_id=m.id_resource, # Станки опциональны, но используем
                                start=current_start,
                                end=current_end
                            ))
                            
                            # Занимаем интервалы ресурсов
                            m.assigned_intervals.append((current_start, current_end))
                            w.assigned_intervals.append((current_start, current_end))

                            next_s = current_end
                            placed = True
                            last_machine_idx = (idx + 1) % len(machines)
                            break
                    if placed: break
                if placed: break
                    
    return result

# --- ИНТЕРФЕЙС И МЕНЮ УПРАВЛЕНИЯ ---

def main():
    # Автоматически создаем таблицы в PostgreSQL, если их нет
    database.init_db()
    # Читаем CSV-файлы и наполняем пустую базу (без дублирования)
    database.load_csv_data()

    machines, workers, all_ops, all_tps, orders = [], [], {}, {}, []
    current_schedule = []
    active_rule = "FIFO"

    while True:
        print(f"\n=== ПЛАНИРОВЩИК ПРОИЗВОДСТВА (Текущее правило: {active_rule}) ===")
        print("1. Загрузить исходные данные из Базы Данных")
        print("2. Сменить правило приоритета (FIFO, EDD, SPT, LPT, WSPT)")
        print("3. Рассчитать оптимальное расписание и сохранить в БД")
        print("4. Вывести оценку эффективности расписания")
        print("0. Выход")
        
        choice = input("Выберите действие: ")
        
        if choice == "1":
            try:
                machines, workers, all_ops, all_tps, orders = database.load_data_from_db()
                print(f"Данные успешно подтянуты из PostgreSQL! Заказов: {len(orders)}, Ресурсов: {len(machines) + len(workers)}")
            except Exception as e:
                print(f"Ошибка загрузки. Возможно база не создана: {e}")
                
        elif choice == "2":
            rule_input = input("Введите новое правило (FIFO/EDD/SPT/LPT/WSPT): ").upper()
            if rule_input in ["FIFO", "EDD", "SPT", "LPT", "WSPT"]:
                active_rule = rule_input
                print(f"Правило успешно изменено на {active_rule}!")
            else:
                print("Неверное имя правила.")
                
        elif choice == "3":
            if not orders:
                print("Ошибка: Сначала загрузите данные из базы (Пункт 1).")
            else:
                # Считаем расписание
                current_schedule = build_schedule(orders, machines, workers, all_ops, all_tps, active_rule)
                
                # Подготавливаем данные для записи в БД согласно schedule_item
                db_items = []
                for item in current_schedule:
                    db_items.append({
                        'order_id': item.order_id,
                        'operation_id': item.operation_id,
                        'employee_id': item.employee_id,
                        'machine_id': item.machine_id,
                        'start_time': item.start,
                        'end_time': item.end
                    })
                
                # Сохраняем в базу через безопасную транзакцию
                database.save_schedule_transaction(db_items)
                
                # Автоматическая выгрузка в schedule.csv с сортировкой
                db_items.sort(key=lambda x: (x['order_id'], x['start_time']))
                with open('schedule.csv', mode='w', encoding='utf-8', newline='') as f:
                    writer = csv.writer(f)
                    writer.writerow(["order_id", "operation_id", "employee_id", "machine_id", "start_time", "end_time"])
                    for item in db_items:
                        writer.writerow([item['order_id'], item['operation_id'], item['employee_id'], item['machine_id'], item['start_time'], item['end_time']])
                print("Файл schedule.csv успешно обновлен и сохранен!")
                
        elif choice == "4":
            if not current_schedule:
                print("Расписание ещё не рассчитано (Пункт 3).")
            else:
                fine, count, max_d, end_t = evaluate_schedule(orders, current_schedule)
                print("\n--- КРИТЕРИИ ОЦЕНКИ РЕШЕНИЯ ---")
                print(f"Суммарный взвешенный штраф: {fine} руб.")
                print(f"Количество просроченных заказов: {count}")
                print(f"Максимальная просрочка: {max_d} ч.")
                if end_t:
                    print(f"Время окончания всех работ: {end_t.strftime('%d.%m.%Y %H:%M')}")
                    
        elif choice == "0":
            print("Выход из программы.")
            break
        else:
            print("Неверный ввод. Попробуйте снова.")

if __name__ == "__main__":
    main()
