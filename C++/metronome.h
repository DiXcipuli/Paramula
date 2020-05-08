#ifndef METRONOME_H
#define METRONOME_H

#include <QObject>

#include <QElapsedTimer>
#include <QSoundEffect>
#include <QTimer>

class Metronome : public QObject
{
  Q_OBJECT

public:
  Metronome(QObject *parent = 0);

  int bpm() const { return m_bpm; }
  void setBpm(const int newBpm);

  int beats() const { return m_beats; }
  void setBeats(const int newBeats);

  int currentBeat() const { return m_currentBeat; }
  void setCurrentBeat(int cb);

  int currentBar() const { return m_currentBar;}
  void setCurrentBar(int cb);

  bool isActive() const { return m_timer.isActive(); }

  bool tap();

public Q_SLOTS:
  void start();
  void stop();
  void toggle();

Q_SIGNALS:
  void changed();
    void newBarSignal();
    void newBeatSignal();

private Q_SLOTS:
  void performBeat();

private:
  QTimer m_timer;
  QElapsedTimer m_tap;

  int m_bpm;
  int m_currentBeat;
  int m_beats;
  int m_currentBar;

  QSoundEffect m_accent;
  QSoundEffect m_tick;
};

#endif
