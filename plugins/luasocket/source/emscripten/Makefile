TARGET=luasocket
SRCS=auxiliar buffer except inet io luasocket \
	options select tcp timeout udp usocket mime luasocket_stub
INCS=../../../Sdk/include

include ../../../../emscripten/Plugin.mk
CFLGS+=-DLUASOCKET_INET_ATON -DLUASOCKET_INET_PTON