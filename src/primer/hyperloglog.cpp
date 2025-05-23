#include "primer/hyperloglog.h"

namespace bustub {

template <typename KeyType>
HyperLogLog<KeyType>::HyperLogLog(int16_t n_bits)
    : cardinality_(0), b_(n_bits > 0 ? n_bits : 0), buckets_(std::vector<uint64_t>(n_bits > 0 ? 1 << n_bits : 1)) {}

template <typename KeyType>
auto HyperLogLog<KeyType>::ComputeBinary(const hash_t &hash) const -> std::bitset<BITSET_CAPACITY> {
  std::bitset<BITSET_CAPACITY> bits(hash);
  return bits;
}

template <typename KeyType>
auto HyperLogLog<KeyType>::PositionOfLeftmostOne(const std::bitset<BITSET_CAPACITY> &bset) const -> uint64_t {
  int i = BITSET_CAPACITY - 1 - b_;
  uint64_t ans = 1;
  while (i >= 0 && !bset.test(i)) {
    ans++;
    i--;
  }
  return ans;
}

template <typename KeyType>
auto HyperLogLog<KeyType>::AddElem(KeyType val) -> void {
  hash_t hashcode = CalculateHash(val);
  std::bitset<BITSET_CAPACITY> bits = ComputeBinary(hashcode);
  uint64_t p = PositionOfLeftmostOne(bits);
  uint64_t index = (bits >> (BITSET_CAPACITY - b_)).to_ulong();
  buckets_[index] = std::max(buckets_[index], p);
}

template <typename KeyType>
auto HyperLogLog<KeyType>::ComputeCardinality() -> void {
  double sum = 0;
  uint64_t m = (1 << b_);
  for (uint64_t i = 0; i < m; i++) {
    // 教训:一定不要对无符号数取负数，会变成一个很大的数！要先转换为有符号数后再取负数，要么就少用无符号数！
    sum += pow(2.0, -static_cast<double>(buckets_[i]));
  }
  cardinality_ = (CONSTANT * m * m) / sum;
}

template class HyperLogLog<int64_t>;
template class HyperLogLog<std::string>;

}  // namespace bustub
