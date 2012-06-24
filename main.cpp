#include <QtGui/QApplication>
#include "receiver.h"
#include "client.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Client w;
    w.show();
    
    return a.exec();
}
