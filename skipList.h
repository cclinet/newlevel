#include <atomic>
#include <cassert>
#include <compare>
#include <cstddef>
#include <new>

template <std::three_way_comparable Key> class SkipList {
private:
  struct Node;

public:
  SkipList();

  void insert(const Key &key);
  bool contains(const Key &key) const;

private:
  static constexpr int kMaxHeight = 12;

  Node *new_node(const Key &key, int height);

  Node *head_;
  std::atomic<int> max_height_;

  Node *find_greater_or_equal(const Key &key, Node **prev) const;
};

// Node 定义
template <std::three_way_comparable Key> struct SkipList<Key>::Node {
public:
  Key const key;

  explicit Node(const Key &k) : key(k){};

  Node *next(int n) {
    assert(n >= 0);
    return next_[n].load(std::memory_order_acquire);
  }

  void set_next(int n, Node *x) {
    assert(n >= 0);
    next_[n].store(x, std::memory_order_release);
  }

private:
  std::atomic<Node *> next_[1];
};

template <std::three_way_comparable Key>
SkipList<Key>::Node *SkipList<Key>::new_node(const Key &key, int height) {
  std::byte *node_memory = static_cast<std::byte *>(operator new(
      sizeof(Node) + sizeof(std::atomic<Node *>) * (height - 1)));
  return new (node_memory) Node(key);
};

template <std::three_way_comparable Key>
SkipList<Key>::SkipList() : head_(new_node(0, kMaxHeight)), max_height_(1) {
  for (int i = 0; i < kMaxHeight; i++) {
    head_->set_next(i, nullptr);
  }
};

template <std::three_way_comparable Key>
void SkipList<Key>::insert(const Key &key) {
  Node *prev[kMaxHeight];
  Node *x = find_greater_or_equal(key, prev);
  int height = 3; // TODO: use random
  x = new_node(key, height);
  for (int i = 0; i < height; i++) {
    x->set_next(i, prev[i]->next(i));
    prev[i]->set_next(i, x);
  }
};

template <std::three_way_comparable Key>
bool SkipList<Key>::contains(const Key &key) const {
  Node *x = find_greater_or_equal(key, nullptr);
  if (x != nullptr && key == x->key)
    return true;
  return false;
};

template <std::three_way_comparable Key>
typename SkipList<Key>::Node *
SkipList<Key>::find_greater_or_equal(const Key &key, Node **prev) const {
  Node *x = head_;
  int level = kMaxHeight - 1;

  while (true) {
    Node *next = x->next(level);
    if (next != nullptr && next->key < key) {
      x = next;
    } else {
      if (prev != nullptr)
        prev[level] = x;

      if (level == 0)
        return next;
      else
        level--;
    }
  }
};
