#include "metronome.h"
#include <QUrl>
#include <iostream>

static const int MINUTE_IN_MS = 60 * 1000;

Metronome::Metronome(QObject *parent)
  : QObject(parent)
{
  m_timer.setTimerType(Qt::PreciseTimer);
  connect(&m_timer, &QTimer::timeout, this, &Metronome::performBeat);

  m_accent.setSource(QUrl("qrc:/sound/accent"));
  m_tick.setSource(QUrl("qrc:/sound/tick"));

  setBpm(120);
  setBeats(4);
}

void Metronome::setBpm(const int newBpm)
{
  m_bpm = qBound(10, newBpm, 999);

  Q_EMIT changed();
}

void Metronome::setBeats(const int newBeats)
{
  m_beats = qBound(1, newBeats, 9);

  Q_EMIT changed();
}

void Metronome::setCurrentBeat(int cb){
    m_currentBeat = cb;
}

void Metronome::setCurrentBar(int cb){
    m_currentBar = cb;
}

void Metronome::start()
{
  m_currentBeat = 0;
  m_timer.start(0);

  Q_EMIT changed();
}

void Metronome::stop()
{
  m_timer.stop();

  Q_EMIT changed();
}

void Metronome::toggle()
{
  if(isActive())
    stop();
  else
    start();
}

void Metronome::performBeat()
{
  if(m_currentBeat++ >= m_beats){
    m_currentBeat = 1;
    m_currentBar++;
    Q_EMIT newBarSignal();
  }

  (m_currentBeat == 1 ? &m_accent : &m_tick)->play();
  Q_EMIT newBeatSignal();

  const int interval = MINUTE_IN_MS / m_bpm;
  if(m_timer.interval() != interval)
    m_timer.setInterval(interval);

  Q_EMIT changed();
}

bool Metronome::tap()
{
  if(m_tap.isValid()) {
    setBpm(MINUTE_IN_MS / m_tap.elapsed());
    m_tap.invalidate();

    return true;
  }

  m_tap.start();
  return false;
}
