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
            Node *curr = head;
            Node* tmp = nullptr;
            while (curr != nullptr){
                tmp = curr;
                curr = curr->next;
                delete tmp;
            }
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
              pthread_mutex_init(&m, NULL);
          }
          ~Node(){
              pthread_mutex_destroy(&m);
          }
          /*
          Node* getNext(){
              return next;
          }
          void setNext(Node* node_next){
              next = node_next;
          }
          T getData(){
              return data;
          }
           */
          void lockNode(){
              pthread_mutex_lock(&m);
          }
          void unlockNode(){
              pthread_mutex_unlock(&m);
          }
        };

        /**
         * Insert new node to list while keeping the list ordered in an ascending order
         * If there is already a node has the same data as @param data then return false (without adding it again)
         * @param data the new data to be added to the list
         * @return true if a new node was added and false otherwise
         */
        bool insert(const T& data) {
            //First of all - lets take care of the list head
            //List is empty -> insert node in head
            pthread_mutex_lock(&head_lock);
            int curr_size = getSize();
            if(curr_size == 0){
                head = new Node(data);
                size += 1;
                __insert_test_hook();
                pthread_mutex_unlock(&head_lock);
                return true;
            }
            //New data smaller than head data -> need to replace head
            else if(data < head->data){
                Node* new_head = new Node(data);
                new_head->next = head;
                size += 1;
                __insert_test_hook();
                pthread_mutex_unlock(&head_lock);
                return true;
            }
            //New data is equal to head data - do nothing{
            else if(data == head->data){
                pthread_mutex_unlock(&head_lock);
                return false;
            }
            Node* middle = head;
            pthread_mutex_unlock(&head_lock);
            middle -> lockNode();
            //If only head - enter the element after it
            if(middle->next == nullptr){
                Node* to_add = new Node(data);
                middle->next = to_add;
                size += 1;
                __insert_test_hook();
                middle->unlockNode();
                return true;
            }
            //Next node exits - traverse the list and keep hand to hand lock
            Node* next = middle->next;
            Node* prev = nullptr;
            next->lockNode();
            while(next->data <= data){
                if(next->data == data){ //Already in the list!
                    middle->unlockNode();
                    next->unlockNode();
                    return false;
                }
                if(prev != nullptr){
                    prev->unlockNode();
                }
                prev = middle;
                middle = next;
                next = next->next;
                //End of list
                if(next == nullptr){
                    Node* to_add = new Node(data);
                    middle->next=to_add;
                    size+=1;
                    __insert_test_hook();
                    prev->unlockNode();
                    middle->unlockNode();
                    return true;
                }
                next->unlockNode();
            }
            //Found where to add
            Node* to_add = new Node(data);
            to_add->next = next;
            middle->next = to_add;
            size+=1;
            __insert_test_hook();
            if(prev != nullptr){
                prev->unlockNode();
            }
            middle->unlockNode();
            next->unlockNode();
            return true;

        }

        /**
         * Remove the node that its data equals to @param value
         * @param value the data to lookup a node that has the same data to be removed
         * @return true if a matched node was found and removed and false otherwise
         */
        bool remove(const T& value) {
			pthread_mutex_lock(&head_lock);
			int curr_size = getSize();
			if(curr_size == 0){
			    pthread_mutex_unlock(&head_lock);
			    return false;
			}
			else if(value < head->data){
                pthread_mutex_unlock(&head_lock);
                return false;
			}
			else if(value == head->data){
			    Node* to_remove = head;
			    head = head->next;
			    size -= 1;
			    delete to_remove;
			    __remove_test_hook();
			    pthread_mutex_unlock(&head_lock);
			    return true;
			}

            Node* pred = head;
            pthread_mutex_unlock(&head_lock);
            pred -> lockNode();
            //If only head - enter the element after it
            if(pred->next == nullptr){
               pred->unlockNode();
                return false;
            }
            //Next node exits - traverse the list and keep hand to hand lock
            Node* curr = pred->next;
            curr->lockNode();
            while(curr->data <= value) {
                if (curr->data == value) { //Found to remove
                    pred->next = curr->next;
                    size -= 1;
                    __remove_test_hook();
                    curr->unlockNode();
                    pred->unlockNode();
                    delete curr;
                    return true;
                }
                pred->unlockNode();
                pred = curr;
                curr = curr->next;
                if(curr == nullptr){
                    pred->unlockNode();
                    return false;
                }
                curr->lockNode();
            }
            pred->unlockNode();
            curr->unlockNode();
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
        int size;
        pthread_mutex_t size_lock;
        pthread_mutex_t head_lock;
    // TODO: Add your own methods and data members
};

#endif //THREAD_SAFE_LIST_H_