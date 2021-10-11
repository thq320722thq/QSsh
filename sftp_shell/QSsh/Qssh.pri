QT += network

INCLUDEPATH += $$PWD/include/ssh

QSSH_ROOT = $$PWD/lib
message($$QSSH_ROOT)

#LIBS += -L$${QSSH_ROOT} -lQSsh

win32:CONFIG(release, debug|release): LIBS += -L$${QSSH_ROOT} -lQSsh
else:win32:CONFIG(debug, debug|release): LIBS += -L$${QSSH_ROOT} -lQSshd
