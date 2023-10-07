#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <iostream>
#include <unordered_map>
#include <list>
#include <string>

/**
 * This struct is used to obtain hash of custom types
*/
template <typename KeyType>
struct PairHash {
    std::size_t operator()(const KeyType& key) const {      //defining the object like this you can call the Object as a function -> PairHash p -> p(key) its an overload of () operator
        std::ostringstream oss1;
        oss1 << key;   

        std::size_t h1 = std::hash<std::string>{}(oss1.str());
        return h1;
    }

    
};


/**
 * 
 * This class implement a cache that could be used to store address for the sticky session or to store value 
 * 
*/
template <typename KeyType, typename ValueType>
class LRUCache {

    public:

        LRUCache() {}


        LRUCache(long unsigned int capacity) : capacity(capacity) {}


        ValueType* get(const KeyType& key) {
            auto it = cache.find(key);
            if (it != cache.end()) {
                updateLRU(it);  // Move the accessed item to the front (most recently used)
                return &it->second;
            }
            return nullptr;
        }


        void put(const KeyType& key, const ValueType& value) {
            if (cache.size() >= capacity) {
                // Remove the least recently used item from both the map and the list
                KeyType lruKey = lruList.back();
                lruList.pop_back();
                cache.erase(lruKey);
            }
            // Insert the new item into the map and list
            cache[key] = value;
            lruList.push_front(key);
        }


        void printCache() {
            std::cout << "LRU Cache Contents:" << std::endl;
            for (const auto& key : lruList) {
                std::cout << "Key: " << key << ", Value: " << cache[key] << std::endl;
            }
        }


    private:

        long unsigned int capacity;
        std::unordered_map<KeyType, ValueType, PairHash<KeyType>> cache;
        std::list<KeyType> lruList;


        void updateLRU(typename std::unordered_map<KeyType, ValueType, PairHash<KeyType>>::iterator it) {
            KeyType key = it->first;
            lruList.remove(key);
            lruList.push_front(key);
        }
};

#endif
