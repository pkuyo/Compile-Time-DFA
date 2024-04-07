//
// Created by pkuyo on 2024/4/5.
//

#ifndef COMPILETIMEDFA_CT_STRING_H
#define COMPILETIMEDFA_CT_STRING_H

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

    template<size_t t>
    constexpr ct_stringData(const ct_stringData<N - t> *s, const ct_stringData<t+1>& s2,std::index_sequence<t>) {
        std::copy_n(s->value, N - t - 1, value);
        std::copy_n(s2.value,t,value + N - t - 1);
        value[N - 1] = '\0';
    }
    constexpr ct_stringData(const ct_stringData<N - 1> *s, char append, size_t index) {
        for(size_t i = 0; i< N-1;i++)
            value[i] = i > index ? s->value[i-1] : i == index ? append :s->value[i];

    }

public:


    template<size_t Start, size_t Length>
    consteval ct_stringData<Length> SubStr() const {
        return ct_stringData<Length>(this, Start);
    }

    consteval ct_stringData<N + 1> Append(char c) const {
        return ct_stringData<N + 1>(this, c);
    }

    template<size_t t>
    consteval auto Append(const ct_stringData<t>& c) const {
        return ct_stringData<N + t - 1>(this, c, std::index_sequence<t-1>());
    }
    consteval ct_stringData<N + 1> Insert(char c,size_t index = 0) const {
        return ct_stringData<N + 1>(this, c, index);
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


#endif //COMPILETIMEDFA_CT_STRING_H
