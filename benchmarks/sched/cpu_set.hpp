#ifndef CPU_SET_HPP
#define CPU_SET_HPP

#include <sched.h>
#include <stdexcept>
#include <cstring>

// C++ wrapper over C's cpu_set_t
class cpu_set {
public:
    static constexpr int max_cpus = 64;

    cpu_set() : s(CPU_ALLOC(max_cpus)) { zero(); }
    cpu_set(const cpu_set& o) : s(CPU_ALLOC(max_cpus)) { memcpy(s, o.s, size()); }
    cpu_set(cpu_set&& o) : s(o.s) { o.s = nullptr; }
    cpu_set(long int mask) : cpu_set() {
        for (size_t i = 0; i < sizeof(mask) * 8; i++)
            if (mask & (1UL<<i))
                set(i); }
    ~cpu_set() { CPU_FREE(s); }

    void zero() { CPU_ZERO_S(size(), s); }
    bool is_set(unsigned i) const { return !!CPU_ISSET_S(i, size(), s); }
    void set(unsigned i) { CPU_SET_S(i, size(), s); }
    void clr(unsigned i) { CPU_CLR_S(i, size(), s); }
    cpu_set& operator &=(const cpu_set& o) { CPU_AND_S(size(), s, s, o.s); return *this; }
    cpu_set& operator ^=(const cpu_set& o) { CPU_XOR_S(size(), s, s, o.s); return *this; }
    cpu_set& operator |=(const cpu_set& o) { CPU_OR_S(size(), s, s, o.s); return *this; }
    bool operator ==(cpu_set& o) { return CPU_EQUAL_S(size(), s, o.s); }
    bool operator !=(cpu_set& o) { return !(o == *this); }
    unsigned count() const { return CPU_COUNT_S(size(), s); }
    size_t size() const { return CPU_ALLOC_SIZE(max_cpus); }
    cpu_set_t* ptr() const { return s; }
    int as_int() const { int mask = 0; for (int i = 0; i < max_cpus; i++) if (is_set(i)) mask |= 1<<i; return mask; }
    explicit operator bool() const { return count() > 0; }

    cpu_set &operator =(const cpu_set& o) { memcpy(s, o.s, size()); return *this; }
    cpu_set operator &(const cpu_set& o) const { return cpu_set(*this) &= o; }
    cpu_set operator |(const cpu_set& o) const { return cpu_set(*this) |= o; }
    cpu_set operator ^(const cpu_set& o) const { return cpu_set(*this) ^= o; }

    unsigned highest() const {
        for (int i = max_cpus - 1; i >= 0; i--)
            if (is_set(i))
                return i;
        throw std::runtime_error("empty cpu_set");
    }
    unsigned lowest() const {
        for (int i = 0; i < max_cpus; i++)
            if (is_set(i))
                return i;
        throw std::runtime_error("empty cpu_set");
    }
    unsigned next_set(int j) {
        int i = j + 1;
        do {
            if (is_set(i))
                return i;
            i = (i + 1) % max_cpus;
        } while (i != j + 1);
        throw std::runtime_error("empty cpu_set");
    }
private:
    cpu_set_t *s;
};

#endif
