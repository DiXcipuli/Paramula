#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QLineEdit>
#include <QSlider>
#include <QtSerialPort/QSerialPort>
#include <QByteArray>
#include "metronome.h"
#include "customplayersettingswidget.h"
#include "customarduinobuttonwidget.h"
#include "qcustomplot.h"
#include <opencv2/opencv.hpp>
#include <QTimer>
#include <QPalette>
#include <QDir>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QFont>
#include <QFile>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum class Mode { normal, cascading, record, loop, empty, ready, readyWarning, refresh, tab};

struct QueueItem{
    int index;
    std::string name;
    std::string imgPath;
    int level;
    float step;
    ~QueueItem(){};
};

class CustomListWidgetItem : public QListWidgetItem{
public:
    CustomListWidgetItem(std::string path){
        path_ = path;
    }
    std::string path_;
    ~CustomListWidgetItem(){}
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void createPlayersSettings(int);
    void createBeats(int b);
    void createBars(int b);
    void removeBars();
    void removeBeats();
    void removePlayersSettings();
    void findArduino();
    void createArduinoButton(int);
    void removeArduinoButton();
    float stepComputation(QueueItem*);
    void addPlayersToQueue();
    void addTabsToQueue();
    void createPlayersQueueItem();
    void clearQueue();
    void clearQueueGraphics();
    void stopQueue();
    void newActivePlayerInQueue();
    void drawQueue();
    void updateGraphicsQueue();
    void removeLastQueueItem();
    void stopSession();
    void setTemplateRecordMat();
    void createTabFromFile(QFile& file);
    void checkDirectories();
    void loadUserPref();
    void saveTab(std::string path);
    void destroyPlayers();
    virtual void keyPressEvent(QKeyEvent *event);

private slots:

    void readArduino();

    void arduinoReadyToSync();

    void stopRecording();

    void on_confirmButton_clicked();

    void on_nbPlayersA_currentIndexChanged(const QString &arg1);

    void on_productID_textChanged(const QString &arg1);

    void on_vendorID_textChanged(const QString &arg1);

    void on_findArduinoButton_clicked();

    void sliderChanged(int);

    void updatePlot();

    void on_settingsButton_clicked();

    void checkBarCount();

    void updateCurrentBeat();

    void updateCurrentBar();

   // void updateGraphicViewSize();

    void scrollView();

    void on_tabEditorButton_clicked();

    void on_playButton_clicked();

    void on_exitButton_clicked();

    void on_loadTabButton_clicked();

    void on_importTab_clicked();

    void on_deleteTab_clicked();

    void on_clearAll_clicked();

    void on_copyTab_clicked();

    void on_pasteTab_clicked();

    void on_saveTab_clicked();

    void on_settingsFromTab_clicked();

    void on_bpmA_currentIndexChanged(const QString &arg1);

    void on_beatsA_currentIndexChanged(const QString &arg1);

    void on_restartTab_clicked();

    void on_buttonThreshold1_textChanged(const QString &arg1);

    void on_buttonThreshold2_textChanged(const QString &arg1);

    void on_buttonThreshold3_textChanged(const QString &arg1);

    void on_buttonThreshold4_textChanged(const QString &arg1);

    void on_savePrefButton_clicked();

    void on_saveTabAs_clicked();

private:
    Ui::MainWindow *ui;

    //default Value
    int defaultPlayerNumber_ = 4;
    int defaultThresholdValue_ = 200;
    int defaultButtonMinValue_ = 0;
    int defaultButtonMaxValue_ = 1023;
    const int defaultBeatsNumber_ = 4;
    const int defaultDelayToSyncArduino_ = 1000;
    const std::string defaultTabName_ = "RawRecordsTab";
    const int defaultBpm_ = 120;
    const int arduinoByteArraySize_ = 16;
    const int defaultQueueItemImageWidth_ = 500;
    const int defaultQueueItemImageHeight_ = 500;
    const int defaultRecordMarkLineLength_ = 30;
    int ButtonVoltageThreshold[4] = {200,500,600,800};

    QString sessionName_;
    int playerNumber_;
    Metronome metronome_;
    int beatsNumber_;
    bool isBinary_;
    bool isTimeSub_;
    int delayCompensation_;
    std::vector<int> thresholdValueVector_;
    std::vector<QueueItem*> playerVector_;
    std::vector<int> inputPiezoVoltageVector_;
    std::vector<std::pair<QueueItem*, Mode>> queueItemVector_;
    std::vector<QCustomPlot*> customPlotVector_;
    std::vector<QGraphicsPixmapItem*> graphicPixmapVector_;
    std::vector<QLineEdit *> ButtonVoltageWidgetVector_;
    //Tab
    std::vector<CustomListWidgetItem *> tabItemVector_;
    std::vector<CustomListWidgetItem *> copiedTabVector_;

    QGraphicsScene* graphicScene_;
    QueueItem* queueItemEventTemplate_;
    QTimer plotTimer_;
    QTimer scrollTimer_;
    QTimer resizeLayoutTimer_;
    QTimer recordTimer_;
    QTimer delayTimerToSyncArduino_;
    QElapsedTimer elapsedTimer_;
    int scrollTimerPeriod_ = 10;
    Mode previousMode_;
    QFont beatBarFont_;
    int recordsIndex_ = 1;
    int currentTabIndex_ = -1;

    //Arduino
    QSerialPort * arduino_;
    bool isArduinoFound_ = false;
    bool isArduinoSync_ = false;
    bool isArduinoReadyToSync_ = false;
    bool isRecordMode_ = false;
    bool isPlayMode_ = false;
    bool hasNewRecords_ = false;
    bool isSessionRunning_ = false;
    bool isFootswitchReleased_ = true;
    bool isTabSavePathDefined_ = false;
    bool isTabMode_ = false;
    quint16 arduinoProductId_ = 67;
    quint16 arduinoVendorId_ = 9025;
    std::vector<std::pair<int, int>> arduinoButtonIntervalVector_;
    QByteArray arduinoProcessedByteArray_;
    quint16 footSwitchInputVoltage;
    std::map<Mode, std::string> modeMap_;

    //opencv
    cv::Mat recordedTabMat_;
    cv::Mat templateNormalMat_;
    cv::Mat templateCascadeMat_;

    //Qdir
//    QDir dir;
    std::string paramulaPath_;
    std::string picturesPath_;
    std::string userPrefPath_;
    std::string saveTabPath_;
//    QFile file_;

    //Vectors of widget, to get values
    std::vector<QSlider*> thresholdSliderVector_;
    std::vector<CustomPlayerSettingsWidget*> customPlayerSettinsWidgetVector_;
    std::vector<CustomArduinoButtonWidget*> customArduinoButtonWidgetVector_;

    //beats and bars vector
    std::vector<QLineEdit*> barsVector_;
    std::vector<QLineEdit*> beatsVector_;

    //Palettes
    QPalette greenPal_;
    QPalette redPal_;
    QPalette whitePal_;
};
#endif // MAINWINDOW_H
