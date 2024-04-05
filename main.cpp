#include <iostream>
#include <format>

#include "ct_regex.h"

using namespace std;
template<typename T,size_t size>
void checkString(const CompileTimeDfa<T,size>& regex, string_view instr){
    cout << format("Regex:{}, String:{}, Result:{}\n", regex.rawString.value, instr, regex.CheckString(instr));
}

int main() {
    auto regex = DefineRegex<"Number:(0-9)*:(0-9)*">();
    checkString(regex,"Number:0432:2990231");
    checkString(regex,"Number::");
    checkString(regex,"Number:19592345");
    checkString(regex,"Num:0432:2990231");

    string str;
    cin >> str;
    checkString(regex,str);
    return 0;
}
