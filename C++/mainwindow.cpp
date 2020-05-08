#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "customplayersettingswidget.h"
#include "qcustomplot.h"

#include <QSlider>
#include <QFrame>
#include <QTabWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QScrollArea>
#include <iostream>
#include <QtSerialPort/QSerialPortInfo>
#include <QModelIndex>
#include <QStringListModel>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , arduino_(new QSerialPort)
    , graphicScene_(new QGraphicsScene)
    , beatBarFont_( "Arial", 20, QFont::Bold)
{
    ui->setupUi(this);
    ui->productID->setText(QString::number(arduinoProductId_));
    ui->vendorID->setText(QString::number(arduinoVendorId_));
    ui->tabWidget->setCurrentIndex(0);
    ui->stackedWidget->setCurrentIndex(0);
    ui->graphicsView->setScene(graphicScene_);
    //this->showFullScreen();

    previousMode_ = Mode::empty;
    createPlayersSettings(ui->nbPlayersA->currentText().toInt());

    //Set Default Button Threshold Interval
    ui->buttonThreshold0->setText(QString::number(defaultButtonMinValue_));
    ui->buttonThreshold1->setText(QString::number(ButtonVoltageThreshold[0]));
    ui->buttonThreshold2->setText(QString::number(ButtonVoltageThreshold[1]));
    ui->buttonThreshold3->setText(QString::number(ButtonVoltageThreshold[2]));
    ui->buttonThreshold4->setText(QString::number(ButtonVoltageThreshold[3]));

    //Set queueItemEventTemplate_
    queueItemEventTemplate_ = new QueueItem();
    queueItemEventTemplate_->index = -1;
    queueItemEventTemplate_->level = 1;
    queueItemEventTemplate_->name = "Group";
    queueItemEventTemplate_->imgPath = ":/img/Group";
    queueItemEventTemplate_->step = stepComputation(queueItemEventTemplate_);

    //Palettes
    greenPal_.setColor(QPalette::Base, Qt::green);
    redPal_.setColor(QPalette::Base, Qt::red);
    whitePal_.setColor(QPalette::Base, Qt::white);

    //conections
    recordTimer_.setSingleShot(true);
    recordTimer_.setTimerType(Qt::PreciseTimer);
    plotTimer_.start(30);
    connect(&plotTimer_, SIGNAL(timeout()), this, SLOT(updatePlot()));
    //connect(&resizeLayoutTimer_, SIGNAL(timeout()), this, updateGraphicViewSize());
    connect(&scrollTimer_, SIGNAL(timeout()), this, SLOT(scrollView()));
    connect(&metronome_, SIGNAL(newBarSignal()), this, SLOT(checkBarCount()));
    connect(&metronome_, SIGNAL(newBeatSignal()), this, SLOT(updateCurrentBeat()));
    delayTimerToSyncArduino_.setSingleShot(true);
    connect(&delayTimerToSyncArduino_, SIGNAL(timeout()), this, SLOT(arduinoReadyToSync()));
    connect(&recordTimer_, SIGNAL(timeout()), this, SLOT(stopRecording()));

    //ModMap
    modeMap_[Mode::normal] = "Normal";
    modeMap_[Mode::cascading] = "Cascade";
    modeMap_[Mode::record] = "Record";
    modeMap_[Mode::loop] = "Loop";
    modeMap_[Mode::ready] = "Ready";
    modeMap_[Mode::refresh] = "Refresh";
    modeMap_[Mode::readyWarning] = "ReadyWarning";

    //ButtonVoltage Threshold
    ButtonVoltageWidgetVector_.push_back(ui->buttonThreshold0);
    ButtonVoltageWidgetVector_.push_back(ui->buttonThreshold1);
    ButtonVoltageWidgetVector_.push_back(ui->buttonThreshold2);
    ButtonVoltageWidgetVector_.push_back(ui->buttonThreshold3);
    ButtonVoltageWidgetVector_.push_back(ui->buttonThreshold4);



    //Default Settings
    metronome_.setBpm(120);
    beatsNumber_ = 4;
    metronome_.setBeats(4);
    metronome_.setCurrentBar(1);
    metronome_.setCurrentBeat(0);
    delayCompensation_ = 0;
    isBinary_ = true;
    isTimeSub_ = false;

    checkDirectories();
    loadUserPref();

    //find Arduino at start;
    findArduino();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::arduinoReadyToSync(){//Slot called few milli sec after founding the Arduino,otherwise it creates
    isArduinoReadyToSync_ = true;       //conflicts
    arduino_->clear();
    arduinoProcessedByteArray_.clear();
}

void MainWindow::on_confirmButton_clicked()
{
    if(isTabMode_)
        addTabsToQueue();

    else{
        createPlayersQueueItem();
        addPlayersToQueue();
    }

    setTemplateRecordMat();
    createBeats(metronome_.beats());
    createBars(queueItemVector_.front().first->level);
    updateCurrentBeat();
    updateCurrentBar();
    drawQueue();
    resizeLayoutTimer_.start(30);

    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::createPlayersQueueItem(){
    for(int i = 0; i<playerNumber_; i++){
        QueueItem* player = new QueueItem();
        player->index = i;
        player->name = customPlayerSettinsWidgetVector_[i]->getPlayerName().toStdString();
        player->imgPath = ":/img/player" + std::to_string(i+1);
        player->level = std::stoi(customPlayerSettinsWidgetVector_[i]->getPlayerSkill().toStdString().substr(0,1));
        player->step = stepComputation(player);
        playerVector_.push_back(player);
    }
}

float MainWindow::stepComputation(QueueItem* player){
    float numerator = defaultQueueItemImageWidth_ * scrollTimerPeriod_;
    float denominator = metronome_.beats() * player->level * (60/(float)metronome_.bpm()) * 1000;
    return numerator / denominator;
}

void MainWindow::addPlayersToQueue(){
    QueueItem* readyQueueItem = new QueueItem();
    queueItemEventTemplate_->step = stepComputation(queueItemEventTemplate_);
    *readyQueueItem = *queueItemEventTemplate_;
    std::pair<QueueItem*, Mode> startQueueItem;
    startQueueItem.first = readyQueueItem;

    if(!isArduinoSync_ || !isArduinoFound_)
        startQueueItem.second = Mode::readyWarning;
    else{
        startQueueItem.second = Mode::ready;
    }
    queueItemVector_.push_back(startQueueItem);
    for(int i = 0; i < playerNumber_; i++){
        playerVector_[i]->step = stepComputation(playerVector_[i]);
        std::pair<QueueItem*, Mode> p(playerVector_[i], Mode::normal);
        queueItemVector_.push_back(p);
    }
}

void MainWindow::addTabsToQueue(){

    QueueItem * ready = new QueueItem();
    queueItemEventTemplate_->step = stepComputation(queueItemEventTemplate_);
    *ready = *queueItemEventTemplate_;
    std::pair<QueueItem*, Mode> startQueueItem;
    startQueueItem.first = ready;

    if(!isArduinoSync_ || !isArduinoFound_)
        startQueueItem.second = Mode::readyWarning;
    else{
        startQueueItem.second = Mode::ready;
    }
    queueItemVector_.push_back(startQueueItem);

    for(int i = 0; i < tabItemVector_.size(); i++){
        QueueItem* tab = new QueueItem();
        tab->index = i;
        tab->step = stepComputation(queueItemEventTemplate_);
        tab->name = "";
        tab->level = 1;
        tab->imgPath = tabItemVector_[std::find(tabItemVector_.begin(), tabItemVector_.end(), ui->listWidget->item(i)) - tabItemVector_.begin()]->path_;
        std::pair<QueueItem*, Mode> p(tab, Mode::tab);
        queueItemVector_.push_back(p);
    }
}

void MainWindow::createBeats(int b){
    for(int i = 0; i < b; i++){
        QLineEdit* beat = new QLineEdit(this);
        beat->setText(QString::number(i+1));
        beat->setAlignment(Qt::AlignHCenter);
        beat->setFont(beatBarFont_);
        beat->setReadOnly(true);
        beatsVector_.push_back(beat);
        ui->beatsLayout->addWidget(beatsVector_.back());

        if(i == 0)
            beatsVector_.back()->setPalette(greenPal_);
    }
}

void MainWindow::createBars(int nbBars){
    for(int i = 0; i < nbBars; i++){
        QLineEdit* bar = new QLineEdit(this);
        bar->setText(QString::number(i+1));
        bar->setAlignment(Qt::AlignHCenter);
        bar->setFont(beatBarFont_);
        bar->setReadOnly(true);
        barsVector_.push_back(bar);
        ui->barsLayout->addWidget(barsVector_.back());

        if(i == 0)
            barsVector_.back()->setPalette(redPal_);
    }
}

void MainWindow::removeBeats(){
    for(QLineEdit* le : beatsVector_){
        ui->beatsLayout->removeWidget(le);
        delete le;
    }
    beatsVector_.clear();
}

void MainWindow::removeBars(){
    for(QLineEdit* le : barsVector_){
        ui->barsLayout->removeWidget(le);
        delete le;
    }
    barsVector_.clear();
}

void MainWindow::removePlayersSettings(){
    delete ui->scrollArea->findChild<QFrame*>("playerSettingsFrame");
    delete ui->frame_3->findChild<QTabWidget*>("plotTab");
}

void MainWindow::on_nbPlayersA_currentIndexChanged(const QString &arg1)
{
    removePlayersSettings();
    customPlayerSettinsWidgetVector_.clear();
    createPlayersSettings(arg1.toInt());
}

void MainWindow::createPlayersSettings(int playerNumber)
{
    thresholdSliderVector_.clear();
    thresholdValueVector_.clear();
    inputPiezoVoltageVector_.clear();
    customPlotVector_.clear();
    playerNumber_ = playerNumber;

    QFrame* playerSettingsFrame = new QFrame(ui->scrollArea);
    playerSettingsFrame->setObjectName("playerSettingsFrame");
    playerSettingsFrame->setLayout(new QVBoxLayout(playerSettingsFrame));
    QTabWidget *plotTab = new QTabWidget(ui->frame_3);
    plotTab->setObjectName("plotTab");
    plotTab->setLayout(new QHBoxLayout(plotTab));
    ui->scrollArea->setWidget(playerSettingsFrame);
    ui->frame_3->layout()->addWidget(plotTab);
    ui->horizontalLayout_3->setStretch(0,1);
    ui->horizontalLayout_3->setStretch(1,1);

    for(int i =0; i < playerNumber; i++){

        //CustomPlayerSettingsWidget
        CustomPlayerSettingsWidget* cpsw = new CustomPlayerSettingsWidget(playerSettingsFrame, QString::number(i+1));
        customPlayerSettinsWidgetVector_.push_back(cpsw);
        playerSettingsFrame->layout()->addWidget(cpsw);

        //QCustomPlot
        QCustomPlot * customPlot = new QCustomPlot(plotTab);
        customPlot->addGraph();
        customPlot->addGraph();
        customPlot->graph(0)->setPen(QPen(QColor(40, 110, 255)));
        customPlot->graph(1)->setPen(QPen(QColor(255, 0, 0)));
        customPlot->axisRect()->setupFullAxesBox();
        customPlot->yAxis->setRange(0, 1400);

        customPlotVector_.push_back(customPlot);

        //Create Slider
        QSlider * thresholdSlider = new QSlider(plotTab);
        //thresholdSlider->setStyleSheet("QSlider::groove:horizontal {border: 1px solid #262626;height: 5px;background: #393939;margin: 0 12px;}");
        thresholdSliderVector_.push_back(thresholdSlider);
        thresholdSlider->setSliderDown(true);
        thresholdSlider->setMinimum(1);
        thresholdSlider->setMaximum(1023);
        thresholdSlider->setSliderPosition(defaultThresholdValue_);
        thresholdValueVector_.push_back(defaultThresholdValue_);
        inputPiezoVoltageVector_.push_back(100);

        //Connect ThresholdSlider to slider value vector
        connect(thresholdSliderVector_.back(), SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)));

        QWidget* page = new QWidget(plotTab);
        QHBoxLayout * hLayout = new QHBoxLayout(page);
        hLayout->addWidget(thresholdSlider);
        hLayout->addWidget(customPlot);
        page->setLayout(hLayout);

        std::string label = "Player " + std::to_string(i+1);
        plotTab->addTab(page, QString::fromStdString(label));
    }
}

void MainWindow::sliderChanged(int value){
    thresholdValueVector_[ui->frame_3->findChild<QTabWidget*>("plotTab")->currentIndex()] = value;
}

void MainWindow::updatePlot(){
    static QTime time(QTime::currentTime());
    // calculate two new data points:
    double key = time.elapsed()/1000.0; // time elapsed since start of demo, in seconds
    // add data to lines:
    for(int i = 0; i < playerNumber_; i++){
        customPlotVector_[i]->graph(0)->addData(key, thresholdValueVector_[i]);
        customPlotVector_[i]->graph(1)->addData(key, inputPiezoVoltageVector_[i]);
        // make key axis range scroll with the data (at a constant range size of 8):
        customPlotVector_[i]->xAxis->setRange(key, 8, Qt::AlignRight);
        customPlotVector_[i]->replot();
    }
}

void MainWindow::findArduino(){
    isArduinoFound_ = false;
    isArduinoSync_ = false;
    isArduinoReadyToSync_ = false;
    arduinoProcessedByteArray_.clear();

    ui->isArduinoFound->setText("NO");
    ui->isArduinoFound->setStyleSheet("QLineEdit{background: red;}");
    ui->isArduinoSync->setText("NO");
    ui->isArduinoSync->setStyleSheet("QLineEdit{background: red;}");

    int i = 1;
    ui->textEdit->setText("");
    ui->textEdit->append(QString::fromStdString("Number of available ports: " + std::to_string(QSerialPortInfo::availablePorts().length())));
        foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()){
            if(serialPortInfo.hasVendorIdentifier()){
                ui->textEdit->append(QString::fromStdString("Device nb : " + std::to_string(i) + " [PortName: " + serialPortInfo.portName().toStdString() + " ] has vendor id: " + std::to_string(serialPortInfo.vendorIdentifier())));
            }
            if(serialPortInfo.hasProductIdentifier()){
                ui->textEdit->append(QString::fromStdString("Device nb : " + std::to_string(i) + " [PortName: " + serialPortInfo.portName().toStdString() + " ] has product id: " + std::to_string(serialPortInfo.productIdentifier())));
            }
            i++;
        }

    ui->textEdit->append(QString::fromStdString("Searching for device with Product ID = " + std::to_string(arduinoProductId_) + " and Vendor ID = " + std::to_string(arduinoVendorId_)));
        foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()){
            if(serialPortInfo.hasVendorIdentifier() && serialPortInfo.hasProductIdentifier()){
                if(serialPortInfo.vendorIdentifier() == arduinoVendorId_){
                    if(serialPortInfo.productIdentifier() == arduinoProductId_){

                        isArduinoFound_ = true;
                        arduino_->setPortName(serialPortInfo.portName());
                        arduino_->open(QSerialPort::ReadOnly);
                        arduino_->setBaudRate(QSerialPort::Baud115200);
                        arduino_->setDataBits(QSerialPort::Data8);
                        arduino_->setParity(QSerialPort::NoParity);
                        arduino_->setStopBits(QSerialPort::OneStop);
                        arduino_->setFlowControl(QSerialPort::NoFlowControl);
                        std::cout<<"COnnected"<<std::endl;
                        connect(arduino_, SIGNAL(readyRead()), this, SLOT(readArduino()));
                        std::cout<<"COnnected"<<std::endl;
                        delayTimerToSyncArduino_.start(defaultDelayToSyncArduino_);
                  }
                }
            }
        }
        if(isArduinoFound_){
            ui->textEdit->append("Arduino Found!");
            ui->isArduinoFound->setText("YES");
            ui->isArduinoFound->setStyleSheet("QLineEdit{background: green;}");
        }
        else{
            ui->textEdit->append("Arduino NOT Found!");
        }
}

void MainWindow::readArduino(){
    int temps;
    if(isRecordMode_){
        temps = elapsedTimer_.elapsed();
    }
    if(isArduinoFound_){
        QByteArray currentByte(arduino_->readAll());

        if(!isArduinoSync_ && isArduinoReadyToSync_){
            ui->textEdit->append("Trying to sync Arduino ...");
            arduinoProcessedByteArray_.append(currentByte);
            if(arduinoProcessedByteArray_.size() >= 4){
                for(int i = 0; i < arduinoProcessedByteArray_.size() - 3; i++){
                    if(static_cast<int>(arduinoProcessedByteArray_[i]) == 83 &&       //S
                            static_cast<int>(arduinoProcessedByteArray_[i+1]) == 89 &&    //Y
                            static_cast<int>(arduinoProcessedByteArray_[i+2]) == 78 &&    //N
                            static_cast<int>(arduinoProcessedByteArray_[i+3]) == 67)      //C
                    {
                        isArduinoSync_ = true;
                        ui->confirmButton->setEnabled(true);
                        arduinoProcessedByteArray_.remove(0,i);
                        ui->textEdit->append("Arduino has been sync !");
                        ui->isArduinoSync->setText("YES");
                        ui->isArduinoSync->setStyleSheet("QLineEdit{background: green;}");
                        break;
                    }
                }
            }
        }

        else if(isArduinoSync_){
            arduinoProcessedByteArray_.append(currentByte);
            while(arduinoProcessedByteArray_.size() >= arduinoByteArraySize_){
                //qDebug() << arduinoProcessedByteArray_.toHex();
                QByteArray footSwitchByte;
                footSwitchByte.append(arduinoProcessedByteArray_[4]);
                footSwitchByte.append(arduinoProcessedByteArray_[5]);
                footSwitchInputVoltage = *reinterpret_cast<const quint16 *>(footSwitchByte.constData());
                footSwitchInputVoltage = ((footSwitchInputVoltage << 8) | (footSwitchInputVoltage >> 8));
                ui->arduinoFootswitchVoltage->setText(QString::number(footSwitchInputVoltage));
                for (int i = 0; i < playerNumber_; i++){
                    QByteArray ba;
                    ba.append(arduinoProcessedByteArray_[6+2*i]);
                    ba.append(arduinoProcessedByteArray_[7+2*i]);

                    quint16 value;
                    value = *reinterpret_cast<const quint16 *>(ba.constData());
                    value = ((value << 8) | (value >> 8));
                    inputPiezoVoltageVector_[i] = value;
                }

                arduinoProcessedByteArray_.remove(0,arduinoByteArraySize_);
                if(isRecordMode_){
                    for(int i = 0; i < playerNumber_; i++){
                        if(inputPiezoVoltageVector_[i]>= thresholdValueVector_[i]){
                            cv::Point p1 (defaultQueueItemImageWidth_ *  temps/(metronome_.beats() * (60/(float)metronome_.bpm()) * 1000) - delayCompensation_, i*(defaultQueueItemImageHeight_/playerNumber_) + (defaultQueueItemImageHeight_/playerNumber_)/2 - defaultRecordMarkLineLength_/2);
                            cv::Point p2 (defaultQueueItemImageWidth_ *  temps/(metronome_.beats() * (60/(float)metronome_.bpm()) * 1000) - delayCompensation_, i*(defaultQueueItemImageHeight_/playerNumber_) + (defaultQueueItemImageHeight_/playerNumber_)/2 + defaultRecordMarkLineLength_/2);
                            cv::line(recordedTabMat_, p1, p2, cv::Scalar(0,0,0), 6);
                        }
                    }
                }

                if(metronome_.isActive() && isFootswitchReleased_){
                    if(footSwitchInputVoltage >= defaultButtonMinValue_ && footSwitchInputVoltage < ButtonVoltageThreshold[0]){
                        std::cout<<"1"<<std::endl;
                        if(queueItemVector_.back().second != Mode::record){
                            std::cout<<"A"<<std::endl;
                            for(auto& i : playerVector_){
                                std::pair<QueueItem*, Mode> pair(i, Mode::normal);
                                queueItemVector_.push_back(pair);
                                std::cout<<i->index<<std::endl;
                            }
                            std::cout<<"A"<<std::endl;
                        }
                        else{
                            std::cout<<"B"<<std::endl;
                            for(auto& i : playerVector_){
                                std::pair<QueueItem*, Mode> pair(i, Mode::refresh);
                                queueItemVector_.push_back(pair);
                            }
                        }
                        isFootswitchReleased_ = false;
                        updateGraphicsQueue();
                    }

                    if(footSwitchInputVoltage >= ButtonVoltageThreshold[0] && footSwitchInputVoltage < ButtonVoltageThreshold[1]){
                        for(auto& i : playerVector_){
                            std::pair<QueueItem*, Mode> pair(i, Mode::cascading);
                            queueItemVector_.push_back(pair);
                        }
                        isFootswitchReleased_ = false;
                        std::cout<<"2"<<std::endl;
                        updateGraphicsQueue();
                    }
                    if(footSwitchInputVoltage >= ButtonVoltageThreshold[1] && footSwitchInputVoltage < ButtonVoltageThreshold[2]){
                        if(queueItemVector_.back().second != Mode::record){
                            std::pair<QueueItem*, Mode> pair(queueItemEventTemplate_, Mode::record);
                            if(isTabMode_)
                                queueItemVector_.insert(queueItemVector_.begin() + 1, pair);
                            else{
                                queueItemVector_.push_back(pair);
                            }
                            isFootswitchReleased_ = false;
                            std::cout<<"3"<<std::endl;
                            updateGraphicsQueue();
                        }
                    }
                    if(footSwitchInputVoltage >= ButtonVoltageThreshold[2] && footSwitchInputVoltage < ButtonVoltageThreshold[3]){
                        std::pair<QueueItem*, Mode> pair(queueItemEventTemplate_, Mode::loop);
                        queueItemVector_.push_back(pair);
                        isFootswitchReleased_ = false;
                        std::cout<<"4"<<std::endl;
                        updateGraphicsQueue();
                    }
                }
                else if(!isFootswitchReleased_ && footSwitchInputVoltage >= ButtonVoltageThreshold[3]){
                    isFootswitchReleased_ = true;
                }
            }
        }
    }
}

void MainWindow::on_productID_textChanged(const QString &arg1)
{
    arduinoProductId_ = arg1.toInt();
}

void MainWindow::on_vendorID_textChanged(const QString &arg1)
{
    arduinoVendorId_ = arg1.toInt();
}


void MainWindow::on_findArduinoButton_clicked()
{
    findArduino();
}


void MainWindow::on_settingsButton_clicked()
{
    stopQueue();

    clearQueue();

    destroyPlayers();

    ui->stackedWidget->setCurrentIndex(0);
}



void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if( event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
    {
        if(metronome_.isActive()){
            stopQueue();

            if(queueItemVector_[0].second != Mode::ready && queueItemVector_[0].second != Mode::readyWarning){
                QueueItem* ready = new QueueItem();
                * ready = * queueItemEventTemplate_;
                std::pair<QueueItem*, Mode> pair;
                pair.first = ready;

                if(!isArduinoFound_ || !isArduinoSync_)
                    pair.second = Mode::readyWarning;
                else{
                    pair.second = Mode::ready;
                }
                queueItemVector_.insert(queueItemVector_.begin(),pair);
            }

            updateCurrentBeat();
            updateCurrentBar();
            clearQueueGraphics();
            drawQueue();
        }
        else{
            metronome_.start();
            scrollTimer_.start(scrollTimerPeriod_);
        }
    }

    if( event->key() == Qt::Key_D && queueItemVector_.size() > 2)
    {
       removeLastQueueItem();
    }
}

void MainWindow::stopSession(){
    metronome_.stop();
    metronome_.setCurrentBar(1);
    metronome_.setCurrentBeat(0);
    clearQueue();
}

void MainWindow::removeLastQueueItem(){
    graphicScene_->removeItem(graphicPixmapVector_.back());
    graphicPixmapVector_.erase(graphicPixmapVector_.end() - 1);
    queueItemVector_.erase(queueItemVector_.end() - 1);
}

void MainWindow::scrollView(){
    switch(queueItemVector_[0].second){
        case Mode::normal:
        case Mode::refresh:
        case Mode::record:
        case Mode ::loop:
        case Mode::ready:
        case Mode::readyWarning:
        case Mode::tab:
            for(int i = 0; i < graphicPixmapVector_.size(); i++){
                graphicPixmapVector_[i]->setPos(graphicPixmapVector_[i]->pos().x() - queueItemVector_[0].first->step, graphicPixmapVector_[i]->pos().y());
            }
        break;
        case Mode::cascading:
            for(int i = 0; i < graphicPixmapVector_.size(); i++){
                graphicPixmapVector_[i]->setPos(graphicPixmapVector_[i]->pos().x() - 2* queueItemVector_[0].first->level *queueItemVector_[0].first->step, graphicPixmapVector_[i]->pos().y());
            }
        break;
    }
}

void MainWindow::checkBarCount(){
    metronome_.currentBar();
    if(queueItemVector_[0].second == Mode::cascading){
        newActivePlayerInQueue();
        metronome_.setCurrentBar(1);
    }
    else{
        switch(queueItemVector_[0].first->level){
            case 1:
                if(metronome_.currentBar() == 2){
                    newActivePlayerInQueue();
                    metronome_.setCurrentBar(1);
                }
            break;
            case 2:
                if(metronome_.currentBar() == 3){
                    newActivePlayerInQueue();
                    metronome_.setCurrentBar(1);
                }
            break;
            case 3:
                if(metronome_.currentBar() == 4){
                    newActivePlayerInQueue();
                    metronome_.setCurrentBar(1);
                }
            break;
            case 4:
                if(metronome_.currentBar() == 5){
                    newActivePlayerInQueue();
                    metronome_.setCurrentBar(1);
                }
            break;
        }
    }
    updateCurrentBar();
}

void MainWindow::updateCurrentBeat(){
    int index = 1;
    for(auto & el : beatsVector_){
        if(metronome_.currentBeat() == index){
            el->setPalette(greenPal_);
        }
        else{
            el->setPalette(whitePal_);
        }
        index++;
    }
}

void MainWindow::updateCurrentBar(){
    int index = 1;
    for(auto & el : barsVector_){
        if(metronome_.currentBar() >= index){
            el->setPalette(redPal_);
        }
        else{
            el->setPalette(whitePal_);
        }
        index++;
    }
}

void MainWindow::newActivePlayerInQueue(){
    //Set the previous mode, needed for the recording mode
    if(queueItemVector_[0].second != Mode::loop && queueItemVector_[0].second != Mode::record)
        previousMode_ = queueItemVector_[0].second;

    //update Queue
    for(int i = 0; i < queueItemVector_.size() - 1; i++){
       queueItemVector_[i] = queueItemVector_[i+1];
    }
    queueItemVector_.erase(queueItemVector_.end() - 1);

    //Remove Beats and Bars and create new one.
    removeBeats();
    removeBars();

    if(queueItemVector_[0].second == Mode::cascading){
        metronome_.setBeats(beatsNumber_/2);
        createBeats(metronome_.beats());
        createBars(1);
    }
    else{
        metronome_.setBeats(beatsNumber_);
        createBeats(metronome_.beats());
        createBars(queueItemVector_.front().first->level);
    }


    //Check if the current Mode is the Recording Mode
    if(queueItemVector_[0].second == Mode::record){
       isRecordMode_ = true;

       switch(previousMode_){
            case Mode::refresh:
       case Mode::tab:
           case Mode::normal:{
                std::cout<<"Normal"<<std::endl;
               recordTimer_.start(metronome_.beats() * (60/(float)metronome_.bpm()) * 1000);
               elapsedTimer_.start();
               templateNormalMat_.copyTo(recordedTabMat_);
           }
           break;
           case Mode::cascading:
           {   std::cout<<"Cascade"<<std::endl;
                recordTimer_.start(metronome_.beats() * (60/(float)metronome_.bpm()) * 1000);
               elapsedTimer_.start();
               templateCascadeMat_.copyTo(recordedTabMat_);
           }
           break;
       }
    }
    if(queueItemVector_.size() == 1){
        if(isTabMode_){
            for(int i = 0; i < tabItemVector_.size(); i++){
                QueueItem * tab = new QueueItem();
                //tab->index = i;
                tab->step = queueItemEventTemplate_->step;
                tab->name = "";
                tab->level = 1;
                tab->imgPath = tabItemVector_[std::find(tabItemVector_.begin(), tabItemVector_.end(), ui->listWidget->item(i)) - tabItemVector_.begin()]->path_;

                std::pair<QueueItem*, Mode> pair(tab, Mode::tab);
                queueItemVector_.push_back(pair);
            }
        }
        else{
           for(auto p : playerVector_){
               std::pair<QueueItem*, Mode> pair(p, Mode::normal);
               queueItemVector_.push_back(pair);
           }
       }
    }

    if(isTabMode_ && queueItemVector_.front().second == Mode::tab)
        currentTabIndex_ = queueItemVector_.front().first->index;
    clearQueueGraphics();
    drawQueue();
}

void MainWindow::drawQueue(){ //For each element of the queue, create a picture and add it to the scene

    for (int i = 0; i < queueItemVector_.size(); i++){
        QPixmap img;
        QGraphicsPixmapItem * pixmapItem = new QGraphicsPixmapItem();
        if(queueItemVector_[i].second == Mode::tab)
            img.load(QString::fromStdString(queueItemVector_[i].first->imgPath));

        else{
            img.load(QString::fromStdString(queueItemVector_[i].first->imgPath + modeMap_[queueItemVector_[i].second]));
        }

        img = img.scaled(defaultQueueItemImageWidth_,ui->graphicsView->height()) ;
        pixmapItem->setPixmap(img);
        pixmapItem->setPos(img.width() * i,0);
        graphicPixmapVector_.push_back(pixmapItem);
        graphicScene_->addItem(graphicPixmapVector_[i]);
    }
}

void MainWindow::updateGraphicsQueue(){
    for (int i = graphicPixmapVector_.size(); i < queueItemVector_.size(); i++){
        QPixmap img;
        QGraphicsPixmapItem * pixmapItem = new QGraphicsPixmapItem();
        img.load(QString::fromStdString(queueItemVector_[i].first->imgPath + modeMap_[queueItemVector_[i].second]));
        img = img.scaled(defaultQueueItemImageWidth_,ui->graphicsView->height()) ;
        pixmapItem->setPixmap(img);
        pixmapItem->setPos(img.width() * graphicPixmapVector_.size() + graphicPixmapVector_[0]->pos().x() ,0);
        graphicPixmapVector_.push_back(pixmapItem);
        graphicScene_->addItem(graphicPixmapVector_.back());
    }
}

void MainWindow::clearQueueGraphics(){

    for(int i = 0; i < graphicPixmapVector_.size(); i++){
            graphicScene_->removeItem(graphicPixmapVector_[i]);
            delete graphicPixmapVector_[i];
        }
        graphicPixmapVector_.clear();
}

void MainWindow::clearQueue(){
    clearQueueGraphics();
    queueItemVector_.clear();

    removeBars();
    removeBeats();
}

void MainWindow::stopQueue(){
    scrollTimer_.stop();
    metronome_.stop();
    metronome_.setCurrentBeat(0);
    metronome_.setCurrentBar(1);
}

void MainWindow::stopRecording(){
    std::cout<<"Stop Recording" <<std::endl;
    std::cout << "Elapsed = " << elapsedTimer_.elapsed() << std::endl;
    isRecordMode_ = false;
    std::string tabName = std::to_string(recordsIndex_) + ".jpg";
    cv::imwrite( picturesPath_ + "/" + tabName , recordedTabMat_);
    recordsIndex_ ++;
    //Reset Matrix
    recordedTabMat_.release();



//    QFile file(QString::fromStdString(sessionPath_) + "/" + ui->tabName->text() + ".txt");
//    file.open(QIODevice::ReadWrite);
//    int line_count=0;
//    QTextStream in(&file);
//    std::vector<QString> storedFile;
//    while( !in.atEnd())
//    {
//        storedFile.push_back(in.readLine());
//        line_count++;
//    }
//    storedFile.push_back(QString::fromStdString(sessionPath_ + "/" + tabName));
//    file.resize(0);
//    for(auto f : storedFile){
//        in<<f<<'\n';
//    }

//    std::cout<<"TextFile lines = "<<line_count<<std::endl;
//    file.close();

    CustomListWidgetItem * customWidget = new CustomListWidgetItem(picturesPath_ + "/" + tabName);
    customWidget->setIcon(QIcon(QString::fromStdString(customWidget->path_)));

    tabItemVector_.push_back(customWidget);
    ui->listWidget->insertItem(currentTabIndex_ + 1, customWidget);

    hasNewRecords_ = true;
}

void MainWindow::setTemplateRecordMat(){
    cv::Scalar greenColor(0,255,0);
    cv::Scalar blueColor(0,0,255);
    templateNormalMat_ = cv::Mat(cv::Size(defaultQueueItemImageWidth_, defaultQueueItemImageHeight_), CV_8UC3, greenColor);
    templateCascadeMat_ = cv::Mat(cv::Size(defaultQueueItemImageWidth_, defaultQueueItemImageHeight_), CV_8UC3, blueColor);

    for(int i = 0; i < beatsNumber_; i++){
        cv::Point p1 (i*(defaultQueueItemImageWidth_/beatsNumber_), 0);
        cv::Point p2 (i*(defaultQueueItemImageWidth_/beatsNumber_), defaultQueueItemImageHeight_);
        cv::line(templateNormalMat_, p1, p2, cv::Scalar(0,0,255), 2);
    }
    for(int i = 0; i < playerNumber_; i++){
        cv::Point p1 (0, i*(defaultQueueItemImageHeight_/playerNumber_) + (defaultQueueItemImageHeight_/playerNumber_)/2);
        cv::Point p2 (defaultQueueItemImageWidth_, i*(defaultQueueItemImageHeight_/playerNumber_) + (defaultQueueItemImageHeight_/playerNumber_)/2);
        cv::line(templateNormalMat_, p1, p2, cv::Scalar(0,0,0), 2);
    }
}

void MainWindow::on_tabEditorButton_clicked()
{
    stopQueue();
    clearQueue();
    destroyPlayers();

//    if(hasNewRecords_){
//        for(auto tab : tabItemVector_){
//            ui->listWidget->removeItemWidget(tab);
//            delete tab;
//        }
//        tabItemVector_.clear();


////        QFile file(QString::fromStdString(sessionPath_) + "/" + ui->tabName->text() + ".txt");
////        createTabFromFile(file);
//    }

//    else{
//        ui->playButton->setEnabled(true);
//    }

    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::on_playButton_clicked()
{
    //ui->graphicsView->fitInView(graphicScene_->sceneRect());
    hasNewRecords_ = false;

    createPlayersQueueItem();

    if(ui->listWidget->count() != 0){
        addTabsToQueue();
        isTabMode_ = true;
        currentTabIndex_ = 0;
    }
    else{
        addPlayersToQueue();
        isTabMode_ = false;
    }

    createBars(queueItemVector_.front().first->level);
    createBeats(metronome_.beats());
    drawQueue();
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::on_exitButton_clicked()
{
    QApplication::quit();
}

void MainWindow::on_loadTabButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::on_importTab_clicked()
{
    std::string path = (QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toStdString() + "/" + "ParamulaSessions";
    QStringList files = QFileDialog::getOpenFileNames(this,tr("Open Image"), QString::fromStdString(path), tr("Image Files (*.png *.jpg *.bmp *.txt)"));

    for(auto fileName : files){
        if(fileName.toStdString().substr(fileName.toStdString().size()-3, 3) == "txt"){
            if(ui->listWidget->count() == 0){
                ui->tabSavedPath->setText(fileName);
                ui->tabSavedPath->setStyleSheet("QLineEdit{background : green;}");
                isTabSavePathDefined_ = true;
                saveTabPath_ = fileName.toStdString();
            }
            else{
                ui->tabSavedPath->setText("Not Defined!");
                ui->tabSavedPath->setStyleSheet("QLineEdit{background : red;}");
                isTabSavePathDefined_ = false;
                saveTabPath_ = "";
            }
            QFile file(fileName);
            createTabFromFile(file);
        }
        else if(fileName.toStdString().substr(fileName.toStdString().size()-3, 3) == "jpg"){
            ui->tabSavedPath->setText("Not Defined!");
            ui->tabSavedPath->setStyleSheet("QLineEdit{background : red;}");
            isTabSavePathDefined_ = false;
            saveTabPath_ = "";
            CustomListWidgetItem * customWidget = new CustomListWidgetItem(fileName.toStdString());
            customWidget->setIcon(QIcon(QString::fromStdString(customWidget->path_)));

            if(ui->listWidget->selectedItems().size() == 1)
                ui->listWidget->insertItem(ui->listWidget->row(ui->listWidget->selectedItems()[0]), customWidget);

            else{
                ui->listWidget->addItem(customWidget);
            }
            tabItemVector_.push_back(customWidget);
        }
    }
}

void MainWindow::createTabFromFile(QFile& file){
    file.open(QIODevice::ReadOnly);
    QTextStream stream(&file);
    std::vector<QString> storedFile;
    while( !stream.atEnd())
    {
        CustomListWidgetItem * item = new CustomListWidgetItem(stream.readLine().toStdString());
        item->setIcon(QIcon(QString::fromStdString(item->path_)));

        if(ui->listWidget->selectedItems().size() == 1)
            ui->listWidget->insertItem(ui->listWidget->row(ui->listWidget->selectedItems()[0]), item);
        else{
            ui->listWidget->addItem(item);
        }
        tabItemVector_.push_back(item);
    }
    file.close();

}

void MainWindow::on_deleteTab_clicked()
{
    copiedTabVector_.clear();
    for(auto widget : ui->listWidget->selectedItems()){
        ui->listWidget->removeItemWidget(widget);
        tabItemVector_.erase(std::find(tabItemVector_.begin(), tabItemVector_.end(), widget));
        delete widget;
    }
}

void MainWindow::on_clearAll_clicked()
{
    isTabMode_ = false;
    copiedTabVector_.clear();
    for(int i = 0; i < ui->listWidget->count(); i++){
        delete tabItemVector_[i];
    }
    ui->listWidget->clear();
    tabItemVector_.clear();

    ui->tabSavedPath->setText("Not Defined!");
    ui->tabSavedPath->setStyleSheet("QLineEdit{background : red;}");
    isTabSavePathDefined_ = false;
}

void MainWindow::on_copyTab_clicked()
{
    copiedTabVector_.clear();
    for(auto widget : ui->listWidget->selectedItems()){
        copiedTabVector_.push_back(tabItemVector_.at(std::find(tabItemVector_.begin(), tabItemVector_.end(), widget) - tabItemVector_.begin()));
    }
}

void MainWindow::on_pasteTab_clicked()
{
    for(auto widget : copiedTabVector_){
        CustomListWidgetItem * customWidget = new CustomListWidgetItem(widget->path_);
        customWidget->setIcon(QIcon(QString::fromStdString(customWidget->path_)));
        if(ui->listWidget->selectedItems().size() == 1)
            ui->listWidget->insertItem(ui->listWidget->row(ui->listWidget->selectedItems()[0]), customWidget);
        else{
            ui->listWidget->addItem(customWidget);
        }
        tabItemVector_.push_back(customWidget);
    }
}

void MainWindow::on_saveTab_clicked()
{
    if(isTabSavePathDefined_)
        saveTab(saveTabPath_);
    else{
        on_saveTabAs_clicked();
    }
}

void MainWindow::on_settingsFromTab_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::on_bpmA_currentIndexChanged(const QString &arg1)
{
    metronome_.setBpm(arg1.toInt());
}

void MainWindow::on_beatsA_currentIndexChanged(const QString &arg1)
{
    metronome_.setBeats(arg1.toInt());
    beatsNumber_ = arg1.toInt();
}

void MainWindow::on_restartTab_clicked()
{
    stopQueue();
    clearQueue();
    if(ui->listWidget->count() != 0){
        addTabsToQueue();
        currentTabIndex_ = 0;
    }
    else{
        addPlayersToQueue();
    }

    createBars(queueItemVector_.front().first->level);
    createBeats(metronome_.beats());

    drawQueue();
}

void MainWindow::checkDirectories(){
    //Create a session directory in the document folder
    paramulaPath_ = (QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toStdString() + "/" + "ParamulaSessions";
    QDir dir(QString::fromStdString(paramulaPath_));
    if(!dir.exists())
        dir.mkpath(".");

    userPrefPath_ = paramulaPath_ + "/UserPref";
    dir.setPath(QString::fromStdString(userPrefPath_));
    if(!dir.exists())
        dir.mkpath(".");

    picturesPath_ = paramulaPath_ + "/Pictures";
    dir.setPath(QString::fromStdString(picturesPath_));
    if(!dir.exists())
        dir.mkpath(".");

    QDirIterator dirIt(QString::fromStdString(picturesPath_),QStringList() << "*.jpg", QDir::Files, QDirIterator::Subdirectories);
    while (dirIt.hasNext()) {
        recordsIndex_++;
        dirIt.next();
    }
    std::cout<<"Record index is  = "<<recordsIndex_<<std::endl;
}

void MainWindow::on_buttonThreshold1_textChanged(const QString &arg1)
{
    ButtonVoltageThreshold[0] = arg1.toInt();
}

void MainWindow::on_buttonThreshold2_textChanged(const QString &arg1)
{
    ButtonVoltageThreshold[1] = arg1.toInt();
}

void MainWindow::on_buttonThreshold3_textChanged(const QString &arg1)
{
    ButtonVoltageThreshold[2] = arg1.toInt();
}

void MainWindow::on_buttonThreshold4_textChanged(const QString &arg1)
{
    ButtonVoltageThreshold[3] = arg1.toInt();
}

void MainWindow::on_savePrefButton_clicked()
{
        QFile file(QString::fromStdString(userPrefPath_ + "/userPref.txt"));
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);
        file.resize(0);
        out<<ButtonVoltageThreshold[0]<<'\n';
        out<<ButtonVoltageThreshold[1]<<'\n';
        out<<ButtonVoltageThreshold[2]<<'\n';
        out<<ButtonVoltageThreshold[3]<<'\n';
        file.close();
}

void MainWindow::loadUserPref(){
    QFile file(QString::fromStdString(userPrefPath_ + "/userPref.txt"));
    file.open(QIODevice::ReadOnly);
    QTextStream in(&file);
    int i = 0;
    while( !in.atEnd())
    {
        ButtonVoltageThreshold[i] = in.readLine().toInt();
        ButtonVoltageWidgetVector_[i + 1]->setText(QString::number(ButtonVoltageThreshold[i]));
        i++;
    }
}

void MainWindow::destroyPlayers(){
    int j = 1;
    for(QueueItem* p : playerVector_){
        delete p;
        j++;
    }
    playerVector_.clear();
}

void MainWindow::on_saveTabAs_clicked()
{
    if(ui->listWidget->count() != 0){
        QString filename = QFileDialog::getSaveFileName(this, QString("Save Tabs"), QString::fromStdString(paramulaPath_));
        if(filename.size() !=0)
            saveTab(filename.toStdString());
    }
}

void MainWindow::saveTab(std::string path){
    QFile f(QString::fromStdString(path));
     f.open( QIODevice::WriteOnly );
     QTextStream out(&f);
     f.resize(0);

     for(int i = 0; i<ui->listWidget->count(); i++){
         out<<QString::fromStdString(tabItemVector_.at(std::find(tabItemVector_.begin(), tabItemVector_.end(), ui->listWidget->item(i)) - tabItemVector_.begin())->path_)<<'\n';
     }
     f.close();
}
