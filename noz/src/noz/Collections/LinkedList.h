///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_LinkedList_h__
#define __noz_LinkedList_h__

namespace noz {

  template <typename T> class LinkedList;

  template <typename T> class LinkedListNode {
    template<typename U> friend class LinkedList;

    private: LinkedList<T>* list_;
    private: LinkedListNode* next_;
    private: LinkedListNode* prev_;
    private: T* value_;

    public: LinkedListNode(T* value) {
      list_ = nullptr;
      value_ = value;
      next_ = nullptr;
      prev_ = nullptr;
    }

    public: LinkedList<T>* GetList(void) const {return list_;}
    public: LinkedListNode* GetPrev(void) const {return prev_;}
    public: LinkedListNode* GetNext(void) const {return next_;}
    public: T* GetValue(void) const {return value_;}
  };

  template <typename T> class LinkedList {
    private: LinkedListNode<T>* first_;
    private: LinkedListNode<T>* last_;
    private: noz_uint32 count_;

    public: LinkedList(void) {
      first_ = nullptr;
      last_ = nullptr;
      count_ = 0;
    }

    public: bool IsEmpty(void) const {return count_==0;}
    public: noz_uint32 GetCount(void) const {return count_;}
    public: const LinkedListNode<T>* GetFirst(void) const {return first_;}
    public: const LinkedListNode<T>* GetLast(void) const {return last_;}

    public: void AddBefore (LinkedListNode<T>* before, LinkedListNode<T>* n) {
      noz_assert(nullptr==before || before->list_ == this);
      noz_assert(n);
      noz_assert(n->list_ == nullptr);
      
      // Insert at the head?
      if(before==nullptr || before==first_) {
        n->next_ = first_;
        if(first_) {
          first_->prev_ = n;
        } else {
          last_ = n;
        }
        first_ = n;

      // Insert before the given node
      } else {
        n->next_ = before;
        n->prev_ = before->prev_;
        if(before->prev_) before->prev_->next_ = n;
        before->prev_ = n;
      }

      n->list_ = this;
      count_++;
    }

    public: void AddAfter(LinkedListNode<T>* after, LinkedListNode<T>* n) {
      noz_assert(nullptr==after || after->list_ == this);
      noz_assert(n);
      noz_assert(n->list_ == nullptr);
      
      // Insert at the tail?
      if(after==nullptr || after==last_) {
        n->prev_ = last_;
        if(last_) {
          last_->next_ = n;
        } else {
          first_ = n;
        }
        last_ = n;

      // Insert after the given node
      } else {
        n->prev_ = after;
        n->next_ = after->next_;
        if(after->next_) after->next_->prev_ = n;
        after->prev_ = n;
      }

      n->list_ = this;
      count_++;
    }

    public: void AddFirst(LinkedListNode<T>* n) {
      AddBefore(first_,n);
    }

    public: void AddLast(LinkedListNode<T>* n) {
      AddAfter(last_,n);
    }

    public: void Clear(void) {
      while(first_) Remove(first_);
    }

    public: void Remove(LinkedListNode<T>* n) {
      noz_assert(n);
      noz_assert(n->list_ == this);
      noz_assert(count_ > 0);

      if(n->prev_) n->prev_->next_ = n->next_;
      if(n->next_) n->next_->prev_ = n->prev_;

      if(first_ == n) first_ = n->next_;
      if(last_ == n) last_ = n->prev_;

      n->next_ = nullptr;
      n->prev_ = nullptr;
      n->list_ = nullptr;
      count_ --;
    }

    public: void RemoveFirst(void) {
      noz_assert(first_);
      Remove(first_);
    }

    public: void RemoveLast(void) {
      noz_assert(last_);
      Remove(last_);
    }
  };

} // namespace noz


#endif //__noz_LinkedList_h__

