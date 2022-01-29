#ifndef _utility_h_
#define _utility_h_

//Linked list node data structure
struct node_structure {
	struct node_structure *next;
	struct node_structure *prev;
	void* value;
};
typedef struct node_structure node;

//Linked list data structure
struct linked_list_structure {
	node * head;
	node * tail;
	int size;
};
typedef struct linked_list_structure linked_list;

//Linked list functions
node* create_node(void* value);
linked_list * create_linked_list();
void add_node(linked_list* ll, void* value);
void remove_data(linked_list* ll, void* value);
void remove_node(linked_list* ll, node* n);
void add_after(linked_list* ll, node *after_node, void* value);
void sort(linked_list *ll, int (*cmp)(void *value1, void *value2));
void swap_nodes(node *a, node *b);

//Queue data structure
typedef struct linked_list_structure queue;

//Queue functions
queue * create_queue();
void enqueue(queue * q, void * value);
void * dequeue(queue * q);

#endif