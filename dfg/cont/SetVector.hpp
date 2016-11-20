#pragma once

/*
MapSet.hpp

Implements contiguous storage set with the following properties:
-Guarantees contiguous storage.
-Sorting is controlled by a per-object runtime flag and thus searching is either linear or logarithmic.
-Invalidation of iterators is dependent on runtime properties, especially capacity and sorting.
    -Insert to set with size() == capacity() will invalidate all iterators.
    -Insert to unsorted set with size() < capacity() will invalidate only end() iterator.
    -Insert to sorted set with size() < capacity() invalidates iterators only for those items greater in order than new value.
    -Single deletion from unsorted set invalidates only iterators of deleted item, last item and end(). TODO: revise this.
    -Deletion from sorted set invalidates only those items whose key is after deleted key. // TODO: verify behaviour of vector erase.
-Provides reserve() for preallocation.
-Allows values to be modified (programmer responsibility to use it correctly).
-Values can be searched without contructing value_type (for example std::string items can be searched with const char* without constructing std::string)

Related reading and implementations:
-boost::container::flat_set
*/

#include "../dfgDefs.hpp"
#include "../alg/eraseByTailSwap.hpp"
#include "../alg/find.hpp"
#include "../alg/sortMultiple.hpp"
#include <algorithm>
#include <utility>
#include <vector>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(cont) {

    template <class Key_T>
    class DFG_CLASS_NAME(SetVector)
    {
    public:
        typedef std::vector<Key_T>                  ContainerT;
        typedef typename ContainerT::iterator       iterator;
        typedef typename ContainerT::const_iterator const_iterator;
        typedef Key_T                               key_type;
        typedef Key_T                               value_type;
        typedef size_t                              size_type;

        DFG_CLASS_NAME(SetVector)() :
            m_bSorted(true)
        {}

        bool                empty() const       { return m_storage.empty(); }
        size_t              size() const        { return m_storage.size(); }
        void                clear()             { m_storage.clear(); }

        iterator            begin()             { return m_storage.begin(); }
        const_iterator      begin() const       { return m_storage.begin(); }
        const_iterator      cbegin() const      { return m_storage.cbegin(); }
        iterator            end()               { return m_storage.end(); }
        const_iterator      end() const         { return m_storage.end(); }
        const_iterator      cend() const        { return m_storage.cend(); }

        iterator            backIter()          { return end() - 1; }
        const_iterator      backIter() const    { return end() - 1; }

        value_type&         front()             { return *begin(); }
        const value_type&   front() const       { return *begin(); }
        value_type&         back()              { return *backIter(); }
        const value_type&   back() const        { return *backIter(); }

        iterator    erase(iterator iterDeleteFrom) { return (iterDeleteFrom != end()) ? erase(iterDeleteFrom, iterDeleteFrom + 1) : end(); }
        template <class T>
        size_type   erase(const T& key) { auto rv = erase(find(key)); return (rv != end()) ? 1 : 0; } // Returns the number of elements removed.
        iterator    erase(iterator iterRangeFirst, const iterator iterRangeEnd)
        {
            if (m_bSorted)
                return m_storage.erase(iterRangeFirst, iterRangeEnd);
            else // With unsorted, swap to-be-removed items to end and erase items from end -> O(1) for single element removal.
            {
                return DFG_MODULE_NS(alg)::eraseByTailSwap(*this, iterRangeFirst, iterRangeEnd,
                                                            [&](const iterator& i0, const iterator& i1) { std::iter_swap(i0, i1); },
                                                            [&](const iterator& i0, const iterator& i1) { m_storage.erase(i0, i1); });
            }
        }

        template <class T0, class T1>
        static bool isKeyMatch(const T0& a, const T1& b)
        {
            return DFG_MODULE_NS(alg)::isKeyMatch(a, b);
        }

        template <class This_T, class T>
        static auto findInsertPos(This_T& rThis, const T& key) -> decltype(rThis.begin())
        {
            const auto iterBegin = rThis.begin();
            const auto iterEnd = rThis.end();
            auto iter = DFG_MODULE_NS(alg)::findInsertPosLinearOrBinary(rThis.isSorted(), iterBegin, iterEnd, key);
            return iter;
        }

        template <class This_T, class T>
        static auto findImpl(This_T& rThis, const T& key) -> decltype(rThis.begin())
        {
            if (rThis.isSorted())
            {
                auto iter = findInsertPos(rThis, key);
                return (iter != rThis.end() && isKeyMatch(*iter, key)) ? iter : rThis.end();
            }
            else
                return DFG_MODULE_NS(alg)::findLinear(rThis.begin(), rThis.end(), key);
        }

        template <class T> iterator         find(const T& key)          { return findImpl(*this, key); }
        template <class T> const_iterator   find(const T& key) const    { return findImpl(*this, key); }

        template <class T> bool hasKey(const T& key) const              { return find(key) != end(); }

        iterator insertNonExistingTo(key_type&& value)
        {
            return insertNonExistingTo(value, findInsertPos(*this, value));
        }

        iterator insertNonExistingTo(key_type&& value, iterator insertPos)
        { 
            return m_storage.insert(insertPos, value);
        }

        template <class T>
        std::pair<iterator, bool> insert(const T& newVal)
        {
            auto iter = findInsertPos(*this, newVal);
            if (iter != end() && isKeyMatch(*iter, newVal))
                return std::pair<iterator, bool>(iter, false);
            else
            {
                iter = insertNonExistingTo(key_type(newVal), iter);
                return std::pair<iterator, bool>(iter, true);
            }
        }

        template <class T>
        std::pair<iterator, bool> insert(key_type&& newVal)
        {
            auto iter = findInsertPos(*this, newVal);
            if (iter != end() && isKeyMatch(*iter, newVal))
                return std::pair<iterator, bool>(iter, false);
            else
            {
                iter = insertNonExistingTo(std::move(newVal), iter);
                return std::pair<iterator, bool>(iter, true);
            }
        }

        void reserve(const size_t nReserve) { m_storage.reserve(nReserve); }

        size_t capacity() const             { return m_storage.capacity(); }

        void sort()
        {
            std::sort(m_storage.begin(), m_storage.end(), [](const value_type& left, const value_type& right)
            {
                return left < right;
            });
        }

        void setSorting(const bool bSort)
        {
            if (bSort == m_bSorted)
                return;
            if (bSort)
                sort();
            m_bSorted = bSort;
        }

        bool isSorted() const { return m_bSorted; }

        bool m_bSorted;
        ContainerT m_storage;
    }; // class SetVector

} } // Module namespace
