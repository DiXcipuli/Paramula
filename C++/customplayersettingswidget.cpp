#include "customplayersettingswidget.h"
#include <QPixmap>
#include <QVBoxLayout>

CustomPlayerSettingsWidget::CustomPlayerSettingsWidget(QWidget *parent, QString index):
    QFrame(parent),
    slSkillList_({"1 - God (1 bar to jam)", 
                    "2 - Expert (2 bars to jam)",
                     "3 - Normal (3 bars to jam)",
                        "4 - Newbie (4 bars to jam)"})
{
    //vertical layout
    QVBoxLayout* nameLayout = new QVBoxLayout(hLayout_.widget());
    QVBoxLayout* skillsLayout = new QVBoxLayout(hLayout_.widget());

    //Set ComboBox with QStringList
    cbSkills_.addItems(slSkillList_);

    leSkills_.setText("Skills:");
    leSkills_.setReadOnly(true);
    leNameQ_.setText("Player Name:");
    leNameQ_.setReadOnly(true);
    leNameA_.setText(QString::fromStdString("Player" + index.toStdString()));

    //set vertical layouts
    nameLayout->addWidget(&leNameQ_);
    nameLayout->addWidget(&leNameA_);
    skillsLayout->addWidget(&leSkills_);
    skillsLayout->addWidget(&cbSkills_);

    //Image
    QPixmap pixmap(QString::fromStdString(":/img/player" + index.toStdString() + "Normal"));
    pixmap = pixmap.scaled(100,100, Qt::KeepAspectRatio);
    lImg_.setPixmap((pixmap));

    //layout
    this->setLayout(&hLayout_);
    hLayout_.addLayout(nameLayout);
    hLayout_.addWidget(&lImg_);
    hLayout_.addLayout(skillsLayout);
    this->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    this->setObjectName("CustomPlayerSettingsWidget");
    this->setStyleSheet("#CustomPlayerSettingsWidget{border: 3px solid black;}");
}

//Getters
QString CustomPlayerSettingsWidget::getPlayerName() const{
    return leNameA_.text();
}

QString CustomPlayerSettingsWidget::getPlayerSkill() const{
    return cbSkills_.currentText();
}

CustomPlayerSettingsWidget::~CustomPlayerSettingsWidget(){

}
