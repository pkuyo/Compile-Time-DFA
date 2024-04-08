//
// Created by pkuyo on 2024/4/5.
//

//Testing platform : Windows 11, gcc 13.2, stdc++20

//Recommended compilation options:
//  -fconstexpr-loop-limit=1000000000
//  -fconstexpr-ops-limit=1000000000

#include <iostream>
#include "ct_regex.h"

int main() {

    constexpr auto variable = DefineRegex<"[a-z_][a-z0-9_]*",true/*false for not minimize*/>();
    constexpr auto number = DefineRegex<"[0-9]+(.[0-9]+f?)?",true/*false for not minimize*/>();

    std::cout << std::boolalpha << std::endl;
    std::cout << number.CheckString("1.23f") << std::endl;
    std::cout << number.CheckString("72") << std::endl;
    std::cout << number.CheckString("3.1415926") << std::endl;
    std::cout << number.CheckString("23.") << std::endl;
    std::string str;
    while(str != "exit") {
        std::cin >> str;
        std::cout << variable.CheckString(str) << std::endl;
    }
    return 0;
}
