#ifndef THREAD_SAFE_LIST_H_
#define THREAD_SAFE_LIST_H_

#include <pthread.h>
#include <iostream>
#include <iomanip> // std::setw

using namespace std;

template <typename T>
class List
{
public:
    /**
     * Constructor
     */
    List() {
        head = nullptr;
        dummy = new Node(T());
        dummy->next = head;
        size = 0;
        int res1 = pthread_mutex_init(&size_lock, NULL);
        int res2 = pthread_mutex_init(&head_lock, NULL);
        if(res1 != 0 || res2 != 0){
            perror("pthread_mutex_init: failed");
            exit(-1);
        }
    }

    /**
     * Destructor
     */
    ~List(){
        while(size > 0){
            remove(this->head->data);
        }
        delete(this->dummy);
        pthread_mutex_destroy(&size_lock);
        pthread_mutex_destroy(&head_lock);
    }

    class Node {
    public:
        T data;
        Node *next;
        pthread_mutex_t m;
        // TODO: Add your methods and data members
        Node(T data) : data(data), next(nullptr){
            pthread_mutex_init(&m, nullptr);
        }
        ~Node(){
            pthread_mutex_destroy(&m);
        }
    };

    /**
     * Insert new node to list while keeping the list ordered in an ascending order
     * If there is already a node has the same data as @param data then return false (without adding it again)
     * @param data the new data to be added to the list
     * @return true if a new node was added and false otherwise
     */
    bool insert(const T& data) {
        bool is_head = true;
        Node* prev = dummy;
        pthread_mutex_lock(&(dummy->m));
        Node* curr = dummy->next;
        //int curr_size = getSize();
        if(curr == nullptr){
            head = new Node(data);
            dummy->next = head;
            setSize(true);
            __insert_test_hook();
            pthread_mutex_unlock(&(dummy->m));
            return true;
        }
        //curr is not null
        pthread_mutex_lock(&(curr->m));
        while(curr->data <= data){
            if(curr->data == data){
                pthread_mutex_unlock(&(prev->m));
                pthread_mutex_unlock(&(curr->m));
                return false;
            }
            pthread_mutex_unlock(&(prev->m));
            prev = curr;
            curr = curr->next;
            if(curr == nullptr){
                Node* to_add = new Node(data);
                prev->next = to_add;
                setSize(true);
                __insert_test_hook();
                pthread_mutex_unlock(&(prev->m));
                return true;
            }
            pthread_mutex_lock(&(curr->m));
            is_head = false;
        }
        Node* to_add = new Node(data);
        prev->next = to_add;
        to_add->next = curr;
        if(is_head){
            pthread_mutex_lock(&head_lock);
            head = to_add;
            pthread_mutex_unlock(&head_lock);
        }
        setSize(true);
        __insert_test_hook();
        pthread_mutex_unlock(&(prev->m));
        pthread_mutex_unlock(&(curr->m));
        return true;
    }

    /**
     * Remove the node that its data equals to @param value
     * @param value the data to lookup a node that has the same data to be removed
     * @return true if a matched node was found and removed and false otherwise
     */
    bool remove(const T& value) {
        bool is_head = true;
        Node* pred = dummy;
        pthread_mutex_lock(&(dummy->m));
        Node* curr = pred->next;
        if(curr == nullptr){
            pthread_mutex_unlock(&(pred->m));
            return false;
        }
        pthread_mutex_lock(&(curr->m));
        while(curr->data <= value){
            if(curr->data == value){
                pred->next = curr->next;
                if(is_head){
                    pthread_mutex_lock(&head_lock);
                    head = curr->next;
                    pthread_mutex_unlock(&head_lock);
                }
                setSize(false);
                __remove_test_hook();
                pthread_mutex_unlock(&(curr->m));
                delete curr;
                pthread_mutex_unlock(&(pred->m));
                return true;
            }
            pthread_mutex_unlock(&(pred->m));
            pred = curr;
            curr = curr->next;
            if(curr == nullptr){
                pthread_mutex_unlock(&(pred->m));
                return false;
            }
            pthread_mutex_lock(&(curr->m));
            is_head = false;
        }
        pthread_mutex_unlock(&(pred->m));
        pthread_mutex_unlock(&(curr->m));
        return false;
    }

    /**
     * Returns the current size of the list
     * @return current size of the list
     */
    unsigned int getSize() {
        pthread_mutex_lock(&size_lock);
        int return_size = size;
        pthread_mutex_unlock(&size_lock);
        return return_size;
    }

    void setSize(bool increase){
        pthread_mutex_lock(&size_lock);
        int to_add = increase ? 1 : -1;
        size += to_add;
        pthread_mutex_unlock(&size_lock);
    }

    // Don't remove
    void print() {
        Node* temp = head;
        if (temp == NULL)
        {
            cout << "";
        }
        else if (temp->next == NULL)
        {
            cout << temp->data;
        }
        else
        {
            while (temp != NULL)
            {
                cout << right << setw(3) << temp->data;
                temp = temp->next;
                cout << " ";
            }
        }
        cout << endl;
    }

    // Don't remove
    virtual void __insert_test_hook() {}
    // Don't remove
    virtual void __remove_test_hook() {}

private:
    Node* head;
    Node* dummy;
    int size;
    pthread_mutex_t size_lock;
    pthread_mutex_t head_lock;
    // TODO: Add your own methods and data members
};

#endif //THREAD_SAFE_LIST_H_