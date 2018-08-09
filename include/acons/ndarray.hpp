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
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <iterator>
  
namespace acons {

template <size_t n, typename Base, size_t m>
typename std::enable_if<m == n, size_t>::type
get_offset(const std::array<size_t,n>& strides) 
{
    return 0;
}

template <size_t n, typename Base, size_t m>
typename std::enable_if<m+1 == n, size_t>::type
get_offset(const std::array<size_t,n>& strides, size_t index) 
{
    return Base::rebase_to_zero(index)*strides[n-1];
}

template <size_t n, typename Base, size_t m, typename... Indices>
typename std::enable_if<(m+1 < n), size_t>::type
get_offset(const std::array<size_t,n>& strides, size_t index, Indices... indices)
{
    const size_t mplus1 = m + 1;
    size_t i = Base::rebase_to_zero(index)*strides[m] + get_offset<n, Base, mplus1>(strides,indices...);

    return i;
}

template <size_t N, typename Base>
size_t get_offset(const std::array<size_t,N>& strides, const std::array<size_t,N>& indices)
{
    size_t offset = 0;
    for (size_t i = 0; i < N; ++i)
    {
        offset += Base::rebase_to_zero(indices[i])*strides[i];
    }

    return offset;
}

struct zero_based
{
    static size_t rebase_to_zero(size_t index)
    {
        return index;
    }
};

struct one_based
{
    static size_t rebase_to_zero(size_t index)
    {
        return index - 1;
    }
};

struct row_major
{
    template <size_t N>
    static void calculate_strides(const std::array<size_t,N>& dim, std::array<size_t,N>& strides, size_t& size)
    {
        size = 1;
        for (size_t i = 0; i < N; ++i)
        {
            strides[N-i-1] = size;
            size *= dim[N-i-1];
        }
    }

    template <typename T, size_t N>
    static bool compare(const T* data1, const std::array<size_t,N>& dim1, const std::array<size_t,N>& strides1, 
                        const T* data2, const std::array<size_t,N>& dim2, const std::array<size_t,N>& strides2)
    {
        for (size_t i = 0; i < N; ++i)
        {
            if (dim1[i] != dim2[i])
            {
                return false;
            }
        }
        std::array<size_t,N> indices;
        bool result = compare(dim1, data1, strides1, data2, strides2, indices, 0); 
        return result;
    }

    template <typename T, size_t N>
    static bool compare(const std::array<size_t,N>& dim,
                        const T* data1, const std::array<size_t,N>& strides1, 
                        const T* data2, const std::array<size_t,N>& strides2, 
                        std::array<size_t,N>& indices, size_t index)
    {
        if (index + 1 == N)
        {
            indices[index] = 0;
            size_t offset1 = get_offset<N,zero_based>(strides1,indices);
            size_t offset2 = get_offset<N,zero_based>(strides2,indices);
            const T* p1 = data1 + offset1;
            const T* p2 = data2 + offset2;
            const T* end = p1 + dim[index];
            while (p1 != end)
            {
                if (*p1++ != *p2++)
                {
                    return false;
                }
            }
        }
        else
        {
            for (size_t i = 0; i < dim[index]; ++i)
            {
                indices[index] = i;
                if (!compare(dim,data1,strides1,data2,strides2,indices,index+1))
                {
                    return false;
                }
            }
        }
        return true;
    }
};

struct column_major
{
    template <size_t N>
    static void calculate_strides(const std::array<size_t,N>& dim, std::array<size_t,N>& strides, size_t& size)
    {
        size = 1;
        for (size_t i = 0; i < N; ++i)
        {
            strides[i] = size;
            size *= dim[i];
        }
    }

    template <typename T, size_t N>
    static bool compare(const T* data1, const std::array<size_t, N>& dim1, const std::array<size_t, N>& strides1,
                        const T* data2, const std::array<size_t, N>& dim2, const std::array<size_t, N>& strides2)
    {
        for (size_t i = 0; i < N; ++i)
        {
            if (dim1[i] != dim2[i])
            {
                return false;
            }
        }
        std::array<size_t,N> indices;
        bool result = compare(dim1, data1, strides1, data2, strides2, indices, N-1); 
        return result;
    }

    template <typename T, size_t N>
    static bool compare(const std::array<size_t,N>& dim,
                        const T* data1, const std::array<size_t,N>& strides1, 
                        const T* data2, const std::array<size_t,N>& strides2, 
                        std::array<size_t,N>& indices, size_t index)
    {
        if (index == 0)
        {
            for (size_t i = 0; i < dim[index]; ++i)
            {
                indices[index] = 0;
                size_t offset1 = get_offset<N,zero_based>(strides1,indices);
                size_t offset2 = get_offset<N,zero_based>(strides2,indices);
                const T* p1 = data1 + offset1;
                const T* p2 = data2 + offset2;
                const T* end = p1 + dim[index];
                while (p1 != end)
                {
                    if (*p1++ != *p2++)
                    {
                        return false;
                    }
                }
            }
        }
        else
        {
            for (size_t i = 0; i < dim[index]; ++i)
            {
                indices[index] = i;
                if (!compare(dim,data1,strides1,data2,strides2,indices,index-1))
                {
                    return false;
                }
            }
        }
        return true;
    }
};

template <typename T, size_t N, typename Order=row_major, typename Base=zero_based, typename Allocator=std::allocator<T>>
class ndarray;

template <typename T, size_t M, typename Order=row_major, typename Base=zero_based>
class ndarray_view;  

// ndarray_ref

template <typename T, size_t N, typename Order, typename Base>
class ndarray_ref
{
public:
    typedef T* iterator;
    typedef const T* const_iterator;

protected:
    T* data_;
    size_t size_;
    std::array<size_t,N> dim_;
    std::array<size_t,N> strides_;

    ndarray_ref()
        : data_(nullptr), size_(0)
    {
        for (size_t i = 0; i < N; ++i)
        {
            dim_[i] = strides_[i] = 0;
        }
    }

    ndarray_ref(T* data, size_t size, const std::array<size_t,N>& dim, const std::array<size_t,N>& strides)
        : data_(data), size_(size), dim_(dim), strides_(strides)
    {
    }

    ndarray_ref(T* data, const std::array<size_t,N>& dim)
        : data_(data), size_(0), dim_(dim)
    {
        Order::calculate_strides(dim_, strides_, size_);
    }

public:
    size_t size() const noexcept
    {
        return size_;
    }

    bool empty() const noexcept
    {
        return size_ == 0;
    }

    const std::array<size_t,N>& dimensions() const {return dim_;}
    const std::array<size_t,N>& strides() const {return strides_;}

    T* data()
    {
        return data_;
    }

    const T* data() const 
    {
        return data_;
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
        size_t off = get_offset<n, Base, 0>(strides_,indices...);
        assert(off < size());
        return data_[off];
    }

    template <size_t n=N, typename... Indices>
    typename std::enable_if<(n == N),const T&>::type 
    operator()(Indices... indices) const
    {
        size_t off = get_offset<n, Base, 0>(strides_,indices...);
        assert(off < size());
        return data_[off];
    }

    template <size_t n=N-1, typename... Indices>
    typename std::enable_if<(n == N-1),iterator>::type 
    begin(Indices... indices) 
    {
        size_t off = get_offset<n, Base, 0>(strides_,indices...);
        assert(off < size());
        return &data_[off];
    }

    template <size_t n=N-1, typename... Indices>
    typename std::enable_if<(n == N-1),iterator>::type 
    end(Indices... indices) 
    {
        size_t off = get_offset<n, Base, 0>(strides_,indices...);
        assert(off < size());
        return &data_[off] + dim_[n];
    }

    template <size_t n=N-1, typename... Indices>
    typename std::enable_if<(n == N-1),const_iterator>::type 
    begin(Indices... indices) const
    {
        size_t off = get_offset<n, Base, 0>(strides_,indices...);
        assert(off < size());
        return &data_[off];
    }

    template <size_t n=N-1, typename... Indices>
    typename std::enable_if<(n == N-1),const_iterator>::type 
    end(Indices... indices) const
    {
        size_t off = get_offset<n, Base, 0>(strides_,indices...);
        assert(off < size());
        return &data_[off] + dim_[n];
    }
};


// ndarray

template <class T>
struct array_item
{
    typedef typename std::vector<array_item<T>>::iterator iterator;
    typedef typename std::vector<array_item<T>>::const_iterator const_iterator;

    bool is_array_;
    std::vector<array_item<T>> v_;
    T val_;

    array_item(const std::vector<array_item<T>>& a)
        : is_array_(true), v_(a), val_(0)
    {
    }

    array_item(std::initializer_list<array_item<T>> list)
        : is_array_(true), v_(list), val_(0)
    {
    }
    array_item(T val)
        : is_array_(false), val_(val)
    {
    }

    array_item() 
        : is_array_(false)
    {
    }
    array_item(const array_item&)  = default;
    array_item(array_item&&)  = default;

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

// ndarray_base

template <class Allocator>
class ndarray_base
{
protected:
    Allocator allocator_;
public:
    typedef Allocator allocator_type;

    allocator_type get_allocator() const
    {
        return allocator_;
    }
protected:
    ndarray_base(const Allocator& alloc)
        : allocator_(alloc)
    {
    }
};

template<size_t Pos>
struct init_helper
{
    using next = init_helper<Pos - 1>;

    template <typename Array, typename... Args>
    static void init(std::array<size_t,Array::dimension>& dim, Array& a, size_t n, Args... args)
    {
        dim[Array::dimension - Pos] = n;
        next::init(dim, a, args...);
    }
};

template<>
struct init_helper<0>
{
    template <typename Array>
    static void init(std::array<size_t, Array::dimension>& dim, Array& a)
    {
        a.init();
    }
    template <typename Array>
    static void init(std::array<size_t, Array::dimension>& dim, Array& a, typename Array::const_reference val)
    {
        a.init(val);
    }
};

template <typename T, size_t N, typename Order, typename Base, typename Allocator>
class ndarray : public ndarray_base<Allocator>, public ndarray_ref<T, N, Order, Base>
{
    friend struct init_helper<0>;

public:
    using typename ndarray_base<Allocator>::allocator_type;
    using ndarray_base<Allocator>::get_allocator;
    using ndarray_ref<T, N, Order, Base>::size;

    typedef T value_type;
    typedef const T& const_reference;
    static const size_t dimension = N;

    typedef T* iterator;
    typedef const T* const_iterator;

    ndarray()
        : ndarray_base<Allocator>(allocator_type()),
          ndarray_ref<T, N, Order, Base>()
    {
    }

    template <typename... Args>
    ndarray(size_t i, Args... args)
        : ndarray_base<Allocator>(allocator_type()) 
    {
        init_helper<N>::init(this->dim_, *this, i, args ...);
    }

    explicit ndarray(const std::array<size_t,N>& dim)
        : ndarray_base<Allocator>(allocator_type()), 
          ndarray_ref<T, N, Order, Base>(nullptr, dim)
    {
        this->data_ = get_allocator().allocate(this->size_);
    }

    ndarray(const std::array<size_t,N>& dim, const Allocator& alloc)
        : ndarray_base<Allocator>(alloc), 
          ndarray_ref<T, N, Order, Base>(nullptr, dim)
    {
        this->data_ = get_allocator().allocate(this->size_);
    }

    ndarray(const std::array<size_t,N>& dim, T val)
        : ndarray_base<Allocator>(allocator_type()), 
          ndarray_ref<T, N, Order, Base>(nullptr, dim)
    {
        this->data_ = get_allocator().allocate(this->size_);
        for (size_t i = 0; i < this->size_; ++i)
        {
            this->data_[i] = val;
        }
    }

    ndarray(const std::array<size_t,N>& dim, T val, const Allocator& alloc)
        : ndarray_base<Allocator>(alloc), 
          ndarray_ref<T, N, Order, Base>(nullptr, dim)
    {
        this->data_ = get_allocator().allocate(this->size_);
        for (size_t i = 0; i < this->size_; ++i)
        {
            this->data_[i] = val;
        }
    }

    ndarray(std::initializer_list<array_item<T>> list) 
        : ndarray_base<Allocator>(allocator_type())
    {
        dim_from_initializer_list(list, 0);

        Order::calculate_strides(this->dim_, this->strides_, this->size_);
        this->data_ = get_allocator().allocate(this->size_);
        std::array<size_t,N> indices;
        data_from_initializer_list(list,indices,0);
    }

    ndarray(std::initializer_list<array_item<T>> list, const Allocator& alloc) 
        : ndarray_base<Allocator>(alloc)
    {
        dim_from_initializer_list(list, 0);

        // Initialize multipliers and size
        Order::calculate_strides(this->dim_, this->strides_, this->size_);
        this->data_ = get_allocator().allocate(this->size_);
        std::array<size_t,N> indices;
        data_from_initializer_list(list,indices,0);
    }

    ndarray(const ndarray& a)
        : ndarray_base<Allocator>(a.get_allocator()), 
          ndarray_ref<T, N, Order, Base>(nullptr, 0, a.dim_, a.strides_)
    {
        this->size_ = a.size();
        this->data_ = get_allocator().allocate(a.size());

        for (size_t i = 0; i < this->size_; ++i)
        {
            this->data_[i] = a.data_[i];
        }
    }

    ndarray(const ndarray& a, const Allocator& alloc)
        : ndarray_base<Allocator>(alloc), 
          ndarray_ref<T, N, Order, Base>(nullptr, 0, a.dim_, a.strides_)
    {
        this->size_ = a.size();
        this->data_ = get_allocator().allocate(a.size());

        for (size_t i = 0; i < this->size_; ++i)
        {
            this->data_[i] = a.data_[i];
        }
    }

    ndarray(const ndarray_view<T,N,Order,Base>& a)
        : ndarray_base<Allocator>(allocator_type()), 
          ndarray_ref<T, N, Order, Base>(nullptr, 0, a.dim_, a.strides_)
    {
        this->size_ = a.size();
        this->data_ = get_allocator().allocate(a.size());

        for (size_t i = 0; i < this->size_; ++i)
        {
            this->data_[i] = a.data_[i];
        }
    }

    ndarray(const ndarray_view<T,N,Order,Base>& a, const Allocator& alloc)
        : ndarray_base<Allocator>(alloc), 
          ndarray_ref<T, N, Order, Base>(nullptr, 0, a.dim_, a.strides_)
    {
        this->size_ = a.size();
        this->data_ = get_allocator().allocate(a.size());

        for (size_t i = 0; i < this->size_; ++i)
        {
            this->data_[i] = a.data_[i];
        }
    }

    ndarray(ndarray&& a)
        : ndarray_base<Allocator>(a.get_allocator()), 
          ndarray_ref<T, N, Order, Base>(a.data_, a.size_, a.dim_, a.strides_)
    {
        a.data_ = nullptr;
        a.size_ = 0;
        for (size_t i = 0; i < N; ++i)
        {
            a.dim_[i] = 0;
            a.strides_[i] = 0;
        }
    }

    ndarray(ndarray&& a, const Allocator& alloc)
        : ndarray_base<Allocator>(alloc), 
          ndarray_ref<T, N, Order, Base>(a.data_, a.size_, a.dim_, a.strides_)
    {
        a.data_ = nullptr;
        a.size_ = 0;
        for (size_t i = 0; i < N; ++i)
        {
            a.dim_[i] = 0;
            a.strides_[i] = 0;
        }
    }

    ~ndarray()
    {
        get_allocator().deallocate(this->data_,this->size_);
    }

    ndarray& operator=(const ndarray<T,N,Order,Base,Allocator>& other)
    {
        if (&other != this)
        {
            assign_copy(other,
                   typename std::allocator_traits<Allocator>::propagate_on_container_copy_assignment());
        }
        return *this;
    }

    ndarray& operator=(ndarray<T,N,Order,Base,Allocator>&& other) noexcept
    {
        if (&other != this)
        {
            assign_move(std::forward<ndarray>(other),
                   typename std::allocator_traits<Allocator>::propagate_on_container_move_assignment());
        }
        return *this;
    }

    void assign_move(ndarray<T,N,Order,Base,Allocator>&& other, std::true_type) noexcept
    {
        std::swap(allocator_,other.get_allocator());
        std::swap(data_,other.data_);
        std::swap(size_,other.size_);
        std::swap(dim_,other.dim_);
        std::swap(strides_,other.strides_);
    }

    void assign_move(ndarray<T,N,Order,Base,Allocator>&& other, std::false_type) noexcept
    {
        if (this->size() != other.size())
        {
            get_allocator().deallocate(this->data_,this->size_);
            this->size_ = other.size();
            this->data_ = get_allocator().allocate(other.size());
        }
        this->dim_ = other.dimensions();
        this->strides_ = other.strides();
        for (size_t i = 0; i < this->size_; ++i)
        {
            this->data_[i] = other.data_[i];
        }
    }

    void assign_copy(const ndarray<T,N,Order,Base,Allocator>& other, std::true_type)
    {
        allocator_ = other.get_allocator();
        get_allocator().deallocate(this->data_,this->size_);
        this->size_ = other.size();
        this->data_ = get_allocator().allocate(other.size());
        this->dim_ = other.dimensions();
        this->strides_ = other.strides();
        for (size_t i = 0; i < this->size_; ++i)
        {
            this->data_[i] = other.data_[i];
        }
    }

    void assign_copy(const ndarray<T,N,Order,Base,Allocator>& other, std::false_type)
    {
        if (this->size() != other.size())
        {
            get_allocator().deallocate(this->data_,this->size_);
            this->size_ = other.size();
            this->data_ = get_allocator().allocate(other.size());
        }
        this->dim_ = other.dimensions();
        this->strides_ = other.strides();
        for (size_t i = 0; i < this->size_; ++i)
        {
            this->data_[i] = other.data_[i];
        }
    }

    void swap(ndarray<T,N,Order,Base,Allocator>& other) noexcept
    {
        swap(other,std::allocator_traits<allocator_type>::propagate_on_container_swap::value );
    }

    void swap(ndarray<T,N,Order,Base,Allocator>& other, std::true_type) noexcept
    {
        std::swap(allocator_,other.allocator_);
        std::swap(data_,other.data_);
        std::swap(size_,other.size_);
        std::swap(dim_,other.dim_);
        std::swap(strides_,other.strides_);
    }

    void swap(ndarray<T,N,Order,Base,Allocator>& other, std::false_type) noexcept
    {
    }

private:
    void init()
    {
        Order::calculate_strides(this->dim_, this->strides_, this->size_);
        this->data_ = get_allocator().allocate(this->size_);
    }

    void init(const T& val)
    {
        Order::calculate_strides(this->dim_, this->strides_, this->size_);
        this->data_ = get_allocator().allocate(this->size_);
        for (size_t i = 0; i < this->size_; ++i)
        {
            this->data_[i] = val;
        }
    }

    void dim_from_initializer_list(const array_item<T>& init, size_t dim)
    {
        bool is_array = false;
        size_t size = 0;

        size_t i = 0;
        for (const auto& item : init)
        {
            if (i == 0)
            {
                is_array = item.is_array();
                size = item.size();
                if (dim < N)
                {
                    this->dim_[dim++] = init.size();
                    if (is_array)
                    {
                        dim_from_initializer_list(item, dim);
                    }
                }
            }
            else
            {
                if (is_array)
                {
                    if (!item.is_array() || item.size() != size)
                    {
                        throw std::invalid_argument("initializer list contains non-conforming shapes");
                    }
                }
                else if (dim != N)
                {
                    throw std::invalid_argument("initializer list incompatible with array dimensionality");
                }
            }
            ++i;
        }
    }

    void data_from_initializer_list(const array_item<T>& init, std::array<size_t,N>& indices, size_t index)
    {
        size_t i = 0;
        for (const auto& item : init)
        {
            indices[index] = i;
            if (item.is_array())
            {
                data_from_initializer_list(item,indices,index+1);
            }
            else 
            {
                size_t offset = get_offset<N,zero_based>(this->strides_,indices);
                if (offset < size())
                {
                    this->data_[offset] = item.value();
                }
            }
            ++i;
        }
    }
};

template <typename T, size_t N, typename Order, typename Base, typename CharT>
void output(std::basic_ostream<CharT>& os, const T* data, const std::array<size_t,N>& strides, const std::array<size_t,N>& dimensions,
            std::array<size_t,N>& indices, size_t index)
{
    if (index+1 < strides.size())
    {
        os << '[';
        for (size_t i = 0; i < dimensions[index]; ++i)
        {
            if (i > 0)
            {
                os << ',';
            }
            indices[index] = i; 
            {
                output<T,N,Order,Base,CharT>(os,data,strides,dimensions,indices, index + 1);
            }
        }
        os << ']';
    }
    else
    {
        os << '[';
        for (size_t i = 0; i < dimensions[index]; ++i)
        {
            indices[index] = i; 
            if (i > 0)
            {
                os << ',';
            }
            size_t offset = get_offset<N,Base>(strides,indices);

            os << data[get_offset<N,Base>(strides,indices)];
        }
        os << ']';
    }
}

template <typename T, size_t N, typename Order, typename Base, typename Allocator, typename CharT>
std::basic_ostream<CharT>& operator <<(std::basic_ostream<CharT>& os, ndarray<T, N, Order, Base, Allocator>& a)
{
    std::array<size_t,N> indices;
    output<T,N,Order,Base,CharT>(os, a.data(), a.strides(), a.dimensions(), indices, 0);
    return os;
}


template <typename T, size_t N, typename Order, typename Enable = void>
class ndarray_view_iterator;

template <typename T, size_t N, typename Order>
class ndarray_view_iterator<T, N, Order, 
                            typename std::enable_if<N==1 && std::is_same<Order,row_major>::value>::type>
{
T* p_;
public:
    typedef ptrdiff_t difference_type;
    typedef T& reference;
    typedef T* pointer;
    typedef std::forward_iterator_tag iterator_category;

    ndarray_view_iterator()
        : p_(nullptr)
    {
    }

    ndarray_view_iterator(T* data)
        : p_(data)
    {
    }

    ndarray_view_iterator(const ndarray_view_iterator<T,N,Order>& val) 
        : p_(val.p_)
    {
    } 

    ndarray_view_iterator<T, N, Order>& operator++()
    {
        ++p_;
        return *this;
    }

    ndarray_view_iterator<T, N, Order>& operator++(int)
    {
        ndarray_view_iterator<T,N,Order> temp(*this);
        ++p_;
        return temp;
    }

    reference operator*() const
    {
        return *p_;
    }

    friend bool operator==(const ndarray_view_iterator<T,N,Order>& it1, const ndarray_view_iterator& it2)
    {
        return it1.p_ == it2.p_;
    }

    friend bool operator!=(const ndarray_view_iterator<T,N,Order>& it1, const ndarray_view_iterator& it2)
    {
        return !(it1 == it2);
    }
};

template <typename T, size_t N, typename Order>
class ndarray_view_iterator<T, N, Order, 
                            typename std::enable_if<N != 1 && std::is_same<Order,row_major>::value>::type>
{
T* data_;
public:
    ndarray_view_iterator(T* data)
        : data_(data)
    {
    }
};

template <typename T, size_t N, typename Order>
class ndarray_view_iterator<T, N, Order, 
    typename std::enable_if<N == 1 && std::is_same<Order,column_major>::value>::type>
{
public:
};

template <typename T, size_t N, typename Order>
class ndarray_view_iterator<T, N, Order, 
    typename std::enable_if<N != 1 && std::is_same<Order,column_major>::value>::type>
{
public:
};

template <typename T, size_t M, typename Order, typename Base>
class ndarray_view : public ndarray_ref<T, M, Order, Base> 
{
public:
    typedef T* iterator;
    typedef const T* const_iterator;

    template <typename Allocator>
    ndarray_view(ndarray<T, M, Order, Base, Allocator>& a)
        : ndarray_ref<T, M, Order, Base>(a.data(), a.size(), a.dimensions(), a.strides())
    {
    }

    template<size_t m = M, size_t N, typename Allocator>
    ndarray_view(ndarray<T, N, Order, Base, Allocator>& a, const std::array<size_t,N>& indices, const std::array<size_t,M>& dim, 
               typename std::enable_if<m <= N>::type* = 0)
        : ndarray_ref<T, M, Order, Base>()
    {
        this->dim_ = dim;
        size_t offset = get_offset<N, Base>(a.strides(),indices);

        this->data_ = a.data() + offset;
        this->size_ = a.size() - offset;

        size_t diff = N - M;
        for (size_t i = 0; i < M; ++i)
        {
            this->strides_[i] = a.strides()[i];
        }
    }

    ndarray_view(T* data, const std::array<size_t,M>& dim) 
        : ndarray_ref<T, M, Order, Base>(data, dim)
    {
    }
};

template <typename T, size_t N, typename Order, typename Base, typename Allocator>
bool operator==(const ndarray<T, N, Order, Base, Allocator>& lhs, const ndarray<T, N, Order, Base, Allocator>& rhs)
{
    if (&lhs == &rhs)
    {
        return true;
    }
    for (size_t i = 0; i < N; ++i)
    {
        if (lhs.size(i) != rhs.size(i))
        {
            return false;
        }
    }
    for (size_t i = 0; i < lhs.size(); ++i)
    {
        if (lhs.data()[i] != rhs.data()[i])
        {
            return false;
        }
    }
    return true;
}

template <typename T, size_t N, typename Order, typename Base, typename Allocator>
bool operator!=(const ndarray<T, N, Order, Base, Allocator>& lhs, const ndarray<T, N, Order, Base, Allocator>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, size_t M, typename Order, typename Base>
bool operator==(const ndarray_view<T, M, Order, Base>& lhs, 
                const ndarray_view<T, M, Order, Base>& rhs)
{
    if (&lhs == &rhs)
    {
        return true;
    }

    return Order::compare(lhs.data(), lhs.dimensions(), lhs.strides(),
                          rhs.data(), rhs.dimensions(), lhs.strides());
}

template <typename T, size_t M, typename Order, typename Base, typename Allocator>
bool operator==(const ndarray<T, M, Order, Base, Allocator>& lhs, 
                const ndarray_view<T, M, Order, Base>& rhs)
{
    return Order::compare(lhs.data(), lhs.dimensions(), lhs.strides(),
                          rhs.data(), rhs.dimensions(), lhs.strides());
}

template <typename T, size_t M, typename Order, typename Base, typename Allocator>
bool operator==(const ndarray_view<T, M, Order, Base>& lhs, 
                const ndarray<T, M, Order, Base, Allocator>& rhs)
{
    return Order::compare(lhs.data(), lhs.dimensions(), lhs.strides(),
                          rhs.data(), rhs.dimensions(), lhs.strides());
}

template <typename T, size_t M, typename Order, typename Base>
bool operator!=(const ndarray_view<T, M, Order, Base>& lhs, const ndarray_view<T, M, Order, Base>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, size_t M, typename Order, typename Base, typename Allocator>
bool operator!=(const ndarray<T, M, Order, Base, Allocator>& lhs, 
                const ndarray_view<T, M, Order, Base>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, size_t M, typename Order, typename Base, typename Allocator>
bool operator!=(const ndarray_view<T, M, Order, Base>& lhs, 
                const ndarray<T, M, Order, Base, Allocator>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, size_t N, typename Order, typename Base, typename CharT>
std::basic_ostream<CharT>& operator <<(std::basic_ostream<CharT>& os, ndarray_view<T, N, Order, Base>& a)
{
    std::array<size_t,N> indices;
    output<T,N,Order,Base,CharT>(os, a.data(), a.strides(), a.dimensions(), indices, 0);
    return os;
}

}

#endif
