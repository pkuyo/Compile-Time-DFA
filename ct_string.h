//
// Created by pkuyo on 2024/4/5.
//

#ifndef CLION_CT_STRING_H
#define CLION_CT_STRING_H

#include <algorithm>

template<size_t N>
struct ct_stringData {
    constexpr ct_stringData(const char(&str)[N]) {
        std::copy_n(str, N, value);
    }

    template<size_t M>
    constexpr ct_stringData(const ct_stringData<M> *s, size_t start) {
        std::copy_n(s->value + start, N - 1, value);
        value[N - 1] = '\0';
    }

    template<size_t M>
    constexpr ct_stringData(const ct_stringData<M> *s) {
        std::copy_n(s->value, N - 1, value);
        value[N - 1] = '\0';
    }

    constexpr ct_stringData(const ct_stringData<N - 1> *s, char append) {
        std::copy_n(s->value, N - 2, value);
        value[N - 2] = append;
        value[N - 1] = '\0';
    }

public:


    template<size_t Start, size_t Length>
    consteval ct_stringData<Length> SubStr() const {
        return ct_stringData<Length>(this, Start);
    }

    consteval ct_stringData<N + 1> Append(char c) const {
        return ct_stringData<N + 1>(this, c);
    }

    consteval auto AppendUnique(char c) const {
        if constexpr (std::find(std::begin(value), std::end(value), c) != std::end(value))
            return ct_stringData<N>(this);
        else
            return Append(c);
    }

    [[nodiscard]] constexpr char First(size_t startIndex = 0) const {
        return value[startIndex];
    }

    [[nodiscard]] constexpr char Last(size_t lastIndex = 0) const {
        return value[N - lastIndex - 2];
    }

public:
    static constexpr size_t size = N;
    char value[N]{};
};


template<ct_stringData cts>
struct CompileTimeString {
    static constexpr auto value = cts;
};


template<ct_stringData cts>
consteval auto operator ""_ct() {
    return cts;
}


#endif //CLION_CT_STRING_H
