//
// Created by pkuyo on 2024/4/5.
//

#ifndef COMPILETIMEDFA_CT_LIST_H
#define COMPILETIMEDFA_CT_LIST_H

#include <algorithm>
#include <tuple>

namespace pkuyo_detail {
    template<size_t N>
    struct ct_listData {

        //list with template parameters
        template<size_t ...M>
        constexpr explicit ct_listData(std::index_sequence<M...> = std::make_index_sequence<N>()) {
            if constexpr (N != 0)
                setValue(std::make_index_sequence<N>(), std::make_tuple(M...));
            else
                data[0] =  sortData[0] = 0;
        }
        //list with array and certain count
        template<size_t M = N>
        constexpr ct_listData(size_t (&m)[M], std::index_sequence<N>) {
            if constexpr (N != 0)
            {
                std::copy_n(m,m+N,data);
                std::copy_n(m,m+N,sortData);
                std::sort(sortData, sortData + N);
            }
            else
                data[0] = sortData[0] = 0;
        }
        //list with array
        constexpr ct_listData(size_t (&m)[N]) {
            if constexpr (N != 0)
            {
                std::copy(m,m+N,data);
                std::copy(m,m+N,sortData);
                std::sort(sortData, sortData + N);
            }
            else
                data[0] = sortData[0] = 0;
        }
        template<size_t ...M, typename Tuple>
        constexpr void setValue(std::index_sequence<M...>, Tuple &&tuple) {
            (setValue_impl<M>(std::forward<Tuple>(tuple)), ...);
            std::sort(sortData, sortData + N);
        }

        template<size_t M, typename Tuple>
        constexpr void setValue_impl(Tuple &&tuple) {
            data[M] = sortData[M] = std::get<M>(std::forward<Tuple>(tuple));
        }

        [[nodiscard]] constexpr size_t operator[](size_t index) const{
            return data[index];
        }
        [[nodiscard]] constexpr size_t sortAt(size_t index) const{
            return sortData[index];
        }

        [[nodiscard]] constexpr bool contains(size_t value) const{
            if constexpr (N == 0)
                return false;
            else
                return std::find(data,data+N,value) != data + N;
        }


        size_t sortData[N + 1]{0};
        size_t data[N + 1]{0};

    };

    template<size_t ...M>
    static constexpr ct_listData<sizeof...(M)> newList() {
        return ct_listData<sizeof...(M)>(std::index_sequence<M...>());
    }
}
template<size_t... N> requires ((N < 10000000) &&...) || (sizeof...(N) == 0)
struct ct_list;

template<size_t item, size_t M, typename Tuple>
constexpr bool contains_item_impl(Tuple &&tuple) {
    return std::get<M>(tuple) == item;
}

template<typename list, size_t item, size_t... N>
constexpr bool contains_item(std::index_sequence<N...> &&) {
    return (contains_item_impl<item, N>(list::tuple)|| ...);
};


template<typename list1, typename list2>
constexpr bool is_value_equal_v =
        list1::size == list2::size && std::equal(list1::data.sortData, list1::data.sortData + list1::size, list2::data.sortData);

namespace pkuyo_detail
{
    template<size_t expectLength,size_t length1,size_t length2>
    consteval auto merge_list_new_impl(pkuyo_detail::ct_listData<length1> list1, pkuyo_detail::ct_listData<length2> list2)
    {
        size_t rawArray[expectLength]{0};
        size_t outSize = 0;
        for(size_t index1 = 0,index2 = 0; index1 < length1 || index2 < length2;)
        {
            if(index1 == length1)
                rawArray[outSize++] = list2.sortAt(index2++);
            else if (index2 == length2)
                rawArray[outSize++] = list1.sortAt(index1++);
            else if(list1.sortAt(index1) == list2.sortAt(index2))
            {
                rawArray[outSize++] = list1.sortAt(index1++);
                index2++;
            }
            else if(list1.sortAt(index1) <list2.sortAt(index2))
                rawArray[outSize++] = list1.sortAt(index1++);
            else
                rawArray[outSize++] = list2.sortAt(index2++);
        }

        return pkuyo_detail::ct_listData<expectLength>(rawArray);
    }

    template<pkuyo_detail::ct_listData array,size_t ...N>
    consteval auto merge_list_new_ret(std::index_sequence<N...>)
    {
        return ct_list<array.sortAt(N)...>();
    }

    template<size_t length1,size_t length2>
    consteval size_t merge_list_new_getLength(pkuyo_detail::ct_listData<length1> list1, pkuyo_detail::ct_listData<length2> list2)
    {
        size_t outSize = 0;
        for(size_t index1 = 0,index2 = 0; index1 < length1 || index2 < length2;)
        {
            if(index1 == length1) outSize++,index2++;
            else if (index2 == length2) outSize++,index1++;
            else if(list1.sortAt(index1) == list2.sortAt(index2))outSize++,index1++,index2++;
            else if(list1.sortAt(index1) < list2.sortAt(index2))outSize++,index1++;
            else outSize++,index2++;
        }

        return outSize;
    }

    template<typename List1, typename List2>
    consteval auto merge_list_new()
    {
        constexpr auto list1 = List1::data;
        constexpr auto list2 = List2::data;
        constexpr size_t size = merge_list_new_getLength(list1,list2);
        return merge_list_new_ret<merge_list_new_impl<size>(list1,list2)>(std::make_index_sequence<size>());
    }
}


template<typename ...lists>
struct merge_list;
template<typename list>
struct merge_list<list> {
    using Type = list;
};

template<typename list1>
struct merge_list<list1, ct_list<>> {
    using Type = list1;
};

template<typename list1, typename list2>
struct merge_list<list1, list2> {
    using Type = decltype(pkuyo_detail::merge_list_new<list1,list2>());
};

template<typename list1, typename list2, typename ...list>
struct merge_list<list1, list2, list...> {
    using Type = typename merge_list<typename merge_list<list1, list2>::Type, typename merge_list<list...>::Type>::Type;
};


template<typename ...list>
using merge_list_t = typename merge_list<list...>::Type;

template<typename list, size_t count, typename = void>
struct list_remove;

template<typename list, size_t count>
struct list_remove<list, count, std::enable_if_t<list::size >= count>> {
    using Type = decltype(list::_remove(std::make_index_sequence<list::size - count>()));
};

template<typename list, size_t count>
struct list_remove<list, count, std::enable_if_t<list::size <= count - 1>> {
    using Type = ct_list<>;
};

template<size_t... N> requires ((N < 10000000) &&...) || (sizeof...(N) == 0)
struct ct_list {
public:
    static constexpr pkuyo_detail::ct_listData<sizeof...(N)> data{pkuyo_detail::newList<N...>()};
    static constexpr size_t size = sizeof...(N);

    static constexpr size_t last = data.data[size > 0 ? size -1 :0];

    template<size_t index>
    static constexpr size_t value = data.data[index];


    template<size_t index>
    static constexpr size_t sortValue = data.sortData[index];


    template<size_t... M>
    static ct_list<value<M>...> _remove(std::index_sequence<M...>);


    template<size_t toFind>
    static constexpr bool contains = data.contains(toFind);

    template<size_t app>
    using Append = std::conditional_t<contains<app>, ct_list<N...>, ct_list<N..., app>>;

    template<size_t app>
    using AppendNoUnique = ct_list<N..., app>;

    template<size_t count>
    using Remove = typename list_remove<ct_list<N...>, count>::Type;


};

template<size_t... M, size_t ...N>
auto operator|(ct_list<M...>, ct_list<N...>) {
    return merge_list_t<ct_list<M...>, ct_list<N...>>();
}

template<size_t ...N>
auto operator|(ct_list<>, ct_list<N...>) {
    return ct_list < N...>();
}

template<size_t ...N>
auto operator|(ct_list<N...>, ct_list<>) {
    return ct_list <N...>();
}


auto operator|(ct_list<>, ct_list<>) {
    return ct_list <> ();
}

#endif //COMPILETIMEDFA_CT_LIST_H
