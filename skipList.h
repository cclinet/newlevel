#include "random.h"
#include <assert.h>
#include <atomic>
#include <compare>

template <typename Key> class SkipList {
private:
  struct Node;

public:
  explicit SkipList(std::compare_three_way cmp);
  SkipList(const SkipList &) = delete;
  SkipList operator=(SkipList &) = delete;

  void insert(const Key &key);
  bool contains(const Key &key) const;

private:
  static constexpr std::size_t kMaxHeight = 12;

  std::compare_three_way const compare_;

  Node *new_node(const Key &key, int height);
  Node *find_greater_or_equal(const Key &key, Node **prev) const;
  bool key_is_after_node(const Key &key, Node *n) const;
  Node *const head_;
  std::atomic<int> max_height_;

  inline int get_max_height() const {
    return max_height_.load(std::memory_order_relaxed);
  }

  Random rnd_;
  int random_height();

public:
};

template <typename Key> struct SkipList<Key>::Node {
  explicit Node(const Key &k) : key(k){};
  Key const key;
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

template <typename Key>
struct SkipList<Key>::Node *SkipList<Key>::new_node(const Key &key,
                                                    int height) {
  std::byte *node_memory = static_cast<std::byte *>(operator new(
      sizeof(Node) + sizeof(std::atomic<Node *>) * (height - 1)));
  return new (node_memory) Node(key);
}

template <typename Key>
void SkipList<Key>::insert(const Key &key) {
  Node *prev[kMaxHeight];
  Node *x = find_greater_or_equal(key, prev);
  // assert(x == nullptr || !Equal(key, x->key));

  int height = random_height();
  x = new_node(key, height);
  for (int i = 0; i < height; i++) {
    x->set_next(i, prev[i]->next(i));
    prev[i]->set_next(i, x);
  }
}

template <typename Key> int SkipList<Key>::random_height() {
  static const unsigned int kBranching = 4;
  int height = 1;
  while (height < kMaxHeight && rnd_.one_in(kBranching)) {
    height++;
  }
  assert(height > 0);
  assert(height <= kMaxHeight);
  return height;
}

template <typename Key>
SkipList<Key>::Node *SkipList<Key>::find_greater_or_equal(const Key &key,
                                                          Node **prev) const {
  Node *x = head_;
  int level = get_max_height() - 1;

  while (true) {

    Node *next = x->next(level);

    if (key_is_after_node(key, next)) {
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

template <typename Key>
bool SkipList<Key>::key_is_after_node(const Key &key, Node *n) const {
  // null n is considered infinite
  return (n != nullptr) && (compare_(n->key, key) < 0);
}

template <typename Key>
inline SkipList<Key>::SkipList(std::compare_three_way cmp)
    : compare_(cmp), head_(new_node(0, kMaxHeight)), max_height_(1) {
  for (int i = 0; i < kMaxHeight; i++) {
    head_->set_next(i, nullptr);
  }
};
