#ifndef LOG_HPP
#define LOG_HPP

/// 获取文件名
#ifndef __FILENAME__
#undef __FILENAME__
#endif
#ifdef _WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1):__FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#endif


#define std_log std::cout <<  std::string().append(" [")  \
        .append(__FILENAME__).append(":")                \
        .append(std::to_string(__LINE__)).append(":")    \
        .append(__func__)                                \
        .append("] ") 


#endif