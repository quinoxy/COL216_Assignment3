#ifndef DLL_HPP
#define DLL_HPP

class doublyLinkedList {

public:
    struct Node {
        unsigned data;
        Node* prev;
        Node* next;
        Node(unsigned value);
    };
    Node* head;
    Node* tail;
    doublyLinkedList();
    void insertAtHead(Node* node);
    void deleteNode(Node* node);

  
    

};

#endif // DLL_HPP