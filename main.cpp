#include <iostream>
#include <format>

#include "ct_regex.h"

template<typename T,size_t size>
void checkString(const CompileTimeDfa<T,size>& regex, std::string_view instr){
    std::cout << std::format("Regex:{}, String:{}, Result:{}\n", regex.rawString.value, instr, regex.CheckString(instr));
}

int main() {

    //std::cout << "abcdefg"_ct.SubStr<2,("abcdefg"_ct).size - 2>().value << std::endl;
    auto regex = DefineRegex<"\\([0-9]*(.[0-9]+)?\\)",false>();
    std::cout << regex.CheckString("(2)")<< std::endl;
    std::cout << regex.CheckString("(2.3)")<< std::endl;
    std::cout << regex.CheckString("(2.)")<< std::endl;

    std::string str;
    std::cin >> str;
    checkString(regex,str);
    return 0;
}
