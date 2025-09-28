TARGET_GEN  = gennum
LIBNAME_GEN = gennumcore
LIBFILE_GEN = lib$(LIBNAME_GEN).a

TARGET_VER  = verbank
LIBNAME_VER = vercore
LIBFILE_VER_ACCOUNT = vercore_account/lib$(LIBNAME_VER).so
LIBFILE_VER_PID     = vercore_pid/lib$(LIBNAME_VER).so

CPPFLAGS   += -g
ifeq ($(shell uname -m), x86_64)
 CPPFLAGS  += -fPIC
endif
LDFLAGS_GEN += -L.
LDFLAGS_VER += -Lvercore_account -Lvercore_pid
LDLIBS_GEN += -l$(LIBNAME_GEN)
LDLIBS_VER += -l$(LIBNAME_VER)

all: $(TARGET_GEN) $(LIBFILE_VER_ACCOUNT) $(LIBFILE_VER_PID) $(TARGET_VER)

# static library
$(LIBFILE_GEN): gennumcore.o
	ar r $@ $^

# shared (dynamic) libraries
$(LIBFILE_VER_ACCOUNT): vercore_account/vercore.o
	g++ $(CPPFLAGS) $^ -shared $(LDFLAGS_VER) -o $@

$(LIBFILE_VER_PID): vercore_pid/vercore.o
	g++ $(CPPFLAGS) $^ -shared $(LDFLAGS_VER) -o $@

# executables
$(TARGET_GEN): $(TARGET_GEN).cpp $(LIBFILE_GEN)
	g++ $(CPPFLAGS) $< $(LDFLAGS_GEN) $(LDLIBS_GEN) -o $@

$(TARGET_VER): $(TARGET_VER).cpp $(LIBFILE_VER_ACCOUNT)
	g++ $(CPPFLAGS) $< $(LDFLAGS_VER) $(LDLIBS_VER) -o $@

clean:
	rm -f $(TARGET_GEN) $(LIBFILE_GEN) $(TARGET_VER) $(LIBFILE_VER_ACCOUNT) $(LIBFILE_VER_PID) *.o vercore_account/*.o vercore_pid/*.o
