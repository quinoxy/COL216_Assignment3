#ifndef DLL_HPP
#define DLL_HPP

class doublyLinkedList {
private:
    struct Node {
        int data;
        Node* prev;
        Node* next;
        Node(int value);
    };
    Node* head;
    Node* tail;

public:
    doublyLinkedList();
    void insertAtHead(Node* node);
    void deleteNode(Node* node);
};

#endif // DLL_HPP