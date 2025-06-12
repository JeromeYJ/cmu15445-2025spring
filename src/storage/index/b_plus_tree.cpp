#include "storage/index/b_plus_tree.h"
#include "storage/index/b_plus_tree_debug.h"

namespace bustub {

INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(std::string name, page_id_t header_page_id, BufferPoolManager *buffer_pool_manager,
                          const KeyComparator &comparator, int leaf_max_size, int internal_max_size)
    : index_name_(std::move(name)),
      bpm_(buffer_pool_manager),
      comparator_(std::move(comparator)),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size),
      header_page_id_(header_page_id) {
  WritePageGuard guard = bpm_->WritePage(header_page_id_);
  auto root_page = guard.AsMut<BPlusTreeHeaderPage>();
  root_page->root_page_id_ = INVALID_PAGE_ID;
}

/*
 * Helper function to decide whether current b+tree is empty
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty() const -> bool {
  ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  auto head_page = guard.As<BPlusTreeHeaderPage>();
  return head_page->root_page_id_ == INVALID_PAGE_ID;
}

/**
 * 使用二分查找进行键查找的函数
 * 内部节点与叶子节点的查找方式和返回形式有所不同
 * @return 键对应的值在数组中的index(叶子结点) or 键对应的下一层要查找的page_id在值数组中的index(内部结点)
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::KeyBinarySearch(BPlusTreePage *page, const KeyType &key) -> int {
  int l, r;

  if (page->IsLeafPage()) {
    LeafPage *leaf_page = static_cast<LeafPage*>(page);
    l = 0, r = leaf_page->GetSize() - 1;
    while (l <= r) {
      int mid = (l + r) >> 1;
      if (comparator_(key, leaf_page->KeyAt(mid)) == 0) {
        return mid;
      }
      if (comparator_(key, leaf_page->KeyAt(mid)) < 0) {
        r = mid - 1;
      } else {
        l = mid + 1;
      }
    }
  } 
  else {
    InternalPage *internal_page = static_cast<InternalPage*>(page);
    // 注意这里 l 要为 1
    l = 1, r = internal_page->GetSize() - 1;
    int size = internal_page->GetSize();
    while (l <= r) {
      int mid = (l + r) >> 1;
      if (comparator_(internal_page->KeyAt(mid), key) <= 0) {
        if (mid + 1 >= size || comparator_(internal_page->KeyAt(mid + 1), key) > 0) {
          return mid;
        }
        l = mid + 1;
      }
      else {
        r = mid - 1;
      }
    }  // 1 3 4 5 6 8 9 12 14
  }

  return -1;
}


/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/**
 * Return the only value that associated with input key
 * This method is used for point query(点查询)
 * @return : true means key exists
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetValue(const KeyType &key, std::vector<ValueType> *result) -> bool {
  // Declaration of context instance.
  Context ctx;

  // ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  // auto head_page = guard.As<BPlusTreeHeaderPage>();
  // ctx.root_page_id_ = head_page->root_page_id_;
  // guard.Drop();

  // if (ctx.root_page_id_ == INVALID_PAGE_ID) {
  //   return false;
  // }

  // ReadPageGuard page_guard = bpm_->ReadPage(ctx.root_page_id_);
  // auto page = page_guard.As<BPlusTreePage>();
  // while (!page->IsLeafPage()) {
    
  // }


  // (void)ctx;
  return false;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/**
 * Insert constant key & value pair into b+ tree
 * if current tree is empty, start new tree, update root page id and insert
 * entry, otherwise insert into leaf page.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value) -> bool {
  // Declaration of context instance.
  Context ctx;

  WritePageGuard head_guard = bpm_->WritePage(header_page_id_);
  ctx.header_page_ = std::make_optional(std::move(head_guard));
  ctx.root_page_id_ = head_guard.AsMut<BPlusTreeHeaderPage>()->root_page_id_;

  /* (1) 如果tree是空的 */
  if (IsEmpty()) {
    // 创建根节点，根节点不需要遵循最小size原则。同时该根节点也是叶子节点
    page_id_t root_page_id = bpm_->NewPage();
    WritePageGuard root_guard = bpm_->WritePage(root_page_id);
    auto root_page = root_guard.AsMut<LeafPage>();
    root_page->Init(leaf_max_size_);
    root_page->SetKeyAt(0, key);
    root_page->SetValueAt(0, value);

    auto head_page = ctx.header_page_->AsMut<BPlusTreeHeaderPage>();
    head_page->root_page_id_ = root_page_id;
    
    // 维护Context类型对象
    // ctx.root_page_id_ = root_page_id;
    // ctx.write_set_.push_back(std::move(root_guard));

    return true;
  }

  /* (2) 如果tree不是空的 */


  // (void)ctx;
  return false;
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Delete key & value pair associated with input key
 * If current tree is empty, return immediately.
 * If not, User needs to first find the right leaf page as deletion target, then
 * delete entry from leaf page. Remember to deal with redistribute or merge if
 * necessary.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key) {
  // Declaration of context instance.
  Context ctx;
  (void)ctx;
}

/*****************************************************************************
 * INDEX ITERATOR
 *****************************************************************************/
/*
 * Input parameter is void, find the leftmost leaf page first, then construct
 * index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin() -> INDEXITERATOR_TYPE { return INDEXITERATOR_TYPE(); }

/*
 * Input parameter is low key, find the leaf page that contains the input key
 * first, then construct index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin(const KeyType &key) -> INDEXITERATOR_TYPE { return INDEXITERATOR_TYPE(); }

/*
 * Input parameter is void, construct an index iterator representing the end
 * of the key/value pair in the leaf node
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::End() -> INDEXITERATOR_TYPE { return INDEXITERATOR_TYPE(); }

/**
 * @return Page id of the root of this tree
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t { return 0; }

template class BPlusTree<GenericKey<4>, RID, GenericComparator<4>>;

template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>>;

template class BPlusTree<GenericKey<16>, RID, GenericComparator<16>>;

template class BPlusTree<GenericKey<32>, RID, GenericComparator<32>>;

template class BPlusTree<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
