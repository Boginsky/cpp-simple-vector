#pragma once

#include <initializer_list>
#include <string>
#include <stdexcept>
#include <utility>
 
#include "array_ptr.h"

using namespace std::literals;

struct ReserveProxyObj {
    size_t capacity = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return {capacity_to_reserve};
};

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) 
    : size_(size), capacity_(size), data_(size) {
        std::fill(data_.Get(), data_.Get() + size_, Type());
    }
    
    SimpleVector(ReserveProxyObj obj) {
        capacity_ = obj.capacity;
        Reserve(capacity_);
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) 
    : size_(size), capacity_(size), data_(size) {
        std::fill(data_.Get(), data_.Get() + size_, value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) 
    : size_(init.size()), capacity_(init.size()), data_(init.size()) {
        std::copy(init.begin(), init.end(), data_.Get());
    }
    
    SimpleVector(SimpleVector<Type>& other) {
        size_t capacity = std::distance(other.begin(), other.end());
        SimpleVector<Type> temp(capacity);
        std::copy(other.begin(), other.end(), temp.begin());
        swap(temp);
    }
    
    SimpleVector(SimpleVector&& other)
        : size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)),
          data_(std::move(other.data_)) {
    }
   
    SimpleVector& operator=(SimpleVector&& other) {
        if (this == &other) {
            return *this;
        }
        
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
        data_ = std::move(other.data_);
        
        return *this;
    }
    
    SimpleVector& operator=(SimpleVector<Type>& other) {
        if (this == &other) {
            return *this;
        }
        
        SimpleVector<Type> temp(other);
        swap(temp);
        return *this;
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0u;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Out of range"s);
        }
        
        return data_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Out of range"s);
        }
        
        return data_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size > capacity_) {
            auto temp = ArrayPtr<Type>(new_size);
            
            std::generate(temp.Get(), temp.Get() + new_size, []() { return Type(); });
            std::move(data_.Get(), data_.Get() + size_, temp.Get());
            
            data_.swap(temp);
            capacity_ = new_size;
        }
        
        if (new_size > size_) {
            std::generate(data_.Get() + size_, data_.Get() + new_size, []() { return Type(); });
        } 
        
        size_ = new_size;
    }
    
    void PushBack(const Type& element) {
        if (size_ >= capacity_) {
            size_t increased_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> temp(increased_capacity);
            
            std::fill(temp.Get(), temp.Get() + increased_capacity, Type());
            std::copy(data_.Get(), data_.Get() + size_, temp.Get());
            
            data_.swap(temp);
            capacity_ = increased_capacity;
        }
        
        data_[size_] = element;
        ++size_;
    }
    
    void PushBack(Type&& element) {
        if (size_ >= capacity_) {
            size_t increased_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            ArrayPtr<Type> temp(increased_capacity);
            
            std::generate(temp.Get(), temp.Get() + increased_capacity, []() { return Type(); });
            std::move(data_.Get(), data_.Get() + size_, temp.Get());
            
            data_.swap(temp);
            capacity_ = increased_capacity;
        }
        
        data_[size_] = std::move(element);
        ++size_;
    }
    
    void PopBack() noexcept {
        --size_;
    }
    
    void Reserve(size_t reserved_capacity) {
        if (reserved_capacity <= capacity_) {
            return;
        }
    
        ArrayPtr<Type> new_data(reserved_capacity);
        std::fill(new_data.Get(), new_data.Get() + capacity_, Type());
        std::copy(data_.Get(), data_.Get() + size_, new_data.Get());
        data_.swap(new_data);
        capacity_ = reserved_capacity;
    }
    
    Iterator Erase(Iterator pos) {
        size_t removing_index = pos - data_.Get();
        
        for (auto begin = pos; begin != end() - 1; ++begin) {
            *begin = std::move(*(begin + 1));
        }
        
        --size_;
        
        return Iterator(data_.Get() + removing_index);
    }
        
    void swap(SimpleVector<Type>& other) noexcept {
        data_.swap(other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    Iterator Insert(Iterator pos, const Type& element) {
        size_t inserting_index = pos - data_.Get();
        
        if (size_ >= capacity_) {
            size_t increased_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            
            ArrayPtr<Type> temp(increased_capacity);
            std::fill(temp.Get(), temp.Get() + increased_capacity, Type());
            
            std::copy(data_.Get(), pos, temp.Get());
            
            temp[inserting_index] = element;
            
            std::copy(pos, data_.Get() + size_, temp.Get() + inserting_index + 1);
            
            data_.swap(temp);
            capacity_ = increased_capacity;
        } else {
            std::copy_backward(pos, data_.Get() + size_, data_.Get() + size_ + 1);
            data_[inserting_index] = element;
        }
        
        ++size_;
        
        return Iterator(data_.Get() + inserting_index);
    }
    
    Iterator Insert(Iterator pos, Type&& element) {
        size_t inserting_index = pos - data_.Get();
        
        if (size_ >= capacity_) {
            size_t increased_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
            
            ArrayPtr<Type> temp(increased_capacity);
            std::generate(temp.Get(), temp.Get() + increased_capacity, []() { return Type(); });
            
            std::move(data_.Get(), pos, temp.Get());
            
            temp[inserting_index] = std::move(element);
            
            std::move(pos, data_.Get() + size_, temp.Get() + inserting_index + 1);
            
            data_.swap(temp);
            capacity_ = increased_capacity;
        } else {
            std::move_backward(pos, data_.Get() + size_, data_.Get() + size_ + 1);
            data_[inserting_index] =std::move(element);
        }
        
        ++size_;
        
        return Iterator(data_.Get() + inserting_index);
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return data_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return data_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return data_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return data_.Get() + size_;
    }
    
private: 
    size_t size_ = 0;
    size_t capacity_ = 0;
    ArrayPtr<Type> data_;
};

template <typename Type>
bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs.GetSize() == rhs.GetSize() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return lhs < rhs || lhs == rhs;
}

template <typename Type>
bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (rhs < lhs) || (rhs == lhs);
}
