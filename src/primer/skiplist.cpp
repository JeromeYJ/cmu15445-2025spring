//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// skiplist.cpp
//
// Identification: src/primer/skiplist.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "primer/skiplist.h"
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "common/macros.h"
#include "fmt/core.h"

namespace bustub {

/** @brief Checks whether the container is empty. */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Empty() -> bool { return size_ == 0; }

/** @brief Returns the number of elements in the skip list. */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Size() -> size_t { return size_; }

/**
 * @brief Iteratively deallocate all the nodes.
 *
 * We do this to avoid stack overflow when the skip list is large.
 *
 * If we let the compiler handle the deallocation, it will recursively call the destructor of each node,
 * which could block up the the stack.
 */
SKIPLIST_TEMPLATE_ARGUMENTS void SkipList<K, Compare, MaxHeight, Seed>::Drop() {
  // 由于使用shared_ptr，所以这里不需要手动释放内存
  // shared_ptr在引用计数为0时会自动释放内存
  // 由此需要使得各个shared_ptr的引用计数为0
  for (size_t i = 0; i < MaxHeight; i++) {
    auto curr = std::move(header_->links_[i]);
    while (curr != nullptr) {
      // std::move sets `curr` to the old value of `curr->links_[i]`,
      // and then resets `curr->links_[i]` to `nullptr`.
      curr = std::move(curr->links_[i]);
    }
  }
}

/**
 * @brief Removes all elements from the skip list.
 *
 * Note: You might want to use the provided `Drop` helper function.
 */
SKIPLIST_TEMPLATE_ARGUMENTS void SkipList<K, Compare, MaxHeight, Seed>::Clear() {
  size_ = 0;
  Drop();
}

/**
 * @brief Inserts a key into the skip list.
 *
 * Note: `Insert` will not insert the key if it already exists in the skip list.
 *
 * @param key key to insert.
 * @return true if the insertion is successful, false if the key already exists.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Insert(const K &key) -> bool {
  std::unique_lock lk(rwlock_);
  // 默认pre全为header_，目的为在插入高度大于height_的Node时，索引大于height_的部分有正确的pre值header
  std::vector<std::shared_ptr<SkipNode>> pre(MaxHeight, header_);
  std::shared_ptr<SkipNode> curr = header_;
  for (int i = static_cast<int>(height_) - 1; i >= 0; i--) {
    while (curr->Next(i) != nullptr && compare_(curr->Next(i)->Key(), key)) {
      curr = curr->Next(i);
    }
    pre[i] = curr;
  }
  curr = curr->Next(0);
  if (curr != nullptr && !compare_(curr->Key(), key) && !compare_(key, curr->Key())) {
    return false;
  }

  // 先做一些成员遍历的更新，以免忘记
  size_++;
  size_t height = RandomHeight();
  if (height > height_) {
    height_ = height;
  }

  auto new_node = std::make_shared<SkipNode>(height, key);
  for (size_t i = 0; i < height; i++) {
    auto tmp = pre[i]->Next(i);
    new_node->SetNext(i, tmp);
    pre[i]->SetNext(i, new_node);
    // pre[i]->Next(i) = newNode;
  }
  return true;
}

/**
 * @brief Erases the key from the skip list.
 *
 * @param key key to erase.
 * @return bool true if the element got erased, false otherwise.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Erase(const K &key) -> bool {
  std::unique_lock lk(rwlock_);
  std::vector<std::shared_ptr<SkipNode>> pre(height_, header_);
  std::shared_ptr<SkipNode> curr = header_;
  for (int i = static_cast<int>(height_) - 1; i >= 0; i--) {
    while (curr->Next(i) != nullptr && compare_(curr->Next(i)->Key(), key)) {
      curr = curr->Next(i);
    }
    pre[i] = curr;
  }

  curr = curr->Next(0);
  if (curr != nullptr && !compare_(curr->Key(), key) && !compare_(key, curr->Key())) {
    size_t height = curr->Height();
    for (int i = 0; i < static_cast<int>(height); i++) {
      pre[i]->SetNext(i, curr->Next(i));
      // pre[i]->Next(i) = std::move(curr->Next(i));
    }
    size_--;
    // 删除node后，需要维护height_
    while (height_ >= 1 && header_->Next(height_ - 1) == nullptr) {
      height_--;
    }
    return true;
  }
  return false;
}

/**
 * @brief Checks whether a key exists in the skip list.
 *
 * @param key key to look up.
 * @return bool true if the element exists, false otherwise.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::Contains(const K &key) -> bool {
  std::shared_lock lk(rwlock_);
  // Following the standard library: Key `a` and `b` are considered equivalent if neither compares less
  // than the other: `!compare_(a, b) && !compare_(b, a)`.
  std::shared_ptr<SkipNode> curr = header_;
  for (int i = static_cast<int>(height_) - 1; i >= 0; i--) {
    while (curr->Next(i) != nullptr && compare_(curr->Next(i)->Key(), key)) {
      curr = curr->Next(i);
    }
    // 若直接找到目标，返回true
    if (curr->Next(i) != nullptr && !compare_(curr->Next(i)->Key(), key) && !compare_(key, curr->Next(i)->Key())) {
      return true;
    }
  }
  curr = curr->Next(0);
  return (curr != nullptr && !compare_(curr->Key(), key) && !compare_(key, curr->Key()));
}

/**
 * @brief Prints the skip list for debugging purposes.
 *
 * Note: You may modify the functions in any way and the output is not tested.
 */
SKIPLIST_TEMPLATE_ARGUMENTS void SkipList<K, Compare, MaxHeight, Seed>::Print() {
  std::shared_lock lk(rwlock_);
  auto node = header_->Next(LOWEST_LEVEL);
  while (node != nullptr) {
    fmt::println("Node {{ key: {}, height: {} }}", node->Key(), node->Height());
    node = node->Next(LOWEST_LEVEL);
  }
}

/**
 * @brief Generate a random height. The height should be cappped at `MaxHeight`.
 * Note: we implement/simulate the geometric process to ensure platform independence.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::RandomHeight() -> size_t {
  // Branching factor (1 in 4 chance), see Pugh's paper.
  static constexpr unsigned int branching_factor = 4;
  // Start with the minimum height
  size_t height = 1;
  while (height < MaxHeight && (rng_() % branching_factor == 0)) {
    height++;
  }
  return height;
}

/**
 * @brief Gets the current node height.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::SkipNode::Height() const -> size_t {
  return links_.size();
}

/**
 * @brief Gets the next node by following the link at `level`.
 *
 * @param level index to the link.
 * @return std::shared_ptr<SkipNode> the next node, or `nullptr` if such node does not exist.
 */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::SkipNode::Next(size_t level) const
    -> std::shared_ptr<SkipNode> {
  if (level >= links_.size()) {
    throw "In Next() : level " + std::to_string(level) + " out of range.";
  }
  return links_[level];
}

/**
 * @brief Set the `node` to be linked at `level`.
 *
 * @param level index to the link.
 *
 * 写这里时这里对于引用传递的理解出现了偏颇，这里node和links_[level]之间的赋值还是值的copy，只有在函数中对node进行的改变会影响到外界的node才是引用传递发挥作用之处
 * 同时这里用了const，node不可改变。这里用引用的主要作用是节省空间与其他开销（比如智能指针的引用计数维护开销）
 */
SKIPLIST_TEMPLATE_ARGUMENTS void SkipList<K, Compare, MaxHeight, Seed>::SkipNode::SetNext(
    size_t level, const std::shared_ptr<SkipNode> &node) {
  if (level >= links_.size()) {
    throw "In SetNext() : level " + std::to_string(level) + " out of range.";
  }
  links_[level] = node;
}

/** @brief Returns a reference to the key stored in the node. */
SKIPLIST_TEMPLATE_ARGUMENTS auto SkipList<K, Compare, MaxHeight, Seed>::SkipNode::Key() const -> const K & {
  return key_;
}

// Below are explicit instantiation of template classes.
template class SkipList<int>;
template class SkipList<std::string>;
template class SkipList<int, std::greater<>>;
template class SkipList<int, std::less<>, 8>;

}  // namespace bustub
