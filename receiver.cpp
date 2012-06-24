 #include <QtGui>
 #include <QtNetwork>

 #include "receiver.h"

 Receiver::Receiver(QWidget *parent)
     : QDialog(parent)
 {
     groupAddress = QHostAddress("224.0.0.1");
     port = 7777;

     statusLabel = new QLabel(tr("Listening for multicasted messages"));
     quitButton = new QPushButton(tr("&Quit"));

     udpSocket = new QUdpSocket(this);
     udpSocket->bind(7777, QUdpSocket::ShareAddress);
     udpSocket->joinMulticastGroup(groupAddress);

     connect(udpSocket, SIGNAL(readyRead()),
             this, SLOT(processPendingDatagrams()));
     connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));

     QHBoxLayout *buttonLayout = new QHBoxLayout;
     buttonLayout->addStretch(1);
     buttonLayout->addWidget(quitButton);
     buttonLayout->addStretch(1);

     QVBoxLayout *mainLayout = new QVBoxLayout;
     mainLayout->addWidget(statusLabel);
     mainLayout->addLayout(buttonLayout);
     setLayout(mainLayout);

     setWindowTitle(tr("Multicast Receiver"));
 }

 void Receiver::processPendingDatagrams()
 {
     while (udpSocket->hasPendingDatagrams()) {
         QByteArray datagram;
         datagram.resize(udpSocket->pendingDatagramSize());
         udpSocket->readDatagram(datagram.data(), datagram.size(), &groupAddress, &port);
         statusLabel->setText(tr("Received datagram: \"%1\"")
                              .arg(datagram.data()));
     }
 }

