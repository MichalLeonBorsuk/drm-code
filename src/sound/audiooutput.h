#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <QObject>
#include <QAudioOutput>
#include <QAudioBuffer>

class AudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(QObject *parent = nullptr);

signals:

public slots:
    void audioDecoded(const QAudioBuffer&);
private:
    QAudioOutput* pOutput;
    QIODevice* pDevice;

    void initialise(const QAudioFormat&, const QString="");
};

#endif // AUDIOOUTPUT_H
