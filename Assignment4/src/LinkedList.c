#include "LinkedList.h"

void add(Node* root, int waiter)
{
	Node* temp = root;
	while(temp->next != NULL)
	{
		temp = temp->next;
	}
	Node* new = malloc(sizeof(Node));
        new->next = NULL;
	new->waiter = waiter;
	temp->next = new;
}

void print(Node* root)
{
	Node* temp = root;
	int i = 0;
	while(temp != NULL)
	{
		printf("%d->",temp->waiter);
		i++;
		temp = temp->next;
	}
	printf("\n");
}

void clear(Node** root)
{
	Node* temp = (*root)->next;
	Node* temp2 = temp;
	while(temp->next != NULL)
	{
		temp2 = temp->next;
		free(temp);
		temp = temp2;
	}
	free(temp);
	(*root)->next = NULL;
}

Node createNode(int waiter, int resource)
{
	Node new;
	new.next = NULL;
	new.waiter = waiter;
	new.resource = resource;
	new.status = NOTDISCOVERED;
	return new;
}

//removes the first Node
void pop(Node** root)
{
	*root = (*root)->next;
}
