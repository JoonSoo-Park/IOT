#ifndef QUEUE_H
#define QUEUE_H

typedef struct {
    int capacity;
    int size;
    int front;
    int rear;
    int *elements;
} queue;


queue* createQueue(int maxElement)
{
    queue *Q;
    Q = (queue*)malloc(sizeof(queue)); 

    Q->elements = (int*)malloc(sizeof(int) * maxElement);
    Q->capacity = maxElement;
    Q->size = 0;
    Q->front = 0;
    Q->rear = -1;

    return Q;
}

int empty(queue *Q)
{
    return Q->size == 0;
}

int full(queue *Q)
{
    return Q->front == Q->capacity;
}

void pop(queue *Q)
{
    if (!empty(Q)) {
        Q->front++;
        Q->size--;

        if (Q->front == Q->capacity)
            Q->front = 0;
    }
}

int peek(queue *Q)
{
    return Q->elements[Q->front];
}

void push(queue *Q, int value)
{
    if (!full(Q)) {
        Q->size++;
        Q->rear++;

        if (Q->rear == Q->capacity)
            Q->rear = 0;

        Q->elements[Q->rear] = value;
    }
}

#endif