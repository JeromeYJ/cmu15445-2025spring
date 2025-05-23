#include "primer/hyperloglog_presto.h"

namespace bustub {

template <typename KeyType>
HyperLogLogPresto<KeyType>::HyperLogLogPresto(int16_t n_leading_bits)
    : dense_bucket_(1 << (n_leading_bits > 0 ? n_leading_bits : 0), std::bitset<DENSE_BUCKET_SIZE>()),
      cardinality_(0),
      leading_bits_(n_leading_bits > 0 ? n_leading_bits : 0) {}

template <typename KeyType>
auto HyperLogLogPresto<KeyType>::AddElem(KeyType val) -> void {
  /** @TODO(student) Implement this function! */
  hash_t hashcode = CalculateHash(val);
  const int size = sizeof(hash_t) * 8;
  std::bitset<size> bits(hashcode);
  int index = (bits >> (size - leading_bits_)).to_ulong();
  int i = 0;
  uint64_t cnt = 0;
  // 这里leading_bits_的几位不用统计0数量，不在统计范围内
  while (i < (size - leading_bits_) && !bits.test(i)) {
    cnt++;
    i++;
  }
  uint64_t overflow_value = overflow_bucket_[index].to_ulong();
  uint64_t dense_value = dense_bucket_[index].to_ulong();
  if ((overflow_value << DENSE_BUCKET_SIZE) + dense_value < cnt) {
    overflow_value = cnt >> DENSE_BUCKET_SIZE;
    // 这个使得二进制数保留后n位的运算方式要记住！
    // 且下句操作等价cnt % (1 << DENSE_BUCKET_SIZE)
    dense_value = cnt & ((1 << DENSE_BUCKET_SIZE) - 1);
    overflow_bucket_[index] = overflow_value;
    dense_bucket_[index] = dense_value;
  }
}

template <typename T>
auto HyperLogLogPresto<T>::ComputeCardinality() -> void {
  /** @TODO(student) Implement this function! */
  double sum = 0;
  uint16_t m = (1 << leading_bits_);
  for (uint16_t i = 0; i < m; i++) {
    // 教训:一定不要对无符号数取负数，会变成一个很大的数！要先转换为有符号数后再取负数，要么就少用无符号数！
    int tmp = (overflow_bucket_[i].to_ulong() << DENSE_BUCKET_SIZE) + dense_bucket_[i].to_ulong();
    sum += pow(2.0, -tmp);
  }
  cardinality_ = (CONSTANT * m * m) / sum;
}

template class HyperLogLogPresto<int64_t>;
template class HyperLogLogPresto<std::string>;
}  // namespace bustub
