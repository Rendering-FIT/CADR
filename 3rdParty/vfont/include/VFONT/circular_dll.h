/**
 * @file circular_dll.h
 * @author Christian Saloň
 */

#pragma once

#include <stdexcept>

#include "edge.h"

namespace vft {

/**
 * @brief Circular doubly linked list
 */
template <typename T>
class CircularDLL {
public:
    /**
     * @class Node
     *
     * @brief One element of CircularDLL
     */
    class Node {
    public:
        T value;        /**< Value of elemnt */
        Node *previous; /**< Previous element of list */
        Node *next;     /**< Next element of list */

        Node(T value);
        ~Node() = default;
    };

protected:
    unsigned int _size{0}; /**< Size of list */

    Node *_front{nullptr}; /**< First element of list */
    Node *_back{nullptr};  /**< Last element of list */

public:
    CircularDLL() = default;
    ~CircularDLL();

    CircularDLL(const CircularDLL &other);
    CircularDLL &operator=(const CircularDLL &other);

    void clear();

    void insertAt(T value, unsigned int index);
    void insertFirst(T value);
    void insertLast(T value);

    void deleteAt(unsigned int index);
    void deleteFirst();
    void deleteLast();

    Node *getAt(unsigned int index) const;
    Node *getValue(T value) const;
    Node *getFirst() const;
    Node *getLast() const;

    unsigned int size() const;
};

}  // namespace vft
