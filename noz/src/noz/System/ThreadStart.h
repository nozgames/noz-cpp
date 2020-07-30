///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ThreadStart_h__
#define __noz_ThreadStart_h__

namespace noz {

  class ThreadStart : public Object {
    private: class Delegate {
      public: virtual ~Delegate(void) {}
      public: virtual void operator() (void) = 0;
      public: virtual Delegate* Clone(void) = 0;
    };

    private: template <typename T> class DelegateT : public Delegate {
      public: T* target_;
      public: void (T::*method_)(void);
      public: DelegateT(T* target, void (T::*method)(void)) {
        target_ = target;
        method_ = method;
      }
      public: virtual void operator() (void) override {
        (target_->*method_)();
      }
      public: virtual Delegate* Clone(void) override {
        return new DelegateT(target_,method_);
      }
    };

    private: Delegate* delegate_;

    public: ThreadStart(void) {
      delegate_ = nullptr;
    }

    public: ~ThreadStart(void) {
      delete delegate_;
    }

    public: ThreadStart(const ThreadStart& copy) {
      delegate_ = nullptr;
      *this = copy;
    }      

    public: ThreadStart& operator= (const ThreadStart& copy) {
      delete delegate_;
      if(copy.delegate_) {
        delegate_ = copy.delegate_->Clone();
      } 
      return *this;      
    }

    public: template <typename T> ThreadStart (T* target, void (T::*member)(void)) {
      delegate_ = new DelegateT<T> (target,member);
    }

    public: virtual void operator() (void) {
      if(nullptr != delegate_) {
        (*delegate_)();
      }
    }
  };

} // namespace noz


#endif // __noz_ThreadStart_h__
