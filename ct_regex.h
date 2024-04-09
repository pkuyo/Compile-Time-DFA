//
// Created by pkuyo on 2024/4/5.
//

#ifndef COMPILETIMEDFA_CT_REGEX_H
#define COMPILETIMEDFA_CT_REGEX_H

#include <string_view>
#include <type_traits>
#include <cstdint>
#include "ct_utils.h"
#include "ct_list.h"
#include "ct_string.h"

namespace pkuyo_detail
{
    template<typename Dfa, ct_stringData cts,typename MoveList>
    consteval auto CreateExecuteDfa();

    template<typename Dfa, ct_stringData cts,typename MoveList>
    consteval auto CreateMinimizeExecuteDfa();
}

template<size_t letterLength>
class CompileTimeNode {

    __int64 index = -1;
    __int64 connects[letterLength]{0};
    bool isEnd = false;
    size_t connectLength = 0;
private:

    template<typename T/*DfaNode*/, size_t ...N>
    constexpr CompileTimeNode(T, __int64 index, bool isEnd, std::index_sequence<N...>,const int16_t (&letterMap)[256]) : index(index), isEnd(isEnd) {
        std::fill(connects,connects+letterLength,-1);
        (AddNewPath(T::ConnectionChecks::template value<N>, T::Connections::template value<N>,letterMap), ...);
    }

    template<size_t length>
    constexpr CompileTimeNode(const int16_t (&stateChecks)[length],const __int64(&stateConnects)[length],__int64 index, bool isEnd)
    : index(index), isEnd(isEnd) {
        std::fill(connects,connects+letterLength,-1);
        for(int i=0;i<length;i++){
            if(stateChecks[i] == -1) break;
            AddNewPath(stateChecks[i],stateConnects[i]);
        }
    }
    constexpr void AddNewPath(uint8_t c, __int64 toIndex) {
        connects[c] = toIndex;
        connectLength++;
    }

    constexpr void AddNewPath(char c, __int64 toIndex,const int16_t (&letterMap)[256]) {
        connects[letterMap[c]] = toIndex;
        connectLength++;
    }



public:

    constexpr CompileTimeNode() = default;

    [[nodiscard]] constexpr bool IsEnd() const {
        return isEnd;
    }

    __int64 NextPath(char c) const {
        return connects[c];
    }

    template<typename rawStr, size_t length,size_t _letterLength>
    friend class CompileTimeDfa;

};

template<typename rawStr, size_t length,size_t letterLength>
class CompileTimeDfa;


template<ct_stringData cts, size_t length,size_t letterLength>
class CompileTimeDfa<CompileTimeString<cts>, length,letterLength> {

    CompileTimeNode<letterLength> nodes[length]{};

public:
    size_t startIndex = 0;
    static constexpr auto rawString = cts;
    constexpr static size_t size = length;

    int16_t letterMap[256]{0};

private:


    struct MinimizeData
    {
        int16_t stateChecks[length][letterLength]{0};
        __int64 stateConnects[length][letterLength]{0};
        int16_t stateFlags[length];
        size_t minimizeLength = 0;
        constexpr MinimizeData(int16_t (&inChecks)[length][letterLength], __int64(&inConnects)[length][letterLength],  int16_t (&inFlags)[length],size_t newSize) : minimizeLength(newSize)
        {
            for(int i=0;i<size;i++) {
                std::copy(inChecks[i],inChecks[i]+letterLength,stateChecks[i]);
                std::copy(inConnects[i],inConnects[i]+letterLength,stateConnects[i]);
            }
            std::copy(inFlags,inFlags+size,stateFlags);
        }
    };

    template<typename Graph, size_t N>
    consteval void CreateExecuteDfa_single_impl(CompileTimeDfa *exec) {
        using Node = typename Graph::template Node<N>;
        auto execNode = CompileTimeNode<letterLength>(Node(), Node::_index, Node::_isEnd,
                                        std::make_index_sequence<Node::Connections::size>(),exec->letterMap);
        exec->nodes[Node::_index] = execNode;
    }

    template<typename StateList/*ct_list<>*/,typename Graph, size_t ...N>
    consteval CompileTimeDfa(StateList,std::index_sequence<N...>, Graph) {
        std::fill(letterMap,letterMap+256,(int16_t)-1);
        for(size_t i =0;i < StateList::size;i++)
            letterMap[StateList::data[i]] = i;
        (CreateExecuteDfa_single_impl<Graph, N>(this), ...);

    }

    template<typename StateList/*ct_list<>*/,size_t size,size_t charLength>
    consteval CompileTimeDfa(StateList,const int16_t (&stateChecks)[size][charLength],const __int64(&stateConnects)[size][charLength], const int16_t (&stateFlags)[size]) {
        std::fill(letterMap,letterMap+256,(int16_t)-1);
        for(size_t i =0;i < StateList::size;i++)
            letterMap[StateList::data[i]] = i;

        for(int i=0;i<length;i++) {
            nodes[i] = CompileTimeNode<letterLength>(stateChecks[i], stateConnects[i], i, stateFlags[i] & 1);
            if(stateFlags[i] & 2)
                startIndex = i;
        }

    }

    consteval static void Dfs_findUnreachable(const CompileTimeDfa & dfa,__int64 (&reachable)[size],int currentNode = 0)
    {
        reachable[currentNode] =  dfa.nodes[currentNode].IsEnd() ? 1 : 0;
        for(int i=0;i<letterLength;i++){
            if(dfa.nodes[currentNode].connects[i] == -1) continue;
            if(reachable[dfa.nodes[currentNode].connects[i]] == -1)
                Dfs_findUnreachable(dfa,reachable,dfa.nodes[currentNode].connects[i]);
        }
    }

    template<typename MoveList,size_t size>
    consteval static auto Minimize_impl(const CompileTimeDfa & dfa)
    {
        //old state node map to new state
        __int64 statesMap[size]{0};
        std::fill(statesMap,statesMap+size,-1);
        //if newStates[i]<0 it's a reachable node
        Dfs_findUnreachable(dfa,statesMap,0);

        __int64 statesReverseMap[size][size]{0};
        __int64 newStatesCount = 2;

        // 1 -> End 2-> Start
        int16_t statesFlag[size]{0};

        //old state node in new state length
        __int64 statesLength[size]{0};


        //init new state group : end / not-end state group
        for(size_t i = 0; i < size; i++) {
            if(statesMap[i] >= 0){
                statesReverseMap[statesMap[i]][ statesLength[statesMap[i]]++ ] = i;
            }
        }

        bool needCheckAgain = false;
        do {
            needCheckAgain = false;
            for(size_t i = 0; i < MoveList::size; i++){
                auto c = i;
                auto lastNewLength = newStatesCount;

                for(size_t j = 0;j < lastNewLength; j++){

                    //group state move to ...
                    __int64 tmpMoveTo[size]{0};
                    size_t tmpMoveToLength = 0;

                    auto lastState = statesLength[j];
                    auto LastLength = newStatesCount - 1;

                    //separate single group
                    for(size_t k = 0; k < statesLength[j]; k++){
                        auto nodeIndex = statesReverseMap[j][k];

                        __int64 moveTo = dfa.nodes[nodeIndex].connects[c] == -1 ? -1 : statesMap[dfa.nodes[nodeIndex].connects[c]];

                        size_t moveIndex = 0;
                        for(; moveIndex < tmpMoveToLength; moveIndex++)
                            if(tmpMoveTo[moveIndex] == moveTo)
                                break;

                        if(moveIndex == tmpMoveToLength){
                            tmpMoveTo[tmpMoveToLength++] = moveTo;
                            if(moveIndex != 0) {
                                newStatesCount++;
                                needCheckAgain = true;
                            }
                        }
                        //separate state
                        if(moveIndex != 0){
                            auto newStateIndex = LastLength+moveIndex;
                            statesMap[nodeIndex] = newStateIndex;
                            statesReverseMap[newStateIndex][statesLength[newStateIndex]++] = nodeIndex;
                            statesReverseMap[j][k] = -1;
                            statesLength[j]--;
                        }
                    }

                    //clean empty slot
                    size_t insertPos[lastState]{0};
                    size_t insertEnd = 0;
                    size_t insertStart = 0;
                    for(size_t k = 0; k< lastState; k++){
                        if(statesReverseMap[j][k] == -1){
                            insertPos[insertEnd++] = k;
                        }
                        else if(insertStart!=insertEnd) {
                            statesReverseMap[j][insertPos[insertStart++]] = statesReverseMap[j][k];
                            statesReverseMap[j][k--] = -1;
                        }
                    }

                }
            }
        } while (needCheckAgain);

        //new state node connect to
        __int64 statesConnect[size][MoveList::size]{0};
        //new state node connect check char
        int16_t statesCheck[size][MoveList::size]{0};

        __int64 statesConnectLength[size]{0};

        for(size_t i = 0; i < newStatesCount; i++){
            for(size_t j = 0; j< MoveList::size;j++){
                auto c = j;
                if(dfa.nodes[statesReverseMap[i][0]].connects[c] == -1|| statesMap[dfa.nodes[statesReverseMap[i][0]].connects[c]] < 0) continue;
                statesConnect[i][statesConnectLength[i]] = statesMap[ dfa.nodes[statesReverseMap[i][0]].connects[c] ];
                statesCheck[i][statesConnectLength[i]++] = c;
            }
            if(statesConnectLength[i] != MoveList::size)
                statesCheck[i][statesConnectLength[i]] = -1; //set end
            for(int j = 0;j< statesLength[i];j++){
                if(dfa.nodes[statesReverseMap[i][j]].IsEnd())
                    statesFlag[i] |= 1;
                if (statesReverseMap[i][j] == 0)
                    statesFlag[i] |= 2;
            }


        }

        return MinimizeData(statesCheck, statesConnect, statesFlag, newStatesCount);
    }

public:

    [[nodiscard]] constexpr bool CheckString(std::string_view view) const {
        __int64 node = startIndex;
        __int64 index = 0;
        while (index != view.size()) {
            if(letterMap[view[index]] == -1)
                return false;
            node = nodes[node].NextPath(letterMap[view[index++]]);
            if (node == -1)
                return false;
        }
        return nodes[node].IsEnd();
    }


    template<typename Dfa, ct_stringData c,typename MoveList>
    friend consteval auto pkuyo_detail::CreateExecuteDfa();

    template<typename Dfa, ct_stringData c,typename MoveList>
    friend consteval auto  pkuyo_detail::CreateMinimizeExecuteDfa();

};



namespace pkuyo_detail {

    //character operation functions
    enum RegexEscapeChar
    {
        REC_OR = 1,         // '|'
        REC_AND = 2,        // '&'
        REC_ANY = 3,        // '*'
        REC_MAY = 4,        // '?'
        REC_L_ROUND = 5,    // '('
        REC_R_ROUND = 6,    // ')'
        REC_L_SQUARE = 7,   // '['
        REC_R_SQUARE = 8,   // ']'
        REC_TO = 9,         // '-'
        REC_EMPTY = 10,     // ' '
        REC_ADD = 11,       // '+'
    };


    constexpr char CharEscape(char c)
    {
        switch(c)
        {
            case '|':return REC_OR;
            case '&':return REC_AND;
            case '*':return REC_ANY;
            case '?':return REC_MAY;
            case '(':return REC_L_ROUND;
            case ')':return REC_R_ROUND;
            case '[':return REC_L_SQUARE;
            case ']':return REC_R_SQUARE;
            case '-':return REC_TO;
            case ' ':return REC_EMPTY;
            case '+':return REC_ADD;
            default:return c;
        }
    }

    constexpr int SymbolSpeed(char c) {
        if (c == REC_OR || c == REC_AND)
            return 2;
        if (c == REC_ANY || c == REC_MAY || c == REC_ADD)
            return 3;
        if (c == REC_L_ROUND || c == REC_R_ROUND)
            return 1;
        return 0;
    }

    constexpr char CharEscapeBack(char c)
    {
        switch(c)
        {
            case REC_OR:return '|';
            case REC_AND: return '&';
            case REC_ANY:return '*';
            case REC_MAY:return '?';
            case REC_L_ROUND:return '(';
            case REC_R_ROUND:return ')';
            case REC_L_SQUARE:return '[';
            case REC_R_SQUARE:return ']';
            case REC_TO:return '-';
            case REC_EMPTY:return ' ';
            case REC_ADD:return '+';
            default:return c;
        }
    }


    constexpr bool IsSingle(char c) { return SymbolSpeed(c) == 3; }

    constexpr bool IsExecSymbol(char c) { return SymbolSpeed(c) != 0; }

    constexpr bool IsTwoSideSymbol(char c) { return (SymbolSpeed(c) != 0 && !IsSingle(c)); }

    constexpr bool IsRightBracket(char c) {
        return c == REC_R_ROUND || c == REC_R_SQUARE;
    }
    constexpr bool IsLeftBracket(char c) {
        return c == REC_L_ROUND || c == REC_L_SQUARE;
    }

    constexpr bool IsLatePreprocessSymbol(char c) {
        return c == REC_TO;
    }
    constexpr bool IsHeadPreprocessSymbol(char c) {
        return c == REC_R_SQUARE || c == REC_L_SQUARE;
    }


    //escape regex
    template<ct_stringData cts, ct_stringData out = "", bool isEscape = false,__int64 bracketCount = 0>
    consteval auto ctPreprocess_Escape() {
        if constexpr (cts.size == 1) {
            static_assert(bracketCount == 0,"mismatched brackets");
            return CompileTimeString<out>();
        }
        else if constexpr (isEscape) {
            static_assert((cts.First() == '\\' || CharEscape(cts.First())!= cts.First()) && cts.First() != ' ',"invalid \\ character");
            return ctPreprocess_Escape<cts.template SubStr<1, cts.size - 1>(),
                    out.template SubStr<0, out.size - 1>().Append(cts.First()),false,bracketCount>();
        }
        else {
            return ctPreprocess_Escape<cts.template SubStr<1, cts.size - 1>(),
                    out.Append(CharEscape(cts.First())),
                            cts.First() == '\\',
                            (bracketCount + (IsRightBracket(CharEscape(cts.First())) ? -1 :
                             (IsLeftBracket(CharEscape(cts.First())) ? 1 : 0)))>();
        }
    }

    //insert & or | in regex, expand A-Z to A|B|...|Z
    template<ct_stringData cts, ct_stringData out = "",char insertChar = REC_AND>
    consteval auto ctPreprocess_InsertOpera() {
        if constexpr (cts.size == 1) {
            static_assert(insertChar == REC_AND,"mismatched brackets [ ]");
            return CompileTimeString<out>();
        }
        else if constexpr (IsHeadPreprocessSymbol(cts.First())){

                //change [] to ()
                if constexpr (cts.First() == REC_L_SQUARE)
                    return ctPreprocess_InsertOpera<cts.template SubStr<1, cts.size - 1>(), out,REC_OR>();
                else if constexpr (cts.First() == REC_R_SQUARE)
                    return ctPreprocess_InsertOpera<cts.template SubStr<1, cts.size - 1>(), out,REC_AND>();
        }
        // default symbol (without '(')
        else if constexpr (out.size == 1 ||
                            ((IsExecSymbol(cts.First()) || IsLatePreprocessSymbol(cts.First())) &&
                            !IsLeftBracket(cts.First()))) {
            return ctPreprocess_InsertOpera<cts.template SubStr<1, cts.size - 1>(), out.Append(cts.First()),insertChar>();
        } else if constexpr (IsLatePreprocessSymbol(out.Last())) {

            //'-'
            if constexpr (out.Last() == REC_TO) {
                //inside []
                if constexpr (insertChar == REC_OR) {

                    static_assert(CharEscapeBack(out.Last(1)) == out.Last(1) && CharEscapeBack(cts.First()) == cts.First(),
                            "invalid operation '-': please use with single character");
                    static_assert(out.Last(1) <= cts.First(),"invalid operation '-': left is greater than right");

                    if constexpr (out.Last(1) == cts.First())
                        return ctPreprocess_InsertOpera<cts.template SubStr<1, cts.size - 1>(), out.template SubStr<0,
                                out.size - 1>(),insertChar>();
                    else
                        return ctPreprocess_InsertOpera<cts, out.template SubStr<0, out.size - 1>().Append(REC_OR).Append(
                                out.Last(1) + 1).Append(REC_TO),insertChar>();
                }
                //outside []
                else
                    return ctPreprocess_InsertOpera<cts.Insert('-'), out.template SubStr<0,
                            out.size - 1>(),insertChar>();
            }

        }
        else if constexpr (!IsTwoSideSymbol(out.Last()) || IsRightBracket(out.Last())) {
            return ctPreprocess_InsertOpera<cts.template SubStr<1, cts.size - 1>(), out.Append(insertChar).Append(cts.First()),insertChar>();
        } else
            return ctPreprocess_InsertOpera<cts.template SubStr<1, cts.size - 1>(), out.Append(cts.First()),insertChar>();
    }

    //parse (A) + to (A)(A)*
    template<ct_stringData cts, ct_stringData out = "",ct_stringData indexData = "">
    consteval auto ctPreprocess_Parse() {


        if constexpr (cts.size == 1) {
            return CompileTimeString<out>();
        }
        else if constexpr (cts.First() == REC_ADD)
        {
            if constexpr (IsRightBracket(out.Last())) {
                constexpr auto str = out.template SubStr<indexData.Last(),out.size - indexData.Last()>();
                return ctPreprocess_Parse<cts.template SubStr<1, cts.size - 1>(),
                        out.Append(REC_AND).Append(str).Append(REC_ANY),
                        indexData.template SubStr<0, indexData.size - 1>()>();
            }
            else
                return ctPreprocess_Parse<cts.template SubStr<1, cts.size - 1>(),out.Append(out.Last()).Append(REC_ANY),
                        indexData>();
        }
        else if constexpr (IsLeftBracket(cts.First())) {
            // '[' --> '(['
            if constexpr (cts.First() == REC_L_SQUARE) {
                return ctPreprocess_Parse<cts.template SubStr<1, cts.size - 1>(), out.Append(REC_L_ROUND).Append(
                        cts.First()),
                        indexData.Append(out.size - 1)>();
            } else {
                return ctPreprocess_Parse<cts.template SubStr<1, cts.size - 1>(), out.Append(
                        cts.First()), indexData.Append(out.size - 1)>();
            }
        }
        // ']' --> ')]'
        else if constexpr (cts.First() == REC_R_SQUARE) {
            return ctPreprocess_Parse<cts.template SubStr<1, cts.size - 1>(), out.Append(REC_R_ROUND).Append(
                    cts.First()),
                    indexData>();
        }
        //Very annoying special sentence
        else if constexpr (out.size == 1)
            return ctPreprocess_Parse<cts.template SubStr<1, cts.size - 1>(),out.Append(cts.First()),indexData>();
        //clean index
        else if constexpr (IsRightBracket(out.Last())) {
            return ctPreprocess_Parse<cts.template SubStr<1, cts.size - 1>(), out.Append(cts.First()),
                    indexData.template SubStr<0, indexData.size - 1>()>();
        }

        else
            return ctPreprocess_Parse<cts.template SubStr<1, cts.size - 1>(),out.Append(cts.First()),indexData>();


    }

    //preprocess input regex
    template<ct_stringData cts>
    consteval auto ctPreprocess()
    {
        return  ctPreprocess_InsertOpera<decltype(ctPreprocess_Parse<decltype(ctPreprocess_Escape<cts>())::value>())::value>();
    }

    //nfa node graph
    template<char check, size_t index, typename list = ct_list<>>
    struct NfaNode {

        template<int toIndex>
        using ConnectTo = NfaNode<check, index, typename list::template Append<toIndex>>;

        static constexpr size_t _index = index;
        static constexpr char _check = check;

        using ConnectList = list;
    };

    template<size_t index, bool hasMatched = false, typename CurrentFindNode = ct_empty>
    struct NfaNodeSearcher {
        template<typename NextNode>
        auto operator|(NextNode) {
            if constexpr (!hasMatched && index == NextNode::_index)
                return NfaNodeSearcher<index, true, NextNode>();
            else
                return NfaNodeSearcher();
        }

        using FindNode = CurrentFindNode;
    };

    template<typename... NodeTypes/*NfaNode<...>...*/>
    struct NfaNodeGraph {

        template<size_t index>
        static auto _find() {
            return (NfaNodeSearcher<index>() | ... | NodeTypes());
        }

        constexpr explicit NfaNodeGraph(std::nullptr_t) {}

        static constexpr size_t size = sizeof...(NodeTypes);


        template<typename ...T/*NfaNode<...>*/>
        using AppendNode = NfaNodeGraph<NodeTypes..., T...>;

        template<char check>
        using AppendNormalNode = NfaNodeGraph<NodeTypes..., NfaNode<check, size, ct_list<size + 1>>, NfaNode<REC_EMPTY,
                size + 1>>;


        template<size_t index>
        using FindType = decltype(_find<index>())::FindNode;

        template<size_t index, typename NewType/*NfaNode<...>*/>
        using ReplaceTypeGraph = NfaNodeGraph<std::conditional_t<NodeTypes::_index == index, NewType, NodeTypes>...>;

        template<size_t index, size_t connectNew>
        using ReplaceIndexGraph = NfaNodeGraph<std::conditional_t<NodeTypes::_index == index,
                typename FindType<index>::template ConnectTo<connectNew>, NodeTypes>...>;

    };


    //stack in building nfa
    template<char check, size_t inIndex, size_t outIndex = inIndex + 1>
    struct NfaIndex {
        static constexpr int _inIndex = inIndex;
        static constexpr int _outIndex = outIndex;

        static constexpr char _check = check;
    };

    template<typename... IndexTypes/*NfaIndex<...>...*/>
    struct NfaIndexStack;

    template<typename ... IndexTypes>
    struct NfaStackLastType {
        using Type = std::tuple_element_t<sizeof...(IndexTypes) - 1, std::tuple<IndexTypes...>>;
    };

    template<>
    struct NfaStackLastType<> {
        using Type = std::nullptr_t;
    };

    template<typename Stack, size_t count, typename = void>
    struct NfaStackRemoveType;

    template<typename Stack, size_t count>
    struct NfaStackRemoveType<Stack, count, std::enable_if_t<Stack::size >= count>> {
        using Type = typename Stack::template SubType<0, Stack::size - count>;
    };

    template<typename Stack, size_t count>
    struct NfaStackRemoveType<Stack, count, std::enable_if_t<Stack::size <= count - 1>> {
        using Type = NfaIndexStack<>;
    };
    template<typename Stack>
    struct nfa_stack_last_check {
        using Last = typename Stack::LastType;
        static constexpr char value = Last::_check;
    };
    template<typename Stack>
    constexpr char nfa_stack_last_check_v = nfa_stack_last_check<Stack>::value;

    template<typename... IndexTypes/*NfaIndex<...>*/>
    struct NfaIndexStack {
        static constexpr size_t size = sizeof...(IndexTypes);

        template<size_t start, size_t... N>
        requires (start + sizeof...(N) <= sizeof...(IndexTypes))
        static constexpr NfaIndexStack<std::tuple_element_t<start + N, std::tuple<IndexTypes...>>...>
        _Sub(std::index_sequence<N...>);

        template<typename ...T>
        using AppendType = NfaIndexStack<IndexTypes..., T...>;

        template<char c, size_t i>
        using AppendIndex = NfaIndexStack<IndexTypes..., NfaIndex<c, i>>;

        template<size_t start, size_t count>
        using SubType = decltype(_Sub<start>(std::make_index_sequence<count>()));

        template<size_t count>
        using RemoveType = typename NfaStackRemoveType<NfaIndexStack, count>::Type;

        using LastType = typename NfaStackLastType<IndexTypes...>::Type;

        using LastLastType = typename RemoveType<1>::LastType;
    };


    //sortData from nfaExecute, contains Graph and node index of start/end
    template<typename NfaGraph, size_t startIndex, size_t endIndex>
    struct NfaData {
        using Graph = NfaGraph;
        static constexpr size_t start = startIndex;
        static constexpr size_t end = endIndex;

    };


    //executions for building nfa
    namespace nfaExecute {
        template<char c, typename Graph /*NfaNodeGraph<>*/, typename Stack /*NfaIndexStack<>*/, typename = void>
        struct NfaExecute
        {
            using OutStack = std::nullptr_t;
            using OutGraph = std::nullptr_t;
        };

        template<typename Graph, typename Stack>
        struct NfaExecute<REC_OR, Graph, Stack, std::enable_if_t<nfa_stack_last_check_v<Stack> != REC_EMPTY
                && !std::is_same_v<typename Stack::LastLastType, std::nullptr_t> >> {
            using Index2 = typename Stack::LastType;
            using Index1 = typename Stack::LastLastType;

            using EmitNode1 = NfaNode<REC_EMPTY, Graph::size, ct_list<Index1::_inIndex, Index2::_inIndex>>;
            using EmitNode2 = NfaNode<REC_EMPTY, Graph::size + 1>;

            using OutStack = typename Stack::template RemoveType<2>::template AppendType<NfaIndex<REC_EMPTY, Graph::size>>;

            using OutGraph = typename Graph::template ReplaceIndexGraph<Index1::_outIndex, Graph::size + 1>::
            template ReplaceIndexGraph<Index2::_outIndex, Graph::size + 1>::
            template AppendNode<EmitNode1, EmitNode2>;

        };
        template<typename Graph, typename Stack>
        struct NfaExecute<REC_OR, Graph, Stack, std::enable_if_t<nfa_stack_last_check_v<Stack> == REC_EMPTY &&
                !std::is_same_v<typename Stack::LastType, std::nullptr_t>> > {
            using Index1 = typename Stack::LastLastType;
            using RootIndex = typename Stack::LastType;

            using OutStack = typename Stack::template RemoveType<2>::template AppendType<RootIndex>;

            using OutGraph = typename Graph::template ReplaceIndexGraph<Index1::_outIndex, RootIndex::_outIndex>::
            template ReplaceIndexGraph<RootIndex::_inIndex, Index1::_inIndex>;

        };

        template<typename Graph, typename Stack>
        struct NfaExecute<REC_MAY, Graph, Stack, std::enable_if_t<nfa_stack_last_check_v<Stack> != REC_EMPTY
                && !std::is_same_v<typename Stack::LastType, std::nullptr_t> >> {
            using Index = typename Stack::LastType;

            using EmitNode = NfaNode<REC_EMPTY, Graph::size, ct_list<Index::_outIndex, Index::_inIndex>>;

            using OutStack = typename Stack::template RemoveType<1>::template AppendType<NfaIndex<REC_EMPTY, Graph::size, Index::_outIndex>>;
            using OutGraph = typename Graph::template AppendNode<EmitNode>;
        };
        template<typename Graph, typename Stack>
        struct NfaExecute<REC_MAY, Graph, Stack, std::enable_if_t<nfa_stack_last_check_v<Stack> == REC_EMPTY
                && !std::is_same_v<typename Stack::LastType, std::nullptr_t> >> {
            using Index = typename Stack::LastType;
            using OutStack = Stack;
            using OutGraph = typename Graph::template ReplaceIndexGraph<Index::_inIndex, Index::_outIndex>;

        };

        template<typename Graph, typename Stack>
        struct NfaExecute<REC_AND, Graph, Stack, std::enable_if_t<!std::is_same_v<typename Stack::LastLastType,std::nullptr_t>>> {
            using Index2 = typename Stack::LastType;
            using Index1 = typename Stack::LastLastType;


            using OutStack = typename Stack::template RemoveType<2>::template AppendType<NfaIndex<Index1::_check, Index1::_inIndex, Index2::_outIndex>>;

            using OutGraph = typename Graph::template ReplaceIndexGraph<Index1::_outIndex, Index2::_inIndex>;

        };

        template<typename Graph, typename Stack>
        struct NfaExecute<REC_ANY, Graph, Stack, std::enable_if_t<!std::is_same_v<typename Stack::LastType, std::nullptr_t>> > {
            using Index = typename Stack::LastType;

            using EmitNode1 = NfaNode<REC_EMPTY, Graph::size, ct_list<Index::_inIndex, Graph::size + 1>>;
            using EmitNode2 = NfaNode<REC_EMPTY, Graph::size + 1>;

            using OutStack = typename Stack::template RemoveType<1>::template AppendType<NfaIndex<REC_EMPTY, Graph::size>>;

            using OutGraph = typename Graph::template ReplaceIndexGraph<Index::_outIndex, Index::_inIndex>::
            template ReplaceIndexGraph<Index::_outIndex, Graph::size + 1>::
            template AppendNode<EmitNode1, EmitNode2>;

        };
    }

    //create nfa from string
    template<ct_stringData cts, ct_stringData symbol = "", typename NfaStack = NfaIndexStack<>, typename NfaGraph = NfaNodeGraph<>>
    consteval auto connectNfa() {
        static_assert(!std::is_same_v<NfaStack,std::nullptr_t>,"Invalid Regex expression");
        if constexpr (cts.size == 1) {
            if constexpr (symbol.size != 1) {
                using Exec = nfaExecute::NfaExecute<symbol.Last(), NfaGraph, NfaStack>;
                return connectNfa<cts, symbol.template SubStr<0,
                        symbol.size - 1>(), typename Exec::OutStack, typename Exec::OutGraph>();

            } else {
                return NfaData<NfaGraph, NfaStack::LastType::_inIndex, NfaStack::LastType::_outIndex>();
            }
        } else if constexpr (cts.First() == REC_R_ROUND) {

            if constexpr (symbol.Last() == REC_L_ROUND) {
                return connectNfa<cts.template SubStr<1, cts.size - 1>(), symbol.template SubStr<0,
                        symbol.size - 1>(), NfaStack, NfaGraph>();
            } else {
                using Exec = nfaExecute::NfaExecute<symbol.Last(), NfaGraph, NfaStack>;
                return connectNfa<cts, symbol.template SubStr<0,
                        symbol.size - 1>(), typename Exec::OutStack, typename Exec::OutGraph>();
            }
        } else if constexpr (IsExecSymbol(cts.First())) {

            if constexpr (IsSingle(cts.First())) {

                using Exec = nfaExecute::NfaExecute<cts.First(), NfaGraph, NfaStack>;
                return connectNfa<cts.template SubStr<1,
                        cts.size - 1>(), symbol, typename Exec::OutStack, typename Exec::OutGraph>();
            } else if constexpr (symbol.size == 1) {
                return connectNfa<cts.template SubStr<1, cts.size - 1>(), symbol.Append(
                        cts.First()), NfaStack, NfaGraph>();
            } else if constexpr (SymbolSpeed(cts.First()) >= SymbolSpeed(symbol.Last()) || IsLeftBracket(cts.First())) {
                return connectNfa<cts.template SubStr<1, cts.size - 1>(), symbol.Append(
                        cts.First()), NfaStack, NfaGraph>();
            } else {
                using Exec = nfaExecute::NfaExecute<symbol.Last(), NfaGraph, NfaStack>;
                return connectNfa<cts, symbol.template SubStr<0,
                        symbol.size - 1>(), typename Exec::OutStack, typename Exec::OutGraph>();
            }
        } else
            return connectNfa<cts.template SubStr<1,
                    cts.size - 1>(), symbol, typename NfaStack::template AppendIndex<cts.First(), NfaGraph::size>,
                    typename NfaGraph::template AppendNormalNode<cts.First()>>();
    }


    template<ct_stringData cts, typename list = ct_list<>>
    consteval auto getAllState() {
        if constexpr (cts.size == 1)
            return list();
        else if constexpr (!IsSingle(cts.First()) && !IsTwoSideSymbol(cts.First()) &&
                           !list::template contains<cts.First()>)
            return getAllState<cts.template SubStr<1, cts.size - 1>(), typename list::template Append<cts.First()>>();
        else
            return getAllState<cts.template SubStr<1, cts.size - 1>(), list>();
    }


    template<char check, size_t nowIndex, typename graph /*NfaNodeGraph*/>
    using MoveStep = std::conditional_t<check == graph::template FindType<nowIndex>::_check,
            typename graph::template FindType<nowIndex>::ConnectList, ct_list<>>;

    template<char check, typename list, typename graph /*NfaNodeGraph*/, size_t N = list::size-1>
    consteval auto move() {
        constexpr size_t nowIndex = list::template value<N>;
        if constexpr (N == 0) {
            return MoveStep<check, nowIndex, graph>();
        } else if constexpr (graph::template FindType<nowIndex>::_check == check) {
            return merge_list_t<MoveStep<check, nowIndex, graph>,
                    decltype(move<check, list, graph, N - 1>())>();
        } else {
            return move<check, list, graph, N - 1>();
        }
    }

    template<char check, typename list/*ct_list<>*/, typename graph /*NfaNodeGraph*/, size_t ...N>
    consteval auto eClosure_bfs_impl(std::index_sequence<N...>) {
        if constexpr (sizeof...(N) == 0)
            return ct_list < > ();
        else
            return (MoveStep<check, list::template value<N>, graph>() | ... | ct_list
        <>
        ());
    }


    template<char check, typename list/*ct_list<>*/, typename graph /*NfaNodeGraph*/, typename moveState =
    merge_list_t<list, decltype(eClosure_bfs_impl<check, list, graph>(std::make_index_sequence<list::size>()))>>
    auto eClosure() {
        if constexpr (is_value_equal_v<moveState, list>) {
            return list();
        } else
            return eClosure<check, moveState, graph>();

    }


    template<char check, size_t nowIndex, typename graph /*NfaNodeGraph*/>
    consteval auto eClosure_single() {
        return eClosure<check, ct_list<nowIndex>, graph>();
    }

    template<size_t index, typename state/*ct_list<...> nfa state*/, bool isEnd = false, typename toIndex = ct_list<>, typename toCheck = ct_list<>>
    struct DfaNode {
        template<size_t connectTo, char check>
        using ConnectTo = DfaNode<index, state, isEnd, typename toIndex::template AppendNoUnique<connectTo>, typename toCheck::template AppendNoUnique<check>>;

        using Connections = toIndex;

        using ConnectionChecks = toCheck;

        static constexpr size_t _index = index;

        static constexpr bool _isEnd = isEnd;

        using State = state;
    };

    template<typename FindState, bool hasMatched = false, typename CurrentFindNode = ct_empty>
    struct DfaNodeSearcher {
        template<typename NextNode>
        auto operator|(NextNode) {
            if constexpr (!hasMatched && is_value_equal_v<FindState, typename NextNode::State>)
                return DfaNodeSearcher<FindState, true, NextNode>();
            else
                return DfaNodeSearcher();
        }

        using FindNode = CurrentFindNode;
    };

    template<char checkChar, bool end = false, typename stateList = ct_list<>, size_t _currentIndex = 10000000>
    struct DfaNewState {
        static constexpr char check = checkChar;
        static constexpr bool isEnd = end;
        static constexpr size_t currentIndex = _currentIndex;

        using StateList = stateList;

    };


    template<typename... Nodes/*DfaNode<>*/>
    struct DfaGraph {
        static constexpr size_t size = sizeof...(Nodes);

    private:
        template<typename FindState/*ct_list<>*/>
        static auto find() {
            return (DfaNodeSearcher<FindState>() | ... | Nodes());
        }

    public:

        template<typename ...T>
        using Append = DfaGraph<Nodes..., T...>;

        template<size_t index>
        using Node = std::tuple_element_t<index, std::tuple<Nodes...>>;

        template<typename State>
        static constexpr bool contains = (is_value_equal_v<State, typename Nodes::State> || ...);

        template<typename State>
        using Find = typename decltype(find<State>())::FindNode;

        template<typename T/*DfaNode<>*/, size_t connectTo, char connectCheck>
        using ConnectNode = DfaGraph<std::conditional_t<std::is_same_v<T, Nodes>, typename Nodes::template ConnectTo<connectTo, connectCheck>, Nodes>...>;

        template<char checkChar, bool end, typename stateList /*ct_list<>*/, size_t currentIndex>
        auto operator|(DfaNewState<checkChar, end, stateList, currentIndex>) {
            if constexpr (std::is_same_v<stateList,ct_list<>>)
                return DfaGraph();
            else {
                using NewState = stateList;
                using CurrentNode = Node<currentIndex>;
                if constexpr (contains<NewState>)
                    return ConnectNode<CurrentNode, Find<NewState>::_index, checkChar>();
                else
                    return typename Append<DfaNode<size, NewState, end>>::template ConnectNode<CurrentNode, size, checkChar>();
            }
        }
    };


    template<typename Nfa, typename Dfa, size_t currentState, typename list , size_t endState, size_t moveState>
    consteval auto buildDfa_moveState_impl() {
        using CurrentNode = typename Dfa::template Node<currentState>;
        using NewState = decltype(eClosure<REC_EMPTY,
                decltype(move<list::template value<moveState>, typename CurrentNode::State, Nfa>()),
                Nfa>());
        constexpr bool isEnd = NewState::template contains<endState>;
        return DfaNewState<list::template value<moveState>, isEnd, NewState, currentState>();
    }

    template<typename Nfa, typename Dfa, size_t currentState, typename list, size_t endState, size_t...N>
    consteval auto buildDfa_listMoveState_impl(std::index_sequence<N...>) {
        return (Dfa() | ... | buildDfa_moveState_impl<Nfa, Dfa, currentState, list, endState, N>());
    }


    template<typename Nfa, typename Dfa, size_t currentState, typename list, size_t endState>
    consteval auto buildDfa_impl() {
        if constexpr (Dfa::size == currentState)
            return Dfa();
        else {
            return buildDfa_impl<Nfa, decltype(buildDfa_listMoveState_impl<Nfa, Dfa, currentState, list, endState>
                    (std::make_index_sequence<list::size>())), currentState + 1, list, endState>();
        }
    }

    template<typename Dfa, ct_stringData cts,typename MoveList>
    consteval auto CreateExecuteDfa() {
        return CompileTimeDfa<CompileTimeString<cts>, Dfa::size,MoveList::size>(MoveList(),std::make_index_sequence<Dfa::size>(), Dfa());
    }

    template<typename Dfa, ct_stringData cts,typename MoveList>
    consteval auto CreateMinimizeExecuteDfa() {
        constexpr auto dfa = CompileTimeDfa<CompileTimeString<cts>, Dfa::size,MoveList::size>
                (MoveList(),std::make_index_sequence<Dfa::size>(), Dfa());
        constexpr auto minimizeData = decltype(dfa)::template Minimize_impl<MoveList,Dfa::size>(dfa);
        return CompileTimeDfa<CompileTimeString<cts>,minimizeData.minimizeLength,MoveList::size>
                (MoveList(),minimizeData.stateChecks,minimizeData.stateConnects,minimizeData.stateFlags);
    }

    template<ct_stringData regex>
    struct RegexDefine {
        static constexpr auto rawRegex = regex;
        static constexpr auto useRegex = decltype(ctPreprocess<rawRegex>())::value;
        using NfaData = decltype(pkuyo_detail::connectNfa<useRegex>());
        using NfaGraph = typename NfaData::Graph;

        using StateList = decltype(pkuyo_detail::getAllState<useRegex>());
        using InitState = decltype(pkuyo_detail::eClosure_single<REC_EMPTY, NfaData::start, NfaGraph>());
        using DfaGraph = decltype(pkuyo_detail::buildDfa_impl<NfaGraph, DfaGraph<DfaNode<0, InitState>>, 0, StateList, NfaData::end>());
    };
}


//define and create a regex at compile time
template<ct_stringData regex,bool minimize = false>
consteval auto DefineRegex() {
    if constexpr (minimize)
        return pkuyo_detail::CreateMinimizeExecuteDfa<typename pkuyo_detail::RegexDefine<regex>::DfaGraph,
                regex, typename pkuyo_detail::RegexDefine<regex>::StateList>();
    else
        return pkuyo_detail::CreateExecuteDfa<typename pkuyo_detail::RegexDefine<regex>::DfaGraph, regex,
        typename pkuyo_detail::RegexDefine<regex>::StateList>();
}


#endif //COMPILETIMEDFA_CT_REGEX_H
