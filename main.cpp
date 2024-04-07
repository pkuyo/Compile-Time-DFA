//
// Created by pkuyo on 2024/4/5.
//

#include <iostream>
#include "ct_regex.h"

int main() {

    auto regex = DefineRegex<"\\([0-9]*(.[0-9]+)?\\)",false>();
    std::cout << std::boolalpha << std::endl;
    std::cout << regex.CheckString("(2)")<< std::endl;
    std::cout << regex.CheckString("(2.3)")<< std::endl;
    std::cout << regex.CheckString("(2.)")<< std::endl;

    std::string str;
    while(str != "exit") {
        std::cin >> str;
        std::cout << regex.CheckString(str) << std::endl;
    }
    return 0;
}
