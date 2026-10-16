#include <vix/process/ProcessDomain.hpp>

#include <vix/detail/AllocationFailure.hpp>

#include <atomic>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vix::process
{
  struct detail_process_record_access
  {
    static ProcessId id(std::uint64_t generation, std::uint64_t native) noexcept { return ProcessId{generation, native}; }
    static ExitStatus exited(std::uint32_t code) noexcept { return ExitStatus{ExitKind::exited, code}; }
    static ExitStatus abnormal() noexcept { return ExitStatus{ExitKind::abnormal, std::nullopt}; }
  };
  namespace
  {
    [[noreturn]] void contract_violation() noexcept { std::terminate(); }
    [[nodiscard]] Error error(ErrorIdentity identity, const char *text)
    { return vix::detail::allocation_or_terminate([&] { return Error{identity, text}; }); }
    [[nodiscard]] bool valid_utf8(std::string_view value) noexcept
    {
      for (std::size_t i=0;i<value.size();)
      {
        const auto lead=static_cast<unsigned char>(value[i]);
        if(lead<=0x7f){++i;continue;}
        const std::size_t count=lead>=0xc2&&lead<=0xdf?2:lead>=0xe0&&lead<=0xef?3:lead>=0xf0&&lead<=0xf4?4:0;
        if(count==0||i+count>value.size())return false;
        std::uint32_t code=count==2?(lead&0x1f):count==3?(lead&0x0f):(lead&0x07);
        for(std::size_t j=1;j<count;++j)
        {
          const auto next=static_cast<unsigned char>(value[i+j]);
          if((next&0xc0)!=0x80)return false;
          code=(code<<6)|(next&0x3f);
        }
        if((count==2&&code<0x80)||(count==3&&code<0x800)||(count==4&&code<0x10000)||
           (code>=0xd800&&code<=0xdfff)||code>0x10ffff)return false;
        i+=count;
      }
      return true;
    }
  }
  struct Executable::State { bool search{}; files::Path path; std::string name; State(bool s,files::Path p,std::string n):search(s),path(std::move(p)),name(std::move(n)){} };
  struct Environment::State { struct Change{std::string name;std::optional<std::string> value;}; bool inherited{};std::vector<Change> changes;explicit State(bool v):inherited(v){} };
  struct ProcessSpec::State
  {
    Executable executable;std::vector<std::string> arguments;Environment environment;
    std::optional<files::Path> working_directory;StandardInput input{StandardInput::inherit()};
    StandardOutput output{StandardOutput::inherit()};StandardError error{StandardError::inherit()};
    explicit State(Executable value):executable(std::move(value)),environment(Environment::inherit()){}
  };
  namespace detail { struct ProcessRecord {}; }

  std::string ProcessId::display() const { return vix::detail::allocation_or_terminate([&]{return std::to_string(generation_)+":"+std::to_string(native_);}); }
  Executable::Executable(std::shared_ptr<const State> value) noexcept:state_(std::move(value)){}
  Executable::Executable(const Executable&)=default;Executable::Executable(Executable&&) noexcept=default;
  Executable& Executable::operator=(const Executable&)=default;Executable& Executable::operator=(Executable&&) noexcept=default;Executable::~Executable()=default;
  Result<Executable> Executable::path(files::Path value)
  {if(value.empty())return error(errors::invalid_argument,"executable path is empty");return vix::detail::allocation_or_terminate([&]{return Executable{std::make_shared<State>(false,std::move(value),"")};});}
  Result<Executable> Executable::search_path(std::string_view name)
  {
    if(name.empty()||name.find('\0')!=std::string_view::npos||!valid_utf8(name)||name.find('/')!=std::string_view::npos||name.find('\\')!=std::string_view::npos)return error(errors::invalid_argument,"invalid PATH executable name");
    auto empty=files::Path::from_utf8("");if(!empty)return empty.error();
    return vix::detail::allocation_or_terminate([&]{return Executable{std::make_shared<State>(true,empty.take(),std::string(name))};});
  }
  Environment::Environment(std::shared_ptr<const State> value) noexcept:state_(std::move(value)){}
  Environment::Environment(const Environment&)=default;Environment::Environment(Environment&&) noexcept=default;
  Environment& Environment::operator=(const Environment&)=default;Environment& Environment::operator=(Environment&&) noexcept=default;Environment::~Environment()=default;
  Environment Environment::inherit() noexcept{return Environment{vix::detail::allocation_or_terminate([]{return std::make_shared<State>(true);})};}
  Environment Environment::empty() noexcept{return Environment{vix::detail::allocation_or_terminate([]{return std::make_shared<State>(false);})};}
  Result<Environment> Environment::with(std::string_view name,std::string_view value) const
  {if(name.empty()||name.find('=')!=std::string_view::npos||name.find('\0')!=std::string_view::npos||value.find('\0')!=std::string_view::npos||!valid_utf8(name)||!valid_utf8(value))return error(errors::invalid_environment,"invalid environment overlay");return vix::detail::allocation_or_terminate([&]()->Result<Environment>{auto copy=std::make_shared<State>(*state_);copy->changes.push_back({std::string(name),std::string(value)});return Environment{std::move(copy)};});}
  Result<Environment> Environment::without(std::string_view name) const
  {if(name.empty()||name.find('=')!=std::string_view::npos||name.find('\0')!=std::string_view::npos||!valid_utf8(name))return error(errors::invalid_environment,"invalid environment variable name");return vix::detail::allocation_or_terminate([&]()->Result<Environment>{auto copy=std::make_shared<State>(*state_);copy->changes.push_back({std::string(name),std::nullopt});return Environment{std::move(copy)};});}
  StandardInput StandardInput::inherit() noexcept{return StandardInput{Kind::inherit};}StandardInput StandardInput::null() noexcept{return StandardInput{Kind::null};}StandardInput StandardInput::pipe() noexcept{return StandardInput{Kind::pipe};}StandardInput StandardInput::from_file(files::File&f) noexcept{return StandardInput{Kind::file,&f};}
  StandardOutput StandardOutput::inherit() noexcept{return StandardOutput{Kind::inherit};}StandardOutput StandardOutput::null() noexcept{return StandardOutput{Kind::null};}StandardOutput StandardOutput::pipe() noexcept{return StandardOutput{Kind::pipe};}StandardOutput StandardOutput::to_file(files::File&f) noexcept{return StandardOutput{Kind::file,&f};}
  StandardError StandardError::inherit() noexcept{return StandardError{Kind::inherit};}StandardError StandardError::null() noexcept{return StandardError{Kind::null};}StandardError StandardError::pipe() noexcept{return StandardError{Kind::pipe};}StandardError StandardError::to_file(files::File&f) noexcept{return StandardError{Kind::file,&f};}StandardError StandardError::merge_with_stdout() noexcept{return StandardError{Kind::merge_stdout};}
  ProcessSpec::ProcessSpec(Executable value):state_(vix::detail::allocation_or_terminate([&]{return std::make_unique<State>(std::move(value));})){}
  ProcessSpec::ProcessSpec(const ProcessSpec&o):state_(vix::detail::allocation_or_terminate([&]{return std::make_unique<State>(*o.state_);})){}
  ProcessSpec::ProcessSpec(ProcessSpec&&) noexcept=default;ProcessSpec& ProcessSpec::operator=(const ProcessSpec&o){if(this!=&o)state_=vix::detail::allocation_or_terminate([&]{return std::make_unique<State>(*o.state_);});return *this;}ProcessSpec& ProcessSpec::operator=(ProcessSpec&&) noexcept=default;ProcessSpec::~ProcessSpec()=default;
  Result<void> ProcessSpec::add_argument(std::string_view value){if(value.find('\0')!=std::string_view::npos||!valid_utf8(value))return error(errors::invalid_argument,"invalid process argument");return vix::detail::allocation_or_terminate([&]()->Result<void>{state_->arguments.emplace_back(value);return Result<void>::success();});}
  void ProcessSpec::set_environment(Environment value){state_->environment=std::move(value);}Result<void> ProcessSpec::set_working_directory(files::Path value){if(value.empty())return error(errors::invalid_working_directory,"working directory is empty");state_->working_directory=std::move(value);return Result<void>::success();}
  void ProcessSpec::set_stdin(StandardInput value){state_->input=value;}void ProcessSpec::set_stdout(StandardOutput value){state_->output=value;}void ProcessSpec::set_stderr(StandardError value){state_->error=value;}
  ChildInput::ChildInput(int) noexcept{}ChildInput::ChildInput(ChildInput&&) noexcept=default;ChildInput::~ChildInput() noexcept=default;bool ChildInput::is_open() const noexcept{return false;}Result<void> ChildInput::close(){contract_violation();}Result<std::size_t> ChildInput::write_some(std::span<const std::byte>){contract_violation();}
  ChildOutput::ChildOutput(int) noexcept{}ChildOutput::ChildOutput(ChildOutput&&) noexcept=default;ChildOutput::~ChildOutput() noexcept=default;bool ChildOutput::is_open() const noexcept{return false;}Result<void> ChildOutput::close(){contract_violation();}Result<std::size_t> ChildOutput::read_some(std::span<std::byte>){contract_violation();}
  WaitOptions::WaitOptions(CancellationToken cancellation,std::optional<Deadline> deadline) noexcept:cancellation_(std::move(cancellation)),deadline_(deadline){}
  Process::Process(std::shared_ptr<detail::ProcessRecord> record,int input,int output,int error_fd) noexcept:record_(std::move(record)),stdin_descriptor_(input),stdout_descriptor_(output),stderr_descriptor_(error_fd){}
  Process::Process(Process&&) noexcept=default;Process::~Process() noexcept=default;ProcessId Process::id() const noexcept{contract_violation();}Result<ExitStatus> Process::wait(){contract_violation();}Completion<ExitStatus> Process::wait(WaitOptions){contract_violation();}Result<void> Process::terminate(){contract_violation();}
  Result<ChildInput> Process::take_stdin(){contract_violation();}Result<ChildOutput> Process::take_stdout(){contract_violation();}Result<ChildOutput> Process::take_stderr(){contract_violation();}
  Result<Process> spawn(ProcessSpec)
  {return error(errors::unsupported_capability,"WASI host does not provide native child-process creation");}
}
