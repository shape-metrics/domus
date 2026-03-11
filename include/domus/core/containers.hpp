#pragma once

#include <functional>
#include <memory>

class I_IntPairHashSet;

class PairIntHashSet {
  private:
    std::unique_ptr<I_IntPairHashSet> m_impl;

  public:
    PairIntHashSet();
    ~PairIntHashSet();
    PairIntHashSet(PairIntHashSet&&) noexcept;
    PairIntHashSet& operator=(PairIntHashSet&&) noexcept;

    void add(size_t value_1, size_t value_2);
    bool has(size_t value_1, size_t value_2) const;
    size_t size() const;
    bool empty() const;
    void erase(size_t value_1, size_t value_2);
    void for_each(std::function<void(size_t, size_t)> func) const;
    void clear();
};

class I_IntPair_ToInt_HashMap;

class IntPair_ToInt_HashMap {
  private:
    std::unique_ptr<I_IntPair_ToInt_HashMap> m_impl;

  public:
    IntPair_ToInt_HashMap();
    ~IntPair_ToInt_HashMap();
    IntPair_ToInt_HashMap(IntPair_ToInt_HashMap&&) noexcept;
    IntPair_ToInt_HashMap& operator=(IntPair_ToInt_HashMap&&) noexcept;

    void add(size_t key_1, size_t key_2, size_t value);
    bool has(size_t key_1, size_t key_2) const;
    size_t get(size_t key_1, size_t key_2) const;
    size_t size() const;
    bool empty() const;
    void erase(size_t key_1, size_t key_2);
};

class I_IntHashSet;

class IntHashSet {
  private:
    std::unique_ptr<I_IntHashSet> m_impl;

  public:
    IntHashSet();
    ~IntHashSet();
    IntHashSet(IntHashSet&&) noexcept;
    IntHashSet& operator=(IntHashSet&&) noexcept;

    void add(size_t value);
    bool has(size_t value) const;
    size_t get_one_int() const;
    size_t size() const;
    bool empty() const;
    void erase(size_t value);
    void for_each(std::function<void(size_t)> func) const;
};

class I_Int_ToIntContainer_HashMap;

class Int_ToIntContainer_HashMap {
  private:
    std::unique_ptr<I_Int_ToIntContainer_HashMap> m_impl;

  public:
    Int_ToIntContainer_HashMap();
    ~Int_ToIntContainer_HashMap();
    Int_ToIntContainer_HashMap(Int_ToIntContainer_HashMap&&) noexcept;
    Int_ToIntContainer_HashMap& operator=(Int_ToIntContainer_HashMap&&) noexcept;
    void add(size_t key, size_t value);
    bool has(size_t key, size_t value) const;
    const IntHashSet& get(size_t key) const;
    IntHashSet& get(size_t key);
    void erase(size_t key, size_t value);
    void erase(size_t key);
    void for_each(std::function<void(size_t, const IntHashSet&)> func) const;
};

class I_Int_ToInt_HashMap;

class Int_ToInt_HashMap {
  private:
    std::unique_ptr<I_Int_ToInt_HashMap> m_impl;

  public:
    Int_ToInt_HashMap();
    ~Int_ToInt_HashMap();
    Int_ToInt_HashMap(Int_ToInt_HashMap&&) noexcept;
    Int_ToInt_HashMap& operator=(Int_ToInt_HashMap&&) noexcept;
    void add(size_t key, size_t value);
    void update(size_t key, size_t value);
    bool has(size_t key) const;
    size_t get(size_t key) const;
    void erase(size_t key);
    void for_each(std::function<void(size_t, size_t)> func) const;
    void clear();
    size_t& operator[](size_t key);
};