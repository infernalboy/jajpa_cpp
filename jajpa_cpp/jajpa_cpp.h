/*
=======================================
  Just Another Json Parser C++ (jajpa_cpp)
  RFC 8259 realization
  https://gitlab.com/my_public_projects/jajpa_cpp
  Copyright (C) 2023  InfernalBoy(https://gitlab.com/InfernalBoy)

    * This program is free software. It comes without any warranty, to
    * the extent permitted by applicable law. You can redistribute it
    * and/or modify it under the terms of the WTFPL, Version 2, as published by Sam Hocevar. See
    * http://www.wtfpl.net/ for more details.
=======================================
## Examples 
### Serialization
```
     namespace ja = infernal::jajpa_cpp;
     ja::json::prop json = ja::json::tree {
          { "some_key", "some value" },
          { "some_num", 42 },
          { "some_null", ja::json::null() },
          { "some_sub_tree",
            ja::json::tree {
                 { "sub_tree_key", "sub value" },
            } },
          { "sub_array",
            ja::json::array {
                 1,
                 "str",
                 ja::json::null(),
            } },
     };

     std::ostringstream ss;
     ja::json().stringify( ss, json, true );

     std::cout << ss.str() << std::endl;
```
### Parsing
```
 namespace ja = infernal::jajpa_cpp;

     std::string strJson
          = "{                               "
            "    'example':'example',        "
            "    'num':42,                   "
            "    'sub':{                     "
            "        'one': 'somestr'        "
            "    },                          "
            "   'sub_array':[1,'str',null]   "
            "}                               ";
     std::istringstream stream( strJson );

     std::shared_ptr< ja::json::prop > json;
     try
     {
          json = ja::parser().parse( stream );
     }
     catch( const std::runtime_error& e )
     {
          std::cerr << e.what() << std::endl;
          return;
     }

     std::string val;
     std::string val2;
     std::string val3;
     try
     {
          ja::by_path by_path( *json );
          val = by_path.get_or_throw( "example" ).asVal< std::string >();

          val2 = by_path.get_or_throw( "sub.one" ).asVal< std::string >();

          val3 = by_path.get_or_throw( "sub_array.1" ).asVal< std::string >();
     }
     catch( const ja::by_path::wrong_path& e )
     {
          std::cerr << e.what() << std::endl;
          return;
     }

     std::cout << " Value1:" << val << " Value2:" << val2 << " Value3:" << val3
               << std::endl;

```
#### Or without throw 

```
     namespace ja = infernal::jajpa_cpp;

     std::string strJson
          = "{                               "
            "    'example':'example',        "
            "    'num':42,                   "
            "    'sub':{                     "
            "        'one': 'somestr'        "
            "    },                          "
            "   'sub_array':[1,'str',null]   "
            "}                               ";
     std::istringstream stream( strJson );

     std::shared_ptr< ja::json::prop > json;
     try
     {
          json = ja::parser().parse( stream );
     }
     catch( const std::runtime_error& e )
     {
          std::cerr << e.what() << std::endl;
          return;
     }

     std::string val;
     std::string val2;
     double val3;

     ja::by_path by_path( *json );
     val = by_path.get_or_default< std::string >( "example", "default" );

     val2 = by_path.get_or_default< std::string >( "sub.one", "default" );

     val3 = by_path.get_or_default( "sub_array.0", 0.0 );

     std::cout << " Value1:" << val << " Value2:" << val2 << " Value3:" << val3
               << std::endl;
```
*/

#ifndef JSON_TREE_H
#define JSON_TREE_H

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <vector>
#include <memory>
#include <unordered_map>
#include <list>
#include <set>
#include <functional>
#include <optional>
#include <iostream>

namespace infernal {
namespace jajpa_cpp {

template< typename T >
class property;

///
/// \brief The base_property class - abstract interface for you,
///
class base_property
{
public:
     struct cast_error : public std::runtime_error
     {
          cast_error( const std::type_info& from, const std::type_info& to )
               : std::runtime_error( std::string( " cast from:" ) + from.name() + " to:" + to.name() )
          {}
     };

public:
     virtual ~base_property() = default;

     template< typename T >
     ///
     /// \brief as - cast to const property< T >&  with type validation.
     /// \return
     /// \throws - cast_error if typeid(T) not equals with type of hold value.
     ///
     const property< T >& as() const
     {
          const std::type_info& to = typeid( T );
          if( m_typeid->hash_code() != to.hash_code() )
          {
               throw cast_error( *m_typeid, to );
          }
          return *static_cast< const property< T >* >( this );
     }

     ///
     /// \brief as - cast to const property< T >&  with type validation.
     /// \return
     /// \throws - cast_error if typeid(T) not equals with type of hold value.
     ///
     template< typename T >
     property< T >& as()
     {
          const std::type_info& to = typeid( T );
          if( m_typeid->hash_code() != to.hash_code() )
          {
               throw cast_error( *m_typeid, to );
          }
          return *static_cast< property< T >* >( this );
     }

     template< typename T >
     ///
     /// \brief is_type - compare types
     /// \return - true if type <T> equals type of hold value
     ///
     bool is_type() const
     {
          return m_typeid->hash_code() == typeid( T ).hash_code();
     }

     ///
     /// \brief is_same_type - compare types of hold values with other property
     /// \param other - other property
     /// \return - true if types of hold values is same
     ///
     bool is_same_type( const base_property& other ) const
     {
          return m_typeid->hash_code() == other.m_typeid->hash_code();
     }

     ///
     /// \brief type_hansh
     /// \return  - std::typeinfo.hash of type of hold value
     ///
     std::size_t type_hansh() const
     {
          return m_typeid->hash_code();
     }

     virtual void to_stream( std::ostream& s ) const = 0;

private:
     base_property( const base_property& ) = delete;
     base_property( base_property&& ) = delete;
     base_property& operator=( const base_property& ) = delete;
     base_property& operator=( base_property&& ) = delete;

protected:
     base_property( const std::type_info& type )
          : m_typeid( &type ) {};

private:
     const std::type_info* m_typeid = nullptr;
};

namespace {
template< typename T >
constexpr inline T init_value()
{
     if constexpr( std::is_arithmetic< T >::value )
          return 0;
     return T();
}
} // namespace

template< typename T >
///
/// \brief The property class - holds value of any types
///
class property : public base_property
{
public:
     explicit property()
          : base_property( typeid( T ) )
          , m_val( init_value< T >() )
     {}

     explicit property( const T& val )
          : base_property( typeid( T ) )
          , m_val( val )
     {}

     explicit property( T&& val )
          : base_property( typeid( T ) )
          , m_val( std::move( val ) )
     {}

     explicit property( const property& prop )
          : base_property( typeid( T ) )
          , m_val( prop.m_val )
     {}

     explicit property( property&& prop )
          : base_property( typeid( T ) )
          , m_val( std::move( prop.m_val ) )
     {}

     operator T() const
     {
          return m_val;
     }

     const T& to() const
     {
          return m_val;
     }

     T& to()
     {
          return m_val;
     }

     void to_stream( std::ostream& s ) const override
     {
          if constexpr( std::is_arithmetic< T >::value || std::is_same< std::string, T >::value )
          {
               s << m_val;
          }
          else
          {
               s << typeid( T ).name();
          }
     }

private:
     T m_val;
};

using base_property_ptr = std::shared_ptr< base_property >;
using c_base_property_ptr = std::shared_ptr< const base_property >;

namespace {
template< typename T >
base_property_ptr mk_p_ptr( const T& val )
{
     return std::make_shared< property< T > >( val );
}

template< typename T, typename V >
inline T& to_( V& v )
{
     return static_cast< T& >( v );
}

template< typename T, typename V >
inline T to_( V v )
{
     return static_cast< T >( v );
}

} // namespace

///
/// \brief The json class
///
class json
{
public:
     ///
     /// \brief The type_hash struct
     ///
     struct type_hash
     {
          std::uint64_t operator()( std::size_t t ) const
          {
               return t;
          }
     };

     class prop;
     using tree = std::unordered_map< std::string, prop >;
     using array = std::vector< prop >;

     ///
     /// \brief The null class - represents json null value
     ///
     class null
     {
     public:
          friend std::ostream& operator<<( std::ostream& stream, const json::null& val )
          {
               std::ignore = val;
               stream << std::string( "null" );
               return stream;
          }
     };

     ///
     /// \brief The prop class - wraps property with json types validation
     /// \details holds shared ptr to value data. When copy - OWN DATA is NOT copyed
     ///
     class prop
     {
     public:
         prop()
             : m_value( mk_p_ptr( null() ) ) {};

         prop( prop&& other )
         {
             if( m_value )
             {
                 throw std::runtime_error( "m_value can't be moved" );
             }

             m_value = std::move( other.m_value );
         }

         prop( null value )
             : m_value( mk_p_ptr( value ) )
         {}

         prop( bool value )
             : m_value( mk_p_ptr( value ) )
         {}

         prop( int value )
             : m_value( mk_p_ptr( static_cast< std::int64_t >( value ) ) )
         {}

         prop( std::int64_t value )
             : m_value( mk_p_ptr( static_cast< std::int64_t >( value ) ) )
         {}

         prop( double value )
             : m_value( mk_p_ptr( value ) )
         {}

         prop( const char* value )
             : m_value( mk_p_ptr< std::string >( value ) )
         {}

         prop( const std::string& value )
             : m_value( mk_p_ptr( value ) )
         {}

         prop( std::string&& value )
             : m_value( mk_p_ptr( std::move( value ) ) )
         {}

         prop( const array& value )
             : m_value( mk_p_ptr( value ) )
         {}

         prop( array&& value )
             : m_value( mk_p_ptr( std::move( value ) ) )
         {}

         prop( const tree& value )
             : m_value( mk_p_ptr( value ) )
         {}

         prop( tree&& value )
             : m_value( mk_p_ptr( std::move( value ) ) )
         {}

         ///
         /// \brief prop - constructor copy ref to data from [other]
         /// \param other
         ///
         prop( const prop& other )
         {
             //??? copy data or share?
             m_value = other.m_value;
         }

          prop& operator=( bool value )
          {
               m_value = mk_p_ptr( value );
               return *this;
          }

          prop& operator=( int value )
          {
               m_value = mk_p_ptr( value );
               return *this;
          }

          prop& operator=( std::int64_t value )
          {
               m_value = mk_p_ptr( value );
               return *this;
          }

          prop& operator=( double value )
          {
               m_value = mk_p_ptr( value );
               return *this;
          }

          prop& operator=( const std::string& value )
          {
               m_value = mk_p_ptr( value );
               return *this;
          }

          prop& operator=( std::string&& value )
          {
               m_value = mk_p_ptr( std::move( value ) );
               return *this;
          }

          prop& operator=( const array& value )
          {
               m_value = mk_p_ptr( value );
               return *this;
          }

          prop& operator=( array&& value )
          {
               m_value = mk_p_ptr( std::move( value ) );
               return *this;
          }

          prop& operator=( const tree& value )
          {
               m_value = mk_p_ptr( value );
               return *this;
          }

          prop& operator=( tree&& value )
          {
               m_value = mk_p_ptr( std::move( value ) );
               return *this;
          }

          ///
          /// \brief operator = - copy [value's] shared pointer to in self. DO NOT COPYES THE OWN DATA
          /// \param value - prop vhich refs to other data
          /// \return
          ///
          prop& operator=( const prop& value )
          {
               //??? copy data or share?
               m_value = value.m_value;
               return *this;
          }

          //who knows how to compare this shit?
          bool operator==( const prop& right ) const = delete;
          //          {
          //              return m_value->type_hansh() == right.m_value->type_hansh() &&
          //                      ;
          //          }

          const c_base_property_ptr value() const
          {
               return m_value;
          }

          base_property_ptr value()
          {
               return m_value;
          }

          template< typename T >
          T& asVal()
          {
               return value()->as< T >().to();
          }

          template< typename T >
          const T& asVal() const
          {
               return value()->as< T >().to();
          }

     public:
          base_property_ptr m_value;
     };

public:
    ///
    /// \brief stringify - stringify to output stream
    /// \param stream
    /// \param prop_ - object to stringfy
    /// \param ident
    ///
    void stringify( std::ostream& stream,
                    const prop& prop_,
                    bool ident = false ) const
    {
        stream << std::boolalpha;
        _ident = ident;
        const std::unordered_map<
            std::size_t,
            std::function< void( std::ostream&, std::list< node >& ) >,
            type_hash >
            writers( {
                { typeid( tree ).hash_code(),
                  std::bind( &json::write_tree,
                             this,
                             std::placeholders::_1,
                             std::placeholders::_2 ) },

                { typeid( array ).hash_code(),
                  std::bind( &json::write_array,
                             this,
                             std::placeholders::_1,
                             std::placeholders::_2 ) },
                { typeid( std::string ).hash_code(),
                  std::bind( &json::write_simple,
                             this,
                             std::placeholders::_1,
                             std::placeholders::_2 ) },

                { typeid( double ).hash_code(),
                  std::bind( &json::write_simple,
                             this,
                             std::placeholders::_1,
                             std::placeholders::_2 ) },
                { typeid( std::int64_t ).hash_code(),
                  std::bind( &json::write_simple,
                             this,
                             std::placeholders::_1,
                             std::placeholders::_2 ) },
                { typeid( int ).hash_code(),
                  std::bind( &json::write_simple,
                             this,
                             std::placeholders::_1,
                             std::placeholders::_2 ) },
                { typeid( bool ).hash_code(),
                  std::bind( &json::write_simple,
                             this,
                             std::placeholders::_1,
                             std::placeholders::_2 ) },
                { typeid( json::null ).hash_code(),
                  std::bind( &json::write_simple,
                             this,
                             std::placeholders::_1,
                             std::placeholders::_2 ) },

            } );

        std::list< node > p_stack;
        p_stack.push_back( {
            false,
            prop_,
            "",
            false,
        } );

        while( p_stack.size() )
        {
            auto current = p_stack.back();
            writers.at( current.prop_.value()->type_hansh() )( stream,
                                                               p_stack );
        }
    }

    ///
    /// \brief stringify - wrapper on  stringify( stream, prop( tree_ ), ident );
    /// \param tree_ - json::tree
    /// \param ident
    ///
    void stringify( std::ostream& stream,
                    const tree& tree_,
                    bool ident = false ) const
    {
        stringify( stream, prop( tree_ ), ident );
    }

    ///
    /// \brief stringify  - wrapper on stringify( stream, prop( std::move( tree_ ) ), ident );
    /// \param stream
    /// \param tree_ - rvalue  json::tree reff
    /// \param ident
    ///
    void stringify( std::ostream& stream,
                    tree&& tree_,
                    bool ident = false ) const
    {
        stringify( stream, prop( std::move( tree_ ) ), ident );
    }

    ///
    /// \brief stringify - wrapper on stringify( stream, prop( array_ ), ident );
    /// \param stream
    /// \param array_ - json::array to stringify
    /// \param ident
    ///
    void stringify( std::ostream& stream,
                    const array& array_,
                    bool ident = false ) const
    {
        stringify( stream, prop( array_ ), ident );
    }

    ///
    /// \brief stringify - wrapper on stringify( stream, prop( std::move( array_ ) ), ident );
    /// \param stream
    /// \param array_
    /// \param ident
    ///
    void stringify( std::ostream& stream,
                    array&& array_,
                    bool ident = false ) const
    {
        stringify( stream, prop( std::move( array_ ) ), ident );
    }

private:
     ///
     /// \brief The node struct
     ///
     struct node
     {
          enum class type : int
          {
               poperty,
               item,
               head,
          };

          bool finalize = false;
          prop prop_;
          std::string name;
          bool put_comma = true;
          type type_ = type::head;
     };

private:
     static void write_escaped_string( std::ostream& stream, const std::string& str )
     {
          for( char ch : str )
          {
               const unsigned char c = static_cast< unsigned char >( ch );
               switch( c )
               {
                    case '"':
                         stream << "\\\"";
                         break;
                    case '\\':
                         stream << "\\\\";
                         break;
                    case '\b':
                         stream << "\\b";
                         break;
                    case '\f':
                         stream << "\\f";
                         break;
                    case '\n':
                         stream << "\\n";
                         break;
                    case '\r':
                         stream << "\\r";
                         break;
                    case '\t':
                         stream << "\\t";
                         break;
                    default:
                         if( c < 0x20 )
                         {
                              char buf[ 7 ];
                              std::snprintf( buf, sizeof( buf ), "\\u%04x", c );
                              stream << buf;
                         }
                         else
                         {
                              stream << c;
                         }
                         break;
               }
          }
     }

     void write_tree( std::ostream& stream, std::list< node >& p_stack ) const
     {
          node& tree_ = p_stack.back();
          if( tree_.finalize )
          {
               --_depth;
               write_ident( stream );
               stream << '}';
               if( tree_.put_comma )
               {
                    stream << ',';
               }
               write_new_line( stream );
               p_stack.pop_back();
               return;
          }

          write_ident( stream );
          if( tree_.type_ == node::type::poperty )
          {
               stream << '"';
               write_escaped_string( stream, tree_.name );
               stream << "\":";
          }

          ++_depth;
          tree_.finalize = true;

          stream << '{';
          write_new_line( stream );
          auto&& tr = tree_.prop_.value()->as< tree >().to();
          std::vector< node > reverse;
          reverse.reserve( tr.size() );

          for( auto rit = tr.begin(); rit != tr.end(); ++rit )
          {
               auto next_it = rit;
               ++next_it;
               bool need_comma = next_it != tr.end();
               reverse.push_back( { false, rit->second, rit->first, need_comma, node::type::poperty } );
          }

          for( auto rit = reverse.rbegin(); rit != reverse.rend(); ++rit )
          {
               p_stack.push_back( *rit );
          }
     }

     void write_array( std::ostream& stream, std::list< node >& p_stack ) const
     {
          node& array_ = p_stack.back();
          if( array_.finalize )
          {
               --_depth;
               write_ident( stream );
               stream << ']';
               if( array_.put_comma )
               {
                    stream << ',';
               }
               write_new_line( stream );
               p_stack.pop_back();
               return;
          }

          write_ident( stream );
          if( array_.type_ == node::type::poperty )
          {
               stream << '"';
               write_escaped_string( stream, array_.name );
               stream << "\":";
          }

          ++_depth;
          array_.finalize = true;

          stream << '[';
          write_new_line( stream );
          auto&& tr = array_.prop_.value()->as< array >().to();
          auto&& lastIt = tr.rbegin();
          for( auto rit = tr.rbegin(); rit != tr.rend(); ++rit )
          {
               bool need_comma = rit != lastIt;
               p_stack.push_back( {
                    false,
                    *rit,
                    "",
                    need_comma,
                    node::type::item,
               } );
          }
     }

     void write_simple( std::ostream& stream, std::list< node >& p_stack ) const
     {
          node& field = p_stack.back();

          write_ident( stream );
          if( field.type_ == node::type::poperty )
          {
               stream << '"';
               write_escaped_string( stream, field.name );
               stream << "\":";
          }

          if( field.prop_.value()->is_type< std::string >() )
          {
               stream << '"';
               write_escaped_string( stream, field.prop_.value()->as< std::string >().to() );
               stream << '"';
          }
          else if( field.prop_.value()->is_type< json::null >() )
          {
               stream << field.prop_.value()->as< json::null >().to();
          }
          else
          {
               field.prop_.value()->to_stream( stream );
          }

          if( field.put_comma )
          {
               stream << ',';
          }
          write_new_line( stream );
          p_stack.pop_back();
     }

     inline void write_ident( std::ostream& stream ) const
     {
          if( _ident )
          {
               for( int i = 0; i < _depth; ++i )
               {
                    stream << '\t';
               }
          }
     }

     inline void write_new_line( std::ostream& stream ) const
     {
          if( _ident )
          {
               stream << '\n';
          }
     }

private:
     mutable bool _ident = false;
     mutable int _depth = 0;
};

///
/// \brief The parser class
///
class parser
{
public:
     parser()
     {
          _stack.reserve( 100 );
          _state_stack.reserve( 100 );
     }

     ///
     /// \brief parse - parses from input stream
     /// \param stream - stream with json string
     /// \return s-ptr to prop with result
     /// \throws any exceptions. look bottom.
     ///
     std::shared_ptr< json::prop > parse( std::istream& stream )
     {
          _pos = stream.tellg();
          _stack.clear();
          _state_stack.clear();
          _buffet = stream_buffet< buffet_len >();

          //enum class state : int - as index!
          const std::vector< std::function< void( std::istream & stream ) > > readers {
               []( std::istream& ) {
                    throw std::runtime_error( "UNEXPECTED STATE HANDLER(none)" );
               },
               std::bind( &parser::read_object, this, std::placeholders::_1 ),

               std::bind( &parser::read_object_done, this, std::placeholders::_1 ),

               std::bind( &parser::read_property, this, std::placeholders::_1 ),

               std::bind( &parser::read_array, this, std::placeholders::_1 ),

               std::bind( &parser::read_array_done, this, std::placeholders::_1 ),

               std::bind( &parser::read_array_item, this, std::placeholders::_1 ),
               []( std::istream& ) {
                    throw std::runtime_error( "UNEXPECTED STATE HANDLER(done)" );
               },
          };

          char init_char;
          get_symbol( stream, init_char, true );

          auto init_state = _open_state_modifiers.find( init_char );
          if( init_state == _open_state_modifiers.end() )
          {
               throw unexpected_symbol( _pos, init_char );
          }

          push_state( init_state->second );
          state curr = curr_state();

          _stack.push_back( node { "", 0 } );

          while( curr != state::done )
          {
               readers[ static_cast< std::size_t >( curr ) ]( stream );
               curr = curr_state();
          }

          if( _stack.size() != 1 )
          {
               throw parse_error( "stack is not handled" );
          }

          char trailing;
          while( _buffet.get( stream, trailing ) )
          {
               _pos = _buffet.stream_pos();
               if( trailing == ' ' || trailing == '\t' || trailing == '\n' || trailing == '\r' )
               {
                    continue;
               }
               throw unexpected_symbol( _pos, trailing );
          }

          return std::make_shared< json::prop >( _stack.back().prop );
     }

public:
     ///
     /// \brief The read_error struct
     ///
     struct read_error : public std::runtime_error
     {
          read_error()
               : std::runtime_error( "can't read from stream" )
          {}
     };

     ///
     /// \brief The syntax_error struct
     ///
     struct syntax_error : public std::runtime_error
     {
          syntax_error( const std::string& what )
               : std::runtime_error( what )
          {}
     };

     ///
     /// \brief The unexpected_symbol struct
     ///
     struct unexpected_symbol : public syntax_error
     {
          const std::streamsize pos;
          unexpected_symbol( std::streamsize pos_, char symbol )
               : syntax_error( std::string( "unexpected symbol \'" ) + symbol + "\' at pos:" + std::to_string( pos_ ) )
               , pos( pos_ )
          {}

     protected:
          unexpected_symbol( const std::string& what, std::streamsize pos_ )
               : syntax_error( what )
               , pos( pos_ )
          {}
     };

     ///
     /// \brief The unexpected_brace_sequence struct
     ///
     struct unexpected_brace_sequence : public unexpected_symbol
     {
          unexpected_brace_sequence( std::streamsize pos )
               : unexpected_symbol( std::string( "unexpected brace sequence at pos:" + std::to_string( pos ) ), pos )
          {}
     };

     ///
     /// \brief The non_digit_symbol struct
     ///
     struct non_digit_symbol : public unexpected_symbol
     {
          non_digit_symbol( std::streamsize pos, char ch )
               : unexpected_symbol( std::string( "non digit symbol '" ) + ch + "' at pos:" + std::to_string( pos - 1 ),
                                    pos )
          {}
     };

     ///
     /// \brief The unexpected_eof struct
     ///
     struct unexpected_eof : public syntax_error
     {
          unexpected_eof( std::streamsize pos )
               : syntax_error( std::string( "unexpected end of file at pos:" + std::to_string( pos ) ) )
          {}
     };

     ///
     /// \brief The missing_symbols struct
     ///
     struct missing_symbols : public unexpected_symbol
     {
          missing_symbols( const std::string& what_miss, std::streamsize pos )
               : unexpected_symbol( "missing '" + what_miss + "' at pos:" + std::to_string( pos ), pos )
          {}
     };

     ///
     /// \brief The state_error struct
     ///
     struct state_error : public std::runtime_error
     {
          state_error( const std::string& what )
               : std::runtime_error( what )
          {}
     };

     ///
     /// \brief The parse_error struct
     ///
     struct parse_error : public std::runtime_error
     {
          parse_error( const std::string& what )
               : std::runtime_error( what )
          {}
     };

private:
     ///
     /// \brief The node struct
     ///
     struct node
     {
          std::string name;
          json::prop prop;
     };

     ///
     /// \brief The state enum
     ///
     enum class state : int
     {
          none,
          read_object,
          read_object_done,
          read_object_property,
          read_array,
          read_array_done,
          read_array_item,
          done,
     };

private:
     void read_object( std::istream& stream )
     {
          std::ignore = stream;
          _stack.back().prop = json::prop( json::tree() );
          push_state( state::read_object_property );
     }

     void read_object_done( std::istream& stream )
     {
          if( _stack.size() == 1 )
          {
               push_state( state::done );
               return;
          }

          put_child_and_validate_state_sequence( state::read_object, state::read_object_done );

          eat_comma_or_close_state( stream );
     }

     void read_array( std::istream& stream )
     {
          std::ignore = stream;
          _stack.back().prop = json::prop( json::array() );
          push_state( state::read_array_item );
     }

     void read_array_done( std::istream& stream )
     {
          std::ignore = stream;
          if( _stack.size() == 1 )
          {
               push_state( state::done );
               return;
          }

          put_child_and_validate_state_sequence( state::read_array, state::read_array_done );

          eat_comma_or_close_state( stream );
     }

     void put_child_and_validate_state_sequence( state expect_prev, state expect_curr )
     {
          state r_curr = pop_state();
          state r_prev = pop_state();

          if( r_prev != expect_prev || r_curr != expect_curr )
          {
               throw unexpected_brace_sequence( _pos );
          }

          node child = _stack.back();
          _stack.pop_back();
          node& parent = _stack.back();
          put_child( parent, child );
     }

     void read_property( std::istream& stream )
     {
          char quota = ' ';
          get_symbol( stream, quota, true );

          auto state_mod = _close_state_modifiers.find( quota );
          if( state_mod != _close_state_modifiers.end() )
          {
               replace_state( state_mod->second );
               return;
          }

          if( quota != '\"' && quota != '\'' )
          {
               throw missing_symbols( "\" or \'", _pos );
          }

          json::prop p_name = read_string( stream, quota );

          char colon;
          get_symbol( stream, colon, true );

          if( colon != ':' )
          {
               throw missing_symbols( ":", _pos );
          }

          std::optional< json::prop > prop = read_value( stream );
          switch( curr_state() )
          {
               case state::read_object:
               case state::read_array:
                    _stack.push_back( node { p_name.value()->as< std::string >().to(), prop.value() } );
                    break;
               case state::read_object_done:
                    if( !prop )
                    {
                         return;
                    }
                    [[fallthrough]];
               case state::read_object_property:
               {
                    auto is_unique = _stack.back().prop.value()->as< json::tree >().to().insert(
                         std::pair( p_name.value()->as< std::string >().to(), prop.value() ) );
                    if( !is_unique.second )
                    {
                         throw syntax_error( "duplicate members keys :" + p_name.value()->as< std::string >().to() );
                    }
               }
               break;
               default:
                    throw unexpected_brace_sequence( _pos );
          }
     }

     void read_array_item( std::istream& stream )
     {
          std::optional< json::prop > prop = read_value( stream );
          switch( curr_state() )
          {
               case state::read_object:
               case state::read_array:
                    _stack.push_back( node { "", prop.value() } );
                    break;
               case state::read_array_done:
                    if( !prop )
                    {
                         return;
                    }
                    [[fallthrough]];
               case state::read_array_item:
                    _stack.back().prop.value()->as< json::array >().to().push_back( prop.value() );
                    break;
               default:
                    throw unexpected_brace_sequence( _pos );
          }
     }

     std::optional< json::prop > read_value( std::istream& stream )
     {
          static const std::set< char > null_true_or_false { 'n', 't', 'f' };

          json::prop result( 0 );
          char symbol;
          get_symbol( stream, symbol, true );

          auto state_mod = _open_state_modifiers.find( symbol );
          if( state_mod != _open_state_modifiers.end() )
          {
               push_state( state_mod->second );
               return result;
          }

          if( std::isdigit( symbol ) || symbol == '-' )
          {
               std::string str_num;
               str_num.push_back( symbol );
               str_num += read_number( stream );

               const bool is_integer
                    = str_num.find_first_of( ".eE" ) == std::string::npos;
               if( is_integer )
               {
                    try
                    {
                         std::size_t parsed = 0;
                         const std::int64_t int_val = std::stoll( str_num, &parsed );
                         if( parsed == str_num.size() )
                         {
                              result = json::prop( int_val );
                         }
                         else
                         {
                              result = json::prop( std::stod( str_num ) );
                         }
                    }
                    catch( const std::exception& )
                    {
                         result = json::prop( std::stod( str_num ) );
                    }
               }
               else
               {
                    result = json::prop( std::stod( str_num ) );
               }
          }
          else if( symbol == '\"' || symbol == '\'' )
          {
               result = read_string( stream, symbol );

               eat_comma_or_close_state( stream );
          }

          else if( null_true_or_false.find( symbol ) != null_true_or_false.end() )
          {
               result = read_bool( stream, symbol );
          }
          else
          {
               auto state_mod = _close_state_modifiers.find( symbol );
               if( state_mod != _close_state_modifiers.end() )
               {
                    replace_state( state_mod->second );
                    return {};
               }
               throw unexpected_symbol( _pos, symbol );
          }

          return result;
     }

     void put_child( node& parent, const node& child )
     {
          if( parent.prop.value()->is_type< json::tree >() )
          {
               auto is_unique = parent.prop.value()->as< json::tree >().to().insert( { child.name, child.prop } );
               if( !is_unique.second )
               {
                    throw syntax_error( "duplicate members keys : " + child.name );
               }
          }
          else if( parent.prop.value()->is_type< json::array >() )
          {
               parent.prop.value()->as< json::array >().to().push_back( child.prop );
          }
          else
          {
               throw parse_error( "parent type is not object or array" );
          }
     }

     void eat_comma_or_close_state( std::istream& stream )
     {
          char comma;
          get_symbol( stream, comma, true );

          if( comma != ',' )
          {
               auto close_state = _close_state_modifiers.find( comma );
               if( close_state == _close_state_modifiers.end() )
               {
                    throw unexpected_symbol( _pos, comma );
               }

               replace_state( close_state->second );
          }
     }

     static void append_utf8( std::string& out, unsigned int cp )
     {
          if( cp <= 0x7F )
          {
               out.push_back( static_cast< char >( cp ) );
          }
          else if( cp <= 0x7FF )
          {
               out.push_back( static_cast< char >( 0xC0 | ( cp >> 6 ) ) );
               out.push_back( static_cast< char >( 0x80 | ( cp & 0x3F ) ) );
          }
          else if( cp <= 0xFFFF )
          {
               out.push_back( static_cast< char >( 0xE0 | ( cp >> 12 ) ) );
               out.push_back( static_cast< char >( 0x80 | ( ( cp >> 6 ) & 0x3F ) ) );
               out.push_back( static_cast< char >( 0x80 | ( cp & 0x3F ) ) );
          }
          else
          {
               out.push_back( static_cast< char >( 0xF0 | ( cp >> 18 ) ) );
               out.push_back( static_cast< char >( 0x80 | ( ( cp >> 12 ) & 0x3F ) ) );
               out.push_back( static_cast< char >( 0x80 | ( ( cp >> 6 ) & 0x3F ) ) );
               out.push_back( static_cast< char >( 0x80 | ( cp & 0x3F ) ) );
          }
     }

     unsigned int read_hex4( std::istream& stream )
     {
          unsigned int value = 0;
          for( int i = 0; i < 4; ++i )
          {
               char c = 0;
               get_raw_symbol( stream, c );
               value <<= 4;
               if( c >= '0' && c <= '9' )
               {
                    value |= static_cast< unsigned int >( c - '0' );
               }
               else if( c >= 'a' && c <= 'f' )
               {
                    value |= static_cast< unsigned int >( c - 'a' + 10 );
               }
               else if( c >= 'A' && c <= 'F' )
               {
                    value |= static_cast< unsigned int >( c - 'A' + 10 );
               }
               else
               {
                    throw syntax_error( "bad \\u escape at pos:" + std::to_string( _pos ) );
               }
          }
          return value;
     }

     void read_unicode_escape( std::istream& stream, std::string& out )
     {
          unsigned int cp = read_hex4( stream );
          if( cp >= 0xD800 && cp <= 0xDBFF )
          {
               char bs = 0;
               char u = 0;
               get_raw_symbol( stream, bs );
               get_raw_symbol( stream, u );
               if( bs != '\\' || u != 'u' )
               {
                    throw syntax_error( "bad surrogate pair at pos:" + std::to_string( _pos ) );
               }
               unsigned int low = read_hex4( stream );
               cp = 0x10000 + ( ( cp - 0xD800 ) << 10 ) + ( low - 0xDC00 );
          }
          append_utf8( out, cp );
     }

     json::prop read_string( std::istream& stream, const char quota )
     {
          json::prop prop_res = std::string( "" );
          std::string& result = prop_res.value()->as< std::string >().to();
          result.reserve( 100 );

          char ch = 0;
          const std::streamsize start = _pos;
          bool closed = false;
          while( get_raw_symbol( stream, ch ) )
          {
               if( ch == quota )
               {
                    closed = true;
                    break;
               }

               if( ch == '\\' )
               {
                    char esc = 0;
                    get_raw_symbol( stream, esc );
                    switch( esc )
                    {
                         case '"':
                              result.push_back( '"' );
                              break;
                         case '\'':
                              result.push_back( '\'' );
                              break;
                         case '\\':
                              result.push_back( '\\' );
                              break;
                         case '/':
                              result.push_back( '/' );
                              break;
                         case 'b':
                              result.push_back( '\b' );
                              break;
                         case 'f':
                              result.push_back( '\f' );
                              break;
                         case 'n':
                              result.push_back( '\n' );
                              break;
                         case 'r':
                              result.push_back( '\r' );
                              break;
                         case 't':
                              result.push_back( '\t' );
                              break;
                         case 'u':
                              read_unicode_escape( stream, result );
                              break;
                         default:
                              throw syntax_error( "invalid escape sequence at pos:" + std::to_string( _pos ) );
                    }
                    continue;
               }

               if( static_cast< unsigned char >( ch ) < 0x20 )
               {
                    throw syntax_error( "unescaped control character in string at pos:"
                                        + std::to_string( _pos ) );
               }

               result.push_back( ch );
          }

          if( !closed )
          {
               throw syntax_error( "String not closed at pos:" + std::to_string( start ) );
          }
          return prop_res;
     }

     std::string read_number( std::istream& stream )
     {
          std::string result;

          char ch;
          bool expected_stop = false;
          static const std::set< char > allow_non_digit_symbols {
               '.', 'e', 'E', '+', '-',
          };
          while( get_symbol( stream, ch ) )
          {
               if( ch == ',' )
               {
                    expected_stop = true;
                    break;
               }
               else if( ch == ' ' )
               {
                    expected_stop = true;
                    eat_comma_or_close_state( stream );
                    break;
               }

               auto state_mod = _close_state_modifiers.find( ch );
               if( state_mod != _close_state_modifiers.end() )
               {
                    replace_state( state_mod->second );
                    expected_stop = true;
                    break;
               }

               bool is_allow_non_digit_symbol = allow_non_digit_symbols.find( ch ) != allow_non_digit_symbols.end();

               if( ( !std::isdigit( ch ) ) && !is_allow_non_digit_symbol )
               {
                    throw non_digit_symbol( _pos, ch );
               }

               result.push_back( ch );
          }

          if( !expected_stop )
          {
               throw missing_symbols( "comma or bracket", _pos );
          }

          return result;
     }

     json::prop read_bool( std::istream& stream, char start )
     {
          const static std::set< char > allow_symbols { 't', 'r', 'u', 'e', 'f', 'a', 'l', 's', 'e', 'n' };
          const static std::map< std::string, json::prop > to_bool {
               { "true", true },
               { "false", false },
               { "null", json::null() },
          };

          std::string res;
          res.push_back( start );

          char symbol = start;
          bool expected_stop = false;

          while( get_symbol( stream, symbol ) )
          {
               if( symbol == ',' )
               {
                    expected_stop = true;
                    break;
               }
               else if( symbol == ' ' )
               {
                    expected_stop = true;
                    eat_comma_or_close_state( stream );
                    break;
               }

               auto state_mod = _close_state_modifiers.find( symbol );
               if( state_mod != _close_state_modifiers.end() )
               {
                    replace_state( state_mod->second );
                    expected_stop = true;
                    break;
               }

               bool is_allow_symbol = allow_symbols.find( symbol ) != allow_symbols.end();

               if( !is_allow_symbol )
               {
                    throw unexpected_symbol( _pos, symbol );
               }

               res.push_back( symbol );
          }

          if( !expected_stop )
          {
               throw missing_symbols( "comma or bracket", _pos );
          }

          auto bool_res = to_bool.find( res );

          if( bool_res == to_bool.end() )
          {
               throw syntax_error( "unexpected value at pos:"
                                   + std::to_string( _pos - static_cast< std::streamsize >( res.length() ) ) + " '"
                                   + res + "'" );
          }
          return bool_res->second;
     }

     inline int sqr_find( const char* where, char what, int size )
     {
          for( int i = 0; i < size; ++i )
          {
               if( where[ i ] == what )
               {
                    return i;
               }
          }
          return -1;
     }

     bool get_raw_symbol( std::istream& stream, char& ch )
     {
          if( _buffet.get( stream, ch ) )
          {
               _pos = _buffet.stream_pos();
               return true;
          }

          if( stream.eof() )
          {
               throw unexpected_eof( _pos );
          }
          throw read_error();
     }

     bool get_symbol( std::istream& stream, char& ch, bool skip_spaces = false )
     {
          const static std::vector< char > default_ignore {
               '\t',
               '\n',
          };

          const static std::vector< char > without_spaces_ignore {
               '\t',
               '\n',
               ' ',
          };

          auto& ignore = skip_spaces ? without_spaces_ignore : default_ignore;
          const int ignoreSize = static_cast< int >( ignore.size() );
          const char* ingnoreData = ignore.data();
          while( _buffet.get( stream, ch ) )
          {
               _pos = _buffet.stream_pos();
               //faster than std::set
               auto index = sqr_find( ingnoreData, ch, ignoreSize );
               if( index != -1 )
               {
                    continue;
               }
               return true;
          }

          if( stream.eof() )
          {
               throw unexpected_eof( _pos );
          }
          throw read_error();
     }

     inline state curr_state() const
     {
          return _state_stack.back();
     }

     inline state replace_state( state new_state )
     {
          auto res = pop_state();
          push_state( new_state );
          return res;
     }

     inline state pop_state()
     {
          auto res = _state_stack.back();
          _state_stack.pop_back();
          return res;
     }

     inline void push_state( state new_state )
     {
          if( new_state == state::none )
          {
               return;
          }

          _state_stack.push_back( new_state );
     }

private:
     template< std::size_t size >
     ///
     /// \brief The stream_buffet class
     ///
     class stream_buffet
     {
     public:
          //OmNomNom byteeeeesss!
          stream_buffet() = default;

          inline bool get( std::istream& stream, char& ch )
          {
               if( _pos >= _available )
               {
                    if( stream.fail() || stream.eof() )
                    {
                         return false;
                    }

                    _pos = 0;
                    //slw
                    //_data.fill( 0 );
                    _available = stream.readsome( _data.data(), size );
                    _stream_pos = stream.tellg();
                    if( _available == 0 )
                    {
                         stream.setstate( std::istream::eofbit );
                         return false;
                    }
               }

               ch = _data.at( to_< std::size_t >( _pos++ ) );
               return true;
          }

          inline std::streamsize stream_pos() const
          {
               return _stream_pos - _available + _pos;
          }

     private:
          std::array< char, size > _data;
          std::streamsize _pos = 0;
          std::streamsize _stream_pos = 0;
          std::streamsize _available = 0;
     };

private:
     static constexpr ssize_t buffet_len = 256;
     stream_buffet< buffet_len > _buffet;
     std::streamsize _pos = 0;
     const std::map< char, state > _close_state_modifiers = {
          { '}', state::read_object_done },
          { ']', state::read_array_done },
     };
     const std::map< char, state > _open_state_modifiers = {
          { '{', state::read_object },
          { '[', state::read_array },
     };

     std::vector< state > _state_stack;
     std::vector< node > _stack;
};

///
/// \brief The by_path class - util which give possibility to get value by dotted path
///
class by_path
{
public:
     struct type_error : public std::runtime_error
     {
          type_error()
               : std::runtime_error( "not tree or array" )
          {}
     };

     struct wrong_path : public std::runtime_error
     {
          wrong_path( const std::string& path )
               : std::runtime_error( "Wrong path:" + path )
          {}
     };

     ///
     /// \brief by_path - creates and bind a const reference to
     /// property which contains tree or array
     /// \details not own of any data
     /// \param prop
     ///
     by_path( const json::prop& prop )
          : head_( prop )
     {
          if( !head_.value()->is_type< json::tree >() && !head_.value()->is_type< json::array >() )
          {
               throw type_error();
          }
     }

     ///
     /// \brief get_or_throw - finds value by dotted path like 'some_tree.sub_tree.array.3'.
     /// \warning don't support escaped '\.' delimiters
     /// \param dotted_path
     /// \return prop to found value
     /// \throws if not found by path
     ///
     const json::prop& get_or_throw( const std::string& dotted_path )
     {
          std::vector path = path_splice( dotted_path ).splice( '.' );

          const json::prop* prop = &head_;

          const static std::map< std::size_t,
                                 std::function< const json::prop*( const json::prop&, const std::string& ) > >
               map = { { typeid( json::tree ).hash_code(),
                         []( const json::prop& p, const std::string& item ) -> const json::prop* {
                              return &p.asVal< json::tree >().at( item );
                         } },
                       {
                            typeid( json::array ).hash_code(),
                            []( const json::prop& p, const std::string& item ) -> const json::prop* {
                                 return &p.asVal< json::array >().at( std::stoul( item ) );
                            },
                       } };

          try
          {
               for( auto&& item : path )
               {
                    auto type = prop->value()->type_hansh();
                    prop = map.at( type )( *prop, item );
               }
          }
          catch( const std::exception& e )
          {
               throw wrong_path( dotted_path + " e:" + e.what() );
          }

          return *prop;
     }

     template< typename T >
     ///
     /// \brief get_or_default - get value by dotted path
     /// \param dotted_path
     /// \param default_val
     /// \return default_val - if value not found
     ///
     T get_or_default( const std::string& dotted_path, const T& default_val )
     {
         static_assert( !std::is_scalar_v< T > || std::is_same_v< T, double >
                            || std::is_same_v< T, std::int64_t >,
                        "get_or_default: numbers can be type of "
                        "double|std::int64_t only" );
         try
         {
             return get_or_throw( dotted_path ).asVal< T >();
         }
          catch( const wrong_path& )
          {
               return default_val;
          }
          catch( const base_property::cast_error& )
          {
               return default_val;
          }
     }

public:
     class path_splice
     {
          const std::string& str_;

     public:
          path_splice( const std::string& str )
               : str_( str )
          {}

          std::vector< std::string > splice( char separator )
          {
               std::vector< std::string > out;
               std::uint64_t pos = 0;
               std::uint64_t cur = 0;
               while( cur != std::string::npos )
               {
                    cur = str_.find( separator, pos );
                    out.push_back( str_.substr( pos, cur - pos ) );
                    pos = cur + 1;
               }
               return out;
          }
     };

private:
     const json::prop& head_;
}; // namespace jajpa_cpp

} // namespace jajpa_cpp
} // namespace infernal
#endif // JSON_TREE_H
