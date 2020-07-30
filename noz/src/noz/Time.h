///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Time_h__
#define __noz_Time_h__

namespace noz {

  class Time {
    /// Delta time in seconds since last frame.  If called within FixedUpdate will
    /// return the fixed time interval
    protected: static noz_float delta_time_;

    /// Time at the beginning of the frame
    protected: static noz_float time_;

    /// Time at the beginning of the fixed frame
    protected: static noz_float fixed_time_;

    /// Time for each fixed delta frame.
    protected: static noz_float fixed_delta_time_;

    /// True if within a fixed frame
    protected: static bool fixed_frame_;

    /// Average time of one frame
    protected: static noz_float average_delta_time_;

    /// Returns the amount of elapsed time between
    public: static noz_float GetDeltaTime(void) { return fixed_frame_ ? fixed_delta_time_ : delta_time_; }

    /// Returns the time at the beginning of the frame in seconds since the start of the 
    /// application. If called within a FixedUpdate will return GetFixedTime
    public: static noz_float GetTime(void) {return fixed_frame_ ? fixed_time_ : time_;} 

    public: static noz_float GetFixedTime(void) {return fixed_time_; }

    /// Return the fixed amount of time to give to a fixed frame
    public: static noz_float GetFixedDeltaTime(void) {return fixed_delta_time_;}

    public: static noz_float GetAverageDeltaTime(void) {return average_delta_time_;}

    /// Start a normal frame
    public: static void BeginFrame(void);

    /// Start a fixed frame
    public: static void BeginFixedFrame(void);

    /// End a fixed frame
    public: static void EndFixedFrame(void);

    /// Return the real time since the application was started.
    public: static noz_float GetApplicationTime(void);

    
  };

} // namespace noz


#endif //__noz_Rect_h__


