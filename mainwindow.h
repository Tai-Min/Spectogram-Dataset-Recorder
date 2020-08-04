#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAudioInput>
#include <QBuffer>
#include <QSound>
#include <QImage>

#include "audioprocessor.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_recordType_currentIndexChanged(const QString &arg1);

    void on_directoryButton_clicked();

    void on_stopButton_clicked();

    void on_startButton_clicked();

    void on_startRepeatButton_clicked();

    void audioDevice_notify();

private:
    Ui::MainWindow *ui;

    int mins = 0; //!< keep number of minutes since start of recording.
    int secs = 0; //!< keep number of seconds since start of recording.

    QSound startSound; //!< play start.wav.
    QSound stopSound; //!< play stop.wav.

    bool isRepeating = false; //!< true when START REPEAT is used, false otherwise.

    QAudioInput *audioInput; //!< device used to record data.
    QBuffer audioBuf; //!< raw audio data is stored here.

    long double minMatrix(const AudioProcessor::vec2d & v);
    long double maxMatrix(const AudioProcessor::vec2d & v);
    QImage spectogramToImg(const AudioProcessor::vec2d & v);

    /**
     * @brief Get info about device of given name.
     * @param name Name of the device.
     * @return Info about device of given name.
     */
    QAudioDeviceInfo getAudioDevice(QString name);

    /**
     * @brief Check whether inputs are empty or contain illegal values.
     * @return True is inputs are valid.
     */
    bool validateInputs();

    /**
     * @brief Check if given format is supported by selected audio device.
     * @param format Format to check.
     * @return True if format is supported.
     */
    bool validateFormat(QAudioFormat format);

    /**
     * @brief Creates class folder inside dataset's root directory. If exists then nothing happens.
     */
    void prepareClassFolder();

    /**
     * @brief Finds closest available file name, i.e first recording will be called "1", second "2" etc.
     * If there are files "1" and "3", "2" will be used as file name.
     *
     * @return Available file name.
     */
    QString findAvailableFilename();

    /**
     * @brief TODO
     *
     * @return Spectogram of audio buffer
     */
    AudioProcessor::vec2d processAudioBuffer();

    void savePlain(QString fname, QString dname, const AudioProcessor::vec2d & data);

    void saveColorImg(QString fname, QString dname, const QImage & img);

    void saveGrayscaleImg(QString fname, QString dname, const QImage & img);

    void saveNumpy(QString fname, QString dname, const AudioProcessor::vec2d & data);

    /**
     * @brief TODO
     */
    void saveRecording();

    /**
     * @brief Update time label with recorded time in format "mm:ss".
     *
     * Uses variables "mins" and "secs".
     */
    void setTimeLabel();

    /**
     * @brief clear content of internal buffer of "audioBuf",
     * reset pointer of audioBuf and close file descriptor.
     */
    void closeAndClearAudioBuffer();

    /**
     * @brief Disables every part of UI except STOP button which is being enabled. Used when recording starts.
     */
    void uxRecording();

    /**
     * @brief Enables every part of UI except STOP button which is being disabled and
     *  START REPEAT which can be enabled if "Fixed duration" is selected as Record type, otherwise stays disabled.
     */
    void uxIdle();

    /**
     * @brief Do all stuff to record audio. Validate inputs, format, play start.wav and start recording.
     */
    void startRecording();

    /**
     * @brief Do all stuff required to stop recording audio. Stop recorder, play stop.wav, save record, clear audio buffer and restart recording if necessary.
     */
    void stopRecording();
};

#endif // MAINWINDOW_H
