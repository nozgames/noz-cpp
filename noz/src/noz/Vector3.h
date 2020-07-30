///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Vector3_h__
#define __noz_Vector3_h__

namespace noz {

  class Vector2;

  class Vector3 {    
    public: static Vector3 Zero;

 	  public: noz_float x;
	  public: noz_float y;
	  public: noz_float z;


	  public: Vector3 (void) {
	    x = 0.0f; y = 0.0f; z = 0.0f;
	  }
	  
	  public: Vector3 (float _x, float _y, float _z) {
	    x = _x; y = _y; z = _z;
	  }

    public: Vector3(const Vector2& v);
	  
	  public: const float& operator[] (int i) const {return (&x)[i];}
	  public: float& operator[] (int i) {return (&x)[i];}

  
	  /**
	   * Add another vector
	   */
	  Vector3&	operator+= ( const Vector3& v ) {
	    x += v.x; y += v.y; z += v.z;
	    return *this;
	  }
	  
	  /**
	   * Subtract another vector
	   */
	  Vector3&	operator-= ( const Vector3& v ) {
	    x -= v.x; y -= v.y; z -= v.z;
	    return *this;
	  }
	  
	  /**
	   * Multiply the vector by a scalar
	   */
	  Vector3&	operator*= ( float v ) {
	    x *= v; y *= v; z *= v;
	    return *this;
	  }
	  
	  /**
	   * Divide the vector by a scalar
	   */
	  Vector3& operator/= ( float v ) {
	    v = 1.0f / v;
	    x *= v; y *= v; z *= v;
	    return *this;
	  }
	  
	  /**
	   * Substract one vector from another and return the result as a third
	   */
	  Vector3 operator- ( const Vector3& v ) const {
	    return Vector3 ( x - v.x, y - v.y, z - v.z );
	  }
	  
	  /**
	   * Add one vector to another and return the result as a third vector
	   */
	  Vector3 operator+ ( const Vector3& v ) const {
	    return Vector3 ( x + v.x, y + v.y, z + v.z );
	  }

    /**
     * Perform dot product of two vectors and return the scalar result
     */
	  float operator*	( const Vector3& v ) const {
	    return x * v.x + y * v.y + z * v.z;
	  }
	  
	  /**
	   * Multiply the vector by a scalar and return a new vector
	   */
	  Vector3 operator* ( float v ) const {
	    return Vector3 ( x * v, y * v, z * v );
	  }
	  
	  /** 
	   * Divide the vector by a scalar and return a new vector
	   */
	  Vector3 operator/ ( float v ) const {
	    v = 1.0f / v;
	    return Vector3 ( x * v, y * v, z * v );
	  }
	  
	  /** 
	   * Return an inverse vector
	   */
	  Vector3 operator- ( void ) const { 
	    return Vector3 (-x,-y,-z); 
	  }

    /** 
     * Compare the two vectors for equality
     */
    bool operator== ( const Vector3& v ) const {
      return x == v.x && y == v.y && z == v.z;
    }
    
    /**
     * Compare the two vectors for inequality
     */
	  bool operator!=	( const Vector3& v ) const {
	    return !(x == v.x && y == v.y && z == v.z);
	  }

    /** 
     * Set the value of the vector
     */
    void set ( float _x, float _y, float _z ) {
      x = _x; y = _y; z = _z;
    }

    /**
     * normalize the vector
     */
	  float normalize	( void );

    /**
     * initialize the vector to zero
     */
	  void zero	( void ) {
	    x = 0.0f; y = 0.0f; z = 0.0f;
	  }
	  
	  /**
	   * Return the length of the vector
	   */
    float length ( void ) const {
      return Math::Sqrt( x*x + y*y + z*z);
    }
    
    /**
     * Return the squared length of the vector
     */
    float lengthSquared ( void ) const {
      return x*x + y*y + z*z;
    }
    

    public: float min (void) const {return Math::Min(Math::Min (x,y),z); }
    public: float max (void) const {return Math::Max(Math::Max (x,y),z); }
            
    /**
     * Return a normalized version of the vector
     */
    Vector3 normalized ( void ) const;
    
    /**
     * Return the squared value of the vector
     */
    Vector3 squared ( void ) const {
      return Vector3 ( x * x, y * y, z * z );
    }
    
    /**
     * Square the vector
     */
    void square ( void ) {
      x *= x; y *= y; z *= z;
    }
    
    /**
     * Cross product of two vectors
     */
    Vector3 cross ( const Vector3& v ) {
  	  return Vector3 ( y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x );
  	}
          
    /**
     * Return true if the vector value is zero
     */
    bool isZero ( void ) const {
      return x == 0.0f && y == 0.0f && z == 0.0f;
    }
  };

} // namespace noz


#endif // __noz_Vector3_h__
