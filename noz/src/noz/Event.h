///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Event_h__
#define __noz_Event_h__

namespace noz {

  class EventHandler;

  ///
  /// Extension class which enables a class to observer events.  A class derived
  /// from EventObserver will also automatically unregister any event handlers
  /// associated with it from all events.
  ///
  class EventObserver {
    friend class EventHandler;

    /// Default constructor
    public: EventObserver(void) {
      event_handlers_ = nullptr;
    }

    /// Destructor to handle 
    public: virtual ~EventObserver(void);

    /// Linked list of all handlers which contain delegates which target the observer.
    protected: EventHandler* event_handlers_;
  };

  ///
  /// Base event handler class.
  ///
  class EventHandler {
    /// Next handler in observer handler list.
    private: EventHandler* observer_next_;

    /// Default constructor
    public: EventHandler (void) {
      observer_next_ = nullptr;
    }

    /// Virtual destructor since the EventObserver class can delete handlers 
    /// and it only has pointers to the base handler type.
    public: virtual ~EventHandler(void) {
    }

    /// Bind the handler to the given observer..
    public: void Bind (EventObserver* target) {
      observer_next_ = target->event_handlers_;
      target->event_handlers_ = this;
    }

    /// Unbind the handler from the given observer.
    public: void UnBind (EventObserver* target) {
      if(target->event_handlers_==this) {
        target->event_handlers_ = target->event_handlers_->observer_next_;
        return;
      }

      if(target->event_handlers_) {
        EventHandler* prev = target->event_handlers_;
        for(EventHandler* h=prev->observer_next_; h; prev=h, h=h->observer_next_) {
          if(h==this) {
            prev->observer_next_ = h->observer_next_;
            return;
          }
        }
      }
    }
  };

  /// 
  /// Base event delegate class used to store the target and member function information.  The event
  /// implementation will extend the delegate to add the methods which will populate the data field.
  ///
  class EventDelegate {
    /// 20-Bytes of raw data to store the largest possible member function pointer.  The data is 
    /// stored in a struct with three integral types to allow quick comparison of the delegate.
    protected: struct Data {
      noz_uint64 data0_;
      noz_uint64 data1_;
      noz_uint32 data2_;
    };

    /// Pointer to target that the delegate method should be called on.
    protected: EventObserver* target_;

    /// Raw data which will store the member function pointer.  
    protected: Data data_;  

    /// Default constructor
    public: EventDelegate (EventObserver* target) {
      // Save the target pointer
      target_ = target;

      // It is very important that data is initialized to zero here for comparison reasons.  Most 
      // methods will not use all of the data bytes so we need to ensure they are initialized.
      data_.data0_ = 0;
      data_.data1_ = 0;
      data_.data2_ = 0;
    }

    /// Return the target pointer
    public: EventObserver* GetTarget(void) const {return target_;}

    /// Compare the target and raw data for equality
    public: bool operator== (const EventDelegate& d) const {
      return target_ == d.target_ && data_.data0_ == d.data_.data0_ && data_.data1_ == d.data_.data1_ && data_.data2_ == d.data_.data2_;
    }

    /// Compare the target and raw data for inequality
    public: bool operator!= (const EventDelegate& d) const {
      return target_ != d.target_ || data_.data0_ != d.data_.data0_ || data_.data1_ != d.data_.data1_ || data_.data2_ != d.data_.data2_;
    }
  };

  
  template <typename... ARGS> class Event {
    public: class Delegate : public EventDelegate {
      public: void (*stub_)(EventObserver* t, Data*, ARGS...);

      public: Delegate (void) : EventDelegate(nullptr) {
      }

      public: template <typename T> Delegate (T* t, void (T::*m)(ARGS...)) : EventDelegate(t) {
        struct Local {
          void (T::*method_)(ARGS...);
          static void Stub (EventObserver* t, Data* data, ARGS... args) {
            Local* local = (Local*)data;
            (static_cast<T*>(t)->*(local->method_))(args...);
          }
        };
        Local* local = (Local*)&data_;
        local->method_ = m;
        stub_ = &Local::Stub;
        target_ = t;
      }

      public: void operator() (ARGS... args) {
        stub_(target_,&data_,args...);
      }
    };

    private: class Handler : public EventHandler {
      friend class Event;

      private: Handler* event_next_;
      private: Event* event_;
      private: Delegate delegate_;

      public: Handler(const Delegate& d, Event* e) : delegate_(d), event_(e) {
        event_ = e;
        event_next_ = e->handlers_;
        event_->handlers_ = this;
        Bind(delegate_.GetTarget());
      }

      public: ~Handler(void) {
        UnBind(delegate_.GetTarget());
      
        if(event_->handlers_==nullptr) return;

        if(event_->handlers_ == this) {
          event_->handlers_ = event_->handlers_->event_next_;
        } else {
          Handler* p = event_->handlers_;
          for(Handler* h=p->event_next_; h; p=h, h=h->event_next_) {
            if(h==this) {
              p->event_next_ = h->event_next_;
              break;
            }
          }
        }
      }
    };

    private: Handler* handlers_;

    public: Event(void) {
      handlers_ = nullptr;
    }

    public: ~Event(void) {
      while(handlers_) delete handlers_;
    }

    public: void operator += (const Delegate& d) {
      new Handler(d,this);
    }

    public: void operator -= (const Delegate& d) {
      // Delete all handlers that match.
      for(Handler* h=handlers_; h; ) {
        if(h->delegate_ == d) {
          Handler* hd = h;
          h = hd->event_next_;
          delete hd;
        } else {
          h = h->event_next_;
        }
      }
    }

    public: void operator () (ARGS... args) {
      Handler* n = nullptr;
      for(Handler* h=handlers_; h; h=n) {
        n = h->event_next_;
        h->delegate_(args...);
      }
    }
  };

  inline EventObserver::~EventObserver(void) {
    while(event_handlers_) delete event_handlers_;
  }

  typedef Event<> GenericEvent;
}

#endif // __noz_Event_h__
