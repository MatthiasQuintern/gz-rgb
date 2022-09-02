/* #include "util.hpp" */

#include <iostream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <concepts>
#include <thread>

namespace gz::util {

    // from util.hpp
    template<std::unsigned_integral I, std::unsigned_integral S>
    inline void incrementIndex(I& i, const S containerSize) {
        if (i < containerSize - 1) { i++; }
        else { i = 0; }
    }
    template<std::unsigned_integral I, std::unsigned_integral S>
    inline void decrementIndex(I& i, const S containerSize) {
        if (i > 0) { i--; }
        else { i = containerSize - 1; }
    }
    template<std::unsigned_integral I, std::unsigned_integral S>
    inline I getIncrementedIndex(const I i, const S containerSize) {
        if (i < containerSize - 1) { return i + 1; }
        else { return 0; }
    }
    template<std::unsigned_integral I, std::unsigned_integral S>
    inline I getDecrementedIndex(const I i, const S containerSize) {
        if (i > 0) { return i - 1; }
        else { return containerSize - 1; }
    }
    /// Make wrap incices around: i = size + 2 -> i = 2, i = -2 -> i = size - 2
    template<std::integral I, std::unsigned_integral S>
    size_t getValidIndex(const I i, const S containerSize) {
        if (i < 0) { 
            return containerSize - (-i) % containerSize - 1;
        }
        else if (i >= static_cast<int>(containerSize)) {
            return i % containerSize;
        }
        return i;
    }
    /**
     * @brief A thread-safe queue with a dynamic size up until a maximum size
     * @details
     *  Elements are ordered by the time they were put in the queue: You can only insert an element at the front and you can only get the element from the end.
     *
     *  The queue uses a ringbuffer (which itself uses a vector) for data storage. Reallocations happen when an element is inserted into a queue with n < maxSize elements and the size of the ringbuffer is n.
     *  The queue will then increase the size of the ringbuffer by 10% (at least 3 elements, but the size will never be greater than maxSize). 
     *
     *  Note that "n elements" means n elements that were inserted and not accessed through get(). Elements might still by in memory after they have been get().
     *
     *  The queue is thread safe, IF: the end of the queue is only processed by a single thread. Data races can occur when multiple threads try to retrieve elements (or clear() the queue),
     *  since the information from hasElement() might not be valid anymore until getCopy() gets called. Putting elements into the queue can be done by multiple threads.
     */
    template<std::swappable T>
    class Queue {
        public:
            /**
             * @brief Create a new queue
             * @param size The size the queue can grow to without reallocating memory.
             * @param maxSize The maximum size of the queue. If more than maxSize elements are inserted, the oldest elements are discarded until size == maxSize.
             */
            Queue(size_t size=10, size_t maxSize=-1);

            void push_back(T& t);
            void push_back(T&& t) { push_back(t); };
            void emplace_back(T&& t);

            /**
             * @brief Check if the contains has an element that can be retrieved by get()
             */
            bool hasElement();
            /**
             * @brief Get a reference to the oldest element
             * @returns Reference to the oldest element. 
             * @note The reference is at least valid until the next call to push_back/emplace_back. If you are in a multithreaded environment, it is probably better to use getCopy().
             * @warning Leads to undefined behavior when there is no element to get. Always check hasElement() first.
             */
            T& getRef();
            /**
             * @brief Get a copy of the oldest element
             * @returns Copy of the oldest element. 
             * @warning Leads to undefined behavior when there is no element to get. Always check hasElement() first.
             */
            T getCopy();

            /**
             * @brief Remove all elements
             */
            void clear();

            std::vector<T>& getInternalBuffer() { return buffer; }
        private:
            /**
             * @brief Resize the queue (if possible)
             * @details
             *  After calling this, readIndex and writeIndex will be valid so that a push_back or emplace_back can be performed.
             */
            void resize();

            size_t writeIndex;  ///< Points to the element that was last written
            size_t readIndex;  ///< Points to the element that was last read
            std::vector<T> buffer;
            size_t vectorCapacity;
            size_t maxSize;
            std::mutex mtx;
    };

    template<std::swappable T>
    Queue<T>::Queue(size_t capacity, size_t maxSize)
        : vectorCapacity(capacity), maxSize(maxSize) {
        buffer.reserve(capacity);
        /* buffer.resize(2); */

        writeIndex = capacity - 1;
        readIndex = capacity - 1;
    }


    template<std::swappable T>
    void Queue<T>::resize() {
        std::cout << "resize queue\n";
        // if vector is at maxSize, "loose" the oldest element
        if (buffer.size() == maxSize) {
            incrementIndex(readIndex, buffer.size());
            return;
        }
        // if not at end of vector rotate so that oldest element is first
        if (writeIndex != vectorCapacity - 1) {
            // if not at end, reserve more space and move elements
            std::rotate(buffer.begin(), buffer.begin() + readIndex, buffer.end());
            readIndex = 0;
            writeIndex = vectorCapacity - 1;
        } 
        // reserve 10% more space (at least space for 3 more elements).
        buffer.reserve(std::min(std::max(static_cast<size_t>(1.1 * vectorCapacity), vectorCapacity + 3), maxSize));
        vectorCapacity = buffer.capacity();
    }


    template<std::swappable T>
    void Queue<T>::push_back(T& t) {
        mtx.lock();
        // check if this would write into oldest element
        if (readIndex == getIncrementedIndex(writeIndex, vectorCapacity)) { resize(); }

        util::incrementIndex(writeIndex, vectorCapacity);
        if (buffer.size() < vectorCapacity) {
            buffer.push_back(t); 
        }
        else {
            buffer[writeIndex] = t;
        }
        mtx.unlock();
        std::cout << "queue after pushback. ri: " << readIndex << " - wi: " << writeIndex << " - size: " << buffer.size() << " - cap: " << vectorCapacity << "\n";
    }


    template<std::swappable T>
    void Queue<T>::emplace_back(T&& t) {
        mtx.lock();
        if (readIndex == getIncrementedIndex(writeIndex, vectorCapacity)) { resize(); }

        util::incrementIndex(writeIndex, vectorCapacity);
        if (buffer.size() < vectorCapacity) {
            buffer.emplace_back(std::move(t)); 
        }
        else {
            buffer[writeIndex] = std::move(t);
        }
        mtx.unlock();
    }
    

    template<std::swappable T>
    bool Queue<T>::hasElement() {
        mtx.lock();
        bool hasElement = writeIndex != readIndex;
        mtx.unlock();
        return hasElement;
    }

    
    template<std::swappable T>
    T& Queue<T>::getRef() {
        mtx.lock();
        incrementIndex(readIndex, vectorCapacity);
        size_t i = readIndex;
        T& element = buffer[i];
        mtx.unlock();
        return buffer[i];
    }


    template<std::swappable T>
    T Queue<T>::getCopy() {
        mtx.lock();     
        incrementIndex(readIndex, vectorCapacity);
        size_t i = readIndex;
        mtx.unlock();
        std::cout << "queue after getCopy. ri: " << readIndex << " - wi: " << writeIndex << " - size: " << buffer.size() << " - cap: " << vectorCapacity << "\n";
        return std::move(buffer[i]);  /// @todo could this in some stupid edge case lead to a data race?
    }


    template<std::swappable T>
    void Queue<T>::clear() {
        mtx.lock();     
        writeIndex = 0;
        readIndex = 0;
        mtx.unlock();
    }
}
