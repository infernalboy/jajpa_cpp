/*
=======================================
  Just Another Json Parser C++ (jajpa_cpp)
  RFC 8259 realization
  Copyright (C) 2023  InfernalBoy(https://gitlab.com/InfernalBoy)

    * This program is free software. It comes without any warranty, to
    * the extent permitted by applicable law. You can redistribute it
    * and/or modify it under the terms of the Do What The Fuck You Want
    * To Public License, Version 2, as published by Sam Hocevar. See
    * http://www.wtfpl.net/ for more details.
=======================================
 */

#include <gtest/gtest.h>
#include <fstream>
#include <unistd.h>
#include <ios>
#include <iostream>

#include "jajpa_cpp.h"

using namespace infernal::jajpa_cpp;

namespace {

//////////////////////////////////////////////////////////////////////////////
//
// https://stackoverflow.com/questions/669438/how-to-get-memory-usage-at-runtime-using-c
// process_mem_usage(double &, double &) - takes two doubles by reference,
// attempts to read the system-dependent data for a process' virtual memory
// size and resident set size, and return the results in KB.
//
// On failure, returns 0.0, 0.0
void process_mem_usage( double& vm_usage, double& resident_set )
{
     using std::ios_base;
     using std::ifstream;
     using std::string;

     vm_usage = 0.0;
     resident_set = 0.0;

     // 'file' stat seems to give the most reliable results
     //
     ifstream stat_stream( "/proc/self/stat", ios_base::in );

     // dummy vars for leading entries in stat that we don't care about
     //
     string pid, comm, state, ppid, pgrp, session, tty_nr;
     string tpgid, flags, minflt, cminflt, majflt, cmajflt;
     string utime, stime, cutime, cstime, priority, nice;
     string O, itrealvalue, starttime;

     // the two fields we want
     //
     unsigned long vsize;
     long rss;

     stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
          >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt >> utime
          >> stime >> cutime >> cstime >> priority >> nice >> O >> itrealvalue
          >> starttime >> vsize >> rss; // don't care about the rest

     stat_stream.close();

     long page_size_kb
          = sysconf( _SC_PAGE_SIZE )
            / 1024; // in case x86-64 is configured to use 2MB pages
     vm_usage = static_cast< double >( vsize ) / 1024.0;
     resident_set = static_cast< double >( rss * page_size_kb );
}

static std::shared_ptr< json::prop > j_big_json_test;
static std::shared_ptr< std::stringstream > j_100mb_json_in_memory;
static const json::tree j_tree( {
     { "", "empty name" },
     { "one", 42 },
     { "bool , true", true },
     { "bool, false", false },
     { "null val", json::null() },
     { "two", "some string value" },
     { "three", -4.3 },
     { "sub tree",
       json::tree( {
            { "s_one", -24 },
            { "s_two", "sub tree string value" },
       } ) },
     { "array",
       json::array( {
            { 42 },
            { "Array str" },
            { -3.456e-8 },
            { 3.456E-8 },
            { json::tree( {
                 { "a_one", 12 },
                 { "a_two", "str" },
                 { "s_three", 1.4 },
            } ) },
            { json::array( {
                 32,
                 "str",
                 3.1e+13,
                 -3.1e+13,
                 true,
                 false,
                 json::null(),
            } ) },
            { json::array() },
            { json::tree() },
       } ) },
} );

static const json::array j_array( {
     { 42 },
     { "some string value" },
     { 4.3e+9 },
     { 4e-9 },
     { json::tree( {
          { "s_one", 24 },
          { "s_two", "sub tree string value" },
          { "empty arr", json::array() },
          { "empty obj", json::tree() },
          { "bool_true", true },
          { "bool_false", false },
     } ) },
     { json::array( {
          { 42 },
          { "Array str" },
          { 36.6 },
          true,
          false,
     } ) },
} );

void vec_print( const std::vector< std::string >& vec )
{
     for( auto&& str : vec )
     {
          std::cout << str << " : ";
     }
     std::cout << std::endl;
}

} // namespace

TEST( EXAMPLE, jajpa_cpp_example_parse )
{
     //Parse
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
}

TEST( EXAMPLE, jajpa_cpp_example_parse_without_throw )
{
     //Parse
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
}

TEST( EXAMPLE, jajpa_cpp_example_serialize )
{
     //serialization
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
}

TEST( BY_PATH, splice_path )
{
     namespace ja = infernal::jajpa_cpp;
     const std::string dotted_path = "test.one.two.three";

     std::vector< std::string > res
          = ja::by_path::path_splice( dotted_path ).splice( '.' );

     vec_print( res );
     ASSERT_EQ( res[ 0 ], "test" );
     ASSERT_EQ( res[ 1 ], "one" );
     ASSERT_EQ( res[ 2 ], "two" );
     ASSERT_EQ( res[ 3 ], "three" );
}

TEST( BY_PATH, splice_path_extra_sep )
{
     namespace ja = infernal::jajpa_cpp;
     const std::string dotted_path = ".test.one.two..three.";

     std::vector< std::string > res
          = ja::by_path::path_splice( dotted_path ).splice( '.' );

     vec_print( res );
     ASSERT_EQ( res[ 0 ], "" );
     ASSERT_EQ( res[ 1 ], "test" );
     ASSERT_EQ( res[ 2 ], "one" );
     ASSERT_EQ( res[ 3 ], "two" );
     ASSERT_EQ( res[ 4 ], "" );
     ASSERT_EQ( res[ 5 ], "three" );
}

TEST( BY_PATH, get_by_dotted_path )
{
     namespace ja = infernal::jajpa_cpp;
     std::string path = "sub tree.s_one"; // = -24

     auto val = ja::by_path( ja::json::prop( j_tree ) )
                    .get_or_throw( path )
                    .asVal< std::int64_t >();

     ASSERT_EQ( val, -24 );

     path = "array.4.a_one"; // = 12

     val = ja::by_path( ja::json::prop( j_tree ) )
               .get_or_throw( path )
               .asVal< std::int64_t >();

     ASSERT_EQ( val, 12 );

     path = "non.exists.path";
     ASSERT_THROW(
          ja::by_path( ja::json::prop( j_array ) ).get_or_throw( path ),
          ja::by_path::wrong_path );

     path = "array";
     ASSERT_NO_THROW(
          ja::by_path( ja::json::prop( j_tree ) ).get_or_throw( path ) );

     path = "one"; //42
     ASSERT_NO_THROW( ASSERT_EQ( ja::by_path( ja::json::prop( j_tree ) )
                                     .get_or_throw( path )
                                     .asVal< std::int64_t >(),
                                 42 ) );
}

TEST( BY_PATH, get_by_dotted_path_or_default )
{
     namespace ja = infernal::jajpa_cpp;
     std::string path = "sub tree.s_one"; // = -24

     auto val = ja::by_path( ja::json::prop( j_tree ) )
                    .get_or_default( path, std::int64_t( 0 ) );

     ASSERT_EQ( val, -24 );

     path = "array.4.a_one"; // = 12

     val = ja::by_path( ja::json::prop( j_tree ) )
               .get_or_default< std::int64_t >( path, 0 );

     ASSERT_EQ( val, 12 );

     path = "non.exists.path";
     ASSERT_EQ( ja::by_path( ja::json::prop( j_array ) )
                     .get_or_default< double >( path, 0 ),
                0 );

     path = "array";
     ASSERT_FALSE( ja::by_path( ja::json::prop( j_tree ) )
                        .get_or_default( path, ja::json::array() )
                        .empty() );
     path = "sub tree.s_two";
     const std::string def_str = "def str";
     auto val_str = ja::by_path( ja::json::prop( j_tree ) )
                         .get_or_default( path, def_str );
     ASSERT_NE( def_str, val_str );

     path = "non.exists.path";
     val_str = ja::by_path( ja::json::prop( j_tree ) )
                    .get_or_default( path, def_str );
     ASSERT_EQ( def_str, val_str );

     path = "sub tree.s_two";
     auto valD = ja::by_path( ja::json::prop( j_tree ) )
                     .get_or_default( path, -4.2 );

     ASSERT_EQ( -4.2, valD );
}

TEST( HelloTest, BasicAssertions )
{
     // Expect two strings not to be equal.
     EXPECT_STRNE( "hello", "world" );
     // Expect equality.
     EXPECT_EQ( 7 * 6, 42 );
}

TEST( PROPERTY, default_constructor )
{
     property< bool > p_bool;
     ASSERT_EQ( p_bool.to(), false );

     property< char > p_char;
     ASSERT_EQ( p_char.to(), 0 );

     property< int > p_int;
     ASSERT_EQ( p_int.to(), 0 );

     property< long int > p_long;
     ASSERT_EQ( p_long.to(), 0 );

     property< float > p_float;
     ASSERT_EQ( p_float.to(), 0 );

     property< double > p_double;
     ASSERT_EQ( p_double.to(), 0 );

     property< std::string > p_str;
     ASSERT_EQ( p_str.to(), std::string() );

     property< std::vector< int > > p_vec;
     std::vector< int > empty;
     ASSERT_EQ( p_vec.to(), empty );

     property< std::map< char, std::string > > p_map;
     std::map< char, std::string > empty_map;
     ASSERT_EQ( p_map.to(), empty_map );
}

TEST( PROPERTY, cast_check )
{
     constexpr int exp_int = 5;

     base_property* p_except = new property< char >( exp_int );
     ASSERT_EQ( true, p_except->is_type< char >() );
     ASSERT_THROW( p_except->as< int >(), base_property::cast_error );

     base_property* p_int = new property< int >( exp_int );
     int res_int = 0;
     ASSERT_NO_THROW( res_int = p_int->as< int >() );
     ASSERT_EQ( exp_int, res_int );

     constexpr float exp_float = 36.6f;
     base_property* p_float = new property< float >( exp_float );
     float res_float = 0;
     ASSERT_NO_THROW( res_float = p_float->as< float >() );
     ASSERT_EQ( exp_float, res_float );

     constexpr double exp_double = 42.1;
     base_property* p_double = new property< double >( exp_double );
     double res_double = 0;
     ASSERT_NO_THROW( res_double = p_double->as< double >() );
     ASSERT_EQ( exp_double, res_double );

     const std::vector< std::string > exp_vec_str = { "one", " two", "!" };
     base_property* p_vec_str
          = new property< std::vector< std::string > >( exp_vec_str );
     std::vector< std::string > res_vec_str;
     ASSERT_NO_THROW( res_vec_str
                      = p_vec_str->as< std::vector< std::string > >() );
     ASSERT_EQ( exp_vec_str, res_vec_str );

     const std::string exp_str = "EXP Str";
     base_property* p_str = new property< std::string >( exp_str );
     std::string res_str;
     ASSERT_NO_THROW( res_str = p_str->as< std::string >().to() );
     ASSERT_EQ( exp_str, res_str );
     ASSERT_EQ( true, p_str->is_type< std::string >() );

     base_property* p_map
          = new property< std::map< std::string, base_property* > >(
               { { "one", new property< int >( exp_int ) },
                 { "two", new property< std::string >( exp_str ) } } );
     const auto& map
          = p_map->as< std::map< std::string, base_property* > >().to();
     ASSERT_EQ( exp_int, map.at( "one" )->as< int >().to() );
     ASSERT_EQ( exp_str, map.at( "two" )->as< std::string >().to() );
     bool is_map = p_map->is_type< std::map< std::string, base_property* > >();
     ASSERT_EQ( true, is_map );
     ASSERT_EQ( false, p_map->is_type< std::string >() );
}

TEST( PROPERTY, to_stream )
{
     base_property* p_int = new property< int >( 2 );
     std::stringstream ss_int;
     p_int->to_stream( ss_int );
     ASSERT_EQ( "2", ss_int.str() );

     base_property* p_str = new property< std::string >( "test" );
     std::stringstream ss_str;
     p_str->to_stream( ss_str );
     ASSERT_EQ( "test", ss_str.str() );

     base_property* p_arr = new property< std::vector< base_property* > >();
     std::stringstream ss_vec;
     p_arr->to_stream( ss_vec );
     //prints typeid().name();
     std::cout << ss_vec.str() << std::endl;
}

TEST( JSON_TREE, json_type_hash )
{
     std::unordered_map< std::size_t, std::size_t, json::type_hash > test( {
          { typeid( int ).hash_code(), 0 },
          { typeid( std::int64_t ).hash_code(), 1 },
          { typeid( double ).hash_code(), 2 },
          { typeid( std::string ).hash_code(), 3 },
          { typeid( json::array ).hash_code(), 4 },
          { typeid( json::tree ).hash_code(), 5 },
     } );

     std::vector< std::size_t > res( {
          test[ typeid( int ).hash_code() ],
          test[ typeid( std::int64_t ).hash_code() ],
          test[ typeid( double ).hash_code() ],
          test[ typeid( std::string ).hash_code() ],
          test[ typeid( json::array ).hash_code() ],
          test[ typeid( json::tree ).hash_code() ],
     } );

     for( std::size_t i = 0; i < res.size(); ++i )
     {
          ASSERT_EQ( i, res[ i ] );
     }
}

TEST( JSON_TREE, stringify_object )
{
     std::stringstream ss;
     json().stringify( ss, j_tree );
     std::cout << ss.str() << std::endl;

     std::stringstream ss1;
     json().stringify( ss1, j_tree, true );
     std::cout << ss1.str() << std::endl;
}

TEST( JSON_TREE, stringify_NAN )
{
     std::stringstream ss;
     json().stringify( ss, j_tree );
     std::cout << ss.str() << std::endl;

     std::stringstream ss1;
     json().stringify( ss1, j_tree, true );
     std::cout << ss1.str() << std::endl;
}

TEST( JSON_TREE, stringify_array )
{
     std::stringstream ss;
     json().stringify( ss, j_tree );
     std::cout << ss.str() << std::endl;

     std::stringstream ss1;
     json().stringify( ss1, j_tree, true );
     std::cout << ss1.str() << std::endl;
}

TEST( JSON_PARSER, parse_tree )
{
     std::stringstream ss_tree;
     json().stringify( ss_tree, j_tree );
     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_NO_THROW( result = parser().parse( ss_tree ) );

     std::stringstream ss_result;
     json().stringify( ss_result, *result );

     std::cout << "result:\n\t" << ss_result.str() << std::endl;
}

TEST( JSON_PARSER, parse_tree_with_ident )
{
     std::stringstream ss_tree;
     json().stringify( ss_tree, j_tree, true );
     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_NO_THROW( result = parser().parse( ss_tree ) );

     std::stringstream ss_result;
     json().stringify( ss_result, *result, true );

     std::cout << "result:\n\t" << ss_result.str() << std::endl;
}

TEST( JSON_PARSER, parse_array )
{
     std::stringstream ss_array;
     json().stringify( ss_array, j_array );
     std::cout << "sample:\n\t" << ss_array.str() << std::endl;

     ss_array.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_NO_THROW( result = parser().parse( ss_array ) );

     std::stringstream ss_result;
     json().stringify( ss_result, *result );

     std::cout << "result:\n\t" << ss_result.str() << std::endl;
}

TEST( JSON_PARSER, parse_empty_obj )
{
     std::stringstream ss_tree( "{}" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_NO_THROW( result = parser().parse( ss_tree ) );

     std::stringstream ss_result;
     json().stringify( ss_result, *result );

     std::cout << "result:\n\t" << ss_result.str() << std::endl;
}

TEST( JSON_PARSER, parse_empty_array )
{
     std::stringstream ss_array( "[]" );

     std::cout << "sample:\n\t" << ss_array.str() << std::endl;

     ss_array.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_NO_THROW( result = parser().parse( ss_array ) );

     std::stringstream ss_result;
     json().stringify( ss_result, *result );

     std::cout << "result:\n\t" << ss_result.str() << std::endl;
}

TEST( JSON_PARSER, parse_obj_with_spaces )
{
     std::stringstream ss_tree( " {'test' : 'result ok' , 'second ': 23, }" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_NO_THROW( result = parser().parse( ss_tree ) );

     std::stringstream ss_result;
     json().stringify( ss_result, *result );

     std::cout << "result:\n\t" << ss_result.str() << std::endl;
}

TEST( JSON_PARSER, parse_obj_with_to_much_trailng )
{
     std::stringstream ss_tree(
          " {'test' : 'result ok' , 'second ': 23,,,, }" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_tree ), parser::syntax_error );
}

TEST( JSON_PARSER, parse_array_with_spaces )
{
     std::stringstream ss_array(
          "['test' , 'result', 32 ,'23 ', null , true, false,  ]" );

     std::cout << "sample:\n\t" << ss_array.str() << std::endl;

     ss_array.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_NO_THROW( result = parser().parse( ss_array ) );

     std::stringstream ss_result;
     json().stringify( ss_result, *result );

     std::cout << "result:\n\t" << ss_result.str() << std::endl;
}

TEST( JSON_PARSER, parse_empty_key )
{
     std::stringstream ss_tree( "{'': 1}" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_NO_THROW( result = parser().parse( ss_tree ) );

     std::stringstream ss_result;
     json().stringify( ss_result, *result );

     std::cout << "result:\n\t" << ss_result.str() << std::endl;
}

TEST( JSON_PARSER, parse_slashes )
{
     std::stringstream ss_tree( "{'\\\\\\\\': '\\\''}" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_NO_THROW( result = parser().parse( ss_tree ) );

     std::stringstream ss_result;
     json().stringify( ss_result, *result );

     std::cout << "result:\n\t" << ss_result.str() << std::endl;
}

// #to activate big json tests get tis files
// #https://awesomeopensource.com/project/jdorfman/awesome-json-datasets
// #configure_file("test_data/big_test.json" "${CMAKE_CURRENT_BINARY_DIR}/big_test.json" COPYONLY)
// #https://github.com/seductiveapps/largeJSON/blob/master/100mb.json
// #configure_file("test_data/100mb_test.json" "${CMAKE_CURRENT_BINARY_DIR}/100mb_test.json" COPYONLY)
TEST( JSON_PARSER, parse_big_json )
{
     ASSERT_FALSE( j_big_json_test );

     double resident, virt;

     process_mem_usage( virt, resident );

     std::cout << "before parse res:" << resident << "kb" << std::endl;
     std::ifstream fs_tree( "big_test.json" );

     ASSERT_TRUE( fs_tree.is_open() );

     ASSERT_NO_THROW( j_big_json_test = parser().parse( fs_tree ) );

     double resident_aft, virt_aft;
     process_mem_usage( virt_aft, resident_aft );
     std::cout << "parsed res:" << resident_aft << "kb" << std::endl
               << "change:" << resident_aft - resident << "kb" << std::endl;

     fs_tree.close();
}

// #to activate big json tests get tis files
// #https://awesomeopensource.com/project/jdorfman/awesome-json-datasets
// #configure_file("test_data/big_test.json" "${CMAKE_CURRENT_BINARY_DIR}/big_test.json" COPYONLY)
// #https://github.com/seductiveapps/largeJSON/blob/master/100mb.json
// #configure_file("test_data/100mb_test.json" "${CMAKE_CURRENT_BINARY_DIR}/100mb_test.json" COPYONLY)
TEST( JSON_PARSER, serialize_big_json )
{
     ASSERT_TRUE( j_big_json_test );
     std::ofstream fs_result( "result_big_test.json" );
     ASSERT_TRUE( fs_result.is_open() );

     ASSERT_NO_THROW( json().stringify( fs_result, *j_big_json_test, true ) );

     fs_result.close();
     j_big_json_test = nullptr;
}

// #to activate big json tests get tis files
// #https://awesomeopensource.com/project/jdorfman/awesome-json-datasets
// #configure_file("test_data/big_test.json" "${CMAKE_CURRENT_BINARY_DIR}/big_test.json" COPYONLY)
// #https://github.com/seductiveapps/largeJSON/blob/master/100mb.json
// #configure_file("test_data/100mb_test.json" "${CMAKE_CURRENT_BINARY_DIR}/100mb_test.json" COPYONLY)
TEST( JSON_PARSER, parse_100mb_json )
{
     ASSERT_FALSE( j_big_json_test );
     double resident, virt;

     process_mem_usage( virt, resident );

     std::cout << "before parse res:" << resident << "kb" << std::endl;
     std::ifstream fs_tree( "100mb_test.json" );

     ASSERT_TRUE( fs_tree.is_open() );

     /*DEBUG
     fs_tree.seekg( 1262778 - 20 );
     for( int i = 0; i < 40; ++i )
     {
          if( i == 20 || i == 21 )
          {
               std::cout << "Ж";
          }
          char ch;
          fs_tree.get( ch );
          std::cout << ch;
     }
     std::cout << std::endl;
     fs_tree.seekg( 0 );
     */
     ASSERT_NO_THROW( j_big_json_test = parser().parse( fs_tree ) );
     double resident_aft, virt_aft;
     process_mem_usage( virt_aft, resident_aft );
     std::cout << "parsed res:" << resident_aft << "kb" << std::endl
               << "change:" << resident_aft - resident << "kb" << std::endl;
     fs_tree.close();
}

// #to activate big json tests get tis files
// #https://awesomeopensource.com/project/jdorfman/awesome-json-datasets
// #configure_file("test_data/big_test.json" "${CMAKE_CURRENT_BINARY_DIR}/big_test.json" COPYONLY)
// #https://github.com/seductiveapps/largeJSON/blob/master/100mb.json
// #configure_file("test_data/100mb_test.json" "${CMAKE_CURRENT_BINARY_DIR}/100mb_test.json" COPYONLY)
TEST( JSON_PARSER, serialize_100mb_json )
{
     ASSERT_TRUE( j_big_json_test );
     std::ofstream fs_result( "result_100bm_test.json" );
     ASSERT_TRUE( fs_result.is_open() );

     ASSERT_NO_THROW( json().stringify( fs_result, *j_big_json_test, true ) );

     fs_result.close();
     j_big_json_test = nullptr;
}

// #to activate big json tests get tis files
// #https://awesomeopensource.com/project/jdorfman/awesome-json-datasets
// #configure_file("test_data/big_test.json" "${CMAKE_CURRENT_BINARY_DIR}/big_test.json" COPYONLY)
// #https://github.com/seductiveapps/largeJSON/blob/master/100mb.json
// #configure_file("test_data/100mb_test.json" "${CMAKE_CURRENT_BINARY_DIR}/100mb_test.json" COPYONLY)
TEST( JSON_PARSER, 100mb_json_after_parse )
{
     ASSERT_FALSE( j_big_json_test );
     double resident, virt;

     process_mem_usage( virt, resident );

     std::cout << "before parse res:" << resident << "kb" << std::endl;
     std::ifstream fs_tree( "result_100bm_test.json" );

     ASSERT_TRUE( fs_tree.is_open() );

     ASSERT_NO_THROW( j_big_json_test = parser().parse( fs_tree ) );
     double resident_aft, virt_aft;
     process_mem_usage( virt_aft, resident_aft );
     std::cout << "parsed res:" << resident_aft << "kb" << std::endl
               << "change:" << resident_aft - resident << "kb" << std::endl;
     fs_tree.close();
}

// #to activate big json tests get tis files
// #https://awesomeopensource.com/project/jdorfman/awesome-json-datasets
// #configure_file("test_data/big_test.json" "${CMAKE_CURRENT_BINARY_DIR}/big_test.json" COPYONLY)
// #https://github.com/seductiveapps/largeJSON/blob/master/100mb.json
// #configure_file("test_data/100mb_test.json" "${CMAKE_CURRENT_BINARY_DIR}/100mb_test.json" COPYONLY)
TEST( JSON_PARSER, serialize_100mb_json_after_parse )
{
     ASSERT_TRUE( j_big_json_test );
     std::ofstream fs_result( "result_100bm_test_after.json" );
     ASSERT_TRUE( fs_result.is_open() );

     ASSERT_NO_THROW( json().stringify( fs_result, *j_big_json_test ) );

     fs_result.close();
     j_big_json_test = nullptr;
}

// #to activate big json tests get tis files
// #https://awesomeopensource.com/project/jdorfman/awesome-json-datasets
// #configure_file("test_data/big_test.json" "${CMAKE_CURRENT_BINARY_DIR}/big_test.json" COPYONLY)
// #https://github.com/seductiveapps/largeJSON/blob/master/100mb.json
// #configure_file("test_data/100mb_test.json" "${CMAKE_CURRENT_BINARY_DIR}/100mb_test.json" COPYONLY)
TEST( JSON_PARSER, parse_100mb_json_from_ram_preload )
{
     ASSERT_FALSE( j_100mb_json_in_memory );
     double resident, virt;

     process_mem_usage( virt, resident );

     std::cout << "before load res:" << resident << "kb" << std::endl;
     std::ifstream fs_tree( "100mb_test.json" );

     ASSERT_TRUE( fs_tree.is_open() );

     j_100mb_json_in_memory = std::make_shared< std::stringstream >();
     *j_100mb_json_in_memory << fs_tree.rdbuf();
     fs_tree.close();

     double resident_aft, virt_aft;
     process_mem_usage( virt_aft, resident_aft );
     std::cout << "loaded res:" << resident_aft << "kb" << std::endl
               << "change:" << resident_aft - resident << "kb" << std::endl;
}

// #to activate big json tests get tis files
// #https://awesomeopensource.com/project/jdorfman/awesome-json-datasets
// #configure_file("test_data/big_test.json" "${CMAKE_CURRENT_BINARY_DIR}/big_test.json" COPYONLY)
// #https://github.com/seductiveapps/largeJSON/blob/master/100mb.json
// #configure_file("test_data/100mb_test.json" "${CMAKE_CURRENT_BINARY_DIR}/100mb_test.json" COPYONLY)
TEST( JSON_PARSER, parse_100mb_json_from_ram )
{
     ASSERT_TRUE( j_100mb_json_in_memory );
     double resident, virt = 0;
     std::ignore = virt;

     process_mem_usage( virt, resident );

     std::cout << "before parse res:" << resident << "kb" << std::endl;

     ASSERT_NO_THROW( j_big_json_test
                      = parser().parse( *j_100mb_json_in_memory ) );
     double resident_aft, virt_aft;
     process_mem_usage( virt_aft, resident_aft );
     std::cout << "parsed res:" << resident_aft << "kb" << std::endl
               << "change:" << resident_aft - resident << "kb" << std::endl;
}

TEST( JSON_PARSER_ERROR, parse_utf8 )
{
     std::stringstream ss_tree( "['€?']" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_NO_THROW( result = parser().parse( ss_tree ) );

     std::stringstream ss_result;
     json().stringify( ss_result, *result );

     std::cout << "result:\n\t" << ss_result.str() << std::endl;
}

TEST( JSON_PARSER_ERROR, parse_u0000 )
{
     std::stringstream ss_tree( "['\\u0000']" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_NO_THROW( result = parser().parse( ss_tree ) );

     std::stringstream ss_result;
     json().stringify( ss_result, *result );

     std::cout << "result:\n\t" << ss_result.str() << std::endl;
}

TEST( JSON_PARSER_ERROR, parse_duplicate_key )
{
     std::stringstream ss_tree( "{'test': 1, 'test':2}" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_tree ), parser::syntax_error );
}

TEST( JSON_PARSER_ERROR, parse_array_with_spaces_in_number )
{
     std::stringstream ss_array( "['test' , 'result', 32 ,'23 ',  54 32 ]" );

     std::cout << "sample:\n\t" << ss_array.str() << std::endl;

     ss_array.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_array ),
                   parser::unexpected_symbol );

     ASSERT_EQ( result, nullptr );
}

TEST( JSON_PARSER_ERROR, parse_not_closed_structs )
{
     std::stringstream ss_tree(
          " {'test' : 'result ok' , 'not closed' : { 'second ': 23,  }" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_tree ), parser::syntax_error );
}

TEST( JSON_PARSER_ERROR, parse_not_closed_structs_without_comma )
{
     std::stringstream ss_tree(
          " {'test' : 'result ok' , 'not closed' : { 'second ': 23  }" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_tree ), parser::syntax_error );
}

TEST( JSON_PARSER_ERROR, parse_not_closed_structs_empty_obj )
{
     std::stringstream ss_tree( "[{]" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_tree ),
                   parser::unexpected_brace_sequence );
}

TEST( JSON_PARSER_ERROR, parse_not_closed_structs_array )
{
     std::stringstream ss_tree(
          " {'test' : 'result ok' , 'not closed' : [ 23,  }" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_tree ),
                   parser::unexpected_brace_sequence );
}

TEST( JSON_PARSER_ERROR, parse_not_closed_structs_array_2 )
{
     std::stringstream ss_tree( " ['result ok' ,  [ 23,  ]," );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_tree ), parser::unexpected_eof );
}

TEST( JSON_PARSER_ERROR, parse_not_closed_structs_array_3 )
{
     std::stringstream ss_tree( " ['result ok' ,  {'fgfg' : 23,  ]" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_tree ),
                   parser::unexpected_brace_sequence );
}

TEST( JSON_PARSER_ERROR, parse_not_closed_structs_empty_array )
{
     std::stringstream ss_tree( "[[]  " );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_tree ), parser::unexpected_eof );
}

TEST( JSON_PARSER_ERROR, parse_not_closed_structs_empty_array_2 )
{
     std::stringstream ss_tree( "[}]" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_tree ),
                   parser::unexpected_brace_sequence );
}

TEST( JSON_PARSER_ERROR, parse_double_colon )
{
     std::stringstream ss_tree( "{'test' :: 2}" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_tree ),
                   parser::unexpected_symbol );
}

TEST( JSON_PARSER_ERROR, parse_missing_key )
{
     std::stringstream ss_tree( "{: 2}" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_tree ),
                   parser::missing_symbols );
}

TEST( JSON_PARSER_ERROR, parse_non_string_key )
{
     std::stringstream ss_tree( "{1: 2}" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     ASSERT_THROW( result = parser().parse( ss_tree ),
                   parser::missing_symbols );
}

TEST( JSON_PARSER_ERROR, parse_check_unexpected_symbol_1 )
{
     std::stringstream ss_tree( "{'test': ddd}" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     auto exec = [ & ]() {
          try
          {
               result = parser().parse( ss_tree );
          }
          catch( const parser::unexpected_symbol& e )
          {
               std::cout << e.what() << std::endl;
               ASSERT_EQ( e.pos, 10 );
          }
          catch( const std::runtime_error& e )
          {
               throw e;
          }
     };

     ASSERT_NO_THROW( exec() );
}

TEST( JSON_PARSER_ERROR, parse_check_unexpected_symbol_2 )
{
     std::stringstream ss_tree( "{'test': 2dd}" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     auto exec = [ & ]() {
          try
          {
               result = parser().parse( ss_tree );
          }
          catch( const parser::unexpected_symbol& e )
          {
               std::cout << e.what() << std::endl;
               ASSERT_EQ( e.pos, 11 );
          }
          catch( const std::runtime_error& e )
          {
               throw e;
          }
     };

     ASSERT_NO_THROW( exec() );
}

TEST( JSON_PARSER_ERROR, parse_check_unexpected_symbol_3 )
{
     std::stringstream ss_tree( "{'test': 2 { }" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;
     auto exec = [ & ]() {
          try
          {
               result = parser().parse( ss_tree );
          }
          catch( const parser::unexpected_symbol& e )
          {
               std::cout << e.what() << std::endl;
               ASSERT_EQ( e.pos, 12 );
          }
          catch( const std::runtime_error& e )
          {
               throw e;
          }
     };

     ASSERT_NO_THROW( exec() );
}

TEST( JSON_PARSER_ERROR, parse_check_to_long_num )
{
     std::stringstream ss_tree( "{'test': 1234567890123456789012345678 }" );

     std::cout << "sample:\n\t" << ss_tree.str() << std::endl;

     ss_tree.seekg( 0 );

     std::shared_ptr< json::prop > result;

     ASSERT_NO_THROW( result = parser().parse( ss_tree ); );

     std::stringstream ss_res;
     json().stringify( ss_res, *result );

     std::cout << "result:\n\t" << ss_res.str() << std::endl;
}

//////////////////////////////////////////////////////////////////////////////
// RFC 8259 conformance tests
// https://www.rfc-editor.org/rfc/rfc8259
//////////////////////////////////////////////////////////////////////////////

TEST( JSON_SPEC_NUMBERS, parse_integer_values_as_int64 )
{
     std::stringstream in( "{\"i\":42,\"n\":-24,\"max\":9223372036854775807}" );
     std::shared_ptr< json::prop > root;
     ASSERT_NO_THROW( root = parser().parse( in ) );

     const json::tree& tree = root->asVal< json::tree >();

     ASSERT_TRUE( tree.at( "i" ).value()->is_type< std::int64_t >() );
     ASSERT_EQ( tree.at( "i" ).asVal< std::int64_t >(), std::int64_t( 42 ) );

     ASSERT_TRUE( tree.at( "n" ).value()->is_type< std::int64_t >() );
     ASSERT_EQ( tree.at( "n" ).asVal< std::int64_t >(), std::int64_t( -24 ) );

     ASSERT_TRUE( tree.at( "max" ).value()->is_type< std::int64_t >() );
     ASSERT_EQ( tree.at( "max" ).asVal< std::int64_t >(), std::int64_t( 9223372036854775807LL ) );
}

TEST( JSON_SPEC_NUMBERS, parse_fraction_and_exponent_as_double )
{
     std::stringstream in( "{\"d\":4.3,\"e\":1e5,\"n\":-3.456e-8}" );
     std::shared_ptr< json::prop > root;
     ASSERT_NO_THROW( root = parser().parse( in ) );

     const json::tree& tree = root->asVal< json::tree >();

     ASSERT_TRUE( tree.at( "d" ).value()->is_type< double >() );
     ASSERT_DOUBLE_EQ( tree.at( "d" ).asVal< double >(), 4.3 );

     ASSERT_TRUE( tree.at( "e" ).value()->is_type< double >() );
     ASSERT_DOUBLE_EQ( tree.at( "e" ).asVal< double >(), 1e5 );

     ASSERT_TRUE( tree.at( "n" ).value()->is_type< double >() );
     ASSERT_DOUBLE_EQ( tree.at( "n" ).asVal< double >(), -3.456e-8 );
}

TEST( JSON_SPEC_NUMBERS, accepts_arbitrary_precision_number )
{
     std::stringstream in( "{\"big\":9223372036854775808}" );
     std::shared_ptr< json::prop > root;
     ASSERT_NO_THROW( root = parser().parse( in ) );

     const json::tree& tree = root->asVal< json::tree >();
     ASSERT_TRUE( tree.at( "big" ).value()->is_type< double >()
                 || tree.at( "big" ).value()->is_type< std::int64_t >() );
}

TEST( JSON_SPEC_LITERALS, parse_bool_and_null )
{
     std::stringstream in( "[true,false,null]" );
     std::shared_ptr< json::prop > root;
     ASSERT_NO_THROW( root = parser().parse( in ) );

     const json::array& arr = root->asVal< json::array >();
     ASSERT_EQ( arr.size(), std::size_t( 3 ) );

     ASSERT_TRUE( arr.at( 0 ).value()->is_type< bool >() );
     ASSERT_TRUE( arr.at( 0 ).asVal< bool >() );
     ASSERT_TRUE( arr.at( 1 ).value()->is_type< bool >() );
     ASSERT_FALSE( arr.at( 1 ).asVal< bool >() );
     ASSERT_TRUE( arr.at( 2 ).value()->is_type< json::null >() );
}

TEST( JSON_SPEC_ESCAPES, decode_all_basic_escapes )
{
     std::string json = "[\"";
     json += "\\\""; // escaped quote
     json += "\\\\"; // escaped backslash
     json += "\\/";  // escaped solidus
     json += "\\b\\f\\n\\r\\t";
     json += "\"]";

     std::stringstream in( json );
     std::shared_ptr< json::prop > root;
     ASSERT_NO_THROW( root = parser().parse( in ) );

     std::string expected;
     expected += '"';
     expected += '\\';
     expected += '/';
     expected += '\b';
     expected += '\f';
     expected += '\n';
     expected += '\r';
     expected += '\t';

     ASSERT_EQ( root->asVal< json::array >().at( 0 ).asVal< std::string >(), expected );
}

TEST( JSON_SPEC_ESCAPES, decode_unicode_bmp )
{
     std::stringstream in( "[\"\\u0041\\u00e9\"]" );
     std::shared_ptr< json::prop > root;
     ASSERT_NO_THROW( root = parser().parse( in ) );

     std::string expected = "A";
     expected += static_cast< char >( 0xC3 );
     expected += static_cast< char >( 0xA9 );

     ASSERT_EQ( root->asVal< json::array >().at( 0 ).asVal< std::string >(), expected );
}

TEST( JSON_SPEC_ESCAPES, decode_surrogate_pair )
{
     std::stringstream in( "[\"\\uD83D\\uDE00\"]" );
     std::shared_ptr< json::prop > root;
     ASSERT_NO_THROW( root = parser().parse( in ) );

     std::string expected;
     expected += static_cast< char >( 0xF0 );
     expected += static_cast< char >( 0x9F );
     expected += static_cast< char >( 0x98 );
     expected += static_cast< char >( 0x80 );

     ASSERT_EQ( root->asVal< json::array >().at( 0 ).asVal< std::string >(), expected );
}

TEST( JSON_SPEC_ESCAPES, rejects_invalid_escape )
{
     std::stringstream bad_escape( "[\"\\q\"]" );
     ASSERT_THROW( parser().parse( bad_escape ), parser::syntax_error );

     std::stringstream short_unicode( "[\"\\u12\"]" );
     ASSERT_THROW( parser().parse( short_unicode ), parser::syntax_error );
}

TEST( JSON_SPEC_ESCAPES, serialize_escapes_control_and_specials )
{
     json::array arr;
     arr.push_back( std::string( "a\"b\\c\b\f\n\r\t\x01" ) );

     std::ostringstream out;
     json().stringify( out, arr );

     ASSERT_EQ( out.str(), "[\"a\\\"b\\\\c\\b\\f\\n\\r\\t\\u0001\"]" );
}

TEST( JSON_SPEC_ESCAPES, roundtrip_array_with_escapes )
{
     json::array arr;
     arr.push_back( std::int64_t( 42 ) );
     arr.push_back( std::string( "a\nb\t\"c\\" ) );
     arr.push_back( true );
     arr.push_back( json::null() );
     arr.push_back( 4.3 );

     std::ostringstream first;
     json().stringify( first, arr );

     std::stringstream in( first.str() );
     std::shared_ptr< json::prop > root;
     ASSERT_NO_THROW( root = parser().parse( in ) );

     std::ostringstream second;
     json().stringify( second, *root );

     ASSERT_EQ( first.str(), second.str() );
}

TEST( JSON_SPEC_STRUCTURE, rejects_trailing_tokens )
{
     std::stringstream trailing( "{} x" );
     ASSERT_THROW( parser().parse( trailing ), parser::syntax_error );

     std::stringstream trailing_number( "[] 1" );
     ASSERT_THROW( parser().parse( trailing_number ), parser::syntax_error );

     std::stringstream trailing_ws( "[]\n\t \r" );
     ASSERT_NO_THROW( parser().parse( trailing_ws ) );
}

TEST( JSON_SPEC_STRUCTURE, rejects_unescaped_control_in_string )
{
     std::stringstream tab( "[\"a\tb\"]" );
     ASSERT_THROW( parser().parse( tab ), parser::syntax_error );

     std::stringstream cr( "[\"a\rb\"]" );
     ASSERT_THROW( parser().parse( cr ), parser::syntax_error );
}
