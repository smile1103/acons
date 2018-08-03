#ifndef ACONS_NDARRAY_HPP
#define ACONS_NDARRAY_HPP

#include <memory>
#include <array>
#include <vector>
#include <typeinfo>
#include <stdarg.h>  
#include <assert.h>
#include <algorithm>
#include <initializer_list>
  
namespace acons {

template <typename T, size_t N, typename Order>
class ndarray;

struct row_major
{
    template <size_t N>
    static void calculate_strides(const std::array<size_t,N>& dim, std::array<size_t,N>& strides, size_t& size)
    {
        size = 1;
        for (size_t i = N, j = 0; i >= 1; --i, ++j)
        {
            strides[j] = size;
            size *= dim[i - 1];
        }
    }
};

struct column_major
{
    template <size_t N>
    static void calculate_strides(const std::array<size_t,N>& dim, std::array<size_t,N>& strides, size_t& size)
    {
        size = 1;
        for (size_t i = 0, j = N; j >= 1; ++i, --j)
        {
            strides[j-1] = size;
            size *= dim[i];
        }
    }
};

template <typename T, size_t N, typename Order=row_major>
class array_ref
{
    T* data_;
protected:
    std::array<size_t,N> dim_;
    std::array<size_t,N> strides_;
    size_t size_;
public:
    typedef T* iterator;
    typedef const T* const_iterator;

    std::array<size_t,N> dimensions() const {return dim_;}
    std::array<size_t,N> strides() {return strides_;}

    array_ref& operator=(const array_ref&) = default;
    array_ref& operator=(array_ref&&) = default;

    virtual ~array_ref() = default;

    T* data()
    {
        return data_;
    }

    const T* data() const
    {
        return data_;
    }

    size_t size() const
    {
        return size_;
    }

    size_t size(size_t i) const
    {
        assert(i < dim_.size());
        return dim_[i];
    }

    template <size_t n=N, typename... Indices>
    typename std::enable_if<(n == N),T&>::type 
    operator()(Indices... indices) 
    {
        size_t off = get_offset<n>(indices...);
        assert(off < size_);
        return data_[off];
    }

    template <size_t n=N, typename... Indices>
    typename std::enable_if<(n == N),const T&>::type 
    operator()(Indices... indices) const
    {
        size_t off = get_offset<n>(indices...);
        assert(off < size_);
        return data_[off];
    }

    template <size_t n=N-1, typename... Indices>
    typename std::enable_if<(n == N-1),iterator>::type 
    begin(Indices... indices) 
    {
        size_t off = get_offset<n>(indices...);
        assert(off < size_);
        return &data_[off];
    }

    template <size_t n=N-1, typename... Indices>
    typename std::enable_if<(n == N-1),iterator>::type 
    end(Indices... indices) 
    {
        size_t off = get_offset<n>(indices...);
        assert(off < size_);
        return &data_[off] + dim_[n];
    }

    template <size_t n=N-1, typename... Indices>
    typename std::enable_if<(n == N-1),const_iterator>::type 
    begin(Indices... indices) const
    {
        size_t off = get_offset<n>(indices...);
        assert(off < size_);
        return &data_[off];
    }

    template <size_t n=N-1, typename... Indices>
    typename std::enable_if<(n == N-1),const_iterator>::type 
    end(Indices... indices) const
    {
        size_t off = get_offset<n>(indices...);
        assert(off < size_);
        return &data_[off] + dim_[n];
    }

    bool operator!=(const array_ref<T,N,Order>& rhs) const 
    {
        return !(*this == rhs);
    }
        
    array_ref()
        : data_(nullptr), size_(0)
    {
    }
    array_ref(const array_ref& a) = default;
    array_ref(array_ref&& a) = default;
protected:
    array_ref(const std::array<size_t,N>& dim)
        : data_(nullptr), dim_(dim), size_(0)
    {
    }
    array_ref(std::array<size_t,N>&& dim)
        : data_(nullptr), dim_(std::move(dim)), size_(0)
    {
    }
    void set_data(T* p)
    {
        data_ = p;
    }

    template <size_t n>
    typename std::enable_if<n == 0, size_t>::type
        get_offset() const
    {
        return 0;
    }

    template <size_t n>
    typename std::enable_if<n == 1, size_t>::type
        get_offset(size_t index) const
    {
        return index*strides_[0];
    }

    template <size_t n, typename... Indices>
    typename std::enable_if<(n > 1), size_t>::type
        get_offset(size_t index, Indices... indices) const
    {
        const size_t nminus1 = n - 1;

        size_t i = index*strides_[nminus1] + get_offset<nminus1>(indices...);

        return i;
    }
private:
};

// ndarray

template <class T>
struct array_variant
{
    typedef typename std::vector<array_variant<T>>::iterator iterator;
    typedef typename std::vector<array_variant<T>>::const_iterator const_iterator;

    bool is_array_;
    std::vector<array_variant<T>> v_;
    T val_;

    array_variant(const std::vector<array_variant<T>>& a)
        : is_array_(true), v_(a), val_(0)
    {
    }

    array_variant(std::initializer_list<array_variant<T>> list)
        : is_array_(true), v_(list), val_(0)
    {
    }
    array_variant(T val)
        : is_array_(false), val_(val)
    {
    }

    array_variant() 
        : is_array_(false)
    {
    }
    array_variant(const array_variant&)  = default;
    array_variant(array_variant&&)  = default;

    bool is_array() const
    {
        return is_array_;
    }

    size_t size() const
    {
        return v_.size();
    }

    T value() const
    {
        return val_;
    }

    iterator begin()
    {
        return v_.begin();
    }

    iterator end()
    {
        return v_.end();
    }

    const_iterator begin() const
    {
        return v_.begin();
    }

    const_iterator end() const
    {
        return v_.end();
    }
};

template <typename T, size_t N, typename Order=row_major>
class ndarray : public array_ref<T,N,Order>
{
    template<size_t Pos>
    struct init_helper
    {
        using next = init_helper<Pos - 1>;

        template <typename... Args>
        static void init(ndarray<T,N,Order>& a, size_t n, Args... args)
        {
            a.dim_[N-Pos] = n;
            next::init(a, args...);
        }
    };

    template<>
    struct init_helper<0>
    {
        static void init(ndarray<T,N,Order>& a)
        {
            a.init();
            a.data_.resize(a.size());
            a.set_data(a.data_.data());
        }
        static void init(ndarray<T,N,Order>& a, const T& val)
        {
            a.init();
            a.data_.resize(a.size(),val);
            a.set_data(a.data_.data());
        }
    };

    std::vector<T> data_;
public:
    using array_ref<T,N,Order>::size;
    using array_ref<T,N,Order>::set_data;
    using array_ref<T,N,Order>::get_offset;

    ndarray()
        : array_ref<T,N,Order>()
    {
    }
    ndarray(const ndarray& a)
        : array_ref<T,N,Order>(a), data_(a.data_) 
    {
        set_data(data_.data());
    }
    ndarray(ndarray&& a)
        : array_ref<T,N,Order>(a), data_(std::move(a.data_)) 
    {
        set_data(data_.data());
    }

    template <typename... Args>
    ndarray(size_t k, Args... args)
        : array_ref<T,N,Order>()
    {
        init_helper<N>::init(*this, k, args ...);
    }

    ndarray(const std::array<size_t,N>& a)
        : array_ref<T,N,Order>(a) 
    {
        init();
        data_.resize(size());
        set_data(data_.data());
    }

    ndarray(const std::array<size_t,N>& dim, T val)
        : array_ref<T,N,Order>(dim) 
    {
        init();
        data_.resize(size(),val);
        set_data(data_.data());
    }

    ndarray(std::array<size_t,N>&& dim)
        : array_ref<T,N,Order>(std::move(dim))
    {
        init();
        data_.resize(size());
        set_data(data_.data());
    }

    ndarray(std::array<size_t,N>&& dim, T val)
        : array_ref<T,N,Order>(std::move(dim))
    {
        init();
        data_.resize(size(),val);
        set_data(data_.data());
    }

    ndarray(std::initializer_list<array_variant<T>> list) 
        : array_ref<T,N,Order>() 
    {
        bool is_array = false;

        size_t i = 0;
        size_t dim = 0;
        for (const auto& item : list)
        {
            if (i == 0)
            {
                is_array = item.is_array();
                if (dim < N)
                {
                    this->dim_[dim++] = list.size();
                    if (is_array)
                    {
                        init_dim(item, dim);
                    }
                }
            }
            else
            {
                if (is_array != item.is_array())
                {
                    throw std::runtime_error("Invalid initializer list");
                }
            }
            ++i;
        }

        // Initialize multipliers and size
        init();

        data_.resize(size());
        {
            size_t i = 0;
            for (const auto& item : list)
            {
                if (item.is_array())
                {
                    init_data(item,i);
                }
                else 
                {
                    if (i < size())
                    {
                        data_[i] = item.value();
                        ++i;
                    }
                }
            }
        }
        set_data(data_.data());
    }

    ndarray& operator=(const ndarray&) = default;
    ndarray& operator=(ndarray&&) = default;
private:
    void init()
    {
        Order::calculate_strides(dim_, strides_, size_);
    }

    void init_dim(const array_variant<T>& init, size_t& dim)
    {
        bool is_array = false;

        size_t i = 0;
        for (const auto& item : init)
        {
            if (i == 0)
            {
                is_array = item.is_array();
                if (dim < N)
                {
                    this->dim_[dim++] = init.size();
                    if (is_array)
                    {
                        init_dim(item, dim);
                    }
                }
            }
            else
            {
                if (is_array != item.is_array())
                {
                    throw std::runtime_error("Invalid initializer list");
                }
            }
            ++i;
        }
    }

    void init_data(const array_variant<T>& init, size_t& i)
    {
        for (const auto& item : init)
        {
            if (item.is_array())
            {
                init_data(item,i);
            }
            else 
            {
                if (i < size())
                {
                    data_[i] = item.value();
                    ++i;
                }
            }
        }
    }
};

template <typename T, size_t M, typename Order=row_major>
class subarray  : public array_ref<T,M,Order>
{
public:
    template<size_t m = M, size_t N>
    subarray(array_ref<T,N,Order>& a, const std::array<size_t,N>& indices, const std::array<size_t,M>& dim, 
               typename std::enable_if<m <= N>::type* = 0)
        : array_ref<T,M,Order>(dim)
    {
        this->set_data(a.data() + get_offset2(a.strides(), indices));

        this->size_ = a.size();

        size_t size = 1;
        for (size_t i = M, j = 0; j < M; --i, ++j)
        {
            this->strides_[j] = a.strides()[j];
            size *= this->dim_[i - 1];
        }
    }

private:
    template <size_t N>
    size_t get_offset2(const std::array<size_t,N>& strides, const std::array<size_t,N>& indices) const
    {
        size_t offset = 0;
        for (size_t i = N, j = 0; i >= 1; --i, ++j)
        {
            offset += strides[j]*indices[i-1];
        }
        return offset;
    }

};

template <typename T, size_t M, typename Order=row_major>
class ndarray_view  : public array_ref<T,M,Order>
{
public:
    ndarray_view(T* data, const std::array<size_t,M>& dim) 
        : array_ref<T,M,Order>(dim)
    {
        this->set_data(data);

        this->size_ = 1;
        for (size_t i = M, j = 0; j < M; --i, ++j)
        {
            this->strides_[j] = this->size_;
            this->size_ *= this->dim_[i - 1];
        }
    }
};

}

#endif
