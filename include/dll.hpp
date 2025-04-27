#ifndef DLL_HPP
#define DLL_HPP

class doublyLinkedList {

public:
    struct Node {
        int data;
        Node* prev;
        Node* next;
        Node(int value);
    };
    Node* head;
    Node* tail;
    doublyLinkedList();
    void insertAtHead(Node* node);
    void deleteNode(Node* node);

  
    

};

#endif // DLL_HPP