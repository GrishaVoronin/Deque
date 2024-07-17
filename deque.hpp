#pragma once
#include <iostream>
#include <memory>

template <typename T, typename Allocator = std::allocator<T>>
class Deque {
 public:
  Deque() = default;
  Deque(const Allocator& allocator);
  Deque(const Deque& other);
  Deque(size_t count, const Allocator& alloc = Allocator());
  explicit Deque(size_t count, const T& value,
                 const Allocator& alloc = Allocator());
  Deque(Deque&& other);
  Deque(std::initializer_list<T> init, const Allocator& alloc = Allocator());
  ~Deque();
  Deque<T, Allocator>& operator=(const Deque& other);
  Deque<T, Allocator>& operator=(Deque&& other);
  size_t size() const;
  bool empty() const;
  T& operator[](size_t ind);
  const T& operator[](size_t ind) const;
  T& at(size_t ind);
  const T& at(size_t ind) const;
  void push_back(T&& value);
  void push_back(const T& value);
  void pop_back();
  void push_front(T&& value);
  void push_front(const T& value);
  void pop_front();
  template <typename... Arguments>
  void emplace_back(Arguments&&... args);
  template <typename... Arguments>
  void emplace_front(Arguments&&... args);

  template <bool IsConst>
  class Iterator;

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<Iterator<false>>;
  using const_reverse_iterator = std::reverse_iterator<Iterator<true>>;

  iterator begin();
  const_iterator cbegin() const;
  iterator end();
  const_iterator cend() const;
  reverse_iterator rbegin();
  reverse_iterator rend();
  const_reverse_iterator crbegin() const;
  const_reverse_iterator crend() const;

  void insert(iterator iter, const T& value);
  void erase(iterator iter);
  void emplace(iterator iter, T&& value);

  using allocator_type = Allocator;
  using allocator_traits = std::allocator_traits<allocator_type>;

  using container_allocator =
      typename std::allocator_traits<Allocator>::template rebind_alloc<T*>;
  using container_allocator_traits = std::allocator_traits<container_allocator>;

  allocator_type get_allocator() const { return alloc_; }

 private:
  void clear(size_t cur_bucket);
  void reallocation();
  void swap(Deque& other);
  size_t container_capacity_ = 0;
  size_t size_ = 0;
  T** container_ = nullptr;
  size_t first_element_bucket_ = 0;
  size_t first_element_position_ = 2;
  size_t last_element_bucket_ = 0;
  size_t last_element_position_ = 2;
  static const short int kBucketSize = 32;

  allocator_type alloc_;
  container_allocator container_alloc_;
};

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(const Allocator& allocator)
    : alloc_(allocator), container_alloc_(allocator) {}

template <typename T, typename Allocator>
void Deque<T, Allocator>::clear(size_t cur_bucket) {
  for (size_t i = 0; i < cur_bucket; ++i) {
    for (size_t j = 0; j < kBucketSize; ++j) {
      allocator_traits::destroy(alloc_, container_[i] + j);
    }
    allocator_traits::deallocate(alloc_, container_[i] + kBucketSize,
                                 kBucketSize);
  }
  container_allocator_traits::deallocate(container_alloc_, container_,
                                         container_capacity_);
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(const Deque& other)
    : size_(other.size_),
      alloc_(allocator_traits::select_on_container_copy_construction(
          other.alloc_)),
      container_alloc_(
          container_allocator_traits::select_on_container_copy_construction(
              other.container_alloc_)),
      container_capacity_((other.size_ - 1) / kBucketSize + 1) {
  if (other.container_ != nullptr) {
    container_ = container_allocator_traits::allocate(container_alloc_,
                                                      container_capacity_);
    size_t cur_bucket = 0;
    auto element = other.cbegin();
    try {
      for (; cur_bucket < container_capacity_; ++cur_bucket) {
        container_allocator_traits::construct(
            container_alloc_, container_ + cur_bucket,
            std::allocator_traits<allocator_type>::allocate(alloc_,
                                                            kBucketSize));
        int finish = cur_bucket == container_capacity_ - 1
                         ? (size_ - 1) % kBucketSize + 1
                         : kBucketSize;
        int position = 0;
        try {
          for (; position < finish; ++position) {
            allocator_traits::construct(
                alloc_, container_[cur_bucket] + position, *element);
            ++element;
          }
        } catch (...) {
          for (int i = 0; i < position; ++i) {
            allocator_traits::destroy(alloc_, container_[cur_bucket] + i);
          }
          allocator_traits::deallocate(alloc_, container_[cur_bucket],
                                       kBucketSize);
          throw;
        }
      }
    } catch (...) {
      clear(cur_bucket);
      throw;
    }
    last_element_position_ = (size_ - 1) % kBucketSize;
    last_element_bucket_ = container_capacity_ - 1;
    first_element_bucket_ = 0;
    first_element_position_ = 0;
  }
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(size_t count, const Allocator& alloc)
    : size_(count),
      container_capacity_((count - 1) / kBucketSize + 1),
      alloc_(alloc),
      container_alloc_(alloc) {
  container_ = container_allocator_traits::allocate(container_alloc_,
                                                    container_capacity_);
  size_t cur_bucket = 0;
  try {
    for (; cur_bucket < container_capacity_; ++cur_bucket) {
      container_allocator_traits::construct(
          container_alloc_, container_ + cur_bucket,
          std::allocator_traits<allocator_type>::allocate(alloc_, kBucketSize));
      int finish = cur_bucket == container_capacity_ - 1
                       ? (size_ - 1) % kBucketSize + 1
                       : kBucketSize;
      int position = 0;
      try {
        for (; position < finish; ++position) {
          allocator_traits::construct(alloc_,
                                      container_[cur_bucket] + position);
        }
      } catch (...) {
        for (int i = 0; i < position; ++i) {
          allocator_traits::destroy(alloc_, container_[cur_bucket] + i);
        }
        allocator_traits::deallocate(alloc_, container_[cur_bucket],
                                     kBucketSize);
        throw;
      }
    }
  } catch (...) {
    clear(cur_bucket);
    throw;
  }
  last_element_position_ = (count - 1) % kBucketSize;
  last_element_bucket_ = container_capacity_ - 1;
  first_element_bucket_ = 0;
  first_element_position_ = 0;
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(size_t count, const T& value, const Allocator& alloc)
    : size_(count),
      container_capacity_((count - 1) / kBucketSize + 1),
      alloc_(alloc),
      container_alloc_(alloc) {
  container_ = container_allocator_traits::allocate(container_alloc_,
                                                    container_capacity_);
  size_t cur_bucket = 0;
  try {
    for (; cur_bucket < container_capacity_; ++cur_bucket) {
      container_allocator_traits::construct(
          container_alloc_, container_ + cur_bucket,
          std::allocator_traits<allocator_type>::allocate(alloc_, kBucketSize));
      int finish = cur_bucket == container_capacity_ - 1
                       ? (size_ - 1) % kBucketSize + 1
                       : kBucketSize;
      int position = 0;
      try {
        for (; position < finish; ++position) {
          allocator_traits::construct(alloc_, container_[cur_bucket] + position,
                                      value);
        }
      } catch (...) {
        for (int i = 0; i < position; ++i) {
          allocator_traits::destroy(alloc_, container_[cur_bucket] + i);
        }
        allocator_traits::deallocate(alloc_, container_[cur_bucket],
                                     kBucketSize);
        throw;
      }
    }
  } catch (...) {
    clear(cur_bucket);
    throw;
  }
  last_element_position_ = (count - 1) % kBucketSize;
  last_element_bucket_ = container_capacity_ - 1;
  first_element_bucket_ = 0;
  first_element_position_ = 0;
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(Deque&& other)
    : container_(other.container_),
      alloc_(other.alloc_),
      container_alloc_(other.container_alloc_),
      container_capacity_(other.container_capacity_),
      size_(other.size_),
      first_element_bucket_(other.first_element_bucket_),
      first_element_position_(other.first_element_position_),
      last_element_bucket_(other.last_element_bucket_),
      last_element_position_(other.last_element_position_) {
  other.container_ = nullptr;
  other.container_capacity_ = 0;
  other.size_ = 0;
  other.first_element_bucket_ = 0;
  other.first_element_position_ = 0;
  other.last_element_bucket_ = 0;
  other.last_element_position_ = 0;
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(std::initializer_list<T> init,
                           const Allocator& alloc)
    : size_(init.size()),
      container_capacity_((init.size() - 1) / kBucketSize + 1),
      alloc_(alloc),
      container_alloc_(alloc) {
  container_ = container_allocator_traits::allocate(container_alloc_,
                                                    container_capacity_);
  size_t cur_bucket = 0;
  auto value = init.begin();
  try {
    for (; cur_bucket < container_capacity_; ++cur_bucket) {
      container_allocator_traits::construct(
          container_alloc_, container_ + cur_bucket,
          allocator_traits::allocate(alloc_, kBucketSize));
      size_t border = cur_bucket == container_capacity_ - 1
                          ? (size_ - 1) % kBucketSize + 1
                          : kBucketSize;
      size_t position = 0;
      try {
        for (; position < border; ++position) {
          allocator_traits::construct(alloc_, container_[cur_bucket] + position,
                                      *value);
          ++value;
        }
      } catch (...) {
        for (size_t i = 0; i < position; ++i) {
          allocator_traits::destroy(alloc_, container_[cur_bucket] + i);
        }
        allocator_traits::deallocate(alloc_, container_[cur_bucket],
                                     kBucketSize);
        throw;
      }
    }
  } catch (...) {
    clear(cur_bucket);
    throw;
  }
  last_element_position_ = (size_ - 1) % kBucketSize;
  last_element_bucket_ = container_capacity_ - 1;
  first_element_bucket_ = 0;
  first_element_position_ = 0;
}

template <typename T, typename Allocator>
Deque<T, Allocator>::~Deque() {
  if (container_ != nullptr) {
    for (size_t i = 0; i < first_element_bucket_; ++i) {
      allocator_traits::deallocate(alloc_, container_[i], kBucketSize);
    }
    for (size_t i = first_element_bucket_; i <= last_element_bucket_; ++i) {
      int start = i == first_element_bucket_ ? first_element_position_ : 0;
      int finish =
          i == last_element_bucket_ ? last_element_position_ + 1 : kBucketSize;
      for (; start < finish; ++start) {
        allocator_traits::destroy(alloc_, container_[i] + start);
      }
      allocator_traits::deallocate(alloc_, container_[i], kBucketSize);
    }
    for (size_t i = last_element_bucket_ + 1; i < container_capacity_; ++i) {
      allocator_traits::deallocate(alloc_, container_[i], kBucketSize);
    }
    container_allocator_traits::deallocate(container_alloc_, container_,
                                           container_capacity_);
  }
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::swap(Deque& other) {
  std::swap(container_capacity_, other.container_capacity_);
  std::swap(size_, other.size_);
  std::swap(container_, other.container_);
  std::swap(first_element_bucket_, other.first_element_bucket_);
  std::swap(first_element_position_, other.first_element_position_);
  std::swap(last_element_bucket_, other.last_element_bucket_);
  std::swap(last_element_position_, other.last_element_position_);
  std::swap(alloc_, other.alloc_);
  std::swap(container_alloc_, other.container_alloc_);
}

template <typename T, typename Allocator>
Deque<T, Allocator>& Deque<T, Allocator>::operator=(const Deque& other) {
  if (this != &other) {
    allocator_type next_allocator = alloc_;
    allocator_type old_allocator = alloc_;
    size_t old_size = size_;
    if (std::allocator_traits<
            allocator_type>::propagate_on_container_copy_assignment::value) {
      next_allocator = other.alloc_;
    }
    size_t cur_index = 0;
    auto other_element = other.cbegin();
    try {
      alloc_ = next_allocator;
      for (; cur_index < other.size_; ++cur_index) {
        push_back(*other_element);
        ++other_element;
      }
    } catch (...) {
      for (size_t i = 0; i < cur_index - 1; ++i) {
        pop_back();
      }
      alloc_ = old_allocator;
      throw;
    }
    alloc_ = old_allocator;
    for (size_t i = 0; i < old_size; ++i) {
      pop_front();
    }
    alloc_ = next_allocator;
  }
  return *this;
}

template <typename T, typename Allocator>
Deque<T, Allocator>& Deque<T, Allocator>::operator=(Deque&& other) {
  swap(other);
  return *this;
}

template <typename T, typename Allocator>
size_t Deque<T, Allocator>::size() const {
  return size_;
}

template <typename T, typename Allocator>
bool Deque<T, Allocator>::empty() const {
  return size_ == 0;
}

template <typename T, typename Allocator>
T& Deque<T, Allocator>::operator[](size_t ind) {
  if (ind <= kBucketSize - first_element_position_ - 1) {
    return container_[first_element_bucket_][first_element_position_ + ind];
  }
  ind -= kBucketSize - first_element_position_;
  return container_[first_element_bucket_ + 1 + ind / kBucketSize]
                   [ind % kBucketSize];
}

template <typename T, typename Allocator>
const T& Deque<T, Allocator>::operator[](size_t ind) const {
  if (ind <= kBucketSize - first_element_position_ - 1) {
    return container_[first_element_bucket_][first_element_position_ + ind];
  }
  ind -= kBucketSize - first_element_position_;
  return container_[first_element_bucket_ + 1 + ind / kBucketSize]
                   [ind % kBucketSize];
}

template <typename T, typename Allocator>
T& Deque<T, Allocator>::at(size_t ind) {
  if (ind >= size_) {
    throw std::out_of_range("Index out of range");
  }
  if (ind <= kBucketSize - first_element_position_ - 1) {
    return container_[first_element_bucket_][first_element_position_ + ind];
  }
  ind -= kBucketSize - first_element_position_;
  return container_[first_element_bucket_ + 1 + ind / kBucketSize]
                   [ind % kBucketSize];
}

template <typename T, typename Allocator>
const T& Deque<T, Allocator>::at(size_t ind) const {
  if (ind >= size_) {
    throw std::out_of_range("Index out of range");
  }
  if (ind <= kBucketSize - first_element_position_ - 1) {
    return container_[first_element_bucket_][first_element_position_ + ind];
  }
  ind -= kBucketSize - first_element_position_;
  return container_[first_element_bucket_ + 1 + ind / kBucketSize]
                   [ind % kBucketSize];
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::reallocation() {
  size_t new_container_capacity = container_capacity_ * 3 + 1;
  T** new_container = container_allocator_traits::allocate(
      container_alloc_, new_container_capacity);
  size_t buckets_filled = 0;
  try {
    for (size_t i = 0; i < container_capacity_; ++i) {
      container_allocator_traits::construct(
          container_alloc_, new_container + i,
          allocator_traits::allocate(alloc_, kBucketSize));
      ++buckets_filled;
    }
    for (size_t i = 0; i < container_capacity_; ++i) {
      container_allocator_traits::construct(
          container_alloc_, new_container + container_capacity_ + i,
          container_[i]);
      ++buckets_filled;
    }
    for (size_t i = 2 * container_capacity_; i < new_container_capacity; ++i) {
      container_allocator_traits::construct(
          container_alloc_, new_container + i,
          allocator_traits::allocate(alloc_, kBucketSize));
      ++buckets_filled;
    }
  } catch (...) {
    for (size_t i = 0; i < buckets_filled; ++i) {
      container_allocator_traits::destroy(container_alloc_, new_container + i);
    }
    container_allocator_traits::deallocate(container_alloc_, new_container,
                                           new_container_capacity);
    throw;
  }
  container_allocator_traits::deallocate(container_alloc_, container_,
                                         container_capacity_);
  first_element_bucket_ = container_capacity_ + first_element_bucket_;
  last_element_bucket_ = container_capacity_ + last_element_bucket_;
  container_capacity_ = new_container_capacity;
  container_ = new_container;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_back(T&& value) {
  emplace_back(std::move(value));
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_back(const T& value) {
  emplace_back(value);
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::pop_back() {
  allocator_traits::destroy(
      alloc_, container_[last_element_bucket_] + last_element_position_);
  --size_;
  if (!empty()) {
    if (last_element_position_ == 0) {
      --last_element_bucket_;
      last_element_position_ = kBucketSize - 1;
    } else {
      --last_element_position_;
    }
  }
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_front(T&& value) {
  emplace_front(std::move(value));
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_front(const T& value) {
  emplace_front(value);
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::pop_front() {
  allocator_traits::destroy(
      alloc_, container_[first_element_bucket_] + first_element_position_);
  --size_;
  if (!empty()) {
    if (first_element_position_ == kBucketSize - 1) {
      ++first_element_bucket_;
      first_element_position_ = 0;
    } else {
      ++first_element_position_;
    }
  }
}

template <typename T, typename Allocator>
template <typename... Arguments>
void Deque<T, Allocator>::emplace_back(Arguments&&... args) {
  if (container_capacity_ == 0 ||
      last_element_bucket_ == container_capacity_ - 1 &&
          last_element_position_ == kBucketSize - 1) {
    reallocation();
  }
  try {
    if (!empty()) {
      if (last_element_position_ == kBucketSize - 1) {
        ++last_element_bucket_;
        last_element_position_ = 0;
      } else {
        ++last_element_position_;
      }
    }
    allocator_traits::construct(
        alloc_, container_[last_element_bucket_] + last_element_position_,
        std::forward<Arguments>(args)...);
  } catch (...) {
    allocator_traits::destroy(
        alloc_, container_[last_element_bucket_] + last_element_position_);
    if (last_element_position_ == 0) {
      --last_element_bucket_;
      last_element_position_ = kBucketSize - 1;
    } else {
      --last_element_position_;
    }
    throw;
  }
  ++size_;
}

template <typename T, typename Allocator>
template <typename... Arguments>
void Deque<T, Allocator>::emplace_front(Arguments&&... args) {
  if (container_capacity_ == 0 ||
      first_element_bucket_ == 0 && first_element_position_ == 0) {
    reallocation();
  }
  try {
    if (!empty()) {
      if (first_element_position_ == 0) {
        first_element_position_ = kBucketSize - 1;
        --first_element_bucket_;
      } else {
        --first_element_position_;
      }
    }
    allocator_traits::construct(
        alloc_, container_[first_element_bucket_] + first_element_position_,
        std::forward<Arguments>(args)...);
  } catch (...) {
    allocator_traits::destroy(
        alloc_, container_[first_element_bucket_] + first_element_position_);
    if (first_element_position_ == kBucketSize - 1) {
      ++first_element_bucket_;
      first_element_position_ = 0;
    } else {
      ++first_element_position_;
    }
  }
  ++size_;
}

template <typename T, typename Allocator>
template <bool IsConst>
class Deque<T, Allocator>::Iterator {
 public:
  using iterator_category = std::random_access_iterator_tag;
  using cond_type = std::conditional_t<IsConst, const T, T>;
  using value_type = cond_type;
  using pointer = cond_type*;
  using reference = cond_type&;
  using difference_type = std::ptrdiff_t;

  Iterator(T** ptr, size_t bucket_number, size_t position);
  Iterator(const Iterator& other) = default;
  Iterator& operator=(const Iterator& other) = default;

  Iterator& operator++();
  Iterator& operator--();
  Iterator operator++(int);
  Iterator operator--(int);
  Iterator& operator+=(int number);
  Iterator& operator-=(int number);
  Iterator operator+(int number) const;
  Iterator operator-(int number) const;

  bool operator<(const Iterator& other) const;
  bool operator==(const Iterator& other) const;
  bool operator>(const Iterator& other) const;
  bool operator!=(const Iterator& other) const;
  bool operator<=(const Iterator& other) const;
  bool operator>=(const Iterator& other) const;

  difference_type operator-(const Iterator& other);
  reference operator*() const;
  pointer operator->() const;

 private:
  T** ptr_ = nullptr;
  int bucket_number_ = 0;
  int position_ = 0;
};

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>::difference_type
Deque<T, Allocator>::Iterator<IsConst>::operator-(
    const Deque<T, Allocator>::Iterator<IsConst>& other) {
  return kBucketSize * bucket_number_ + position_ -
         (kBucketSize * other.bucket_number_ + other.position_);
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>::pointer
Deque<T, Allocator>::Iterator<IsConst>::operator->() const {
  return ptr_[bucket_number_] + position_;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::erase(Deque::iterator iter) {
  for (auto position = iter; position != end() - 1; ++position) {
    std::swap(*iter, *(iter + 1));
  }
  pop_back();
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::insert(Deque::iterator iter, const T& value) {
  if (empty()) {
    push_back(value);
  } else {
    auto iter_2 = end() - 1;
    T last_element = *iter_2;
    *iter_2 = value;
    while (iter_2 > iter) {
      std::swap(*(iter_2 - 1), *iter_2);
      --iter_2;
    }
    push_back(last_element);
    std::swap(*(end() - 2), *(end() - 1));
  }
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::emplace(Deque::iterator iter, T&& value) {
  if (empty()) {
    push_back(std::move(value));
  } else {
    auto iter_2 = end() - 1;
    T last_element = *iter_2;
    *iter_2 = value;
    while (iter_2 > iter) {
      std::swap(*(iter_2 - 1), *iter_2);
      --iter_2;
    }
    push_back(last_element);
    std::swap(*(end() - 2), *(end() - 1));
  }
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::const_reverse_iterator
Deque<T, Allocator>::crend() const {
  return std::make_reverse_iterator(cbegin());
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::const_reverse_iterator
Deque<T, Allocator>::crbegin() const {
  return std::make_reverse_iterator(cend());
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::reverse_iterator Deque<T, Allocator>::rend() {
  return std::make_reverse_iterator(begin());
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::reverse_iterator Deque<T, Allocator>::rbegin() {
  return std::make_reverse_iterator(end());
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::const_iterator Deque<T, Allocator>::cend() const {
  if (empty()) {
    return const_iterator(container_, last_element_bucket_,
                          last_element_position_);
  }
  return const_iterator(container_, last_element_bucket_,
                        last_element_position_ + 1);
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::iterator Deque<T, Allocator>::end() {
  if (empty()) {
    return iterator(container_, last_element_bucket_, last_element_position_);
  }
  return iterator(container_, last_element_bucket_, last_element_position_ + 1);
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::const_iterator Deque<T, Allocator>::cbegin()
    const {
  return const_iterator(container_, first_element_bucket_,
                        first_element_position_);
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::iterator Deque<T, Allocator>::begin() {
  return iterator(container_, first_element_bucket_, first_element_position_);
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>::reference
Deque<T, Allocator>::Iterator<IsConst>::operator*() const {
  return ptr_[bucket_number_][position_];
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Deque<T, Allocator>::Iterator<IsConst>::operator==(
    const Deque<T, Allocator>::Iterator<IsConst>& other) const {
  return bucket_number_ * kBucketSize + position_ ==
         other.bucket_number_ * kBucketSize + other.position_;
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Deque<T, Allocator>::Iterator<IsConst>::operator<(
    const Deque<T, Allocator>::Iterator<IsConst>& other) const {
  return bucket_number_ * kBucketSize + position_ <
         other.bucket_number_ * kBucketSize + other.position_;
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Deque<T, Allocator>::Iterator<IsConst>::operator>(
    const Deque<T, Allocator>::Iterator<IsConst>& other) const {
  return other < *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Deque<T, Allocator>::Iterator<IsConst>::operator>=(
    const Deque<T, Allocator>::Iterator<IsConst>& other) const {
  return !(*this < other);
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Deque<T, Allocator>::Iterator<IsConst>::operator<=(
    const Deque<T, Allocator>::Iterator<IsConst>& other) const {
  return !(*this > other);
}

template <typename T, typename Allocator>
template <bool IsConst>
bool Deque<T, Allocator>::Iterator<IsConst>::operator!=(
    const Deque<T, Allocator>::Iterator<IsConst>& other) const {
  return !(*this == other);
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>&
Deque<T, Allocator>::Iterator<IsConst>::operator+=(int number) {
  if (position_ + number < kBucketSize) {
    position_ += number;
  } else {
    number -= kBucketSize - position_;
    bucket_number_ += 1 + number / kBucketSize;
    position_ = number % kBucketSize;
  }
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>&
Deque<T, Allocator>::Iterator<IsConst>::operator-=(int number) {
  if (number <= position_) {
    position_ -= number;
  } else {
    number -= position_ + 1;
    bucket_number_ -= 1 + number / kBucketSize;
    position_ = kBucketSize - 1 - number % kBucketSize;
  }
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator-(int number) const {
  auto tmp = *this;
  tmp -= number;
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator+(int number) const {
  auto tmp = *this;
  tmp += number;
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator--(int) {
  Iterator<IsConst> tmp = *this;
  --(*this);
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator++(int) {
  Iterator<IsConst> tmp = *this;
  ++(*this);
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>&
Deque<T, Allocator>::Iterator<IsConst>::operator--() {
  if (position_ != 0) {
    --position_;
  } else {
    position_ = kBucketSize - 1;
    --bucket_number_;
  }
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>&
Deque<T, Allocator>::Iterator<IsConst>::operator++() {
  if (position_ != kBucketSize - 1) {
    ++position_;
  } else {
    position_ = 0;
    ++bucket_number_;
  }
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
Deque<T, Allocator>::Iterator<IsConst>::Iterator(T** ptr, size_t bucket_number,
                                                 size_t position)
    : ptr_(ptr),
      bucket_number_(static_cast<int>(bucket_number)),
      position_(static_cast<int>(position)) {}