
#
# c、cpp混合编译的makefile模板
#


PLAT = none

TARGET_NAME = libenet.a
CC = $(CROSS)gcc
CXX = $(CROSS)g++
AR = $(CROSS)ar rcu
RANLIB = $(CROSS)ranlib
STRIP = $(CROSS)strip
RM = rm -f

COMMON_FLAGS = -DHAS_FCNTL=1 -DHAS_POLL=1 -DHAS_GETNAMEINFO=1 -DHAS_GETADDRINFO=1 -DHAS_MSGHDR_FLAGS=1 -DHAS_SOCKLEN_T=1
CFLAGS = $(CROSS_FLAGS) $(EXTRA_CFLAGS) $(COMMON_FLAGS)
CXXFLAGS = $(CROSS_FLAGS) $(EXTRA_CXXFLAGS) $(COMMON_FLAGS)

EXTRA_CFLAGS = 
EXTRA_CXXFLAGS = 

EXTRA_INCS = 
EXTRA_LIBS = 


SRC_INCS = -I"./include/"

SRC_LIBS = 


C_SRC_ALL = $(wildcard ./*.c)

CXX_SRC_ALL = $(wildcard ./*.cpp)




PLATS = win-debug win-release linux-debug linux-release
none:
	@echo "Please choose a platform:"
	@echo " $(PLATS)"



win-debug:
	$(MAKE) all EXTRA_CFLAGS="-Wall -D_WIN32 -DDEBUG -g" EXTRA_CXXFLAGS="-Wall -D_WIN32 -DDEBUG -g"

win-release:
	$(MAKE) all EXTRA_CFLAGS="-Wall -D_WIN32 -DNDEBUG -O2" EXTRA_CXXFLAGS="-Wall -D_WIN32 -DNDEBUG -O2"


linux-debug:
	$(MAKE) all EXTRA_CFLAGS="-fPIC -Wall -DDEBUG -g" EXTRA_CXXFLAGS="-fPIC -Wall -DDEBUG -g"

linux-release:
	$(MAKE) all EXTRA_CFLAGS="-fPIC -Wall -DNDEBUG -O2" EXTRA_CXXFLAGS="-fPIC -Wall -DNDEBUG -O2"


all:$(TARGET_NAME)


echo:
	@echo "PLAT = $(PLAT)"
	@echo "CC = $(CC)"
	@echo "CXX = $(CXX)"
	@echo "AR = $(AR)"
	@echo "RANLIB = $(RANLIB)"
	@echo "RM = $(RM)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "CXXFLAGS = $(CXXFLAGS)"
	@echo "EXTRA_CFLAGS = $(EXTRA_CFLAGS)"
	@echo "EXTRA_CXXFLAGS = $(EXTRA_CXXFLAGS)"



C_OBJ_ALL := $(C_SRC_ALL:.c=.o)
CXX_OBJ_ALL := $(CXX_SRC_ALL:.cpp=.o)

#$(OBJS):%.o :%.c  先用$(OBJS)中的一项，比如foo.o: %.o : %.c  含义为:试着用%.o匹配foo.o。如果成功%就等于foo。如果不成功，Make就会警告，然后。给foo.o添加依赖文件foo.c(用foo替换了%.c里的%)
# $@--目标文件，$^--所有的依赖文件，$<--第一个依赖文件。每次$< $@ 代表的值就是列表中的
$(C_OBJ_ALL) : %.o: %.c
	$(CC) -c $< -o $@ $(EXTRA_INCS) $(SRC_INCS) $(CFLAGS)
$(CXX_OBJ_ALL) : %.o: %.cpp
	$(CXX) -c $< -o $@ $(EXTRA_INCS) $(SRC_INCS) $(CXXFLAGS)


$(TARGET_NAME): $(C_OBJ_ALL) $(CXX_OBJ_ALL)
	$(AR) $(TARGET_NAME) $(C_OBJ_ALL) $(CXX_OBJ_ALL)
	$(RANLIB) $(TARGET_NAME)
	$(RM) $(C_OBJ_ALL) $(CXX_OBJ_ALL)


.PHONY: all $(PLATS) clean cleanall echo

clean:
	$(RM) $(TARGET_NAME) $(C_OBJ_ALL) $(CXX_OBJ_ALL)

cleanall:
	$(RM) $(TARGET_NAME) $(C_OBJ_ALL) $(CXX_OBJ_ALL)

