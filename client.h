#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <vector>
#include <arpa/inet.h>
#include <iostream>
using namespace std;

#include <QDialog>
#include <QHostAddress>

 class QDialogButtonBox;
 class QLabel;
 class QPushButton;
 class QTimer;
 class QUdpSocket;
 class QSpinBox;
 class QTextEdit;
 class QTextBrowser;

#define MAXROOMS = 30;

 struct message_t {
    char version[8];
    char roomName[32];
    char id[32];
    uint8_t op;
    uint16_t mesLen;
    char message[32];
 } __attribute__ ((packed));

 class Client : public QDialog
 {
     Q_OBJECT

 public:
     Client(QWidget *parent = 0);

 private slots:
     void processPendingDatagrams();
     void ttlChanged(int newTtl);
     void sendDatagram();
     void eraseInput();
     int isInRoom(string room);
     int isRoom(string room);
     char *listOfRooms();
     bool newRoom(string room);

 private:
     QLabel *ttlLabel;
     QSpinBox *ttlSpinBox;
     QPushButton *okButton;
     QPushButton *cancelButton;
     QUdpSocket *udpSocket;
     QUdpSocket *udpSocketSend;
     QHostAddress groupAddress;
     QTimer *timer;
     QDialogButtonBox *buttonBox;
     QTextBrowser *outputText;
     QTextEdit *inputText;
     quint16 port;
     bool waitList;
     message_t mess;
     vector<string> rooms;
     vector<string> listRooms;
     uint16_t sizeOfRooms;

};

#endif // CLIENT_H
