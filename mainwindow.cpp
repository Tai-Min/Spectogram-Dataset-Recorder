#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qfiledialog.h>
#include <qaudiodeviceinfo.h>
#include <QMessageBox>
#include <QTextStream>
#include <QPixmap>
#include <QTimer>

#include <algorithm>
#include <complex>

#include "thirdparty/cnpy/cnpy.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    recorder(new QTimer(this)),
    counter(new QTimer(this)),
    startSound(":/start.wav"),
    stopSound(":/stop.wav")
{

    ui->setupUi(this);
    this->setFixedSize(QSize(500,652));

    ui->spectogramLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    QDir dir;
    this->ui->directoryDisplay->setText(dir.absolutePath());

    const auto deviceInInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for(const QAudioDeviceInfo &info: deviceInInfos){
        this->ui->recorderDevice->addItem(info.deviceName());
    }

    const auto deviceOutInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for(const QAudioDeviceInfo &info: deviceOutInfos){
        this->ui->recorderDevice->addItem(info.deviceName());
    }

    ui->stopButton->setEnabled(false);

    connect(recorder, &QTimer::timeout, [this](){
        const int sampleRate = ui->sampleRateInput->text().toInt();
        const int numChannels = ui->channelCountInput->text().toInt();
        const int bytesPerSample = ui->sampleSize->currentText().toInt()/8;
        const int recordDuration = ui->durationInput->text().toInt();

        while(audioBuf.buffer().size() < sampleRate*recordDuration*0.001*bytesPerSample*numChannels)
            QCoreApplication::processEvents();

        stopRecording(true);
    });
    connect(counter, &QTimer::timeout, this, &MainWindow::updateTimeLabel);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QImage MainWindow::spectogramToImg(const MatrixMath::vec2d & v){
    const long double minValSrc = MatrixMath::minMatrix(v);
    const long double maxValSrc = MatrixMath::maxMatrix(v);
    const long double colorMax = 0; // red in HSV
    const long double colorMin = 240; // dark blue in HSV
    const long double a = (colorMax - colorMin)/(maxValSrc - minValSrc);
    const long double b = colorMin - a * minValSrc;
    QImage img = QImage(v[0].size(), v.size(), QImage::Format_RGB32);

    for(unsigned int i = 0; i < v.size(); i++){
        for(unsigned int j = 0; j < v[i].size(); j++){
            QColor c = QColor::fromHsv(a*v[i][j] + b, 255, 255);
            img.setPixelColor(j, i, c);
        }
    }
    return img;
}

QAudioDeviceInfo MainWindow::getAudioDevice(QString name) {
    QAudioDeviceInfo device;
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for(int i = 0; i < devices.size(); ++i) {
        if(devices.at(i).deviceName() == name) {
            return devices.at(i);
        }
    }

    devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for(int i = 0; i < devices.size(); ++i) {
        if(devices.at(i).deviceName() == name) {
            return devices.at(i);
        }
    }

    return device;
}

bool MainWindow::validateInputs(){

    QString errText = "";
    // sample rate
    if(ui->sampleRateInput->text() == "")
        errText = "Sample rate input is empty.";
    else if(ui->sampleRateInput->text().toInt() < 1000)
        errText = "Minimum sample rate is 1000.";

    // channel count
    else if(ui->channelCountInput->text() == "")
        errText = "Channel count input is empty.";

    // duration
    else if(ui->recordType->currentText() == "Fixed duration" &&
            ui->durationInput->text().toInt() < 40)
        errText = "Duration input can't be less than 40.";

    // class
    else if(ui->classInput->text() == "")
        errText = "Class input is empty.";

    // pre emphasis
    else if(ui->preEmphasisInput->text() == "")
        errText = "Pre emphasis coeff input is empty.";

    else if(ui->preEmphasisInput->text().toDouble() > 1 || ui->preEmphasisInput->text().toDouble() < 0)
        errText = "Invalid pre emphasis coeff input.\nPre emphasis coeff should be between 0 and 1.";

    // frame size
    else if(ui->frameSizeInput->text() == "")
        errText = "Frame size input is empty.";
    else if(ui->frameSizeInput->text().toInt() < 1)
        errText = "Frame size input can't be less than 1.";

    // frame stride
    else if(ui->frameStrideInput->text() == "")
        errText = "Frame stride input is empty.";
    else if(ui->frameStrideInput->text().toInt() < 1)
        errText = "Frame stride input can't be less than 1.";

    // fft points
    else if(ui->FFTPointsInput->text() == "")
        errText = "FFT input is empty.";
    else if(ui->FFTPointsInput->text().toInt() < 1)
        errText = "FFT input can't be less than 1.";

    // filter banks
    else if(ui->filterBanksInput->text() == "")
        errText = "Filter banks input is empty.";
    else if(ui->filterBanksInput->text().toInt() < 1)
        errText = "Filter banks input can't be less than 1.";

    // MFCC coeffs
    else if(ui->resultMatrix->currentText() == "MFFC" &&
            ui->firstMFCCInput->text() == "")
        errText = "First MFFC input is empty.";
    else if(ui->resultMatrix->currentText() == "MFFC" &&
            ui->lastMFCCInput->text() == "")
        errText = "Last MFFC input is empty.";
    else if(ui->resultMatrix->currentText() == "MFFC" &&
            ui->firstMFCCInput->text().toInt() < 1)
        errText = "First MFFC coeff can't be less than 1.";
    else if(ui->resultMatrix->currentText() == "MFFC" &&
            ui->lastMFCCInput->text().toInt() < 1)
        errText = "Last MFFC coeff can't be less than 1.";
    else if(ui->resultMatrix->currentText() == "MFFC" &&
            ui->firstMFCCInput->text().toInt() > ui->filterBanksInput->text().toInt())
        errText = "First MFFC coeff can't be bigger than number of filter banks.";
    else if(ui->resultMatrix->currentText() == "MFFC" &&
            ui->lastMFCCInput->text().toInt() > ui->filterBanksInput->text().toInt())
        errText = "First MFFC coeff can't be bigger than number of filter banks.";
    else if(ui->resultMatrix->currentText() == "MFFC" &&
            ui->firstMFCCInput->text().toInt() > ui->lastMFCCInput->text().toInt())
        errText = "Last MFFC coeff can't be bigger than last MFCC coeff.";

    // Cepstral lifters
    else if(ui->resultMatrix->currentText() == "MFFC" &&
            ui->lifteringInput->currentText() == "Apply sinusoidal liftering" &&
            ui->cepLiftersInput->text() == "")
        errText = "Cepstral lifter input is empty.";
    else if(ui->resultMatrix->currentText() == "MFFC" &&
            ui->lifteringInput->currentText() == "Apply sinusoidal liftering" &&
            ui->cepLiftersInput->text().toInt() < 1)
        errText = "Cepstral lifter can't be less than 1.";

    // Rescale
    else if(ui->rescaleInput->currentText() == "Rescale" &&
            ui->rescaleMinInput->text() == "")
        errText = "Rescale parameters can't be empty";
    else if(ui->rescaleInput->currentText() == "Rescale" &&
            ui->rescaleMaxInput->text() == "")
        errText = "Rescale parameters can't be empty";
    else if(ui->rescaleInput->currentText() == "Rescale" &&
            ui->rescaleMaxInput->text().toDouble() == ui->rescaleMinInput->text().toDouble())
        errText = "Rescale parameters can't be equal.";

    // Number of repeats for repeating recording
    else if(isRepeating && ui->numRepeatsInput->text().toInt() < 0)
        errText = "Number of repeats must be 0 or bigger.";

    if(errText != ""){
        QMessageBox msgBox;
        msgBox.setText(errText);
        msgBox.exec();
        return 0;
    }

    return 1;
}

bool MainWindow::validateFormat(QAudioFormat format){
    QAudioDeviceInfo devInfo = getAudioDevice(ui->recorderDevice->currentText());
    if(!devInfo.isFormatSupported(format)){
        format = devInfo.nearestFormat(format);

        QMessageBox msgBox;
        msgBox.setText("Format (sample rate or channel count or sample size) not supported for selected device.\n\n"
                       "Supported nearest format:\n"
                       "Sample rate: " + QString::number(format.sampleRate()) + "\n"
                       "Channel count: " + QString::number(format.channelCount()) + "\n"
                       "Sample size: " + QString::number(format.sampleSize()) + "\n");
        msgBox.exec();
        return 0;
    }
    return 1;
}

void MainWindow::prepareClassFolder(){
    QDir dir;
    dir.setPath(ui->directoryDisplay->text());
    if(!dir.exists(ui->classInput->text())){
        dir.mkdir(ui->classInput->text());
    }
}

QString MainWindow::findAvailableFilename(){
    static int fname = 1; // keep it static so it won't iterate over whole dataset everytime it needs to save a file

    QDir dir;
    dir.setPath(ui->directoryDisplay->text());
    dir.cd(ui->classInput->text());

    QString format = ".txt";
    if(ui->fileFormat->currentText() == "Numpy array")
        format = ".npy";
    else if(ui->fileFormat->currentText() == "JPG color image" || ui->fileFormat->currentText() == "JPG grayscale image")
        format = ".jpg";

    while(dir.exists(QString::number(fname) + format)){
        fname++;
    }
    return QString::number(fname) + format;
}

MatrixMath::vec2d MainWindow::processAudioBuffer(){
    const unsigned int bytesPerSample = ui->sampleSize->currentText().toInt()/8;
    const unsigned int numberOfChannels = ui->channelCountInput->text().toInt();
    const unsigned int sampleRate = ui->sampleRateInput->text().toInt();
    const long double emphasisCoeff = static_cast<long double>(ui->preEmphasisInput->text().toDouble());
    const unsigned int frameSize = ui->frameSizeInput->text().toInt();
    const unsigned int frameStride = ui->frameStrideInput->text().toInt();
    const unsigned int NFFT = ui->FFTPointsInput->text().toInt();
    const unsigned int numFilterBanks = ui->filterBanksInput->text().toInt();
    const bool MFCC = ui->resultMatrix->currentText() == "MFCC";
    const unsigned int firstMFCC = ui->firstMFCCInput->text().toInt();
    const unsigned int lastMFCC = ui->lastMFCCInput->text().toInt();
    const bool sinLift = ui->lifteringInput->currentText() == "Apply sinusoidal liftering";
    const unsigned int cepLifter = ui->cepLiftersInput->text().toInt();
    const bool normalize = ui->normalizeData->currentText() == "Normalize";
    const bool rescale = ui->rescaleInput->currentText() == "Rescale";
    const long double scaleMin = static_cast<long double>(ui->rescaleMinInput->text().toDouble());
    const long double scaleMax = static_cast<long double>(ui->rescaleMaxInput->text().toDouble());

    AudioProcessor::config conf = {
        bytesPerSample,
        numberOfChannels,
        sampleRate,
        emphasisCoeff,
        frameSize,
        frameStride,
        NFFT,
        numFilterBanks,
        MFCC,
        firstMFCC,
        lastMFCC,
        sinLift,
        cepLifter,
        normalize,
        rescale,
        scaleMin,
        scaleMax
    };

    AudioProcessor audioProc(conf);
    AudioProcessor::byteVec byteData(audioBuf.buffer().begin(), audioBuf.buffer().end());

    MatrixMath::vec2d spectogram;
    spectogram = audioProc.processBuffer(byteData);

    return spectogram;
}

void MainWindow::savePlain(QString fname, QString dname, const MatrixMath::vec2d & data){
    QFile file(dname + "/" + fname);
    if(!file.open(QIODevice::WriteOnly)){
        QMessageBox msgBox;
        msgBox.setText("Couldn't open file for save.");
        msgBox.exec();
        return;
    }

    QTextStream out(&file);
    for(unsigned int i = 0; i < data.size(); i++){
        for(unsigned int j = 0; j < data[i].size(); j++){
            out << static_cast<double>(data[i][j]) << " ";
        }
        out << '\n';
    }

    file.close();
}

void MainWindow::saveColorImg(QString fname, QString dname, const QImage & img){
    img.save(dname + "/" + fname + ".jpg");
}

void MainWindow::saveGrayscaleImg(QString fname, QString dname, const QImage & img){
    QImage gray = img.convertToFormat(QImage::Format_Grayscale8);
    gray.save(dname + "/" + fname);
}

void MainWindow::saveNumpy(QString fname, QString dname, const MatrixMath::vec2d & data){
    std::string f = fname.toStdString();
    std::string d = dname.toStdString();

    MatrixMath::vec data1d(data.size() * data[0].size());
    for(unsigned int i = 0; i < data.size(); i++){
        if(i == 0)
            std::copy(data[i].begin(), data[i].end(), data1d.begin());
        else
            std::copy(data[i].begin(), data[i].end(), data1d.begin() + i*data[i-1].size());
    }

    cnpy::npy_save(d + "/" + f, &data1d[0], {data[0].size(), data[0].size()}, "w");
}

void MainWindow::saveRecording(){
    MatrixMath::vec2d spectogram = processAudioBuffer();

    // create QImage from spectogram and display it and maybe save as jpg if it's selected
    QImage spectogramImg = spectogramToImg(spectogram);
    ui->spectogramLabel->setPixmap(QPixmap::fromImage(spectogramImg.scaled(QSize(ui->spectogramLabel->width(), ui->spectogramLabel->height()))));

    prepareClassFolder();

    QString fileName = findAvailableFilename();

    QDir dir;
    dir.setPath(ui->directoryDisplay->text());
    dir.cd(ui->classInput->text());

    if(ui->fileFormat->currentText() == "Plain text"){
        savePlain(fileName, dir.path(), spectogram);
    }
    else if(ui->fileFormat->currentText() == "Numpy array"){
        saveNumpy(fileName, dir.path(), spectogram);
    }
    else if(ui->fileFormat->currentText() == "JPG color image"){
        saveColorImg(fileName, dir.path(), spectogramImg);
    }
    else if(ui->fileFormat->currentText() == "JPG grayscale image"){
        saveGrayscaleImg(fileName, dir.path(), spectogramImg);
    }
}

void MainWindow::setTimeLabel(){
    QString minStr = mins < 10 ? "0" + QString::number(mins) : QString::number(mins);
    QString secStr = secs < 10 ? "0" + QString::number(secs) : QString::number(secs);
    ui->timeLabel->setText(minStr + ":" + secStr);
}

void MainWindow::closeAndClearAudioBuffer(){
    audioBuf.buffer().clear();
    audioBuf.reset();
    audioBuf.close();
}

void MainWindow::uxRecording(){
    // disable all recording buttons for now
    ui->stopButton->setEnabled(false);
    ui->startButton->setEnabled(false);
    ui->startRepeatButton->setEnabled(false);
    ui->numRepeatsInput->setEnabled(false);

    // play sound
    startSound.play();
    while(!startSound.isFinished()){
        QCoreApplication::processEvents();
    }

    // disable device settings
    ui->recorderDevice->setEnabled(false);
    ui->sampleRateInput->setEnabled(false);
    ui->channelCountInput->setEnabled(false);
    ui->sampleSize->setEnabled(false);

    // disable recording settings
    ui->recordType->setEnabled(false);
    ui->durationInput->setEnabled(false);

    // disable save settings
    ui->directoryButton->setEnabled(false);
    ui->classInput->setEnabled(false);

    // disable post process settings
    ui->preEmphasisInput->setEnabled(false);
    ui->frameSizeInput->setEnabled(false);
    ui->frameStrideInput->setEnabled(false);
    ui->FFTPointsInput->setEnabled(false);
    ui->filterBanksInput->setEnabled(false);

    // enable recording buttons
    ui->stopButton->setEnabled(true);

    this->ui->timeLabel->clearFocus();
}

void MainWindow::uxIdle(){
    // disable all recording buttons for now
    ui->stopButton->setEnabled(false);
    ui->startButton->setEnabled(false);
    ui->startRepeatButton->setEnabled(false);
    ui->numRepeatsInput->setEnabled(false);

    // play sound
    stopSound.play();
    while(!stopSound.isFinished()){
        QCoreApplication::processEvents();
    }

    // enable device settings
    ui->recorderDevice->setEnabled(true);
    ui->sampleRateInput->setEnabled(true);
    ui->channelCountInput->setEnabled(true);
    ui->sampleSize->setEnabled(true);

    // enable recording settings
    ui->recordType->setEnabled(true);
    ui->durationInput->setEnabled(true);

    // enable save settings
    ui->directoryButton->setEnabled(true);
    ui->classInput->setEnabled(true);

    // enable post process settings
    ui->preEmphasisInput->setEnabled(true);
    ui->frameSizeInput->setEnabled(true);
    ui->frameStrideInput->setEnabled(true);
    ui->FFTPointsInput->setEnabled(true);
    ui->filterBanksInput->setEnabled(true);

    ui->stopButton->setEnabled(false);
    ui->startButton->setEnabled(false);
    ui->startRepeatButton->setEnabled(false);
    ui->numRepeatsInput->setEnabled(false);

    // enable recording buttons
    ui->startButton->setEnabled(true);
    if(ui->recordType->currentText() == "Fixed duration"){
        ui->startRepeatButton->setEnabled(true);
        ui->numRepeatsInput->setEnabled(true);
    }

    this->ui->timeLabel->clearFocus();
}

void MainWindow::startRecording(){
    if(!validateInputs())
        return;

    QAudioFormat format;
    format.setSampleRate(ui->sampleRateInput->text().toInt());
    format.setChannelCount(ui->channelCountInput->text().toInt());
    format.setSampleSize(ui->sampleSize->currentText().toInt());
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    if(!validateFormat(format))
        return;

    uxRecording();

    audioInput = new QAudioInput(getAudioDevice(ui->recorderDevice->currentText()),format, this);

    audioBuf.open(QIODevice::ReadWrite);

    counter->start(1000);
    audioInput->start(&audioBuf);

    if(ui->recordType->currentText() == "Fixed duration"){
        recorder->start(ui->durationInput->text().toInt());
    }

    QCoreApplication::processEvents(); // ??? required as without it app lags a bit if mouse has not moved which also affects recording device
}

void MainWindow::stopRecording(bool fixedDurationSuccess){
    audioInput->stop();
    recorder->stop();
    counter->stop();

    delete audioInput;

    if(ui->recordType->currentText() == "Fixed duration"){

        // fixed duration recording finished successfully
        if(fixedDurationSuccess){
            // make sure that there is expected number of sample in buffer
            // if there is too many samples then trim buffer
            // if there is less then append zeros
            const int sampleRate = ui->sampleRateInput->text().toInt();
            const int numChannels = ui->channelCountInput->text().toInt();
            const int bytesPerSample = ui->sampleSize->currentText().toInt()/8;
            const int recordDuration = ui->durationInput->text().toInt();
            const int expectedBufferSize = sampleRate * bytesPerSample * numChannels * recordDuration*0.001;
            if(audioBuf.buffer().size() > expectedBufferSize){
                const int diff = audioBuf.buffer().size() - expectedBufferSize;
                audioBuf.buffer().remove(expectedBufferSize, diff);
            }
            else if(audioBuf.buffer().size() < expectedBufferSize){
                const int diff = expectedBufferSize - audioBuf.buffer().size();
                audioBuf.buffer().append(diff, 0);
            }

            saveRecording();

            closeAndClearAudioBuffer();

            // repeat recording if START REPEAT was clicked

            if(isRepeating){
                repeatCntr++;
                if(repeatCntr < ui->numRepeatsInput->text().toInt() || ui->numRepeatsInput->text().toInt() == 0)
                    startRecording();
                else
                    isRepeating = false;
            }
        }
        // recording stopped using STOP button so clear stuff and finish
        else{
            isRepeating = false;

            closeAndClearAudioBuffer();
        }
    }
    // until stopped recording stopped by user so save stuff
    else{

        saveRecording();

        closeAndClearAudioBuffer();
    }

    secs = 0;
    mins = 0;
    setTimeLabel();

    if(!isRepeating){
        uxIdle();
    }
}

void MainWindow::on_recordType_currentIndexChanged(const QString &arg1)
{
    if(arg1 != "Fixed duration"){
        ui->durationInput->setEnabled(false);
        ui->startRepeatButton->setEnabled(false);
        ui->numRepeatsInput->setEnabled(false);
    }
    else{
        ui->durationInput->setEnabled(true);
        ui->startRepeatButton->setEnabled(true);
        ui->numRepeatsInput->setEnabled(true);
    }
}

void MainWindow::on_directoryButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::DirectoryOnly);
    QString dirName;
    if(dialog.exec()){
       dirName = dialog.selectedFiles()[0];
       this->ui->directoryDisplay->setText(dirName);
    }
}

void MainWindow::on_stopButton_clicked()
{
    bool flag = ui->recordType->currentText() == "Fixed duration" ? false : true;
    stopRecording(flag);
}

void MainWindow::on_startButton_clicked()
{
    isRepeating = false;
    startRecording();
}

void MainWindow::on_startRepeatButton_clicked()
{
    isRepeating = true;
    repeatCntr = 0;
    startRecording();
}

void MainWindow::updateTimeLabel(){
    secs++;
    if(secs >= 60){
        secs = 0;
        mins++;
    }
    setTimeLabel();
}

void MainWindow::on_resultMatrix_currentTextChanged(const QString &arg1)
{
    if(arg1 == "MFFC"){
        ui->firstMFCCInput->setEnabled(true);
        ui->lastMFCCInput->setEnabled(true);
        ui->lifteringInput->setEnabled(true);
        if(ui->lifteringInput->currentText() == "Apply sinusoidal liftering"){
            ui->cepLiftersInput->setEnabled(true);
        }
    }
    else{
        ui->firstMFCCInput->setEnabled(false);
        ui->lastMFCCInput->setEnabled(false);
        ui->lifteringInput->setEnabled(false);
        ui->cepLiftersInput->setEnabled(false);
    }
}

void MainWindow::on_lifteringInput_currentTextChanged(const QString &arg1)
{
    if(arg1 == "Apply sinusoidal liftering"){
        ui->cepLiftersInput->setEnabled(true);
    }
    else{
        ui->cepLiftersInput->setEnabled(false);
    }
}

void MainWindow::on_rescaleInput_currentTextChanged(const QString &arg1)
{
    if(arg1 == "Rescale"){
        ui->rescaleMaxInput->setEnabled(true);
        ui->rescaleMinInput->setEnabled(true);
    }
    else{
        ui->rescaleMaxInput->setEnabled(false);
        ui->rescaleMinInput->setEnabled(false);
    }
}
