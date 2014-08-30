#ifndef CORE_KEY_SET_H_
#define CORE_KEY_SET_H_

#include <bitset>
#include <string>
#include <vector>

#include "core/key.h"

class KeySet {
public:
    KeySet() {}
    explicit KeySet(Key);
    KeySet(Key, Key);

    void setKey(Key, bool b = true);
    bool hasKey(Key) const;

    bool hasSomeKey() const { return keys_.any(); }
    bool hasIntersection(const KeySet& rhs) const
    {
        const KeySet& lhs = *this;
        return (lhs.keys_ & rhs.keys_).any();
    }

    std::string toString() const;
    int toInt() const { return static_cast<int>(keys_.to_ulong()); }

    friend bool operator==(const KeySet& lhs, const KeySet& rhs)
    {
        return lhs.keys_ == rhs.keys_;
    }

private:
    std::bitset<NUM_KEYS> keys_;
};

class KeySetSeq {
public:
    KeySetSeq() {}
    KeySetSeq(const std::vector<KeySet>& seq) : seq_(seq) {}

    size_t size() const { return seq_.size(); }
    const KeySet& operator[](int idx) const { return seq_[idx]; }

    void add(const KeySet& ks) { seq_.push_back(ks); }

    void clear() { seq_.clear(); }

    std::string toString() const;

private:
    std::vector<KeySet> seq_;
};

#endif
