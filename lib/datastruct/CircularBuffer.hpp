#ifndef _circular_buffer_hpp_
#define _circular_buffer_hpp_


// Circular Buffer of Ptrs to objects of type T
template<class T>
class CircularBuffer
{
    public:
        CircularBuffer(size_t size) 
        {
            data = allocateCapacity(size);
            capacity = int(size);
            front = 1;
            rear = 0;
            population = 0;
        }

        ~CircularBuffer()
        {
            deleteMembers();
            delete[] data;
        }

        bool isFull() const { return population == capacity; }
        bool isEmpty() const { return population == 0; }
        int  getCapacity() const { return capacity; }
        int  getAvailable() const { return capacity - population; }
        int  numMembers() const { return population; }

//        void setCapacity(int capacityDesired) {
//            if (capacityDesired > capacity) {
//                growArray(capacityDesired);
//            } else if (capacityDesired < capacity) {
//                shrinkArray(capacityDesired);
//            }
//        }

        void enQueue(T * pItem)
        {
            if (population < capacity)
            {
                population++;
                rear = (rear + 1) % capacity;
                data[rear] = pItem;
            }
            else
            {
                // Overflow !
                DEBUG("ERROR: -- Circular buffer OVERFLOW: ptr = %p", pItem);
            }
        }

        T * deQueue()
        {
            if (population > 0)
            {
                population--;
                T * pItem = data[front];
                data[front] = NULL;
                front = (front + 1) % capacity;
                return pItem;
            }
            else
            {
                return NULL; // Underflow !
            }
        }

        T * peekAt(size_t iIndex) 
        {
            int idx = (rear - iIndex);
            if (idx < 0) {
                idx += capacity;
            }
            return data[idx];
        }

        void deleteMembers() 
        {
            while (! isEmpty()) {
                T * pItem = deQueue();
                delete pItem;
            }
        }

    private:
        T ** data;
        int capacity; // physical capacity
        int rear; // index of most recent (newest) entry
        int front;// index of oldest entry
        int population; // number of occupants 

        T ** allocateCapacity(int capacityDesired) {
            T ** ptr = new T*[capacityDesired];
            memset(ptr, 0, sizeof(T*) * capacityDesired);
            return ptr;
        }

//        void growArray(int capacityDesired) {
//            T ** new_data = allocateCapacity(capacityDesired);
//            for (int k = 0; k < capacity; k++) {
//                new_data[k] = data[k];
//            }
//            delete[] data;
//            data = new_data;
//            capacity = capacityDesired;
//        }
//
//        void shrinkArray(int capacityDesired) {
//            // This would be more efficient if the buffer had a 'limit' variable that 
//            // can be less or equal to capacity. limit would be the maximum gap between the
//            // front and rear indexes .. or something .. more optimal perhaps but trickier to implement
//
//            T ** new_data = allocateCapacity(capacityDesired);
//
//            for (int k = 0; k < capacityDesired; k++) {
//                new_data[k] = deQueue();
//            }
//
//            deleteMembers();
//            
//            delete[] data;
//            data = new_data;
//            capacity = capacityDesired;
//            front = 0; 
//            rear = capacityDesired - 1;
//            population = capacityDesired;
//        }

};

#endif // _circular_buffer_hpp_
