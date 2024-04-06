#include <iostream>
#include <format>

#include "ct_regex.h"

template<typename T,size_t size>
void checkString(const CompileTimeDfa<T,size>& regex, std::string_view instr){
    std::cout << std::format("Regex:{}, String:{}, Result:{}\n", regex.rawString.value, instr, regex.CheckString(instr));
}

int main() {


    //using Defined = pkuyo_detail::RegexDefine<"(a-z|_)(a-z|1-9|_)*">;
    //cout << typeid(Defined::NfaGraph).name() << endl;
    //cout << Defined::rawRegex.value << endl;
    //cout << Defined::useRegex.value << endl;
    //auto regex = DefineRegex<Defined::rawRegex>();

    auto regex = DefineRegex<"(a-z|_)(a-z|1-9|_)*">();
    std::string str;
    std::cin >> str;
    checkString(regex,str);
    return 0;
}
