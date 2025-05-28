//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include "common/exception.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict() -> std::optional<frame_id_t> {
  std::unique_lock lk(latch_);
  // 先看未达到k个历史引用的frame
  if (!new_frames_.empty()) {
    // 使用反向迭代器进行反向遍历
    for (auto it = new_frames_.rbegin(); it != new_frames_.rend(); ++it) {
      frame_id_t frame_id = *it;
      if (!node_store_[frame_id].is_evictable_) {
        continue;
      }
      node_store_.erase(frame_id);
      new_frames_.erase(new_frames_locator_[frame_id]);
      new_frames_locator_.erase(frame_id);
      curr_size_--;
      return frame_id;
    }
  }

  if (!cache_frames_.empty()) {
    for (auto it = cache_frames_.rbegin(); it != cache_frames_.rend(); ++it) {
      frame_id_t frame_id = *it;
      if (!node_store_[frame_id].is_evictable_) {
        continue;
      }
      node_store_.erase(frame_id);
      cache_frames_.erase(cache_frames_locator_[frame_id]);
      cache_frames_locator_.erase(frame_id);
      curr_size_--;
      return frame_id;
    }
  }

  // 没有frame可以被替换，返回nullopt
  return std::nullopt;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, AccessType access_type) {
  std::unique_lock lk(latch_);
  if (frame_id > static_cast<int>(replacer_size_)) {
    throw std::runtime_error(std::to_string(frame_id) +
                             " is larger than replacer_size :" + std::to_string(replacer_size_));
  }
  // (1)新加入的frame
  if (node_store_.find(frame_id) == node_store_.end()) {
    node_store_[frame_id].history_.push_front(current_timestamp_);
    new_frames_.push_front(frame_id);
    new_frames_locator_[frame_id] = new_frames_.begin();
  }
  // (2)原本history中引用历史数量小于k-1
  else if (node_store_[frame_id].history_.size() < k_ - 1) {
    node_store_[frame_id].history_.push_front(current_timestamp_);
    // 这里小于k时要求是更早进入的先淘汰，即考虑最早的时间戳，不考虑之后的引用情况。与LRU不太一样
    // 也可以用splice()实现迭代器的位置转移
    // new_frames_.erase(new_frames_locator_[frame_id]);
    // new_frames_.push_front(frame_id);
    // new_frames_locator_[frame_id] = new_frames_.begin();
  }
  // (3)原本history中引用历史数量为k-1
  else if (node_store_[frame_id].history_.size() == k_ - 1) {
    node_store_[frame_id].history_.push_front(current_timestamp_);
    new_frames_.erase(new_frames_locator_[frame_id]);
    new_frames_locator_.erase(frame_id);
    auto it = cache_frames_.begin();
    size_t k_timestamp = node_store_[frame_id].history_.back();
    for (frame_id_t id : cache_frames_) {
      if (k_timestamp > node_store_[id].history_.back()) {
        it = cache_frames_locator_[id];
        break;
      }
    }
    auto inserted_it = cache_frames_.insert(it, frame_id);
    cache_frames_locator_[frame_id] = inserted_it;
  }
  // (4)原本history中引用历史数量为k
  else if (node_store_[frame_id].history_.size() == k_) {
    node_store_[frame_id].history_.push_front(current_timestamp_);
    node_store_[frame_id].history_.pop_back();
    size_t k_timestamp = node_store_[frame_id].history_.back();

    auto erase_next = cache_frames_.erase(cache_frames_locator_[frame_id]);
    auto insert_next = erase_next;
    // 还可以有反向迭代器的写法，但是会更麻烦，需要注意迭代器类型转换为反向迭代器
    // 这里由于begin()是第一个元素的迭代器，需要修改for循环内的实现，需要每次循环先做--it，循环后的--it取消
    for (auto it = erase_next; it != cache_frames_.begin();) {
      --it;
      if (k_timestamp < node_store_[*it].history_.back()) {
        break;
      }
      insert_next = it;
    }
    auto inserted_it = cache_frames_.insert(insert_next, frame_id);
    cache_frames_locator_[frame_id] = inserted_it;
  }

  // 最后记得时间戳加一
  current_timestamp_++;
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::unique_lock lk(latch_);
  if (node_store_.find(frame_id) == node_store_.end()) {
    return;
  }
  // 栈内的对象，不要用智能指针管理，会出现提前被delete的情况。尤其是容器内对象有自己的管理方式。智能指针相当于一种“直接接管对象”的形式
  // 智能指针管理new创建的对象，即堆上的对象
  // 智能指针依旧要慎用
  // std::unique_ptr<LRUKNode> ptr = std::make_unique<LRUKNode>(node_store_[frame_id]);
  if (node_store_[frame_id].is_evictable_ && !set_evictable) {
    curr_size_--;
  }
  if (!node_store_[frame_id].is_evictable_ && set_evictable) {
    curr_size_++;
  }
  node_store_[frame_id].is_evictable_ = set_evictable;
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::unique_lock lk(latch_);
  if (node_store_.find(frame_id) == node_store_.end()) {
    return;
  }
  if (!node_store_[frame_id].is_evictable_) {
    throw std::runtime_error(std::to_string(frame_id) + " is not evictable.");
  }

  if (node_store_[frame_id].history_.size() < k_) {
    new_frames_.erase(new_frames_locator_[frame_id]);
    new_frames_locator_.erase(frame_id);
  } else {
    cache_frames_.erase(cache_frames_locator_[frame_id]);
    cache_frames_locator_.erase(frame_id);
  }

  curr_size_--;
  node_store_.erase(frame_id);
}

auto LRUKReplacer::Size() -> size_t {
  std::unique_lock lk(latch_);
  return curr_size_;
}

}  // namespace bustub
