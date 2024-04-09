//
// Created by pkuyo on 2024/4/5.
//

#ifndef COMPILETIMEDFA_CT_UTILS_H
#define COMPILETIMEDFA_CT_UTILS_H

namespace pkuyo::pkuyo_detail{

    struct ct_empty{};

    template<typename T>
    T operator|(ct_empty,T in){
        return in;
    }

    template<typename T>
    T operator|(T in,ct_empty){
        return in;
    }

    ct_empty operator|(ct_empty,ct_empty){
        return {};
    }
}

#endif //COMPILETIMEDFA_CT_UTILS_H
