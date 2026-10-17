#include <array>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <thread>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace
{
  std::filesystem::path path_from_utf8(std::string_view value)
  {
    std::u8string encoded;
    encoded.reserve(value.size());
    for (const char byte : value) encoded.push_back(static_cast<char8_t>(static_cast<unsigned char>(byte)));
    return std::filesystem::path(encoded);
  }

  int write_all(const char *data,std::size_t size)
  {
    std::size_t offset=0;
    while(offset<size)
    {
#if defined(_WIN32)
      const int amount=::_write(1,data+offset,static_cast<unsigned>(size-offset));
#else
      const auto amount=::write(STDOUT_FILENO,data+offset,size-offset);
#endif
      if(amount<=0)return 2;
      offset+=static_cast<std::size_t>(amount);
    }
    return 0;
  }

#if defined(_WIN32)
  int write_wide(const wchar_t *value)
  {
    const int needed=::WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value,-1,nullptr,0,nullptr,nullptr);
    if(needed<=0)return 5;
    std::string encoded(static_cast<std::size_t>(needed),'\0');
    if(::WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value,-1,encoded.data(),needed,nullptr,nullptr)<=0)return 5;
    encoded.resize(static_cast<std::size_t>(needed-1));
    return write_all(encoded.data(),encoded.size());
  }
#endif
}

#if defined(_WIN32)
int wmain(int argc,wchar_t **argv)
#else
int main(int argc,char **argv)
#endif
{
#if defined(_WIN32)
  (void)::_setmode(0,_O_BINARY);
  (void)::_setmode(1,_O_BINARY);
#endif
  if(argc<2)return 64;
#if defined(_WIN32)
  const std::wstring_view mode(argv[1]);
  if(mode==L"args")
  {
    for(int i=2;i<argc;++i)if(write_all("arg:",4)!=0||write_wide(argv[i])!=0||write_all("\n",1)!=0)return 2;
    return 0;
  }
  if(mode==L"env")
  {
    const wchar_t *value=::_wgetenv(L"VIX_PROCESS_TEST_VALUE");
    return value?write_wide(value):65;
  }
  if(mode==L"cwd")
  {
    DWORD needed=::GetCurrentDirectoryW(0,nullptr);if(!needed)return 67;
    std::wstring value(static_cast<std::size_t>(needed),L'\0');
    const DWORD count=::GetCurrentDirectoryW(needed,value.data());if(!count||count>=needed)return 67;
    value.resize(count);return write_wide(value.c_str());
  }
  if(mode==L"stderr")
  {
    const char marker[]="stderr-marker";
    return ::_write(2,marker,sizeof(marker)-1)==static_cast<int>(sizeof(marker)-1)?0:68;
  }
  if(mode==L"handle-check")
  {
    if(argc<4)return 69;
    const auto raw=std::wcstoull(argv[2],nullptr,10);
    const HANDLE candidate=reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(raw));
    std::wstring final_path(32768,L'\0');
    const DWORD length=raw?::GetFinalPathNameByHandleW(candidate,final_path.data(),static_cast<DWORD>(final_path.size()),FILE_NAME_NORMALIZED):0;
    final_path.resize(length>0&&length<final_path.size()?length:0);
    const bool inherited=!final_path.empty()&&final_path.find(argv[3])!=std::wstring::npos;
    return write_all(inherited?"inherited":"isolated",inherited?9:8);
  }
  if(mode==L"copy-stderr")
  {
    std::array<char,4096> buffer{};
    for(;;)
    {
      const int amount=::_read(0,buffer.data(),static_cast<unsigned>(buffer.size()));
      if(amount==0)return 0;
      if(amount<0)return 3;
      std::size_t offset=0;
      while(offset<static_cast<std::size_t>(amount))
      {const int written=::_write(2,buffer.data()+offset,static_cast<unsigned>(amount-static_cast<int>(offset)));if(written<=0)return 4;offset+=static_cast<std::size_t>(written);}
    }
  }
  if(mode==L"delay-file")
  {
    if(argc<4)return 70;
    const auto milliseconds=std::wcstol(argv[2],nullptr,10);
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    const auto encoded=::WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,argv[3],-1,nullptr,0,nullptr,nullptr);
    if(encoded<=0)return 71;
    std::string filename(static_cast<std::size_t>(encoded), '\0');
    if(::WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,argv[3],-1,filename.data(),encoded,nullptr,nullptr)<=0)return 71;
    filename.resize(static_cast<std::size_t>(encoded-1));
    std::ofstream output(path_from_utf8(filename),std::ios::binary|std::ios::trunc);
    output<<"child-survived";return output?0:72;
  }
  if(mode==L"copy")
  {
    std::array<char,4096> buffer{};
    for(;;)
    {
      const int amount=::_read(0,buffer.data(),static_cast<unsigned>(buffer.size()));
      if(amount==0)return 0;
      if(amount<0)return 3;
      if(write_all(buffer.data(),static_cast<std::size_t>(amount))!=0)return 4;
    }
  }
  if(mode==L"exit")return argc>2?static_cast<int>(std::wcstol(argv[2],nullptr,10)):0;
  if(mode==L"sleep-ms")
  {
    const auto milliseconds=argc>2?std::wcstol(argv[2],nullptr,10):1;
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));return 0;
  }
#else
  const std::string_view mode(argv[1]);
  if(mode=="args")
  {
    for(int i=2;i<argc;++i)
    {
      constexpr std::string_view prefix="arg:";
      if(write_all(prefix.data(),prefix.size())!=0||write_all(argv[i],std::strlen(argv[i]))!=0||write_all("\n",1)!=0)return 2;
    }
    return 0;
  }
  if(mode=="env")
  {
    const char *value=std::getenv("VIX_PROCESS_TEST_VALUE");
    return value?write_all(value,std::strlen(value)):65;
  }
  if(mode=="cwd")
  {
    const auto value=std::filesystem::current_path().u8string();
    return write_all(reinterpret_cast<const char *>(value.data()),value.size());
  }
  if(mode=="stderr")
  {
    constexpr char marker[]="stderr-marker";
    return ::write(STDERR_FILENO,marker,sizeof(marker)-1)==static_cast<ssize_t>(sizeof(marker)-1)?0:68;
  }
  if(mode=="fd-check")
  {
    if(argc<4)return 69;
    char *end=nullptr;
    const long descriptor=std::strtol(argv[2],&end,10);
    if(!end||*end!='\0'||descriptor<0)return 69;
    struct stat descriptor_info{}, path_info{};
    const bool inherited=::fstat(static_cast<int>(descriptor),&descriptor_info)==0 &&
      ::stat(argv[3],&path_info)==0 && descriptor_info.st_dev==path_info.st_dev &&
      descriptor_info.st_ino==path_info.st_ino;
    const char *message=inherited?"inherited":"isolated";
    return write_all(message,std::strlen(message));
  }
  if(mode=="copy-stderr")
  {
    std::array<char,4096> buffer{};
    for(;;)
    {
      const auto amount=::read(STDIN_FILENO,buffer.data(),buffer.size());
      if(amount==0)return 0;
      if(amount<0)return 3;
      std::size_t offset=0;
      while(offset<static_cast<std::size_t>(amount))
      {const auto written=::write(STDERR_FILENO,buffer.data()+offset,static_cast<std::size_t>(amount)-offset);if(written<=0)return 4;offset+=static_cast<std::size_t>(written);}
    }
  }
  if(mode=="copy")
  {
    std::array<char,4096> buffer{};
    for(;;)
    {
#if defined(_WIN32)
      const int amount=::_read(0,buffer.data(),static_cast<unsigned>(buffer.size()));
#else
      const auto amount=::read(STDIN_FILENO,buffer.data(),buffer.size());
#endif
      if(amount==0)return 0;
      if(amount<0)return 3;
      if(write_all(buffer.data(),static_cast<std::size_t>(amount))!=0)return 4;
    }
  }
  if(mode=="exit")return argc>2?std::atoi(argv[2]):0;
  if(mode=="sleep-ms")
  {
    const int milliseconds=argc>2?std::atoi(argv[2]):1;
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    return 0;
  }
  if(mode=="delay-file")
  {
    if(argc<4)return 70;
    std::this_thread::sleep_for(std::chrono::milliseconds(std::atoi(argv[2])));
    std::ofstream output(path_from_utf8(argv[3]),std::ios::binary|std::ios::trunc);
    output<<"child-survived";return output?0:72;
  }
#endif
  return 66;
}
