#include <stdio.h>
#include <stdlib.h>
#include "utility.h"

//function definitions for linked_list implementation

//create_linked_list function: creates new linked_list
linked_list * create_linked_list()
{
	linked_list * new_linked_list =  (linked_list*) malloc(sizeof(linked_list));
	new_linked_list->head = NULL;
	new_linked_list->tail = NULL;
	new_linked_list->size = 0;
	return new_linked_list;
}

//create_node function: creates a new node
node* create_node(void* value)
{
	node* new_node = (node*) malloc(sizeof(node));
	new_node->value = value;
	new_node->next = NULL;
	new_node->prev = NULL;
	return new_node;
}

//add_node function: adds a node to the existing linked list
void add_node(linked_list * list, void * value)
{
	node * new_node = create_node(value);
	if(list->size == 0)
	{
		list->head = new_node;
		list->tail = new_node;
		list->size = 1 ;
	} else {
		new_node->prev = list->tail;
		list->tail->next = new_node;
		list->tail = new_node;
		list->size += 1;
	}
}

//remove_value function: semoves a node with input value from the existing linked_list
void remove_value(linked_list* list, void * value)
{
	node* current_node = list->head;

	while(current_node != NULL && current_node->value != value) {
		current_node = current_node->next;
	}

	if(current_node != NULL) {
		if(current_node->prev != NULL) {
			current_node->prev->next = current_node->next;
		}
		if(current_node->next != NULL) {
			current_node->next->prev = current_node->prev;
		}
		if(list->head == current_node) {
			list->head = current_node->next;
		}
		if(list->tail == current_node) {
			list->tail = current_node->prev;
		}
		list->size --;
		free(current_node);
	}
}

//remove_node function: removes a node from existing linked_list
void remove_node(linked_list* list, node * current_node) {
	if(current_node != NULL) {
		if(current_node->prev != NULL) {
			current_node->prev->next = current_node->next;
		}
		if(current_node->next != NULL) {
			current_node->next->prev = current_node->prev;
		}
		if(list->head == current_node) {
			list->head = current_node->next;
		}
		if(list->tail == current_node) {
			list->tail = current_node->prev;
		}
		list->size --;
		free(current_node);
	}
}

//remove_head function: removes a node from existing linked_list
void remove_head(linked_list* list)
{
	node * current_node = list->head;
	if(current_node != NULL) {
		list->head = current_node->next;
		if(list->tail == current_node) {
			list->tail = current_node->prev;
		}
		list->size --;
		free(current_node);
	}
}

//add_after function: adds a new node after a given node in an existing linked_list
void add_after(linked_list* list, node *after_node, void *value)
{
	node* new_node = create_node(value);

	node* next_node = after_node->next;
	new_node->next = next_node;
	if(next_node != NULL) next_node->prev = new_node;

	new_node->prev = after_node;
	after_node->next = new_node;

	if(list->tail == after_node) {
		list->tail = new_node;
	}

	list->size++;
}

//sort function: sorts the input linked_list
void sort(linked_list *list, int (*cmp)(void *value1, void *value2)) {
	node *i = list->head;
	while(i!=NULL) {
		node *j = i->next;
		while(j!=NULL) {
			void * p1 = i->value;
			void * p2 = j->value;
			if((*cmp)(p1,p2) > 0) {
				swap_nodes(i,j);
			}
			j=j->next;
		}
		i = i->next;
	}
}

//swap_nodes function: swaps two linked_list nodes
void swap_nodes(node *a, node *b) {
	void * temp = a->value;
	a->value = b->value;
	b->value = temp;
}

//function definitions for queue implementation
//create_queue: function to create queue
queue * create_queue() {
	return create_linked_list();
}

//enqueue function: adds process at the end of the queue
void enqueue(queue* q, void * value)
{
	node* new_node = create_node(value);
	
	new_node->prev = q->tail;
	if(q->tail != NULL) {
		q->tail->next = new_node;
		q->tail = new_node;
	} else {
		q->tail = new_node;
		q->head = new_node;
	}
	q->size += 1;
}

//dequeue function: removes process from the end of the queue
void* dequeue(queue* q)
{
	if(q->head != NULL) {
		node * current_node = q->head;
		void * value = current_node->value;

		node * next_node = q->head->next;

		if(next_node != NULL) next_node->prev = NULL;
		q->head = next_node; 
		
		if(q->tail == current_node) {
			q->tail = NULL;
		}

		q->size--;
		free(current_node);
		return value;
	}
}
