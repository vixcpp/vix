cmake_minimum_required(VERSION 3.20)

project(vix_example_template LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_template_examples
  02_loop_features
  03_if_else
  04_layout_extends
  05_include_partial
  06_filters_and_escape
  07_shop_dashboard
  08_blog_home
  09_blog_post_page
  10_docs_page
  11_marketing_landing_page
  12_admin_dashboard
)

enable_testing()

foreach (_example IN LISTS _vix_template_examples)
  add_executable("template_${_example}" "${_example}.cpp")
  target_compile_features("template_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("template_${_example}" PRIVATE vix::core vix::template)
endforeach()
