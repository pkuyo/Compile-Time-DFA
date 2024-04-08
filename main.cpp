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

    auto regex = DefineRegex<"\\([0-9]*(.[0-9]+)?\\)",true/*false for not minimize*/>();
    auto regex2 = DefineRegex<"\\([0-9]*(.[0-9]+)?\\)",false/*false for not minimize*/>();

    std::cout << std::boolalpha << std::endl;
    std::cout << regex.CheckString("(13)")<< std::endl;
    std::cout << regex.CheckString("(13.)")<< std::endl;
    std::cout << regex.CheckString("(13.2)")<< std::endl;
    std::cout << regex.CheckString("(.23)")<< std::endl;
    std::cout << regex2.CheckString("(03)")<< std::endl;
    std::cout << regex.CheckString("(192)")<< std::endl;

    std::string str;
    while(str != "exit") {
        std::cin >> str;
        std::cout << regex.CheckString(str) << std::endl;
    }
    return 0;
}
