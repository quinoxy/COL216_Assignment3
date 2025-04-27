#include "dll.hpp"




doublyLinkedList::doublyLinkedList() : head(nullptr), tail(nullptr) {
    head -> next = tail;
    tail->prev = head;
}

void doublyLinkedList::insertAtHead(Node* node){
    Node* nextNode = head->next;
    node -> next = nextNode;
    node -> prev = head;
    head-> next = node;
    nextNode -> prev = node;
}

void doublyLinkedList::deleteNode(Node* node){
    node->prev->next = node ->next;
    node ->next -> prev = node -> prev;
    delete node;
}

doublyLinkedList::Node::Node(unsigned value) : data(value), next(nullptr), prev(nullptr) {}
