
# Just Another Json Parser C++ (jajpa_cpp)
## RFC 8259 realization

## License
**WTFPL v2.0**

*This program is free software. It comes without any warranty, to the extent permitted by applicable law. You can redistribute it and/or modify it under the terms of the Do What The Fuck You Want To Public License, Version 2, as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.*

## Usage
### requires 
        C++17\STL or newer 
Just include **jajpa_cpp/jajpa_cpp.h** to your project.

## TESTS 
### requires 
        cmake, GTESTS 

clone and build and run from jajpa_cpp/CMakeLists.txt (look at build_and_run_tests.sh)

```bash
#run in project root dir
#Specify your own paths 
LIB_STD_17=/usr/local/lib64
CXX_COMPILER=/usr/local/bin/g++
C_COMPILER=/usr/local/bin/gcc


export LD_LIBRARY_PATH=${LIB_STD_17}:${LD_LIBRARY_PATH}
rm -r build_tests
mkdir build_tests
cd build_tests

cmake ../jajpa_cpp/ -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=${CXX_COMPILER} -DCMAKE_C_COMPILER=${C_COMPILER}
cmake --build .

./jsonparse_test


```



## Examples 
### Serialization
```c++
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
```c++
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

```c++
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
