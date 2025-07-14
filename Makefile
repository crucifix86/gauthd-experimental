
TOP_SRCDIR = ..

SINGLE_THREAD = false 
DEBUG_VERSION = false

include ../mk/gcc.defs.mk

OBJS = gauthserver.o gpanelserver.o state.o stubs.o authmanager.o gmysqlclient.o luaman.o bcrypt_auth.o gauthd.o
#DEFINES += -DUSE_EPOLL -g -ggdb 
DEFINES += -DUSE_EPOLL -O3 -I../include -I/usr/include/i386-linux-gnu -I/usr/include/lua5.1 -I../../share/lua

all : gauthd

gauthd : $(SHAREOBJ) $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(SHAREOBJ) $(OBJS) $(SHARE_SOBJ) $(LICENSE_SOBJ) -lmysqlclient -llua5.1 -lbcrypt

include ../mk/gcc.rules.mk

