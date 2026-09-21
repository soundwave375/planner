#include <windows.h>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <cwchar>

// Идентификаторы
#define ID_BTN_BUILD       1001
#define ID_LISTBOX         1002
#define ID_EDIT_NAME       1004
#define ID_BTN_ADD_MACHINE 1009
#define ID_BTN_ADD_WORKER  1010
#define ID_BTN_ADD_ORDER   1005
#define ID_EDIT_START      1006
#define ID_EDIT_DEADLINE   1007
#define ID_EDIT_FINE       1008
#define ID_BTN_ADD_TP      1011
#define ID_EDIT_TP_ID      1012
#define ID_EDIT_TP_OPS     1013
#define ID_COMBO_FILTER    1014
#define ID_BTN_FILTER      1015

using namespace std;

// Структуры
struct DateTime { int year, month, day, hour; };
struct TimeInterval { DateTime start, end; wstring reason; };
struct order { int ID_order; DateTime order_date, date_start, deadline; int fine, TP_ID; };
struct operation { int ID_operation; wstring name; int duration; };
struct technical_process { int ID_tp; vector<int> operation_ids; };
struct resource { int ID_resource; wstring name1; wstring name2; bool is_machine; vector<TimeInterval> assigned_intervals; };
struct ScheduleItem { int orderID, opID, resourceID; DateTime start, end; };

// Глобальные данные
vector<resource> machines = { {101, L"ЧПУ", L"Alpha", true} };
vector<resource> workers = { {1001, L"Иванов", L"И.", false} };
vector<order> orders = { {1, {2025,3,1,10}, {2025,3,5,8}, {2025,3,10,18}, 50000, 500} };
vector<operation> allOps = { {10, L"Этап 1", 3}, {11, L"Этап 2", 2}, {12, L"Этап 3", 1} };
vector<technical_process> allTPs = { {500, {10, 11, 12}} };
vector<ScheduleItem> lastResult;
int g_filterOrderId = 0;
HWND hListBox, hEditName, hComboFilter;

// Вспомогательные функции
const int daysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
long long toTotalHours(const DateTime& d) {
    long long total = (long long)d.year * 8760;
    for (int i = 1; i < d.month; ++i) total += daysInMonth[i] * 24;
    total += (long long)d.day * 24 + d.hour;
    return total;
}
DateTime fromTotalHours(long long totalHours) {
    DateTime d;
    d.year = (int)(totalHours / 8760);
    long long rem = totalHours % 8760;
    d.month = 1;
    for (int i = 1; i <= 12; ++i) {
        int h = daysInMonth[i] * 24;
        if (rem < h) { d.month = i; break; }
        rem -= h;
    }
    d.day = (int)(rem / 24);
    if (d.day == 0) d.day = 1;
    d.hour = (int)(rem % 24);
    return d;
}
bool isResourceAvailable(const resource& res, DateTime start, DateTime end) {
    long long s1 = toTotalHours(start), e1 = toTotalHours(end);
    for (const auto& interval : res.assigned_intervals) {
        if (s1 < toTotalHours(interval.end) && e1 > toTotalHours(interval.start)) return false;
    }
    return true;
}

// Построение расписания
static void buildSchedule(vector<order>& orders, vector<resource>& machines, vector<resource>& workers,
    vector<operation>& allOps, vector<technical_process>& allTPs, vector<ScheduleItem>& result)
{
    sort(orders.begin(), orders.end(),
        [](const order& a, const order& b) {
            long long tA = toTotalHours(a.order_date);
            long long tB = toTotalHours(b.order_date);
            if (tA != tB) return tA < tB;
            return a.fine > b.fine;
        });

    result.clear();
    for (auto& m : machines) m.assigned_intervals.clear();
    for (auto& w : workers) w.assigned_intervals.clear();

    static size_t lastMachineIdx = 0;
    lastMachineIdx = 0;

    for (auto& ord : orders) {
        technical_process* tpPtr = nullptr;
        for (auto& tp : allTPs) if (tp.ID_tp == ord.TP_ID) tpPtr = &tp;
        if (!tpPtr) continue;

        long long nextS = toTotalHours(ord.date_start);
        for (int opID : tpPtr->operation_ids) {
            operation* opPtr = nullptr;
            for (auto& op : allOps) if (op.ID_operation == opID) opPtr = &op;
            if (!opPtr) continue;

            bool placed = false;
            for (long long t = nextS; t < nextS + 500LL; ++t) {
                DateTime dtS = fromTotalHours(t), dtE = fromTotalHours(t + opPtr->duration);

                size_t startIdx = lastMachineIdx;
                for (size_t i = 0; i < machines.size(); ++i) {
                    size_t idx = (startIdx + i) % machines.size();
                    auto& m = machines[idx];

                    for (auto& w : workers) {
                        if (isResourceAvailable(m, dtS, dtE) && isResourceAvailable(w, dtS, dtE)) {
                            result.push_back({ ord.ID_order, opPtr->ID_operation, m.ID_resource, dtS, dtE });
                            result.push_back({ ord.ID_order, opPtr->ID_operation, w.ID_resource, dtS, dtE });
                            m.assigned_intervals.push_back({ dtS, dtE, L"Job" });
                            w.assigned_intervals.push_back({ dtS, dtE, L"Job" });

                            nextS = t + opPtr->duration;
                            placed = true;
                            lastMachineIdx = (idx + 1) % machines.size();
                            break;
                        }
                    }
                    if (placed) break;
                }
                if (placed) break;
            }
        }
    }
}

wstring GetResourceNameByID(int id) {
    for (const auto& m : machines) {
        if (m.ID_resource == id) {
            wstring display = m.name1;
            if (!m.name2.empty()) display += L" " + m.name2;
            return display;
        }
    }
    for (const auto& w : workers) {
        if (w.ID_resource == id) {
            wstring display = w.name1;
            if (!w.name2.empty()) display += L" " + w.name2;
            return display;
        }
    }
    return L"R" + to_wstring(id);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    static int scrollPos = 0;
    static HWND hEditStart = NULL, hEditDl = NULL, hEditFine = NULL;
    static HWND hEditTpId = NULL, hEditTpOps = NULL;
    static HWND hEditNewOpId, hEditNewOpName, hEditNewOpDur;

    switch (msg) {
    
    case WM_VSCROLL:
    {
        SCROLLINFO si = { sizeof(si), SIF_ALL };
        GetScrollInfo(hwnd, SB_VERT, &si);
        int oldPos = si.nPos;

        switch (LOWORD(wp)) {
        case SB_LINEUP: si.nPos -= 30; break;
        case SB_LINEDOWN: si.nPos += 30; break;
        case SB_THUMBTRACK: si.nPos = HIWORD(wp); break;
        }

        si.fMask = SIF_POS;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        GetScrollInfo(hwnd, SB_VERT, &si);

        if (si.nPos != oldPos) {
            scrollPos = si.nPos;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }
    
    case WM_CREATE:
    {
        SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS };
        si.nMin = 0;
        si.nMax = 1500; // Общая высота контента
        si.nPage = 600; // Размер видимой области
        si.nPos = 0;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE); 
        
        CreateWindow(L"BUTTON", L"Рассчитать", WS_CHILD | WS_VISIBLE,
            20, 20, 120, 30, hwnd, (HMENU)ID_BTN_BUILD, NULL, NULL);

        hListBox = CreateWindow(L"LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
            20, 60, 540, 150, hwnd, (HMENU)ID_LISTBOX, NULL, NULL);

        CreateWindow(L"STATIC", L"Имя ресурса:", WS_CHILD | WS_VISIBLE,
            20, 225, 100, 20, hwnd, NULL, NULL, NULL);
        hEditName = CreateWindow(L"EDIT", L"Новый ресурс",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            120, 225, 150, 20, hwnd, NULL, NULL, NULL);
        CreateWindow(L"BUTTON", L"Добавить станок", WS_CHILD | WS_VISIBLE,
            280, 220, 130, 30, hwnd, (HMENU)ID_BTN_ADD_MACHINE, NULL, NULL);
        CreateWindow(L"BUTTON", L"Добавить работника", WS_CHILD | WS_VISIBLE,
            420, 220, 150, 30, hwnd, (HMENU)ID_BTN_ADD_WORKER, NULL, NULL);

        CreateWindow(L"STATIC", L"Старт(час):", WS_CHILD | WS_VISIBLE,
            20, 310, 80, 20, hwnd, NULL, NULL, NULL);
        hEditStart = CreateWindow(L"EDIT", L"8",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            100, 310, 50, 20, hwnd, (HMENU)ID_EDIT_START, NULL, NULL);
        CreateWindow(L"STATIC", L"Дедлайн(ч):", WS_CHILD | WS_VISIBLE,
            160, 310, 80, 20, hwnd, NULL, NULL, NULL);
        hEditDl = CreateWindow(L"EDIT", L"18",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            240, 310, 50, 20, hwnd, (HMENU)ID_EDIT_DEADLINE, NULL, NULL);
        CreateWindow(L"STATIC", L"Штраф:", WS_CHILD | WS_VISIBLE,
            300, 310, 60, 20, hwnd, NULL, NULL, NULL);
        hEditFine = CreateWindow(L"EDIT", L"5000",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            360, 310, 80, 20, hwnd, (HMENU)ID_EDIT_FINE, NULL, NULL);
        CreateWindow(L"BUTTON", L"Добавить заказ", WS_CHILD | WS_VISIBLE,
            450, 305, 120, 30, hwnd, (HMENU)ID_BTN_ADD_ORDER, NULL, NULL);
        CreateWindow(L"STATIC", L"Выбрать ТП:", WS_CHILD | WS_VISIBLE,
            20, 355, 80, 20, hwnd, NULL, NULL, NULL);
        HWND hComboOrderTP = CreateWindow(L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            110, 355, 80, 200, hwnd, (HMENU)1020, NULL, NULL);

        SendMessage(hComboOrderTP, CB_ADDSTRING, 0, (LPARAM)L"500");
        SendMessage(hComboOrderTP, CB_SETCURSEL, 0, 0);

        CreateWindow(L"STATIC", L"ID Оп:", WS_CHILD | WS_VISIBLE,
            20, 395, 40, 20, hwnd, NULL, NULL, NULL);
        CreateWindow(L"EDIT", L"13", WS_CHILD | WS_VISIBLE | WS_BORDER,
            65, 395, 35, 20, hwnd, (HMENU)1030, NULL, NULL);
        CreateWindow(L"STATIC", L"Имя:", WS_CHILD | WS_VISIBLE,
            110, 395, 35, 20, hwnd, NULL, NULL, NULL);
        CreateWindow(L"EDIT", L"Сборка", WS_CHILD | WS_VISIBLE | WS_BORDER,
            150, 395, 80, 20, hwnd, (HMENU)1031, NULL, NULL);
        CreateWindow(L"STATIC", L"Длит:", WS_CHILD | WS_VISIBLE,
            240, 395, 40, 20, hwnd, NULL, NULL, NULL);
        CreateWindow(L"EDIT", L"2", WS_CHILD | WS_VISIBLE | WS_BORDER,
            285, 395, 30, 20, hwnd, (HMENU)1032, NULL, NULL);
        CreateWindow(L"BUTTON", L"Создать ОП", WS_CHILD | WS_VISIBLE,
            330, 390, 100, 30, hwnd, (HMENU)1033, NULL, NULL);

        CreateWindow(L"STATIC", L"ID нов. ТП:", WS_CHILD | WS_VISIBLE,
            20, 435, 80, 20, hwnd, NULL, NULL, NULL);
        hEditTpId = CreateWindow(L"EDIT", L"501",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            110, 435, 50, 20, hwnd, (HMENU)ID_EDIT_TP_ID, NULL, NULL);
        CreateWindow(L"STATIC", L"Опер. (ID,ID):", WS_CHILD | WS_VISIBLE,
            170, 435, 100, 20, hwnd, NULL, NULL, NULL);
        hEditTpOps = CreateWindow(L"EDIT", L"10,11,12",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            275, 435, 155, 20, hwnd, (HMENU)ID_EDIT_TP_OPS, NULL, NULL);
        CreateWindow(L"BUTTON", L"Добавить ТП", WS_CHILD | WS_VISIBLE,
            440, 430, 110, 30, hwnd, (HMENU)ID_BTN_ADD_TP, NULL, NULL);

        CreateWindow(L"STATIC", L"Фильтр по:", WS_CHILD | WS_VISIBLE,
            20, 475, 80, 20, hwnd, NULL, NULL, NULL);
        hComboFilter = CreateWindow(L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            110, 475, 100, 100, hwnd, (HMENU)ID_COMBO_FILTER, NULL, NULL);
        break;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wp) == CBN_SELCHANGE && LOWORD(wp) == ID_COMBO_FILTER) {
            int sel = (int)SendMessage(hComboFilter, CB_GETCURSEL, 0, 0);
            if (sel == 0) {
                g_filterOrderId = 0;
            }
            else {
                wchar_t text[64];
                SendMessage(hComboFilter, CB_GETLBTEXT, sel, (LPARAM)text);
                int id = 0;
                swscanf_s(text, L"Заказ #%d", &id);
                g_filterOrderId = id;
            }
            InvalidateRect(hwnd, NULL, TRUE);
        }
        if (LOWORD(wp) == ID_BTN_ADD_MACHINE) {
            wchar_t buf[64];
            GetWindowText(hEditName, buf, 64);
            wstring name = buf;
            if (name.empty() || name.find_first_not_of(L' ') == wstring::npos) name = L"Станок";
            // Генерация уникального ID
            int newID = 200;
            for (auto& m : machines) if (m.ID_resource >= newID) newID = m.ID_resource + 1;
            machines.push_back({ newID, name, L"", true });
            MessageBox(hwnd, L"Станок добавлен!", L"OK", MB_OK);
        }
        else if (LOWORD(wp) == ID_BTN_ADD_WORKER) {
            wchar_t buf[64];
            GetWindowText(hEditName, buf, 64);
            wstring name = buf;
            if (name.empty() || name.find_first_not_of(L' ') == wstring::npos) name = L"Работник";
            // Генерация уникального ID
            int newID = 1000;
            for (auto& w : workers) if (w.ID_resource >= newID) newID = w.ID_resource + 1;
            workers.push_back({ newID, name, L"", false });
            MessageBox(hwnd, L"Работник добавлен!", L"OK", MB_OK);
        }
        else if (LOWORD(wp) == ID_BTN_ADD_ORDER) {
            wchar_t bStart[10], bDl[10], bFine[20];
            GetWindowText(hEditStart, bStart, 10);
            GetWindowText(hEditDl, bDl, 10);
            GetWindowText(hEditFine, bFine, 20);
            int startHour = _wtoi(bStart);
            int deadlineHour = _wtoi(bDl);
            int fineVal = _wtoi(bFine);
            HWND hComboTP = GetDlgItem(hwnd, 1020);
            int sel = (int)SendMessage(hComboTP, CB_GETCURSEL, 0, 0);
            wchar_t tpBuf[16];
            SendMessage(hComboTP, CB_GETLBTEXT, sel, (LPARAM)tpBuf);
            int selectedTP = _wtoi(tpBuf);

            int newID = (int)orders.size() + 1;
            orders.push_back({ newID, {2025,3,1,10}, {2025,3,5,startHour}, {2025,3,5,deadlineHour}, fineVal, selectedTP });

            MessageBox(hwnd, (L"Заказ #" + to_wstring(newID) + L" добавлен").c_str(), L"OK", MB_OK);
        }
        else if (LOWORD(wp) == 1033) {
            wchar_t bufId[16], bufName[64], bufDur[16];

            GetWindowText(GetDlgItem(hwnd, 1030), bufId, 16);
            GetWindowText(GetDlgItem(hwnd, 1031), bufName, 64);
            GetWindowText(GetDlgItem(hwnd, 1032), bufDur, 16);

            int id = _wtoi(bufId);
            int dur = _wtoi(bufDur);

            if (id > 0 && dur > 0) {
                allOps.push_back({ id, bufName, dur });
                MessageBox(hwnd, L"Новая операция успешно зарегистрирована!", L"Успех", MB_OK);
            }
            else {
                MessageBox(hwnd, L"Введите корректные ID и длительность!", L"Ошибка", MB_ICONERROR);
            }
        }

        else if (LOWORD(wp) == ID_BTN_ADD_TP) {
            wchar_t bufId[16], bufOps[512];
            GetWindowText(hEditTpId, bufId, 16);
            GetWindowText(hEditTpOps, bufOps, 512);
            int tpId = _wtoi(bufId);
            if (tpId == 0) tpId = 1000 + (int)allTPs.size();
            vector<int> opIds;
            wchar_t* ctx = nullptr;
            wchar_t* token = wcstok_s(bufOps, L",", &ctx);
            while (token) {
                int op = _wtoi(token);
                if (op > 0) {
                    opIds.push_back(op);
                    bool found = false;
                    for (auto& existing : allOps) if (existing.ID_operation == op) found = true;
                    if (!found) {
                        allOps.push_back({ op, L"Этап " + to_wstring(op), 2 });
                    }
                }
                token = wcstok_s(nullptr, L",", &ctx);
            }

            allTPs.push_back({ tpId, opIds });

            HWND hComboTP = GetDlgItem(hwnd, 1020);
            SendMessage(hComboTP, CB_ADDSTRING, 0, (LPARAM)to_wstring(tpId).c_str());

            MessageBox(hwnd, L"Техпроцесс добавлен!", L"OK", MB_OK);
        }
        else if (LOWORD(wp) == ID_BTN_FILTER) {
            int sel = (int)SendMessage(hComboFilter, CB_GETCURSEL, 0, 0);
            if (sel == CB_ERR) sel = 0;
            if (sel == 0) g_filterOrderId = 0;
            else {
                wchar_t text[64];
                SendMessage(hComboFilter, CB_GETLBTEXT, sel, (LPARAM)text);
                int id = 0;
                swscanf_s(text, L"Заказ #%d", &id);
                g_filterOrderId = id;
            }
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (LOWORD(wp) == ID_BTN_BUILD) {
            SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
            buildSchedule(orders, machines, workers, allOps, allTPs, lastResult);

            for (auto& i : lastResult) {
                wstring resName = GetResourceNameByID(i.resourceID);
                wstring s = to_wstring(i.start.day) + L"." + to_wstring(i.start.month) + L" | " +
                    resName + L" | Заказ #" + to_wstring(i.orderID) +
                    L" | " + to_wstring(i.start.hour) + L":00 - " +
                    to_wstring(i.end.hour) + L":00";
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)s.c_str());
            }

            // Обновление комбобокса фильтра
            SendMessage(hComboFilter, CB_RESETCONTENT, 0, 0);
            SendMessage(hComboFilter, CB_ADDSTRING, 0, (LPARAM)L"Все");
            for (auto& o : orders) {
                wstring s = L"Заказ #" + to_wstring(o.ID_order);
                SendMessage(hComboFilter, CB_ADDSTRING, 0, (LPARAM)s.c_str());
            }
            SendMessage(hComboFilter, CB_SETCURSEL, 0, 0);
            g_filterOrderId = 0;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HBRUSH brushMac = CreateSolidBrush(RGB(100, 150, 255));
        HBRUSH brushWork = CreateSolidBrush(RGB(100, 200, 100));
        HBRUSH brushFine = CreateSolidBrush(RGB(255, 100, 100));

        int y = 550 - scrollPos;
        int cnt = 0;

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        HRGN hRgn = CreateRectRgn(0, 520, clientRect.right, clientRect.bottom);
        SelectClipRgn(hdc, hRgn);

        //Отрисовка станков
        TextOut(hdc, 20, y, L"Станки:", 7);
        y += 25;
        for (size_t i = 0; i < lastResult.size(); ++i) {
            if (g_filterOrderId != 0 && lastResult[i].orderID != g_filterOrderId) continue;
            bool isMac = false;
            for (auto& m : machines) if (m.ID_resource == lastResult[i].resourceID) { isMac = true; break; }
            if (!isMac) continue;

            int x1 = 120 + (lastResult[i].start.hour * 15);
            int x2 = 120 + (lastResult[i].end.hour * 15);
            RECT r = { x1, y + cnt * 20, x2, y + cnt * 20 + 15 };
            FillRect(hdc, &r, brushMac);
            wstring name = GetResourceNameByID(lastResult[i].resourceID);
            TextOut(hdc, 10, y + cnt * 20, name.c_str(), (int)name.length());
            cnt++;
        }

        //Отрисовка работников
        y += (cnt + 1) * 20;
        TextOut(hdc, 20, y - 20, L"Работники:", 10);
        int cntW = 0;
        for (size_t i = 0; i < lastResult.size(); ++i) {
            if (g_filterOrderId != 0 && lastResult[i].orderID != g_filterOrderId) continue;
            bool isWork = false;
            for (auto& w : workers) if (w.ID_resource == lastResult[i].resourceID) { isWork = true; break; }
            if (!isWork) continue;

            int x1 = 120 + (lastResult[i].start.hour * 15);
            int x2 = 120 + (lastResult[i].end.hour * 15);
            RECT r = { x1, y + cntW * 20, x2, y + cntW * 20 + 15 };
            FillRect(hdc, &r, brushWork);
            wstring name = GetResourceNameByID(lastResult[i].resourceID);
            TextOut(hdc, 10, y + cntW * 20, name.c_str(), (int)name.length());
            cntW++;
        }

        //Отдельная диаграмма штрафов
        y += (cntW + 2) * 20;
        TextOut(hdc, 20, y - 20, L"Штрафы (почасовая ставка):", 26);
        long long totalFine = 0;
        int fineRow = 0;

        for (const auto& ord : orders) {
            long long maxEnd = 0;
            for (const auto& res : lastResult) {
                if (res.orderID == ord.ID_order) {
                    long long endT = toTotalHours(res.end);
                    if (endT > maxEnd) maxEnd = endT;
                }
            }

            long long dlTime = toTotalHours(ord.deadline);
            long long currentFine = 0;

            if (maxEnd > dlTime) {
                long long hoursDelayed = maxEnd - dlTime;
                currentFine = hoursDelayed * ord.fine;
                totalFine += currentFine;
            }

            int barX = 250; 
            int barWidth = (int)(currentFine / 2000);
            RECT rFine = { barX, y + fineRow * 20, barX + barWidth, y + fineRow * 20 + 15 };
            if (currentFine > 0) {
                FillRect(hdc, &rFine, brushFine);
            }

            long long delay = (maxEnd > dlTime) ? (maxEnd - dlTime) : 0;
            wstring fineInfo = L"Заказ #" + to_wstring(ord.ID_order) +
                L" [+" + to_wstring(delay) + L"ч]: " + to_wstring(currentFine);

            TextOut(hdc, 10, y + fineRow * 20, fineInfo.c_str(), (int)fineInfo.length());
            fineRow++;
        }

        y += (fineRow + 1) * 20;
        wstring totalStr = L"ИТОГО К ВЫПЛАТЕ: " + to_wstring(totalFine);
        TextOut(hdc, 20, y, totalStr.c_str(), (int)totalStr.length());

        DeleteObject(brushMac);
        DeleteObject(brushWork);
        DeleteObject(brushFine);
        SelectClipRgn(hdc, NULL);
        DeleteObject(hRgn);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE h, HINSTANCE, LPSTR, int n) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = h;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"Sched";
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(L"Sched", L"Планировщик 2.0", WS_OVERLAPPEDWINDOW | WS_VSCROLL,
        100, 100, 620, 900, NULL, NULL, h, NULL);
    ShowWindow(hwnd, n);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}