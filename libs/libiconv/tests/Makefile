# Makefile for libiconv/tests

#### Start of system configuration section. ####

# Directories used by "make":
srcdir = .

# Programs used by "make":
CC = /opt/devkitpro-r24/devkitPPC/bin/powerpc-eabi-gcc
CFLAGS = -g -O2
CPPFLAGS = -I/opt/devkitpro-r24/portlibs/ppc/include
LDFLAGS = 
INCLUDES = -I. -I$(srcdir) -I../include -I$(srcdir)/../include -I../srclib -I$(srcdir)/../srclib -I../lib
LIBTOOL = /bin/bash $(top_builddir)/libtool
LIBTOOL_COMPILE = $(LIBTOOL) --mode=compile
LIBTOOL_LINK = $(LIBTOOL) --mode=link
LIBTOOL_INSTALL = $(LIBTOOL) --mode=install
LIBTOOL_UNINSTALL = $(LIBTOOL) --mode=uninstall
RM = rm -f


#### End of system configuration section. ####

SHELL = /bin/sh

# Needed by $(LIBTOOL).
top_builddir = ..

all :

check : all table-from table-to ../src/iconv_no_i18n test-shiftseq
	$(srcdir)/check-stateless $(srcdir) ASCII
#	/* General multi-byte encodings */
	$(CC) $(LDFLAGS) $(INCLUDES) $(CFLAGS) $(CPPFLAGS) $(srcdir)/genutf8.c ../srclib/libicrt.a -o genutf8 && \
	./genutf8 > UTF-8.TXT && \
	$(srcdir)/check-stateless . UTF-8 && \
	$(RM) -r genutf8 genutf8.dSYM UTF-8.TXT
	$(srcdir)/check-stateful $(srcdir) UCS-2BE
	$(srcdir)/check-stateful $(srcdir) UCS-2LE
	$(srcdir)/check-stateful $(srcdir) UCS-4BE
	$(srcdir)/check-stateful $(srcdir) UCS-4LE
	$(srcdir)/check-stateful $(srcdir) UTF-16
	$(srcdir)/check-stateful $(srcdir) UTF-16BE
	$(srcdir)/check-stateful $(srcdir) UTF-16LE
	$(srcdir)/check-stateful $(srcdir) UTF-32
	$(srcdir)/check-stateful $(srcdir) UTF-32BE
	$(srcdir)/check-stateful $(srcdir) UTF-32LE
	$(srcdir)/check-stateful $(srcdir) UTF-7
#	/* 8-bit encodings */
	$(srcdir)/check-stateless $(srcdir) ISO-8859-1
	$(srcdir)/check-stateless $(srcdir) ISO-8859-2
	$(srcdir)/check-stateless $(srcdir) ISO-8859-3
	$(srcdir)/check-stateless $(srcdir) ISO-8859-4
	$(srcdir)/check-stateless $(srcdir) ISO-8859-5
	$(srcdir)/check-stateless $(srcdir) ISO-8859-6
	$(srcdir)/check-stateless $(srcdir) ISO-8859-7
	$(srcdir)/check-stateless $(srcdir) ISO-8859-8
	$(srcdir)/check-stateless $(srcdir) ISO-8859-9
	$(srcdir)/check-stateless $(srcdir) ISO-8859-10
	$(srcdir)/check-stateless $(srcdir) ISO-8859-11
	$(srcdir)/check-stateless $(srcdir) ISO-8859-13
	$(srcdir)/check-stateless $(srcdir) ISO-8859-14
	$(srcdir)/check-stateless $(srcdir) ISO-8859-15
	$(srcdir)/check-stateless $(srcdir) ISO-8859-16
	$(srcdir)/check-stateless $(srcdir) KOI8-R
	$(srcdir)/check-stateless $(srcdir) KOI8-U
	$(srcdir)/check-stateless $(srcdir) KOI8-RU
	$(srcdir)/check-stateless $(srcdir) CP1250
	$(srcdir)/check-stateless $(srcdir) CP1251
	$(srcdir)/check-stateless $(srcdir) CP1252
	$(srcdir)/check-stateless $(srcdir) CP1253
	$(srcdir)/check-stateless $(srcdir) CP1254
	$(srcdir)/check-stateless $(srcdir) CP1255
	$(srcdir)/check-stateful $(srcdir) CP1255
	$(srcdir)/check-stateless $(srcdir) CP1256
	$(srcdir)/check-stateless $(srcdir) CP1257
	$(srcdir)/check-stateless $(srcdir) CP1258
	$(srcdir)/check-stateful $(srcdir) CP1258
	$(srcdir)/check-stateless $(srcdir) CP850
	$(srcdir)/check-stateless $(srcdir) CP862
	$(srcdir)/check-stateless $(srcdir) CP866
	$(srcdir)/check-stateless $(srcdir) CP1131
	$(srcdir)/check-stateless $(srcdir) MacRoman
	$(srcdir)/check-stateless $(srcdir) MacCentralEurope
	$(srcdir)/check-stateless $(srcdir) MacIceland
	$(srcdir)/check-stateless $(srcdir) MacCroatian
	$(srcdir)/check-stateless $(srcdir) MacRomania
	$(srcdir)/check-stateless $(srcdir) MacCyrillic
	$(srcdir)/check-stateless $(srcdir) MacUkraine
	$(srcdir)/check-stateless $(srcdir) MacGreek
	$(srcdir)/check-stateless $(srcdir) MacTurkish
	$(srcdir)/check-stateless $(srcdir) MacHebrew
	$(srcdir)/check-stateless $(srcdir) MacArabic
	$(srcdir)/check-stateless $(srcdir) MacThai
	$(srcdir)/check-stateless $(srcdir) HP-ROMAN8
	$(srcdir)/check-stateless $(srcdir) NEXTSTEP
	$(srcdir)/check-stateless $(srcdir) ARMSCII-8
	$(srcdir)/check-stateless $(srcdir) Georgian-Academy
	$(srcdir)/check-stateless $(srcdir) Georgian-PS
	$(srcdir)/check-stateless $(srcdir) KOI8-T
	$(srcdir)/check-stateless $(srcdir) PT154
	$(srcdir)/check-stateless $(srcdir) RK1048
	$(srcdir)/check-stateless $(srcdir) MuleLao-1
	$(srcdir)/check-stateless $(srcdir) CP1133
	$(srcdir)/check-stateless $(srcdir) TIS-620
	$(srcdir)/check-stateless $(srcdir) CP874
	$(srcdir)/check-stateless $(srcdir) VISCII
	$(srcdir)/check-stateless $(srcdir) TCVN
	$(srcdir)/check-stateful $(srcdir) TCVN
#	/* CJK character sets */
	$(srcdir)/check-stateless $(srcdir) ISO646-JP
	$(srcdir)/check-stateless $(srcdir) JIS_X0201
#	$(srcdir)/check-stateless $(srcdir) JIS_X0208 # redundant, see EUC-JP
#	$(srcdir)/check-stateless $(srcdir) JIS_X0212 # redundant, see EUC-JP
	$(srcdir)/check-stateless $(srcdir) ISO646-CN
#	$(srcdir)/check-stateless $(srcdir) GB_2312-80 # redundant, see EUC-CN
	$(srcdir)/check-stateless $(srcdir) ISO-IR-165
#	$(srcdir)/check-stateless $(srcdir) KSC_5601 # redundant, see EUC-KR
#	/* CJK encodings */
	$(srcdir)/check-stateless $(srcdir) EUC-JP
	$(srcdir)/check-stateless $(srcdir) SHIFT_JIS
	$(srcdir)/check-stateless $(srcdir) CP932
	$(srcdir)/check-stateful $(srcdir) ISO-2022-JP
	$(srcdir)/check-stateful $(srcdir) ISO-2022-JP-2
	$(srcdir)/check-stateful $(srcdir) ISO-2022-JP-1
	$(srcdir)/check-stateless $(srcdir) EUC-CN
	$(srcdir)/check-stateless $(srcdir) GBK
	$(srcdir)/check-stateless $(srcdir) CP936
	$(CC) $(LDFLAGS) $(INCLUDES) $(CFLAGS) $(CPPFLAGS) $(srcdir)/gengb18030z.c ../srclib/libicrt.a -o gengb18030z && \
	{ cat $(srcdir)/GB18030-BMP.TXT ; ./gengb18030z ; } | sort > GB18030.TXT && \
	$(srcdir)/check-stateless . GB18030 && \
	$(RM) -r gengb18030z gengb18030z.dSYM GB18030.TXT
	$(srcdir)/check-stateful $(srcdir) ISO-2022-CN
	$(srcdir)/check-stateful $(srcdir) ISO-2022-CN-EXT
	$(srcdir)/check-stateful $(srcdir) HZ
	$(srcdir)/check-stateless $(srcdir) EUC-TW
	$(srcdir)/check-stateless $(srcdir) BIG5
	$(srcdir)/check-stateless $(srcdir) CP950
	$(srcdir)/check-stateless $(srcdir) BIG5-HKSCS:1999
	$(srcdir)/check-stateful $(srcdir) BIG5-HKSCS:1999
	$(srcdir)/check-stateless $(srcdir) BIG5-HKSCS:2001
	$(srcdir)/check-stateful $(srcdir) BIG5-HKSCS:2001
	$(srcdir)/check-stateless $(srcdir) BIG5-HKSCS:2004
	$(srcdir)/check-stateful $(srcdir) BIG5-HKSCS:2004
	$(srcdir)/check-stateless $(srcdir) EUC-KR
	$(srcdir)/check-stateless $(srcdir) CP949
	$(srcdir)/check-stateless $(srcdir) JOHAB
	$(srcdir)/check-stateful $(srcdir) ISO-2022-KR
#	/* system dependent and extra encodings */
	$(MAKE) check-extra
#	/* transliteration */
	$(srcdir)/check-translit $(srcdir) Quotes UTF-8 ISO-8859-1
	$(srcdir)/check-translit $(srcdir) Quotes UTF-8 ASCII
	$(srcdir)/check-translit $(srcdir) Translit1 ISO-8859-1 ASCII
	$(srcdir)/check-translitfailure $(srcdir) TranslitFail1 ISO-8859-1 ASCII
#	/* substitution */
	$(srcdir)/check-subst
#	/* shift sequence before invalid multibyte character */
	./test-shiftseq

check-extra: check-extra-no
check-extra-no:
check-extra-yes: all table-from table-to ../src/iconv_no_i18n
#	/* AIX specific encodings */
	$(srcdir)/check-stateless $(srcdir) CP856
	$(srcdir)/check-stateless $(srcdir) CP922
#	$(srcdir)/check-stateless $(srcdir) CP943
	$(srcdir)/check-stateless $(srcdir) CP1046
	$(srcdir)/check-stateless $(srcdir) CP1124
	$(srcdir)/check-stateless $(srcdir) CP1129
	$(srcdir)/check-stateless $(srcdir) CP1161
	$(srcdir)/check-stateless $(srcdir) CP1162
	$(srcdir)/check-stateless $(srcdir) CP1163
#	/* OSF/1 specific encodings */
	$(srcdir)/check-stateless $(srcdir) DEC-KANJI
	$(srcdir)/check-stateless $(srcdir) DEC-HANYU
#	/* DOS specific encodings */
	$(srcdir)/check-stateless $(srcdir) CP437
	$(srcdir)/check-stateless $(srcdir) CP737
	$(srcdir)/check-stateless $(srcdir) CP775
	$(srcdir)/check-stateless $(srcdir) CP852
	$(srcdir)/check-stateless $(srcdir) CP853
	$(srcdir)/check-stateless $(srcdir) CP855
	$(srcdir)/check-stateless $(srcdir) CP857
	$(srcdir)/check-stateless $(srcdir) CP858
	$(srcdir)/check-stateless $(srcdir) CP860
	$(srcdir)/check-stateless $(srcdir) CP861
	$(srcdir)/check-stateless $(srcdir) CP863
	$(srcdir)/check-stateless $(srcdir) CP864
	$(srcdir)/check-stateless $(srcdir) CP865
	$(srcdir)/check-stateless $(srcdir) CP869
	$(srcdir)/check-stateless $(srcdir) CP1125
#	/* Extra encodings */
	$(srcdir)/check-stateless $(srcdir) EUC-JISX0213
	$(srcdir)/check-stateless $(srcdir) SHIFT_JISX0213
	$(srcdir)/check-stateful $(srcdir) ISO-2022-JP-3
	$(srcdir)/check-stateless $(srcdir) BIG5-2003
	$(srcdir)/check-stateless $(srcdir) TDS565
	$(srcdir)/check-stateless $(srcdir) ATARIST
	$(srcdir)/check-stateless $(srcdir) RISCOS-LATIN1

table-from : table-from.o ../lib/libiconv.la
	$(LIBTOOL_LINK) $(CC) $(LDFLAGS) $(CFLAGS) table-from.o ../srclib/libicrt.a ../lib/libiconv.la -o $@

table-from.o : $(srcdir)/table-from.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $(CPPFLAGS) $(srcdir)/table-from.c

table-to : table-to.o ../lib/libiconv.la
	$(LIBTOOL_LINK) $(CC) $(LDFLAGS) $(CFLAGS) table-to.o ../srclib/libicrt.a ../lib/libiconv.la -o $@

table-to.o : $(srcdir)/table-to.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $(CPPFLAGS) $(srcdir)/table-to.c

test-shiftseq : test-shiftseq.o ../lib/libiconv.la
	$(LIBTOOL_LINK) $(CC) $(LDFLAGS) $(CFLAGS) test-shiftseq.o ../srclib/libicrt.a ../lib/libiconv.la -o $@

test-shiftseq.o : $(srcdir)/test-shiftseq.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $(CPPFLAGS) $(srcdir)/test-shiftseq.c

# The following rule is necessary to avoid a toplevel "make -n check" failure.
../lib/libiconv.la :
	cd ../lib && $(MAKE) libiconv.la

mostlyclean : clean

clean : force
	$(RM) *.o *.lo table-from table-from table-to table-to test-shiftseq test-shiftseq tmp-* genutf8 genutf8 UTF-8.TXT gengb18030z gengb18030z GB18030.TXT core *.stackdump
	$(RM) -r .libs _libs

distclean : clean
	$(RM) Makefile

maintainer-clean : distclean

force :
