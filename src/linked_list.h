#pragma once


template <typename T>
class LinkedListNode {
    public:
        T data;
        LinkedListNode<T>* next;
        LinkedListNode<T>* prev;

        LinkedListNode() : next(NULL), prev(NULL) {}
        LinkedListNode(T& data) : data(data), next(NULL), prev(NULL) {}
};


template <typename T>
class LinkedList {
    private:
        LinkedListNode<T> head;
        size_t len;

    public:
        LinkedList() : head(), len(0) {}

        inline void add(LinkedListNode<T>* node) {
            node->next = head.next;
            node->prev = &head;
            head.next = node;
            this->len++;
        }

        inline void remove(LinkedListNode<T>* node) {
            node->prev->next = node->next;
            if (node->next) node->next->prev = node->prev;
            this->len--;
        }

        inline size_t length() { return len; }

        inline LinkedListNode<T>* first() { return head.next; };
};
