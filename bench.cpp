#include <vector>
#include <list>
#include <string>
#include <utility>
#include <optional>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
class HashMap {
    using Bucket = std::list<std::pair<std::string,int>>;
private:
    std::vector<Bucket> buckets_;
    size_t size_ = 0;

    double max_load_factor_= 0.75;

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
    void max_load_factor(double f) { max_load_factor_ = f; }

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
        if (load_factor() > max_load_factor_){
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
    size_t size() const { return size_; }
    size_t bucket_count() const { return buckets_.size(); }
    double load_factor() const {
        return static_cast<double>(size_) / buckets_.size();
    }
    
    int& operator[](const std::string& key){
        auto& bucket = buckets_[bucket_index(key)];
        for (auto& kv : bucket){
            if (kv.first == key) return kv.second;
        }
        bucket.emplace_back(key, 0);
        int& ref = bucket.back().second;
        ++size_;
        if (load_factor() > max_load_factor_)
            rehash(buckets_.size() * 2);
        return ref;
    }

};

template <typename F>
double time_ms(F&& f){
    auto t0 = std::chrono::steady_clock::now();
    f();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    constexpr int N = 1'000'000;

    std::vector<std::string> keys;
    keys.reserve(N);
    for (int i = 0; i < N; ++i) keys.push_back("key" + std::to_string(i));

    long long sink = 0;

    for (double lf : {0.5, 0.75, 2.0}) {
        HashMap m;
        m.max_load_factor(lf);
        double ins = time_ms([&] { for (int i = 0; i < N; ++i) m.insert(keys[i], i); });
        double fnd = time_ms([&] { for (int i = 0; i < N; ++i) sink += *m.find(keys[i]); });
        std::cout << "HashMap lf=" << lf << "  insert " << ins << " ms  find " << fnd
                  << " ms  buckets " << m.bucket_count() << "\n";
    }

    {
        std::unordered_map<std::string, int> m;
        double ins = time_ms([&] { for (int i = 0; i < N; ++i) m.emplace(keys[i], i); });
        double fnd = time_ms([&] { for (int i = 0; i < N; ++i) sink += m.find(keys[i])->second; });
        std::cout << "std::unordered_map  insert " << ins << " ms  find " << fnd
                  << " ms  buckets " << m.bucket_count() << "\n";
    }

    std::cout << sink << "\n";
}