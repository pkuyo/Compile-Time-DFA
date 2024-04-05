//
// Created by dell on 2024/4/5.
//

#ifndef CLION_CT_REGEX_H
#define CLION_CT_REGEX_H
#pragma once

#include <string_view>

#include "ct_utils.h"
#include "ct_list.h"
#include "ct_string.h"


class CompileTimeNode {
    __int64 index = -1;
    __int64 paths[256]{-1};
    bool isEnd = false;


    template<typename T, size_t ...N>
    constexpr CompileTimeNode(T, __int64 index, bool isEnd, std::index_sequence<N...>) : index(index), isEnd(isEnd) {
        (AddNewPath(T::ConnectionChecks::template value<N>, T::Connections::template value<N>), ...);
    }

    constexpr void AddNewPath(char c, __int64 index) {
        paths[c] = index;
    }

    constexpr CompileTimeNode() = default;

public:


    bool IsEnd() const {
        return isEnd;
    }

    __int64 NextPath(char c) const {
        return paths[c];
    }

    template<typename rawStr, size_t length>
    friend
    class CompileTimeDfa;

};

template<typename rawStr, size_t length>
class CompileTimeDfa;


template<ct_stringData cts, size_t length>
class CompileTimeDfa<CompileTimeString<cts>, length> {
    template<typename Graph, size_t N>
    consteval void CreateExecuteDfa_single_impl(CompileTimeDfa *exec) {
        using Node = typename Graph::template Node<N>;
        auto execNode = CompileTimeNode(Node(), Node::_index, Node::_isEnd,
                                        std::make_index_sequence<Node::Connections::size>());
        exec->nodes[Node::_index] = execNode;
    }

    template<typename Graph, size_t ...N>
    consteval CompileTimeDfa(std::index_sequence<N...>, Graph) {
        (CreateExecuteDfa_single_impl<Graph, N>(this), ...);
    }

public:

    static constexpr auto rawString = cts;
    constexpr static size_t size = length;

    CompileTimeNode nodes[size]{};

    bool CheckString(std::string_view view) const {
        __int64 node = 0;
        __int64 index = 0;
        while (index != view.size()) {
            node = nodes[node].NextPath(view[index++]);
            if (node == -1)
                return false;
        }
        return nodes[node].IsEnd();
    }

    template<typename Dfa, ct_stringData c>
    friend consteval auto CreateExecuteDfa();


};

template<typename Dfa, ct_stringData cts>
consteval auto CreateExecuteDfa() {
    return CompileTimeDfa<CompileTimeString<cts>, Dfa::size>(std::make_index_sequence<Dfa::size>(), Dfa());
}

namespace pkuyo_detail {


    consteval int SymbolSpeed(char c) {
        if (c == '|' || c == '&')
            return 2;
        if (c == '*')
            return 3;
        if (c == '(' || c == '-')
            return 1;
        return 0;
    }

    consteval bool IsSingle(char c) { return SymbolSpeed(c) == 3; }

    consteval bool IsSymbol(char c) { return SymbolSpeed(c) != 0; }

    consteval bool IsTwoSideSymbol(char c) { return (SymbolSpeed(c) != 0 && !IsSingle(c)) || c == ')'; }

    //nfa node graph
    template<char check, size_t index, int toIndex1 = -1, int toIndex2 = -1>
    struct NfaNode {
        template<int toIndex>
        static auto _extend() {
            if constexpr (toIndex1 == -1)
                return NfaNode<check, index, toIndex, -1>();
            else if constexpr (toIndex2 == -1)
                return NfaNode<check, index, toIndex1, toIndex>();
        }

        template<int toIndex>
        using ConnectTo = decltype(_extend<toIndex>());

        static constexpr size_t _index = index;

        static constexpr char _check = check;

        static constexpr size_t _toIndex1 = toIndex1;
        static constexpr size_t _toIndex2 = toIndex2;


    };

    template<size_t index,bool hasMatched = false,typename CurrentFindNode = ct_empty>
    struct NfaNodeSearcher {
        template<typename NextNode>
        auto operator|(NextNode)
        {
            if constexpr (!hasMatched && index == NextNode::_index)
                return NfaNodeSearcher<index,true,NextNode>();
            else
                return NfaNodeSearcher();
        }

        using FindNode = CurrentFindNode;
    };

    template<typename... NodeTypes>
    struct NfaNodeGraph {

        template<size_t index>
        static auto _find() {
            return (NfaNodeSearcher<index>() | ... | NodeTypes());
        }

        constexpr explicit NfaNodeGraph(std::nullptr_t) {}

        static constexpr size_t size = sizeof...(NodeTypes);


        template<typename ...T>
        using AppendNode = NfaNodeGraph<NodeTypes..., T...>;

        template<char check>
        using AppendNormalNode = NfaNodeGraph<NodeTypes..., NfaNode<check, size, size + 1>, NfaNode<'\0', size + 1>>;


        template<size_t index>
        using FindType = decltype(_find<index>())::FindNode;

        template<size_t index, typename NewType>
        using ReplaceTypeGraph = NfaNodeGraph<std::conditional_t<NodeTypes::_index == index, NewType, NodeTypes>...>;

        template<size_t index, size_t connectNew>
        using ReplaceIndexGraph = NfaNodeGraph<std::conditional_t<NodeTypes::_index == index,
                typename FindType<index>::template ConnectTo<connectNew>, NodeTypes>...>;

    };


    //stack in building nfa
    template<size_t inIndex, size_t outIndex = inIndex + 1>
    struct NfaIndex {
        static constexpr int _inIndex = inIndex;
        static constexpr int _outIndex = outIndex;
    };
    template<typename... IndexTypes>
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

    template<typename... IndexTypes>
    struct NfaIndexStack {
        static constexpr size_t size = sizeof...(IndexTypes);

        template<size_t start, size_t... N>
        requires (start + sizeof...(N) <= sizeof...(IndexTypes))
        static constexpr NfaIndexStack<std::tuple_element_t<start + N, std::tuple<IndexTypes...>>...>
        _Sub(std::index_sequence<N...>);

        template<typename ...T>
        using AppendType = NfaIndexStack<IndexTypes..., T...>;

        template<size_t ...I>
        using AppendIndex = NfaIndexStack<IndexTypes..., NfaIndex<I>...>;

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
    namespace nfaExecute
    {
        template<char c, typename Graph, typename Stack,typename = void/*check if items in stack is enough*/>
        struct NfaExecute;

        template<typename Graph, typename Stack>
        struct NfaExecute<'|', Graph, Stack, std::enable_if_t<Stack::size >=2> > {
            using Index2 = typename Stack::LastType;
            using Index1 = typename Stack::LastLastType;

            using EmitNode1 = NfaNode<'\0', Graph::size, Index1::_inIndex, Index2::_inIndex>;
            using EmitNode2 = NfaNode<'\0', Graph::size + 1>;

            using OutStack = typename Stack::template RemoveType<2>::template AppendType<NfaIndex<Graph::size>>;

            using OutGraph = typename Graph::template ReplaceIndexGraph<Index1::_outIndex, Graph::size + 1>::
            template ReplaceIndexGraph<Index2::_outIndex, Graph::size + 1>::
            template AppendNode<EmitNode1, EmitNode2>;

        };

        template<typename Graph, typename Stack>
        struct NfaExecute<'&', Graph, Stack,std::enable_if_t<Stack::size >=2>> {
            using Index2 = typename Stack::LastType;
            using Index1 = typename Stack::LastLastType;


            using OutStack = typename Stack::template RemoveType<2>::template AppendType<NfaIndex<Index1::_inIndex, Index2::_outIndex>>;

            using OutGraph = typename Graph::template ReplaceIndexGraph<Index1::_outIndex, Index2::_inIndex>;

        };

        template<typename Graph, typename Stack>
        struct NfaExecute<'*', Graph, Stack,std::enable_if_t<Stack::size >=1>> {
            using Index = typename Stack::LastType;

            using EmitNode1 = NfaNode<'\0', Graph::size, Index::_inIndex, Graph::size + 1>;
            using EmitNode2 = NfaNode<'\0', Graph::size + 1>;

            using OutStack = typename Stack::template RemoveType<1>::template AppendType<NfaIndex<Graph::size>>;

            using OutGraph = typename Graph::template ReplaceIndexGraph<Index::_outIndex, Index::_inIndex>::
            template ReplaceIndexGraph<Index::_outIndex, Graph::size + 1>::
            template AppendNode<EmitNode1, EmitNode2>;

        };
    }

    //create nfa from string
    template<ct_stringData cts, ct_stringData symbol = "", typename NfaStack = NfaIndexStack<>, typename NfaGraph = NfaNodeGraph<>>
    consteval auto connectNfa() {
        if constexpr (cts.size == 1) {
            if constexpr (symbol.size != 1) {
                using Exec = nfaExecute::NfaExecute<symbol.Last(), NfaGraph, NfaStack>;
                return connectNfa<cts, symbol.template SubStr<0,
                        symbol.size - 1>(), typename Exec::OutStack, typename Exec::OutGraph>();

            } else {
                return NfaData<NfaGraph, NfaStack::LastType::_inIndex, NfaStack::LastType::_outIndex>();
            }
        }

        else if constexpr (cts.First() == ')') {

            if constexpr (symbol.Last() == '(') {
                if constexpr (NfaStack::size == 1)
                    return connectNfa<cts.template SubStr<1, cts.size - 1>(), symbol.template SubStr<0,
                            symbol.size - 1>(), NfaStack, NfaGraph>();
                else
                    return connectNfa<cts.template SubStr<1, cts.size - 1>(), symbol.template SubStr<0,
                            symbol.size - 1>().Append('&'), NfaStack, NfaGraph>();
            }
            else {
                using Exec = nfaExecute::NfaExecute<symbol.Last(), NfaGraph, NfaStack>;
                return connectNfa<cts, symbol.template SubStr<0,
                        symbol.size - 1>(), typename Exec::OutStack, typename Exec::OutGraph>();
            }
        } else if constexpr (IsSymbol(cts.First())) {

            if constexpr (IsSingle(cts.First())) {
                using Exec = nfaExecute::NfaExecute<cts.First(), NfaGraph, NfaStack>;
                return connectNfa<cts.template SubStr<1,
                        cts.size - 1>(), symbol, typename Exec::OutStack, typename Exec::OutGraph>();
            } else if constexpr (symbol.size == 1) {
                return connectNfa<cts.template SubStr<1, cts.size - 1>(), symbol.Append(
                        cts.First()), NfaStack, NfaGraph>();
            }
            else if constexpr (SymbolSpeed(cts.First()) >= SymbolSpeed(symbol.Last())) {
                return connectNfa<cts.template SubStr<1, cts.size - 1>(), symbol.Append(
                        cts.First()), NfaStack, NfaGraph>();
            }

            else {
                using Exec = nfaExecute::NfaExecute<symbol.Last(), NfaGraph, NfaStack>;
                return connectNfa<cts, symbol.template SubStr<0,
                        symbol.size - 1>(), typename Exec::OutStack, typename Exec::OutGraph>();
            }
        } else
            return connectNfa<cts.template SubStr<1,
                    cts.size - 1>(), symbol, typename NfaStack::template AppendIndex<NfaGraph::size>,
                    typename NfaGraph::template AppendNormalNode<cts.First()>>();
    }

    //preprocess input regex
    template<ct_stringData cts, ct_stringData out = "">
    consteval auto ctOpera() {
        if constexpr (cts.size == 1)
            return CompileTimeString<out>();
        else {

            if constexpr (out.size == 1 || IsTwoSideSymbol(cts.First()) || IsSingle(cts.First())) {
                return ctOpera<cts.template SubStr<1, cts.size - 1>(), out.Append(cts.First())>();
            } else if constexpr (!IsTwoSideSymbol(out.Last()) || out.Last() == ')') {
                return ctOpera<cts.template SubStr<1, cts.size - 1>(), out.Append('&').Append(cts.First())>();
            }else if constexpr (out.Last() == '-') {
                if constexpr (out.Last(1) == cts.First())
                    return ctOpera<cts.template SubStr<1, cts.size - 1>(), out.template SubStr<0, out.size - 1>()>();
                else
                    return ctOpera<cts, out.template SubStr<0, out.size - 1>().Append('|').Append(
                            out.Last(1) + 1).Append('-')>();
            } else
                return ctOpera<cts.template SubStr<1, cts.size - 1>(), out.Append(cts.First())>();
        }
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

    template<char check, size_t nowIndex, size_t dir, typename graph, typename toList = ct_list<>>
    consteval auto eClosure_dfs_impl() {
        using NowNode = typename graph::template FindType<nowIndex>;

        if constexpr (NowNode::_check == check) {
            if constexpr (dir == 1 && NowNode::_toIndex1 != -1 && !toList::template contains<NowNode::_toIndex1>) {
                using Left = decltype(eClosure_dfs_impl<check, NowNode::_toIndex1, 1, graph, typename toList::template Append<NowNode::_toIndex1>>());
                return eClosure_dfs_impl<check, NowNode::_toIndex1, 2, graph, Left>();
            } else if constexpr (NowNode::_toIndex2 != -1 && !toList::template contains<NowNode::_toIndex2>) {
                using Left = decltype(eClosure_dfs_impl<check, NowNode::_toIndex2, 1, graph, typename toList::template Append<NowNode::_toIndex2>>());
                return eClosure_dfs_impl<check, NowNode::_toIndex2, 2, graph, Left>();
            } else
                return toList();
        } else
            return toList();
    }

    template<char check, size_t nowIndex, typename graph>
    consteval auto eClosure_single() {
        using Left = decltype(eClosure_dfs_impl<check, nowIndex, 1, graph, ct_list<>::Append<nowIndex>>());
        return eClosure_dfs_impl<check, nowIndex, 2, graph, Left>();
    }


    template<char check, size_t nowIndex, typename graph>
    consteval auto moveState_impl() {
        using NowNode = typename graph::template FindType<nowIndex>;
        if constexpr (NowNode::_check == check) {
            if constexpr (NowNode::_toIndex1 != -1 && NowNode::_toIndex2 != -1)
                return ct_list < NowNode::_toIndex1, NowNode::_toIndex2 > ();
            else if constexpr (NowNode::_toIndex1 != -1)
                return ct_list < NowNode::_toIndex1 > ();
            else if constexpr (NowNode::_toIndex1 != -1)
                return ct_list < NowNode::_toIndex2 > ();
            else
                return ct_list < > ();
        } else
            return ct_list < > ();
    }


    template<char check, typename list, typename graph, size_t N = list::size>
    consteval auto move() {
        constexpr size_t nowIndex = list::template value<N>;
        if constexpr (N == 0) {
            return moveState_impl<check, nowIndex, graph>();
        }

        else if constexpr (graph::template FindType<nowIndex>::_check == check) {
            return merge_list_t<decltype(moveState_impl<check, nowIndex, graph>()),
                    decltype(move<check, list, graph, N - 1>())>();
        } else {
            return move<check, list, graph, N - 1>();
        }
    }

    template<char check, typename list, typename graph, size_t ...N>
    consteval auto eClosure_bfs_impl(std::index_sequence<N...>) {
        if constexpr (sizeof...(N) == 0)
            return ct_list < > ();
        else
            return (moveState_impl<check, list::template value<N>, graph>() | ... | ct_list<>());
    }


    template<char check, typename list, typename graph, typename moveState =
    merge_list_t<list, decltype(eClosure_bfs_impl<check, list, graph>(std::make_index_sequence<list::size>()))>>
    auto eClosure() {
        if constexpr (is_value_equal_v<moveState, list>) {
            return list();
        } else
            return eClosure<check, moveState, graph>();

    }



    template<size_t index, typename state, bool isEnd = false, typename toIndex = ct_list<>, typename toCheck = ct_list<>>
    struct DfaNode {
        template<size_t connectTo, char check>
        using ConnectTo = DfaNode<index, state, isEnd, typename toIndex::template AppendNoUnique<connectTo>, typename toCheck::template AppendNoUnique<check>>;

        using Connections = toIndex;

        using ConnectionChecks = toCheck;

        static constexpr size_t _index = index;

        static constexpr bool _isEnd = isEnd;

        using State = state;
    };
    template<typename FindState,bool hasMatched = false,typename CurrentFindNode = ct_empty>
    struct DfaNodeSearcher {
        template<typename NextNode>
        auto operator|(NextNode)
        {
            if constexpr (!hasMatched && is_value_equal_v<FindState,typename NextNode::State>)
                return DfaNodeSearcher<FindState,true,NextNode>();
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


    template<typename... Nodes>
        requires (sizeof...(Nodes) < 1000)
    struct DfaGraph {
        static constexpr size_t size = sizeof...(Nodes);

    private:
        template<typename FindState>
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

        template<typename T, size_t connectTo, char connectCheck>
        using ConnectNode = DfaGraph<std::conditional_t<std::is_same_v<T, Nodes>, typename Nodes::template ConnectTo<connectTo, connectCheck>, Nodes>...>;

        template<char checkChar, bool end, typename stateList, size_t currentIndex>
        auto operator|(DfaNewState<checkChar, end, stateList, currentIndex>) {
            using NewState = stateList;
            using CurrentNode = Node<currentIndex>;

            if constexpr (contains<NewState>)
                return ConnectNode<CurrentNode, Find<NewState>::_index, checkChar>();

            else
                return typename Append<DfaNode<size, NewState, end>>::template ConnectNode<CurrentNode, size, checkChar>();


        }
    };


    template<typename Nfa, typename Dfa, size_t currentState, typename list, size_t endState, size_t moveState>
    consteval auto buildDfa_moveState_impl() {
        using CurrentNode = typename Dfa::template Node<currentState>;
        using NewState = decltype(eClosure<'\0',
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

    template<ct_stringData regex>
    struct RegexDefine {
        using useRegex = decltype(pkuyo_detail::ctOpera<regex>());

        using NfaData = decltype(pkuyo_detail::connectNfa<useRegex::value>());
        using NfaGraph = typename NfaData::Graph;

        using StateList = decltype(pkuyo_detail::getAllState<useRegex::value>());
        using InitState = decltype(pkuyo_detail::eClosure_single<'\0', NfaData::start, NfaGraph>());
        using DfaGraph = decltype(pkuyo_detail::buildDfa_impl<NfaGraph, DfaGraph<DfaNode<0, InitState>>, 0, StateList, NfaData::end>());
    };
}


template<ct_stringData regex>
consteval auto DefineRegex() {
    return CreateExecuteDfa<typename pkuyo_detail::RegexDefine<regex>::DfaGraph, regex>();
}

#endif //CLION_CT_REGEX_H
