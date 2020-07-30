///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_GlueClass_h__
#define __noz_Editor_GlueClass_h__

namespace noz {
namespace Editor {

  struct GlueState;

  struct GlueClass {
    /// True if the class is a reflected class (contains NOZ_OBJECT, NOZ_TEMPLATE, or NOZ_INTERFACE)
    private: bool reflected_;

    /// True if the class is excluded from the build.
    private: bool excluded_;

    /// True if the class is abstract (pure virtual methods)
    private: bool abstract_;

    /// Points to the nested parent class if this is a nested class.
    private: GlueClass* nested_parent_;

    /// Base class.
    private: GlueClass* base_;

    /// Vector of all base class names
    private: std::vector<Name> base_names_;

    /// Non-Template base class
    private: GlueClass* non_template_base_;

    /// Unqualified class name
    private: Name name_;

    /// Qualified name of the class
    private: Name qualified_name_;

    /// Namespace of class
    private: Name namespace_;

    /// File the class was found in
    private: GlueFile* gf_;

    /// Line within the class file the class was found on
    private: noz_uint32 gf_line_;

    /// True if the class has been processed.
    private: bool processed_; 

    /// True if the class is a template
    private: bool template_;

    /// True if the class is a interface
    private: bool interface_;

    /// True if the class has a default constructor
    private: bool default_constructor_;

    /// Meta associated with class
    private: GlueMeta meta_;

    /// Vector of all known properties
    private: GluePropertyVector properties_;

    /// Vector of all known methods.
    private: GlueMethodVector methods_;

    /// Vector of all known constructors
    private: std::vector<GlueMethod*> constructors_;

    /// Value to sort the class by (includes depth and flags)
    private: noz_int32 sort_key_;

    /// Default constructor
    public: GlueClass(void);

    public: bool ResolveBaseClasses (GlueState* gs);
    
    public: bool Process (GlueState* gs);

    public: bool IsTypeOf(GlueClass* gc) const {return (this==gc || IsSubclassOf(gc));}

    public: bool IsSubclassOf(GlueClass* gc) const;

    public: bool IsNested (void) const {return nested_parent_ != nullptr;}

      
    /// Set the excluded state of the class (default is true)
    public: void SetExcluded (bool excluded) {excluded_ = excluded;}

    /// Set the reflected state of the class (Default is false)
    public: void SetReflected (bool reflected) {reflected_ = reflected;}

    /// Set the interface state of the class (Default is false)
    public: void SetInterface (bool v) {interface_ = v;}

    /// Set the template state of the class (Default is false)
    public: void SetTemplate (bool v) {template_ = v;}

    /// Set the class as an abstract class
    public: void SetAbstract (void) {abstract_ = true;}

    /// Set unqualified class name
    public: void SetName (const Name& name) {name_ = name;}

    /// Set the classes qualified name
    public: void SetQualifiedName (const Name& name) {qualified_name_ = name;}

    /// Set namespace
    public: void SetNamespace (const Name& ns) {namespace_ = ns;}

    /// Set the nested parent class
    public: void SetNestedParent (GlueClass* gc) {nested_parent_ = gc;}

    /// Add a base class anme
    public: void AddBaseName (const Name& name) {base_names_.push_back(name);}

    /// Returns true if the class is excluded from the build
    public: bool IsExcluded (void) const {return excluded_;}

    /// Returns true if the class is a template class
    public: bool IsTemplate (void) const {return template_;}

    /// Returns true if the class is a reflected class
    public: bool IsReflected (void) const {return reflected_;}

    /// Returns true if the class is abstract
    public: bool IsAbstract (void) const {return abstract_;}

    /// Return the nested parent class if this is a nested parent or nullptr if not
    public: GlueClass* GetNestedParent(void) const {return nested_parent_;}

    /// Return the file the class was found in
    public: GlueFile* GetFile (void) const {return gf_;}

    /// Return the line number the class was found on
    public: noz_uint32 GetLineNumber (void) const {return gf_line_;}

    /// Return the base class
    public: GlueClass* GetBase (void) const {return base_; }

    /// Return the non template base of the class
    public: GlueClass* GetNonTemplateBase (void) const {return non_template_base_;}

    /// Return the unqualified class name
    public: const Name& GetName (void) const {return name_;}

    /// Return the qualified name of the class.
    public: const Name& GetQualifiedName (void) const {return qualified_name_;}

    /// Return the namespace
    public: const Name& GetNamespace (void) const {return namespace_;}

    /// Set the file and line number
    public: void SetFile (GlueFile* gf, noz_uint32 line=0) {gf_=gf;gf_line_=line;}

    /// Get the meta 
    public: GlueMeta& GetMeta (void) {return meta_;}

    /// Get vector of methods
    public: const GlueMethodVector& GetMethods(void) const {return methods_;}

    /// Get vector of properties
    public: const GluePropertyVector& GetProperties(void) const {return properties_;}

    /// Add a constructor the constructor vector
    public: GlueMethod* AddConstructor (GlueMethod* gm);

    /// Add a method the method vector
    public: GlueMethod* AddMethod (GlueMethod* gm) {methods_.push_back(gm);return gm;}

    /// Add a property the property vector
    public: GlueProperty* AddProperty (GlueProperty* gp) {properties_.push_back(gp);return gp;}

    /// Return the key used to sort the class
    public: noz_int32 GetSortKey(void) const {return sort_key_;}

    /// Does the class have a default constructor?
    public: bool HasDefaultConstructor (void) const {return default_constructor_ || constructors_.empty();}
  };  

  typedef std::vector<GlueClass*> GlueClassVector;

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_GlueClass_h__
