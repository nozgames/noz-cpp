///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_PerformanceCounter_h__
#define __noz_PerformanceCounter_h__

namespace noz {

  class PerformanceCounter {
    private: Name name_;

    private: noz_uint64 value_;

    private: static std::map<Name,PerformanceCounter*>* counters_;

    public: PerformanceCounter (const Name& name);

    public: ~PerformanceCounter(void);

    /// Clear the value of the performance counter
    public: void Clear (void);

    /// Increment the value of the perforamnce counter
    public: void Increment (void);

    /// Increment the value of the performance counter by the given amount
    public: void IncrementBy (noz_uint64 inc);

    /// Decrement the value of the performance counter
    public: void Decrement (void);

    /// Decrement the value of the performance counter by the given amount
    public: void DecrementBy (noz_uint64 dec);

    /// Print the contents of the counter
    public: void Print (void);

    /// Static method to return the performance counter with the given name
    public: static PerformanceCounter* GetPerformanceCounter (const Name& name);

    /// Clear all performance counters
    public: static void ClearAll (void);

    /// Print all performance counters
    public: static void PrintAll (void);
  };

} // namespace noz


#endif //__noz_PerformanceCounter_h__

