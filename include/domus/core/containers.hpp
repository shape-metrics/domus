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

    void add(int value_1, int value_2);
    bool has(int value_1, int value_2) const;
    size_t size() const;
    bool empty() const;
    void erase(int value_1, int value_2);
    void for_each(std::function<void(int, int)> func) const;
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

    void add(int key_1, int key_2, int value);
    bool has(int key_1, int key_2) const;
    int get(int key_1, int key_2) const;
    size_t size() const;
    bool empty() const;
    void erase(int key_1, int key_2);
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

    void add(int value);
    bool has(int value) const;
    int get_one_int() const;
    size_t size() const;
    bool empty() const;
    void erase(int value);
    void for_each(std::function<void(int)> func) const;
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
    void add(int key, int value);
    bool has(int key, int value) const;
    const IntHashSet& get(int key) const;
    IntHashSet& get(int key);
    void erase(int key, int value);
    void erase(int key);
    void for_each(std::function<void(int, const IntHashSet&)> func) const;
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
    void add(int key, int value);
    void update(int key, int value);
    bool has(int key) const;
    int get(int key) const;
    void erase(int key);
    void for_each(std::function<void(int, int)> func) const;
    void clear();
    int& operator[](int key);
};