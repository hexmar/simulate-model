#include "stdafx.h"

static const unsigned int SIM_TIME = 80 * 60; //������������ �������������

struct Mean { //��������� ��� ���������� �������� �����
	float sum, meanVal;
	unsigned int count; //���������� ���������
	bool updated; //��������� �� ������� ��������
	Mean() : sum(0), meanVal(0), count(0), updated(true) {};
	void add(float val) { //���������� ����������; val - �������� ������ ����������
		sum += val;
		count++;
		updated = false;
	};
	float mean() { //���������� ��������
		if (!updated) meanVal = sum / count; //���� �������� �� ���������, �� ��������� ���
		updated = true;
		return meanVal;
	};
};

struct App { //���������, ����������� ������
	unsigned __int8 type; //����� ������: 1 - A, 2 - B, 3 - C
	unsigned int time; //����� ���������� ������ � �������
	App(unsigned __int8 t) : type(t), time(0) {};
};

struct Queue { //���������, ����������� �������
	struct Entry { //���������, ����������� ������� �������
		App *ptr;
		Entry *next, *prev;
		Entry(App *ptrIn, Entry* nextIn, Entry* prevIn)
			: ptr(ptrIn), next(nextIn), prev(prevIn) {};
	} *start, *end; //��������� �� ����� � ������ �������
	unsigned int curLen; //������� ����� �������
	Mean length, time; //������� ����� ������� � ������� ����� �������� � �������
	Queue() : curLen(0), start(nullptr), end(nullptr) {};
	void push(App *ptr) { //��������� "������" � �������; ptr - ��������� �� "������"
		if (start == nullptr)
			start = end = new Entry(ptr, nullptr, nullptr);
		else
			start = start->prev = new Entry(ptr, start, nullptr);
		curLen++;
	};
	App *pull() { //������� ������ �� �������
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
	void step() { //���������� ������� �������� ������� �������� �������
		Entry *iter(start);
		while (iter != nullptr) {
			iter->ptr->time++;
			iter = iter->next;
		}
	};
};

struct CPU { //���������, ����������� ������������
	unsigned __int8 state; //0(00) - ��������, 1(01) - A, 2(10) - B, 3(11) - C ��� A � B
	Mean busy, time; //���������, ������� ����� ������������
	CPU() : state(0) {};
};

struct Event { //���������, ����������� �������
	char type; //��� �������: n - ����������� ������, c - ���������� ������������ ������
	int param; //�������������� ��������
	Event(char typeIn, int paramIn) : type(typeIn), param(paramIn) {};
};

struct NodeQueue { //���������, ����������� ��������� �������
	struct QueueNode {
		QueueNode *next, *prev;
		Event *ev;
		unsigned int inform;
		QueueNode(Event *ptr, unsigned int inf, QueueNode *n, QueueNode *p)
			: next(n), prev(p), ev(ptr), inform(inf) {};
	} *start, *end;
	NodeQueue() : start(nullptr), end(nullptr) {};
	void push(Event *ptr, unsigned int inf) { //��������� "�������" � �������
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
	Event *pull() { //������� "�������" �� �������
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
	bool notEmpty() { //�� ����� �� �������
		if (start == nullptr)
			return false;
		else
			return true;
	};
	float meanTime() { //������� ����� ����� ���������
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
	NodeQueue que; //��������� �������
	Queue queue, queue1; //������� ������
	CPU proc, proc1; //������������ ������
	unsigned int time(0); //������� �����
	Event *ev(nullptr);
	{ //��������� ������
		unsigned int tmp(0), prev(0);
		while (true) { //������ A
			rand_s(&tmp);
			tmp = (tmp % 11) + 15 + prev;
			if (tmp >= SIM_TIME) break;
			prev = tmp;
			ev = new Event('n', 1);
			que.push(ev, tmp);
		}
		prev = tmp = 0;
		while (true) { //������ B
			rand_s(&tmp);
			tmp = (tmp % 21) + 10 + prev;
			if (tmp >= SIM_TIME) break;
			prev = tmp;
			ev = new Event('n', 2);
			que.push(ev, tmp);
		}
		prev = tmp = 0;
		while (true) { //������ C
			rand_s(&tmp);
			tmp = (tmp % 21) + 20 + prev;
			if (tmp >= SIM_TIME) break;
			prev = tmp;
			ev = new Event('n', 3);
			que.push(ev, tmp);
		}
	}
	float ro(que.meanTime()); //������� �������� ����������� ������
	App *app(nullptr);
	while (time <= SIM_TIME) { //�������� ����
		if (que.notEmpty())
			if (que.end->inform == time) { //����� �������, �������� ������ � �������, ��������� � ������� time
				ev = que.pull();
				switch (ev->type) { //���������� � ����������� �� ����
				case 'n':
					app = new App(ev->param);
					if (ev->param == 3)
						queue1.push(app);
					else
						queue.push(app);
					printf_s("%d\t����������� ������ ����: %c\n", time, 'a' + ev->param - 1);
					break;
				case 'c':
					proc.state = proc.state - ev->param;
					printf_s("%d\t���������� ��������� ������ ����: %c\n", time, 'a' + ev->param - 1);
					break;
				case 'p':
					proc1.state = proc1.state - ev->param;
					printf_s("%d\t���������� ��������� ������ ����: %c\n", time, 'a' + ev->param - 1);
					break;
				}
				delete ev;
				while (queue.curLen != 0) { //��������� ����������� ������������
					unsigned __int8 desiredCPUState(queue.end->ptr->type);
					if ((proc.state & desiredCPUState) == 0) { //�� ������ ���������� �
						unsigned int cpuTime;
						app = queue.pull();
						proc.state = proc.state + desiredCPUState;
						printf_s("%d\t������ ��������� ������ ����: %c\n", time, 'a' + desiredCPUState - 1);
						rand_s(&cpuTime); //��������� ������� ������������ � ����������� �� ������ ������
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
						proc.time.add(cpuTime); //����� ������ � ������� ������������
						ev = new Event('c', desiredCPUState);
						que.push(ev, time + cpuTime);
						delete app;
					}
					else
						break;
				}
				while (queue1.curLen != 0) { //��������� ����������� ������������
					unsigned __int8 desiredCPUState(queue1.end->ptr->type);
					if ((proc1.state & desiredCPUState) == 0) { //�� ������ ���������� �
						unsigned int cpuTime;
						app = queue1.pull();
						proc1.state = proc1.state + desiredCPUState;
						printf_s("%d\t������ ��������� ������ ����: %c\n", time, 'a' + desiredCPUState - 1);
						rand_s(&cpuTime); //��������� ������� ������������ � ����������� �� ������ ������
						cpuTime = (cpuTime % 11) + 23;
						proc1.time.add(cpuTime); //����� ������ � ������� ������������
						ev = new Event('p', desiredCPUState);
						que.push(ev, time + cpuTime);
						delete app;
					}
					else
						break;
				}
			}
			else { //����� �������, �������� ������ � �������, �� ��������� � ������� time
				if (proc.state == 0) //����� ������ � ���������
					proc.busy.add(0);
				else
					proc.busy.add(1);
				if (proc1.state == 0) //����� ������ � ���������
					proc1.busy.add(0);
				else
					proc1.busy.add(1);
				queue.length.add(queue.curLen); //����� ������ � ����� �������
				queue.step(); //����� ������ � ������� ��������
				queue1.length.add(queue1.curLen); //����� ������ � ����� �������
				queue1.step(); //����� ������ � ������� ��������
				time++; //���
			}
	}
	printf_s("\n\t\t\t\t\t\t1\t\t2\n");
	printf_s("������� ����� �������:\t\t\t\t%f\t%f\n", queue.length.mean(), queue1.length.mean());
	printf_s("������� ����� �������� ������ � �������:\t%f\t%f\n", queue.time.mean(), queue1.time.mean());
	printf_s("�������� ���:\t\t\t\t\t%f\t%f\n", proc.busy.mean(), proc1.busy.mean());
	printf_s("������� �������� ����������� ������:\t\t\t%f\n", ro);
	printf_s("������� ����� ������������ ������:\t\t%f\t%f\n", proc.time.mean(), proc1.time.mean());
	ro = (1 / ro) / (1 / proc.time.mean() + 1 / proc1.time.mean());
	printf_s("����������� �������� Ro:\t\t\t\t%f\n", ro);
	getchar();
	return 0;
};
