# encoding=utf-8

platform = 'e:\\platform'
fake = 'e:\\bak\\fake'

flags = [
'-Wall',
'-Wextra',
#'-Werror',
#'-Wc++98-compat',
#'-Wno-long-long',
#'-Wno-variadic-macros',
'-fexceptions',
'-DNDEBUG',
'-std=gnu++11',
'-x',
'c++',

#'-isystem',
#'D:\\llvm\\bin\\..\\lib\\clang\\3.7.0\\include',

# 标准
'-D_WIN32_WINNT=0x0600',
'-D__WINDOWS__',
'-I',
fake,

# BOOST
'-DBOOST_USE_WINDOWS_H',
'-DBOOST_ALL_NO_LIB',
'-I',
platform + '\\Deps\\boost2\\include',

# Libevent
'-I',
platform + '\\Deps\\libevent\\src\\libevent-2.0.21-stable\\include',
'-I',
platform + '\\Deps\\libevent\\src\\libevent-2.0.21-stable\\WIN32-Code',
'-I',
platform + '\\Deps\\libevent\\src\\libevent-2.0.21-stable',

# mysql
'-I',
'c:\\Program Files\\MySQL\\MySQL Connector C 6.0.2\\include',

# 当前
'-I',
'.'
]

def FlagsForFile( filename, **kwargs ):
    return {
        'flags': flags,
        'do_cache': True
    }

if __name__ == '__main__':
    last = " -isystem D:\\llvm\\bin\\..\\lib\\clang\\3.7.0\\include"
    print "env CINDEXTEST_EDITING=1 d:\\LLVMMY\\build\\Debug\\bin\\c-index-test.exe -test-load-source-reparse 5 local " + " ".join(flags) + last
