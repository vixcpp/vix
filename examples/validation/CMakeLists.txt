cmake_minimum_required(VERSION 3.20)

project(vix_example_validation LANGUAGES CXX)

find_package(Vix CONFIG REQUIRED)

set(_vix_validation_examples
  basemodel_basic
  basemodel_cross_field_check
  basemodel_static_validate
  form_bind2_generic_error
  form_cleaned_output
  form_kv_basic
  form_parsed_age
  in_set
  numeric
  optional
  parsed
  schema
  schema_cross_field_check
  schema_fieldspec_basic
  schema_lambda_builder
  simple_string
  validate_enum
  validate_parsed_int
  validate_string
)

set(_vix_validation_expected_rejections
  basemodel_basic
  basemodel_cross_field_check
  basemodel_static_validate
  form_kv_basic
  numeric
  optional
  schema
  schema_cross_field_check
  schema_fieldspec_basic
  schema_lambda_builder
)

enable_testing()

foreach (_example IN LISTS _vix_validation_examples)
  add_executable("validation_${_example}" "${_example}.cpp")
  target_compile_features("validation_${_example}" PRIVATE cxx_std_20)
  target_link_libraries("validation_${_example}" PRIVATE vix::validation)
  add_test(NAME "validation.${_example}" COMMAND "validation_${_example}")

  if (_example IN_LIST _vix_validation_expected_rejections)
    set_tests_properties("validation.${_example}" PROPERTIES WILL_FAIL TRUE)
  endif()
endforeach()
