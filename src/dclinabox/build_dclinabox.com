$!-----------------------------------------------------------------------------
$! BUILD_DCLINABOX.COM
$!
$! P1 == LINK or BUILD or empty (builds)
$!
$! 08-DEC-2012  MGD  reduced warning suppression
$! 04-DEC-2011  MGD  initial
$!-----------------------------------------------------------------------------
$!
$ ON CONTROLY THEN EXIT 44
$!
$ WRITE SYS$OUTPUT ""
$ WRITE SYS$OUTPUT "Building ""DCLINABOX""" +-
                   " for ""''F$GETSYI("ARCH_NAME")'"" architecture"
$!
$ ARCH_NAME = F$EDIT(F$GETSYI("ARCH_NAME"),"UPCASE")
$ IF ARCH_NAME .EQS. "ALPHA" THEN ARCH_NAME = "AXP"
$!
$ DEFINES = " /DEFINE=(__VMS_VER=70000000,__CRTL_VER=70000000)"
$ INCLUDES = " /INCLUDE=[SRC.MISC]"
$ WARNINGS= "/WARNING=(DISABLE=(PREOPTW))"
$!
$ IF F$EDIT(F$GETSYI("ARCH_NAME"),"UPCASE") .EQS. "VAX"
$ THEN
$    CC_OPTIONS = "/DECC /OPTIMIZE /STAND=RELAXED_ANSI /PREFIX=ALL" +-
                  INCLUDES + DEFINES + WARNINGS
$ ELSE
$    CC_OPTIONS = "/DECC /OPTIMIZE /STAND=RELAXED_ANSI /PREFIX=ALL" +-
                  INCLUDES + DEFINES + WARNINGS
$ ENDIF
$!
$ IF P1 .EQS. "LIST"
$ THEN
$    CC_OPTIONS = CC_OPTIONS + "/LIST/MACHINE"
$    P1 = ""
$ ENDIF
$!
$ IF F$SEARCH("OBJ_''ARCH_NAME'.DIR") .EQS. "" -
     THEN CREATE /DIR [.OBJ_'ARCH_NAME']
$ OBJECT_DIR = "[.OBJ_''ARCH_NAME']"
$!
$ IF P1 .EQS. "" .OR. P1 .EQS. "BUILD" .OR. P1 .EQS. "COMPILE"
$ THEN
$    SET NOON
$    SET VERIFY
$    CC 'CC_OPTIONS' /NODEBUG/OBJECT='OBJECT_DIR' DCLINABOX
$    CC 'CC_OPTIONS' /OBJECT='OBJECT_DIR' WSLIB
$!   'F$VERIFY(0)
$    SET ON
$ ENDIF
$!
$ IF P1 .EQS. "" .OR. P1 .EQS. "BUILD" .OR. P1 .EQS. "LINK"
$ THEN
$    SET NOON
$    SET VERIFY
$    LINK /NOTRACE/EXECUTABLE=WASD_EXE:DCLINABOX.EXE -
     'OBJECT_DIR'DCLINABOX,'OBJECT_DIR'WSLIB
$!   'F$VERIFY(0)
$    SET ON
$ ENDIF
$ PURGE /NOLOG 'OBJECT_DIR'
$!
$!-----------------------------------------------------------------------------
