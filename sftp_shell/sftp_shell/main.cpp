#include <QCoreApplication>
#include "sftptest.h"
#include "argumentscollector.h"

/*cli: -h 192.168.1.100 -u root -pwd 123456 -o get -s /opt/zld/xxx -d ./ */
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    bool parseSuccess;
    const Parameters parameters = ArgumentsCollector(a.arguments()).collect(parseSuccess);
    if (!parseSuccess)
        return EXIT_FAILURE;
    SftpTest sftpTest(parameters);
    sftpTest.run();

    return a.exec();
}
