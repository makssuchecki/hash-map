#include <vector>
#include <list>
#include <string>
#include <utility>
#include <optional>
#include <functional>
#include <stdexcept>

class HashMap {
    using Bucket = std::list<std::pair<std::string,int>>;
private:
    std::vector<Bucket> buckets_;
    size_t size_ = 0;
    static constexpr double MAX_LOAD_FACTOR = 0.75;

    static size_t index_for(const std::string& key, size_t bucket_count){
        return std::hash<std::string>{}(key) % bucket_count;
    }
    
    size_t bucket_index(const std::string& key) const {
        return index_for(key, buckets_.size());
    }

    void rehash(size_t new_bucket_count){
        std::vector<Bucket> new_buckets(new_bucket_count);
        for (auto& bucket : buckets_){ 
            while (!bucket.empty()) {
                auto it = bucket.begin();
                auto& target = new_buckets[index_for(it->first, new_bucket_count)];
                target.splice(target.end(), bucket, it);
            }
        }
        buckets_ = std::move(new_buckets);
    }

public:
    explicit HashMap(size_t bucket_count = 16) : buckets_(bucket_count) {
        if (bucket_count == 0){
            throw std::invalid_argument("bucket count must be > 0");
        }
    }

    void insert(const std::string& key, int value){
        size_t idx = bucket_index(key);
        for (auto& kv : buckets_[idx]) {
            if (kv.first == key){
                kv.second = value;
                return;
            }
        }
        buckets_[idx].emplace_back(key, value);
        ++size_;
        if (load_factor() > MAX_LOAD_FACTOR){
            rehash(buckets_.size() * 2);
        } 
    }

    std::optional<int> find(const std::string& key) const {
        for (const auto& kv : buckets_[bucket_index(key)]) {
            if (kv.first == key) return kv.second;
        }
        return std::nullopt;
    }

    bool erase(const std::string& key){
        size_t idx = bucket_index(key);
        auto& bucket = buckets_[idx];
        for (auto it = bucket.begin(); it != bucket.end(); ++it){
            if (it->first == key){
                bucket.erase(it);
                --size_;
                return true;
            }
        }
        return false;
    }
    size_t size() const { return size_; };
    size_t bucket_count() const { return buckets_.size(); };
    double load_factor() const {
        return static_cast<double>(size_) / buckets_.size();
    }

};


#include <cassert>
#include <iostream>

int main(){
    HashMap m(2);

    for (int i = 0; i < 1000; i++)
        m.insert("key"+std::to_string(i), i);

    assert(m.size() == 1000);
    assert(m.load_factor() <= 0.75);
    for (int i = 0; i < 1000 ; ++i)
        assert (m.find("key" + std::to_string(i)) == i);
    
    m.insert("key5", 999);
    assert(m.size() == 1000 && m.find("key5") == 999);

    for (int i = 0; i < 1000; i += 2)
        assert(m.erase("key" + std::to_string(i)));
    assert(m.size() == 500);
    assert(!m.find("key0").has_value());
    assert(!m.erase("nie-ma"));
    assert(m.find("key1") == 1);

    std::cout << "OK, buckets: " << m.bucket_count() << "\n";
}