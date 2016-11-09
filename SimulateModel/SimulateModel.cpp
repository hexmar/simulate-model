#include "stdafx.h"

static const unsigned int SIM_TIME = 80 * 60; //Длительность моделирования

struct Mean { //Структура для нахождения среднего числа
	float sum, meanVal;
	unsigned int count; //Количество слагаемых
	bool updated; //Посчитано ли среднее значение
	Mean() : sum(0), meanVal(0), count(0), updated(true) {};
	void add(float val) { //Добавление слагаемого; val - значение нового слагаемого
		sum += val;
		count++;
		updated = false;
	};
	float mean() { //Нахождение среднего
		if (!updated) meanVal = sum / count; //Если среденее не посчитано, то вычисляем его
		updated = true;
		return meanVal;
	};
};

struct App { //Структура, описывающую заявку
	unsigned __int8 type; //Класс заявки: 1 - A, 2 - B, 3 - C
	unsigned int time; //Время нахождения заявки в очереди
	App(unsigned __int8 t) : type(t), time(0) {};
};

struct Queue { //Структура, описывающая очередь
	struct Entry { //Структура, описывающая элемент очереди
		App *ptr;
		Entry *next, *prev;
		Entry(App *ptrIn, Entry* nextIn, Entry* prevIn)
			: ptr(ptrIn), next(nextIn), prev(prevIn) {};
	} *start, *end; //Указатели на конец и начало очереди
	unsigned int curLen; //Текущая длина очереди
	Mean length, time; //Средняя длина очереди и среднее время ожидания в очереди
	Queue() : curLen(0), start(nullptr), end(nullptr) {};
	void push(App *ptr) { //Поставить "заявку" в очередь; ptr - указатель на "заявку"
		if (start == nullptr)
			start = end = new Entry(ptr, nullptr, nullptr);
		else
			start = start->prev = new Entry(ptr, start, nullptr);
		curLen++;
	};
	App *pull() { //Извлечь заявку из очереди
		if (start != nullptr) {
			App *ret(end->ptr);
			Entry *tmp(end);
			if (start == end) {
				start = end = nullptr;
			}
			else {
				end = end->prev;
				end->next = nullptr;
			}
			delete tmp;
			curLen--;
			if (ret->time != 0)
				time.add(ret->time);
			return ret;
		}
		else
			return nullptr;
	};
	void step() { //Увеличение времени ожидания каждого элемента очереди
		Entry *iter(start);
		while (iter != nullptr) {
			iter->ptr->time++;
			iter = iter->next;
		}
	};
};

struct CPU { //Структура, описывающая обслуживание
	unsigned __int8 state; //0(00) - свободен, 1(01) - A, 2(10) - B, 3(11) - C ИЛИ A И B
	Mean busy, time; //Занятость, среднее время обслуживания
	CPU() : state(0) {};
};

struct Event { //Стурктура, описывающая событие
	char type; //Тип события: n - поступление заявки, c - завершение обслуживания заявки
	int param; //Сопровождающий параметр
	Event(char typeIn, int paramIn) : type(typeIn), param(paramIn) {};
};

struct NodeQueue { //Структура, описывающая календарь событий
	struct QueueNode {
		QueueNode *next, *prev;
		Event *ev;
		unsigned int inform;
		QueueNode(Event *ptr, unsigned int inf, QueueNode *n, QueueNode *p)
			: next(n), prev(p), ev(ptr), inform(inf) {};
	} *start, *end;
	NodeQueue() : start(nullptr), end(nullptr) {};
	void push(Event *ptr, unsigned int inf) { //Поставить "событие" в очередь
		if (start == nullptr)
			start = end = new QueueNode(ptr, inf, nullptr, nullptr);
		else {
			if (start == end)
				if (start->inform > inf)
					end = end->next = new QueueNode(ptr, inf, nullptr, end);
				else
					start = start->prev = new QueueNode(ptr, inf, start, nullptr);
			else {
				QueueNode *iter(start);
				while ((iter != nullptr) && (iter->inform > inf))
					iter = iter->next;
				if (iter == nullptr)
					end = end->next = new QueueNode(ptr, inf, nullptr, end);
				else
					if (iter != start)
						iter->prev = iter->prev->next = new QueueNode(ptr, inf, iter, iter->prev);
					else
						start = start->prev = new QueueNode(ptr, inf, start, nullptr);
			}
		}
	};
	Event *pull() { //Извлечь "событие" из очереди
		if (start != nullptr) {
			Event *ret(end->ev);
			QueueNode *tmp(end);
			if (start == end)
				end = start = nullptr;
			else {
				end = end->prev;
				end->next = nullptr;
			}
			delete tmp;
			return ret;
		}
		else
			return nullptr;
	};
	bool notEmpty() { //Не пуста ли очередь
		if (start == nullptr)
			return false;
		else
			return true;
	};
	float meanTime() { //Среднее время между событиями
		QueueNode *iter(end);
		unsigned int count(0), prevTime(0);
		Mean mean;
		while (iter != nullptr) {
			mean.add(iter->inform - prevTime);
			prevTime = iter->inform;
			iter = iter->prev;
		}
		return mean.mean();
	};
};

int main()
{
	setlocale(LC_CTYPE, "Russian");
	NodeQueue que; //Календарь событий
	Queue queue, queue1; //Очередь заявок
	CPU proc, proc1; //Обслуживание заявок
	unsigned int time(0); //Текущее время
	Event *ev(nullptr);
	{ //Генерация заявок
		unsigned int tmp(0), prev(0);
		while (true) { //Класса A
			rand_s(&tmp);
			tmp = (tmp % 11) + 15 + prev;
			if (tmp >= SIM_TIME) break;
			prev = tmp;
			ev = new Event('n', 1);
			que.push(ev, tmp);
		}
		prev = tmp = 0;
		while (true) { //Класса B
			rand_s(&tmp);
			tmp = (tmp % 21) + 10 + prev;
			if (tmp >= SIM_TIME) break;
			prev = tmp;
			ev = new Event('n', 2);
			que.push(ev, tmp);
		}
		prev = tmp = 0;
		while (true) { //Класса C
			rand_s(&tmp);
			tmp = (tmp % 21) + 20 + prev;
			if (tmp >= SIM_TIME) break;
			prev = tmp;
			ev = new Event('n', 3);
			que.push(ev, tmp);
		}
	}
	float ro(que.meanTime()); //Средний интервал поступления заявок
	App *app(nullptr);
	while (time <= SIM_TIME) { //Основной цикл
		if (que.notEmpty())
			if (que.end->inform == time) { //Время события, стоящего первым в очереди, совпадает с текущим time
				ev = que.pull();
				switch (ev->type) { //Обработать в зависимости от типа
				case 'n':
					app = new App(ev->param);
					if (ev->param == 3)
						queue1.push(app);
					else
						queue.push(app);
					printf_s("%d\tПоступление заявки типа: %c\n", time, 'a' + ev->param - 1);
					break;
				case 'c':
					proc.state = proc.state - ev->param;
					printf_s("%d\tЗавершение обработки заявки типа: %c\n", time, 'a' + ev->param - 1);
					break;
				case 'p':
					proc1.state = proc1.state - ev->param;
					printf_s("%d\tЗавершение обработки заявки типа: %c\n", time, 'a' + ev->param - 1);
					break;
				}
				delete ev;
				while (queue.curLen != 0) { //Проверить возможность обслуживания
					unsigned __int8 desiredCPUState(queue.end->ptr->type);
					if ((proc.state & desiredCPUState) == 0) { //На основе побитового И
						unsigned int cpuTime;
						app = queue.pull();
						proc.state = proc.state + desiredCPUState;
						printf_s("%d\tНачало обработки заявки типа: %c\n", time, 'a' + desiredCPUState - 1);
						rand_s(&cpuTime); //Генерация времени обслуживания в зависимости от класса заявки
						switch (desiredCPUState) {
						case 1:
							cpuTime = (cpuTime % 11) + 15;
							break;
						case 2:
							cpuTime = (cpuTime % 7) + 18;
							break;
						case 3:
							cpuTime = (cpuTime % 11) + 23;
						}
						proc.time.add(cpuTime); //Набор данных о времени обслуживания
						ev = new Event('c', desiredCPUState);
						que.push(ev, time + cpuTime);
						delete app;
					}
					else
						break;
				}
				while (queue1.curLen != 0) { //Проверить возможность обслуживания
					unsigned __int8 desiredCPUState(queue1.end->ptr->type);
					if ((proc1.state & desiredCPUState) == 0) { //На основе побитового И
						unsigned int cpuTime;
						app = queue1.pull();
						proc1.state = proc1.state + desiredCPUState;
						printf_s("%d\tНачало обработки заявки типа: %c\n", time, 'a' + desiredCPUState - 1);
						rand_s(&cpuTime); //Генерация времени обслуживания в зависимости от класса заявки
						cpuTime = (cpuTime % 11) + 23;
						proc1.time.add(cpuTime); //Набор данных о времени обслуживания
						ev = new Event('p', desiredCPUState);
						que.push(ev, time + cpuTime);
						delete app;
					}
					else
						break;
				}
			}
			else { //Время события, стоящего первым в очереди, не совпадает с текущим time
				if (proc.state == 0) //Набор данных о занятости
					proc.busy.add(0);
				else
					proc.busy.add(1);
				if (proc1.state == 0) //Набор данных о занятости
					proc1.busy.add(0);
				else
					proc1.busy.add(1);
				queue.length.add(queue.curLen); //Набор данных о длине очереди
				queue.step(); //Набор данных о времени ожидания
				queue1.length.add(queue1.curLen); //Набор данных о длине очереди
				queue1.step(); //Набор данных о времени ожидания
				time++; //Шаг
			}
	}
	printf_s("\n\t\t\t\t\t\t1\t\t2\n");
	printf_s("Средняя длина очереди:\t\t\t\t%f\t%f\n", queue.length.mean(), queue1.length.mean());
	printf_s("Среднее время ожидания заявки в очереди:\t%f\t%f\n", queue.time.mean(), queue1.time.mean());
	printf_s("Загрузка ЭВМ:\t\t\t\t\t%f\t%f\n", proc.busy.mean(), proc1.busy.mean());
	printf_s("Средний интервал поступления заявок:\t\t\t%f\n", ro);
	printf_s("Среднее время обслуживания заявок:\t\t%f\t%f\n", proc.time.mean(), proc1.time.mean());
	ro = (1 / ro) / (1 / proc.time.mean() + 1 / proc1.time.mean());
	printf_s("Коэффициент загрузки Ro:\t\t\t\t%f\n", ro);
	getchar();
	return 0;
};
