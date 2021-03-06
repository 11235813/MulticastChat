#include <QtGui>
#include <QtNetwork>

#include "client.h"

Client::Client(QWidget *parent)
    : QDialog(parent)
{
    groupAddress = QHostAddress("230.0.0.1");
    port = 7777;

    strcpy(mess.id, "Cristina\0");
    listRooms.push_back(string("room1\0"));
    listRooms.push_back(string("room2\0"));
    listRooms.push_back(string("room3\0"));
    listRooms.push_back(string("name\0"));

    okButton = new QPushButton(tr("&Ok"));
    cancelButton = new QPushButton(tr("&Clear"));

    ttlLabel = new QLabel(tr("TTL for multicast datagrams:"));
    ttlSpinBox = new QSpinBox;
    ttlSpinBox->setRange(0, 255);

    QHBoxLayout *ttlLayout = new QHBoxLayout;
    ttlLayout->addWidget(ttlLabel);
    ttlLayout->addWidget(ttlSpinBox);

    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(okButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(cancelButton, QDialogButtonBox::ActionRole);

    waitList = false;

    udpSocket = new QUdpSocket(this);
    udpSocket->bind(port, QUdpSocket::ShareAddress);
    udpSocket->joinMulticastGroup(groupAddress);

    connect(ttlSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ttlChanged(int)));
    connect(okButton,SIGNAL(clicked()), this, SLOT(sendDatagram()));
    connect(udpSocket, SIGNAL(readyRead()),this, SLOT(processPendingDatagrams()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(eraseInput()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    outputText = new QTextBrowser;
    inputText = new QTextEdit;
    inputText->setFixedHeight(1*30);
    mainLayout->addLayout(ttlLayout);
    mainLayout->addWidget(outputText);
    mainLayout->addWidget(inputText);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    setWindowTitle(tr("Multicast"));
    ttlSpinBox->setValue(1);
}

void Client::ttlChanged(int newTtl)
{
    udpSocket->setSocketOption(QAbstractSocket::MulticastTtlOption, newTtl);
}

void Client::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        printf("HAS PENDING DATAGRAMS...\n");
        QByteArray datagram;
        message_t new_mess;

        char ms[75];

        datagram.resize(udpSocket->pendingDatagramSize());
        memset(&new_mess, 0, datagram.size());
        printf("READING DATAGRAMS OF size %d\n", datagram.size());
        udpSocket->readDatagram((char *) &new_mess, datagram.size());
        fflush(stdout);

        printf("operator: %d\n", new_mess.op);
        if((new_mess.op == 1) && isInRoom(new_mess.roomName) >= 0) {
            /* user joins */
            QString output = "User ";
            output.append(QString::fromStdString(new_mess.id));
            output.append(" has joined room ");
            output.append(QString::fromStdString(rooms.at(isInRoom(new_mess.roomName))));
            output.append("\n");
            outputText->append(output);
        } else if((new_mess.op == 2) && isInRoom(new_mess.roomName) >= 0) {
            /* user leaves */
            QString output = "User ";
            output.append(QString::fromStdString(new_mess.id));
            output.append(" has left the room ");
            output.append(QString::fromStdString(rooms.at(isInRoom(new_mess.roomName))));
            output.append("\n");
            outputText->append(output);
        } else if((new_mess.op == 3) && isInRoom(new_mess.roomName) >= 0) {
            /* message received */
            int mes_len = new_mess.mesLen;
            string message(new_mess.message);
            QString output(QString::fromStdString(new_mess.id));
            output.append(": ");
            output.append(QString::fromStdString(message));
            output.append("\n");
            outputText->append(output);
        } else if(new_mess.op == 4) {
            /* send your list rooms */
            mess.op = 5;

            strcpy(mess.message, "\0");
            sizeOfRooms = 0;
            for(int i = 0; i < rooms.size(); i++){
                char roomI[32];
                strcpy(roomI, rooms.at(i).c_str());
                uint16_t l = strlen(roomI);
                printf("this room length: %d\n", l);
                for(int j = 0; j < l; j++) {
                        mess.message[sizeOfRooms] = roomI[j];
                    sizeOfRooms ++;
                }
                mess.message[sizeOfRooms] = '\0';
                sizeOfRooms++;
            }
            mess.message[sizeOfRooms] = '\0';
            sizeOfRooms++;

            mess.mesLen = sizeOfRooms;
            printf("list of lists: %s with dim = %d\n", mess.message, mess.mesLen);

            /* send back message with the list */
            QByteArray datagram_to_send = QByteArray(mess.version);
            for(int i = strlen(mess.version); i < 8; i++) {
                datagram_to_send += '\0';
            };
            strcpy(mess.roomName,"");
            datagram_to_send += QByteArray(mess.roomName);
            for(int i = strlen(mess.roomName); i < 32; i++) {
                datagram_to_send += '\0';
            };
            datagram_to_send += QByteArray(mess.id);
            for(int i = strlen(mess.id); i < 32; i++) {
                datagram_to_send += '\0';
            }
            datagram_to_send += mess.op;
            printf("--size after operator added: %d\n", datagram_to_send.size());
            datagram_to_send += (char) mess.mesLen;
            datagram_to_send += (mess.mesLen >> 8);
            printf("--size after mesLen added: %d\n", datagram_to_send.size());
            for(int i = 0; i < mess.mesLen; i++)
                datagram_to_send += mess.message[i];
            printf("--size after message added: %d\n", datagram_to_send.size());
            fflush(stdout);
            udpSocket->writeDatagram(datagram_to_send, datagram_to_send.size(),groupAddress, port);
        } else if((new_mess.op == 5) && waitList) {
            /* got answer with list of rooms */
            /* empty the list of rooms */
            listRooms.empty();
            QString output = "Rooms:\n";
            if(new_mess.mesLen > 0){
                string name("");
                printf("mess: %s\n", new_mess.message);

                int nrNulls = 0;
                for(int i = 0; i <=  new_mess.mesLen; i ++) {
                    if(new_mess.message[i] == '\0'){
                        nrNulls ++;
                        if(nrNulls == 2)
                            break;
                        listRooms.push_back(name);
                        output.append(QString::fromStdString(name.c_str()));
                        output.append("\n");
                        name = "";
                    } else {
                        nrNulls = 0;
                        name.push_back(new_mess.message[i]);
                    }
                }
                output.append("\n");
                outputText->append(output);
            }
            fflush(stdout);
        }
    }
}

void Client::eraseInput()
{
    inputText->clear();
}

void Client::sendDatagram()
{
    bool ok = false;

    if(inputText->toPlainText() == "")
        return;

    strcpy(mess.version, "CHATv1\0");

    QString input = inputText->toPlainText();

    if(input == "LIST") {
        ok = true;
        printf("LIST\n");
        strcpy(mess.roomName, "\0");
        mess.op = 4;
        mess.mesLen = 0;
        strcpy(mess.message, "\0");
        waitList = true;
    } else if(input == "QUIT") {
        ok = true;
        waitList = false;
        printf("QUIT\n");
        mess.op = 2;
        mess.mesLen = 0;
        strcpy(mess.message, "\0");
    } else {
        QStringList splitted = input.split(QRegExp("\\s"));
        waitList = false;
        if(splitted.length() == 2){
            if((splitted.at(0) == "JOIN") && (isRoom(splitted.at(1).toUtf8().constData()) >= 0)){
                ok = true;
                printf("JOIN\n");
                /* subscribe to a room */
                strcpy(mess.roomName, splitted.at(1).toUtf8().constData());
                printf("joining room %s...\n", splitted.at(1).toUtf8().constData());
                mess.roomName[strlen(splitted.at(1).toUtf8().constData())] = '\0';

                mess.op = 1;
                mess.mesLen = 0;
                strcpy(mess.message, "\0");
                if(newRoom(mess.roomName))
                    rooms.push_back(mess.roomName);
            } else if((splitted.at(0) == "LEAVE") && (isInRoom(splitted.at(1).toUtf8().constData()) >= 0)){
                /* leave a room */
                ok = true;
                printf("LEAVE\n");
                strcpy(mess.roomName, splitted.at(1).toUtf8().constData());
                printf("leaving room %s...\n", splitted.at(1).toUtf8().constData());
                mess.op = 2;
                mess.mesLen = 0;
                strcpy(mess.message, "\0");
                rooms.erase(rooms.begin() + isInRoom(splitted.at(1).toUtf8().constData()));
            } else if(splitted.at(0) == "NICK"){
                /* set the id */
                ok = true;
                printf("NICK\n");
                strcpy(mess.roomName, "\0");
                strcpy(mess.id, splitted.at(1).toUtf8().constData());
                mess.op = 1;
                mess.mesLen = 0;
                strcpy(mess.message, "\0");
            }
        } else if((splitted.length() >= 3)){
            printf("%d\n", splitted.at(0) == "MSG");
            printf("is in room: %d", isInRoom(splitted.at(1).toUtf8().constData()));
           if((splitted.at(0) == "MSG") && (isInRoom(splitted.at(1).toUtf8().constData()) >= 0)){
            /* send message */
            ok = true;
            printf("MESSAGE\n");
            fflush(stdout);
            strcpy(mess.roomName, splitted.at(1).toUtf8().constData());
            mess.op = 3;

            mess.mesLen = 0;
            memset(mess.message, 0, 1000);
            for(int i = 2; i < splitted.size(); i++) {
                strcat(mess.message, splitted.at(i).toUtf8().constData());
                strcat(mess.message, " ");
                mess.mesLen += splitted.at(i).length() + 1;
            }
            mess.message[strlen(mess.message) - 1] = '\0';
            printf("the message: %s of length %d\n", mess.message,  mess.mesLen);
        }} else{
            ok = false;
            return;
        }
    }

    if(ok == true) {
        if(inputText->toPlainText() == "QUIT") {
            for(int j = 0; j < rooms.size(); j++) {
                strcpy(mess.roomName, rooms[j].c_str());
                mess.roomName[strlen(mess.roomName)] = '\0';
                QByteArray datagram = QByteArray(mess.version);
                for(int i = strlen(mess.version); i < 8; i++) {
                    datagram += '\0';
                };
                datagram += QByteArray(mess.roomName);
                for(int i = strlen(mess.roomName); i < 32; i++) {
                    datagram += '\0';
                };
                datagram += QByteArray(mess.id);
                for(int i = strlen(mess.id); i < 32; i++) {
                    datagram += '\0';
                }
                datagram += mess.op;
                datagram += (char) mess.mesLen;
                datagram += (mess.mesLen >> 8);
                datagram += QByteArray(mess.message);
                printf("writing the datagram ...\n");
                fflush(stdout);
                udpSocket->writeDatagram(datagram, datagram.size(),groupAddress, port);
            }
            udpSocket->leaveMulticastGroup(groupAddress);
            udpSocket->close();
            exit(0);
        }else{
                QByteArray datagram = QByteArray(mess.version);
                for(int i = strlen(mess.version); i < 8; i++) {
                    datagram += '\0';
                };
                datagram += QByteArray(mess.roomName);
                for(int i = strlen(mess.roomName); i < 32; i++) {
                    datagram += '\0';
                };
                datagram += QByteArray(mess.id);
                for(int i = strlen(mess.id); i < 32; i++) {
                    datagram += '\0';
                }
                datagram += mess.op;
                printf("sending operator: %d\n", mess.op);
                datagram += ((char) mess.mesLen);
                datagram += (mess.mesLen >> 8);
                printf("datagram size after adding msg len: %d\n", datagram.size());
                datagram += QByteArray(mess.message);
                if(mess.mesLen > 0)
                    datagram += '\0';
                printf("WRITING DATAGRAMS OF size: %d\n", datagram.size());
                udpSocket->writeDatagram(datagram, datagram.size(),groupAddress, port);
            }
    }
}

int Client::isInRoom(string room){

    for(int i = 0; i < rooms.size(); i++){
        char cp[strlen(room.c_str())];
        rooms.at(i).copy(cp, (int)strlen(room.c_str()));
        cp[strlen(room.c_str())] = '\0';
        printf("CP got: %s\n", cp);
        fflush(stdout);
        printf("comparing rooms: %s and %s", room.c_str(), cp);
        if(strcmp(cp, room.c_str()) == 0){
            return i;
        }
    }
    return -1;
}

int Client::isRoom(string room){
    printf("looking for room: %s\n", room.c_str());
    for(int i = 0; i < listRooms.size(); i++){
        printf("listRooms.at(i) %s\n", listRooms.at(i).c_str());
        if(listRooms.at(i).compare(room) == 0){
            printf("I:%d\n", i);
            return i;
        }
    }
    return -1;
}

char * Client::listOfRooms() {
    char mess[32];
    strcpy(mess, "\0");
    sizeOfRooms = 0;
    for(int i = 0; i < rooms.size(); i++){
        if(i == 0)
            strcat(mess, rooms.at(i).c_str());
        else
            strcat(mess + 1, rooms.at(i).c_str());
        mess[strlen(mess)] = '\0';
        sizeOfRooms += 2;
    }
    mess[strlen(mess) + 1] = '\0';
    sizeOfRooms++;
    return mess;
}

bool Client::newRoom(string room) {
    for(int i = 0; i < rooms.size(); i++)
        if(rooms.at(i).compare(room) == 0)
            return false;
    return true;
}
