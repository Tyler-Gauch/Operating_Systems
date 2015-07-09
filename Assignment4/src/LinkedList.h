#include<stdio.h>
#include<stdlib.h>

#define DISCOVERED 1
#define NOTDISCOVERED 0

typedef struct Node{
	int waiter;
	int resource;
	int status;
	struct Node* next;
} Node;

void add(Node* root, int waiter);
void print(Node* root);
void clear(Node** root);
Node createNode(int waiter, int resource);
void pop(Node** root);

